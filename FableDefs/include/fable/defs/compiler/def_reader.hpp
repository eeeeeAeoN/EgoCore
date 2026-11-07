// Typed binary -> generic value tree for the def decompiler.
#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <fable/defs/visit.hpp>
#include <fable/defs/transfer.hpp>

#include "fable/defs/compiler/binary_reader.hpp"
#include "fable/defs/compiler/manifest.hpp"
#include "fable/defs/compiler/model_registry.hpp"
#include "fable/defs/compiler/type_registry.hpp"
#include "fable/defs/compiler/type_traits.hpp"

namespace fable::defs::compiler {

enum class ValueKind {
    Int,
    Float,
    Bool,
    String,
    WString,
    PString,
    DefIndex,
    DefString,
    Enum,
    Flags,
    List,
    Map,
    Struct,
    Variant,
    Opaque,
};

struct Value {
    ValueKind kind = ValueKind::Int;
    std::int64_t integer = 0;
    std::uint32_t f32_bits = 0;
    bool boolean = false;
    std::string text;
    std::u16string wide;
    std::vector<std::uint8_t> bytes;
    std::vector<Value> list;
    std::vector<std::pair<Value, Value>> map;
    std::vector<std::pair<std::string, Value>> fields;
    std::uint32_t tag = 0;
    std::string type_name;
};

struct ParsedEntry {
    std::string def_type;
    bool is_template = false;
    std::vector<std::pair<std::string, Value>> fields;
};

class Reader {
public:
    Reader(const std::uint8_t *data, std::size_t size) : data_(data), size_(size) {}

    bool u8(std::uint8_t &value) {
        if (pos_ + 1 > size_) {
            return false;
        }
        value = data_[pos_++];
        return true;
    }
    bool u16(std::uint16_t &value) {
        if (pos_ + 2 > size_) {
            return false;
        }
        value = static_cast<std::uint16_t>(data_[pos_] | (data_[pos_ + 1] << 8));
        pos_ += 2;
        return true;
    }
    bool u32(std::uint32_t &value) {
        if (pos_ + 4 > size_) {
            return false;
        }
        value = static_cast<std::uint32_t>(data_[pos_]) |
                (static_cast<std::uint32_t>(data_[pos_ + 1]) << 8) |
                (static_cast<std::uint32_t>(data_[pos_ + 2]) << 16) |
                (static_cast<std::uint32_t>(data_[pos_ + 3]) << 24);
        pos_ += 4;
        return true;
    }
    bool i32(std::int32_t &value) {
        std::uint32_t raw = 0;
        if (!u32(raw)) {
            return false;
        }
        value = static_cast<std::int32_t>(raw);
        return true;
    }
    bool f32_bits(std::uint32_t &value) { return u32(value); }
    bool cstr(std::string &value) {
        const std::size_t start = pos_;
        while (pos_ < size_ && data_[pos_] != 0) {
            ++pos_;
        }
        if (pos_ >= size_) {
            return false;
        }
        value.assign(reinterpret_cast<const char *>(data_ + start), pos_ - start);
        ++pos_;
        return true;
    }
    bool wstr(std::u16string &value) {
        value.clear();
        for (;;) {
            std::uint16_t ch = 0;
            if (!u16(ch)) {
                return false;
            }
            if (ch == 0) {
                return true;
            }
            value.push_back(static_cast<char16_t>(ch));
        }
    }
    bool bytes(std::size_t count, std::vector<std::uint8_t> &value) {
        if (pos_ + count > size_) {
            return false;
        }
        value.assign(data_ + pos_, data_ + pos_ + count);
        pos_ += count;
        return true;
    }
    bool skip(std::size_t count) {
        if (pos_ + count > size_) {
            return false;
        }
        pos_ += count;
        return true;
    }
    std::size_t remaining() const { return size_ - pos_; }
    bool eof() const { return pos_ == size_; }

private:
    const std::uint8_t *data_;
    std::size_t size_;
    std::size_t pos_ = 0;
};

template <class T>
bool read_value(T &value, Reader &reader, Value &out);

namespace detail {

struct ControlReaderVisitor {
    Reader &reader;
    Value &out;
    bool ok = true;

    template <class F>
    void operator()(const char *name, F &field) {
        std::uint32_t id = 0;
        if (!ok || !reader.u32(id)) {
            ok = false;
            return;
        }
        Value child;
        if (!read_value(field, reader, child)) {
            ok = false;
            return;
        }
        out.fields.emplace_back(name, std::move(child));
    }

    template <class F>
    void operator()(const char *name, ::fable::defs::detail::BitfieldRef<F>) {
        std::uint32_t id = 0;
        std::uint8_t value = 0;
        if (!ok || !reader.u32(id) || !reader.u8(value)) {
            ok = false;
            return;
        }
        Value child;
        child.kind = ValueKind::Bool;
        child.boolean = value != 0;
        out.fields.emplace_back(name, std::move(child));
    }
};

struct BareReaderVisitor {
    Reader &reader;
    Value &out;
    bool ok = true;

    template <class F>
    void operator()(const char *name, F &field) {
        if (!ok) {
            return;
        }
        Value child;
        if (!read_value(field, reader, child)) {
            ok = false;
            return;
        }
        out.fields.emplace_back(name, std::move(child));
    }

    template <class F>
    void operator()(const char *name, ::fable::defs::detail::BitfieldRef<F>) {
        std::uint8_t value = 0;
        if (!ok || !reader.u8(value)) {
            ok = false;
            return;
        }
        Value child;
        child.kind = ValueKind::Bool;
        child.boolean = value != 0;
        out.fields.emplace_back(name, std::move(child));
    }
};

template <class T>
bool read_controls(T &value, Reader &reader, Value &out) {
    out.kind = ValueKind::Struct;
    out.fields.clear();
    ControlReaderVisitor visitor{reader, out};
    ::fable::defs::visit_model_fields(value, visitor);
    return visitor.ok;
}

template <class T>
bool read_bare(T &value, Reader &reader, Value &out) {
    out.kind = ValueKind::Struct;
    out.fields.clear();
    BareReaderVisitor visitor{reader, out};
    ::fable::defs::visit_model_fields(value, visitor);
    return visitor.ok;
}

template <class T>
bool read_vector(T &, Reader &reader, Value &out) {
    std::uint32_t count = 0;
    if (!reader.u32(count)) {
        return false;
    }
    out.kind = ValueKind::List;
    out.list.clear();
    out.list.reserve(count);
    using Element = typename T::value_type;
    if constexpr (std::is_same_v<Element, bool>) {
        for (std::uint32_t i = 0; i < count; ++i) {
            std::uint8_t byte = 0;
            if (!reader.u8(byte)) {
                return false;
            }
            Value item;
            item.kind = ValueKind::Bool;
            item.boolean = byte != 0;
            out.list.push_back(std::move(item));
        }
    } else {
        for (std::uint32_t i = 0; i < count; ++i) {
            Element element{};
            Value item;
            if (!read_value(element, reader, item)) {
                return false;
            }
            out.list.push_back(std::move(item));
        }
    }
    return true;
}

} // namespace detail

template <class T>
bool read_value(T &value, Reader &reader, Value &out) {
    using U = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (std::is_same_v<U, bool>) {
        std::uint8_t byte = 0;
        if (!reader.u8(byte)) return false;
        out.kind = ValueKind::Bool;
        out.boolean = byte != 0;
        return true;
    } else if constexpr (std::is_same_v<U, CDefString>) {
        std::int32_t raw = 0;
        if (!reader.i32(raw)) return false;
        value.TablePos = raw;
        out.kind = ValueKind::DefString;
        out.integer = raw;
        return true;
    } else if constexpr (std::is_same_v<U, CDefIndex>) {
        std::int32_t raw = 0;
        if (!reader.i32(raw)) return false;
        value.Val = raw;
        out.kind = ValueKind::DefIndex;
        out.integer = raw;
        return true;
    } else if constexpr (std::is_same_v<U, std::string>) {
        if (!reader.cstr(value)) return false;
        out.kind = ValueKind::String;
        out.text = value;
        return true;
    } else if constexpr (std::is_same_v<U, std::u16string>) {
        if (!reader.wstr(value)) return false;
        out.kind = ValueKind::WString;
        out.wide = value;
        return true;
    } else if constexpr (std::is_enum_v<U>) {
        std::int32_t raw = 0;
        if (!reader.i32(raw)) return false;
        value = static_cast<U>(raw);
        out.kind = ValueKind::Enum;
        out.integer = raw;
        return true;
    } else if constexpr (std::is_floating_point_v<U>) {
        std::uint32_t raw = 0;
        if (!reader.f32_bits(raw)) return false;
        std::memcpy(&value, &raw, sizeof(value));
        out.kind = ValueKind::Float;
        out.f32_bits = raw;
        return true;
    } else if constexpr (std::is_integral_v<U>) {
        if constexpr (std::is_same_v<U, std::uint8_t> || std::is_same_v<U, std::int8_t>) {
            std::uint8_t raw = 0;
            if (!reader.u8(raw)) return false;
            value = static_cast<U>(raw);
            out.kind = ValueKind::Int;
            out.integer = static_cast<std::int64_t>(value);
        } else if constexpr (std::is_same_v<U, std::uint16_t> || std::is_same_v<U, std::int16_t>) {
            std::uint16_t raw = 0;
            if (!reader.u16(raw)) return false;
            value = static_cast<U>(raw);
            out.kind = ValueKind::Int;
            out.integer = static_cast<std::int64_t>(value);
        } else {
            std::uint32_t raw = 0;
            if (!reader.u32(raw)) return false;
            value = static_cast<U>(raw);
            out.kind = ValueKind::Int;
            out.integer = static_cast<std::int64_t>(value);
        }
        return true;
    } else if constexpr (::fable::defs::compiler::detail::IsVector<U>::value) {
        return detail::read_vector(value, reader, out);
    } else if constexpr (::fable::defs::compiler::detail::IsList<U>::value ||
                         ::fable::defs::compiler::detail::IsSet<U>::value) {
        std::uint32_t count = 0;
        if (!reader.u32(count)) return false;
        out.kind = ValueKind::List;
        out.list.clear();
        out.list.reserve(count);
        using Element = typename U::value_type;
        for (std::uint32_t i = 0; i < count; ++i) {
            Element element{};
            Value item;
            if (!read_value(element, reader, item)) return false;
            out.list.push_back(std::move(item));
        }
        return true;
    } else if constexpr (::fable::defs::compiler::detail::IsMap<U>::value ||
                         ::fable::defs::compiler::detail::IsMultiMap<U>::value) {
        std::uint32_t count = 0;
        if (!reader.u32(count)) return false;
        out.kind = ValueKind::Map;
        out.map.clear();
        out.map.reserve(count);
        using Key = typename U::key_type;
        using Mapped = typename U::mapped_type;
        for (std::uint32_t i = 0; i < count; ++i) {
            Key key{};
            Mapped mapped{};
            Value key_value;
            Value mapped_value;
            if (!read_value(key, reader, key_value) ||
                !read_value(mapped, reader, mapped_value)) {
                return false;
            }
            out.map.emplace_back(std::move(key_value), std::move(mapped_value));
            value.emplace(key, mapped);
        }
        return true;
    } else if constexpr (::fable::defs::compiler::detail::IsPair<U>::value) {
        Value first;
        Value second;
        if (!read_value(value.first, reader, first) ||
            !read_value(value.second, reader, second)) {
            return false;
        }
        out.kind = ValueKind::Struct;
        out.fields.emplace_back("first", std::move(first));
        out.fields.emplace_back("second", std::move(second));
        return true;
    } else if constexpr (std::is_array_v<U>) {
        out.kind = ValueKind::List;
        out.list.clear();
        for (auto &element : value) {
            Value item;
            if (!read_value(element, reader, item)) return false;
            out.list.push_back(std::move(item));
        }
        return true;
    } else if constexpr (::fable::defs::compiler::detail::IsSharedPtr<U>::value) {
        if (!value) {
            value = std::make_shared<typename U::element_type>();
        }
        return read_value(*value, reader, out);
    } else if constexpr (std::is_same_v<U, ::fable::defs::CAnimationComponent>) {
        // Tag + the fields for that case, matching wire.hpp.
        std::uint32_t tag = 0;
        if (!reader.u32(tag)) return false;
        value.Tag = tag;
        out.kind = ValueKind::Variant;
        out.tag = tag;
        out.type_name = "CAnimationComponent";
        const auto add_i32 = [&](const char *name, std::int32_t &slot) {
            if (!reader.i32(slot)) return false;
            Value child;
            child.kind = ValueKind::Int;
            child.integer = slot;
            out.fields.emplace_back(name, std::move(child));
            return true;
        };
        const auto add_bool = [&](const char *name, bool &slot) {
            std::uint8_t byte = 0;
            if (!reader.u8(byte)) return false;
            slot = byte != 0;
            Value child;
            child.kind = ValueKind::Bool;
            child.boolean = slot;
            out.fields.emplace_back(name, std::move(child));
            return true;
        };
        const auto add_f32 = [&](const char *name, float &slot) {
            std::uint32_t raw = 0;
            if (!reader.f32_bits(raw)) return false;
            std::memcpy(&slot, &raw, sizeof(slot));
            Value child;
            child.kind = ValueKind::Float;
            child.f32_bits = raw;
            out.fields.emplace_back(name, std::move(child));
            return true;
        };
        const auto add_defstr = [&](const char *name, CDefString &slot) {
            std::int32_t raw = 0;
            if (!reader.i32(raw)) return false;
            slot.TablePos = raw;
            Value child;
            child.kind = ValueKind::DefString;
            child.integer = raw;
            out.fields.emplace_back(name, std::move(child));
            return true;
        };
        switch (tag) {
        case 0: return add_i32("Flags", value.Flags);
        case 1: return add_i32("TransitionInTime", value.TransitionInTime);
        case 2: return add_i32("Delay", value.Delay);
        case 3:
            return add_i32("StartHandedness", value.StartHandedness) &&
                   add_i32("EndHandedness", value.EndHandedness);
        case 4:
            return add_i32("ComboStage", value.ComboStage) &&
                   add_i32("ComboId", value.ComboId);
        case 5: return add_i32("RecoilAnimIndex", value.RecoilAnimIndex);
        case 6:
            return add_bool("MeleeFlourish", value.MeleeFlourish) &&
                   add_bool("MeleeKnockdown", value.MeleeKnockdown);
        case 7:
            return add_bool("TargetOffset", value.TargetOffset) &&
                   add_f32("TargetOffsetVectorX", value.TargetOffsetVectorX) &&
                   add_f32("TargetOffsetVectorY", value.TargetOffsetVectorY);
        case 8: return add_defstr("NextAnimName", value.NextAnimName);
        case 9: return add_f32("AnimationSpeedMultiplier", value.AnimationSpeedMultiplier);
        case 10: return add_i32("Mode", value.Mode);
        case 11: return add_defstr("NextFilter", value.NextFilter);
        case 12: {
            std::uint32_t length = 0;
            std::vector<std::uint8_t> bytes;
            if (!reader.u32(length) || !reader.bytes(length, bytes)) return false;
            value.ResponseAnimName.assign(bytes.begin(), bytes.end());
            Value child;
            child.kind = ValueKind::PString;
            child.bytes = std::move(bytes);
            out.fields.emplace_back("ResponseAnimName", std::move(child));
            return true;
        }
        default:
            return true;
        }
    } else if constexpr (std::is_same_v<U, ::fable::defs::CReactionMatchEntry>) {
        std::uint32_t tag = 0;
        std::int32_t reaction = 0;
        if (!reader.u32(tag) || !reader.i32(reaction)) return false;
        value.Tag = tag;
        value.ReactionType = reaction;
        out.kind = ValueKind::Variant;
        out.tag = tag;
        out.type_name = "CReactionMatchEntry";
        Value reaction_value;
        reaction_value.kind = ValueKind::Int;
        reaction_value.integer = reaction;
        out.fields.emplace_back("ReactionType", std::move(reaction_value));
        const auto add_i32 = [&](const char *name, std::int32_t &slot) {
            if (!reader.i32(slot)) return false;
            Value child;
            child.kind = ValueKind::Int;
            child.integer = slot;
            out.fields.emplace_back(name, std::move(child));
            return true;
        };
        const auto add_f32 = [&](const char *name, float &slot) {
            std::uint32_t raw = 0;
            if (!reader.f32_bits(raw)) return false;
            std::memcpy(&slot, &raw, sizeof(slot));
            Value child;
            child.kind = ValueKind::Float;
            child.f32_bits = raw;
            out.fields.emplace_back(name, std::move(child));
            return true;
        };
        switch (tag) {
        case 0:
            return add_i32("Axis", value.Axis) &&
                   add_f32("LowerBoundOnAxis", value.LowerBoundOnAxis) &&
                   add_f32("InvIntervalOnAxis", value.InvIntervalOnAxis) &&
                   add_f32("MShiftZero", value.MShiftZero) &&
                   add_f32("MShiftWeight", value.MShiftWeight) &&
                   add_f32("RShiftZero", value.RShiftZero) &&
                   add_f32("RShiftWeight", value.RShiftWeight);
        case 1:
            return add_i32("Axis", value.Axis) &&
                   add_f32("CentreOnAxis", value.CentreOnAxis) &&
                   add_f32("RadiusOnAxis", value.RadiusOnAxis) &&
                   add_f32("MShiftZero", value.MShiftZero) &&
                   add_f32("MShiftWeight", value.MShiftWeight) &&
                   add_f32("RShiftZero", value.RShiftZero) &&
                   add_f32("RShiftWeight", value.RShiftWeight);
        case 2:
            return add_i32("XAxisOpinionType", value.XAxisOpinionType) &&
                   add_i32("YAxisOpinionType", value.YAxisOpinionType) &&
                   add_f32("XCentre", value.XCentre) &&
                   add_f32("NegInvXRadius", value.NegInvXRadius) &&
                   add_f32("PosInvXRadius", value.PosInvXRadius) &&
                   add_f32("YCentre", value.YCentre) &&
                   add_f32("NegInvYRadius", value.NegInvYRadius) &&
                   add_f32("PosInvYRadius", value.PosInvYRadius);
        case 3:
            return add_i32("XAxisOpinionType", value.XAxisOpinionType) &&
                   add_i32("YAxisOpinionType", value.YAxisOpinionType) &&
                   add_f32("XCentre", value.XCentre) &&
                   add_f32("NegInvXRadius", value.NegInvXRadius) &&
                   add_f32("PosInvXRadius", value.PosInvXRadius) &&
                   add_f32("YCentre", value.YCentre) &&
                   add_f32("NegInvYRadius", value.NegInvYRadius) &&
                   add_f32("PosInvYRadius", value.PosInvYRadius) &&
                   add_f32("ScarinessShift", value.ScarinessShift) &&
                   add_f32("AgreeablenessShift", value.AgreeablenessShift) &&
                   add_f32("AttractivenessShift", value.AttractivenessShift);
        default:
            return true;
        }
    } else if constexpr (std::is_same_v<U, ::fable::defs::CReactionFrequencyTraits>) {
        std::uint32_t type = 0;
        if (!reader.u32(type)) return false;
        value.Type = type;
        out.kind = ValueKind::Variant;
        out.tag = type;
        out.type_name = "CReactionFrequencyTraits";
        const auto add_f32 = [&](const char *name, float &slot) {
            std::uint32_t raw = 0;
            if (!reader.f32_bits(raw)) return false;
            std::memcpy(&slot, &raw, sizeof(slot));
            Value child;
            child.kind = ValueKind::Float;
            child.f32_bits = raw;
            out.fields.emplace_back(name, std::move(child));
            return true;
        };
        const auto add_u32 = [&](const char *name, std::uint32_t &slot) {
            if (!reader.u32(slot)) return false;
            Value child;
            child.kind = ValueKind::Int;
            child.integer = slot;
            out.fields.emplace_back(name, std::move(child));
            return true;
        };
        const auto add_bool = [&](const char *name, bool &slot) {
            std::uint8_t byte = 0;
            if (!reader.u8(byte)) return false;
            slot = byte != 0;
            Value child;
            child.kind = ValueKind::Bool;
            child.boolean = slot;
            out.fields.emplace_back(name, std::move(child));
            return true;
        };
        switch (type) {
        case 1:
            return add_f32("MinWait", value.MinWait) &&
                   add_f32("WaitRange", value.WaitRange);
        case 2:
            return add_f32("LowFreqPerMin", value.LowFreqPerMin) &&
                   add_f32("HighLowFreqInterval", value.HighLowFreqInterval) &&
                   add_u32("FramesPerMinute", value.FramesPerMinute) &&
                   add_u32("RampUpTimeFrames", value.RampUpTimeFrames) &&
                   add_u32("CutoffIntervalFrames", value.CutoffIntervalFrames);
        case 3:
            return add_bool("AllowIndividualRepeats", value.AllowIndividualRepeats) &&
                   add_u32("MinGapFrames", value.MinGapFrames) &&
                   add_f32("CountRechargePerFrame", value.CountRechargePerFrame) &&
                   add_f32("CurrentAvailableCount", value.CurrentAvailableCount) &&
                   add_f32("MaxCount", value.MaxCount) &&
                   add_f32("InvMaxCount", value.InvMaxCount);
        default:
            return true;
        }
    } else if constexpr (std::is_same_v<U, ::fable::defs::CPhysicalPrimitiveInit>) {
        std::uint32_t type = 0;
        if (!reader.u32(type)) return false;
        value.Type = type;
        out.kind = ValueKind::Variant;
        out.tag = type;
        out.type_name = "CPhysicalPrimitiveInit";
        const auto add_f32 = [&](const char *name, float &slot) {
            std::uint32_t raw = 0;
            if (!reader.f32_bits(raw)) return false;
            std::memcpy(&slot, &raw, sizeof(slot));
            Value child;
            child.kind = ValueKind::Float;
            child.f32_bits = raw;
            out.fields.emplace_back(name, std::move(child));
            return true;
        };
        const auto add_defstr = [&](const char *name, CDefString &slot) {
            std::int32_t raw = 0;
            if (!reader.i32(raw)) return false;
            slot.TablePos = raw;
            Value child;
            child.kind = ValueKind::DefString;
            child.integer = raw;
            out.fields.emplace_back(name, std::move(child));
            return true;
        };
        if (type == 1 || type == 2) {
            if (!add_defstr("BaseName", value.BaseName) || !add_f32("Radius", value.Radius)) {
                return false;
            }
        }
        if (type == 2) {
            if (!add_f32("Height", value.Height)) return false;
        }
        return true;
    } else if constexpr (std::is_same_v<U, ::fable::defs::CParticleMorphs::CEntry>) {
        // Wire layout is MorphType + a 32-byte payload: Blending is stored as
        // an i32 in the retail format, not as the C++ model's bool.
        std::int32_t morph_type = 0;
        std::int32_t blending = 0;
        std::int32_t location = 0;
        std::int32_t location_name = 0;
        std::int32_t location_index = 0;
        std::int32_t particle_index = 0;
        std::int32_t second_particle_index = 0;
        std::uint32_t start_bits = 0;
        std::uint32_t end_bits = 0;
        if (!reader.i32(morph_type) || !reader.i32(blending) ||
            !reader.i32(location) || !reader.i32(location_name) ||
            !reader.i32(location_index) || !reader.i32(particle_index) ||
            !reader.i32(second_particle_index) || !reader.f32_bits(start_bits) ||
            !reader.f32_bits(end_bits)) {
            return false;
        }
        value.MorphType = static_cast<EHeroMorphType>(morph_type);
        value.Blending = blending != 0;
        value.Location = static_cast<EMeshLocationPoint>(location);
        value.LocationName.TablePos = location_name;
        value.LocationIndex = location_index;
        value.ParticleIndex = particle_index;
        value.SecondParticleIndex = second_particle_index;
        std::memcpy(&value.StartAtMorphStrength, &start_bits, sizeof(start_bits));
        std::memcpy(&value.EndAtMorphStrength, &end_bits, sizeof(end_bits));

        out.kind = ValueKind::Struct;
        out.fields.emplace_back("MorphType", Value{ValueKind::Enum, morph_type, 0, false,
                                                   {}, {}, {}, {}, {}, {}, 0, {}});
        out.fields.emplace_back("Blending", Value{ValueKind::Bool, 0, 0, blending != 0,
                                                  {}, {}, {}, {}, {}, {}, 0, {}});
        out.fields.emplace_back("Location", Value{ValueKind::Enum, location, 0, false,
                                                  {}, {}, {}, {}, {}, {}, 0, {}});
        out.fields.emplace_back("LocationName", Value{ValueKind::DefString, location_name, 0,
                                                      false, {}, {}, {}, {}, {}, {}, 0, {}});
        out.fields.emplace_back("LocationIndex", Value{ValueKind::Int, location_index, 0, false,
                                                       {}, {}, {}, {}, {}, {}, 0, {}});
        out.fields.emplace_back("ParticleIndex", Value{ValueKind::Int, particle_index, 0, false,
                                                       {}, {}, {}, {}, {}, {}, 0, {}});
        out.fields.emplace_back("SecondParticleIndex",
                                Value{ValueKind::Int, second_particle_index, 0, false, {}, {}, {},
                                      {}, {}, {}, 0, {}});
        out.fields.emplace_back("StartAtMorphStrength",
                                Value{ValueKind::Float, 0, start_bits, false, {}, {}, {}, {}, {},
                                      {}, 0, {}});
        out.fields.emplace_back("EndAtMorphStrength",
                                Value{ValueKind::Float, 0, end_bits, false, {}, {}, {}, {}, {},
                                      {}, 0, {}});
        return true;
    } else if constexpr (::fable::defs::is_model_class<U>::value) {
        if constexpr (::fable::defs::has_transfer<U>::value) {
            return detail::read_controls(value, reader, out);
        } else {
            return detail::read_bare(value, reader, out);
        }
    } else {
        out.kind = ValueKind::Opaque;
        out.type_name = "<unsupported>";
        return false;
    }
}

/// Parse one binary entry's typed body into a generic value tree.
bool parse_entry(const BinaryEntry &entry, ParsedEntry &out, std::string &error);

} // namespace fable::defs::compiler
