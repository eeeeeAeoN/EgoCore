// Wire serialization for the C++ def model.
#pragma once

#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <string_view>
#include <type_traits>
#include <vector>

#include <fable/defs/visit.hpp>
#include <fable/defs/transfer.hpp>

#include "fable/defs/compiler/ast.hpp"
#include "fable/defs/compiler/type_traits.hpp"

namespace fable::defs::compiler {

/// `CCharString::GetCRC`: zlib polynomial, init 0, no final xor.
std::uint32_t crc32(std::string_view text);

struct WireStats {
    std::size_t fields = 0;
    std::size_t unsupported = 0;
    std::vector<std::string> errors;
};

class ByteWriter {
public:
    void u8(std::uint8_t value) { data_.push_back(value); }
    void i8(std::int8_t value) { data_.push_back(static_cast<std::uint8_t>(value)); }
    void u16(std::uint16_t value) {
        data_.push_back(static_cast<std::uint8_t>(value));
        data_.push_back(static_cast<std::uint8_t>(value >> 8));
    }
    void u32(std::uint32_t value) {
        data_.push_back(static_cast<std::uint8_t>(value));
        data_.push_back(static_cast<std::uint8_t>(value >> 8));
        data_.push_back(static_cast<std::uint8_t>(value >> 16));
        data_.push_back(static_cast<std::uint8_t>(value >> 24));
    }
    void i32(std::int32_t value) { u32(static_cast<std::uint32_t>(value)); }
    void u64(std::uint64_t value) {
        u32(static_cast<std::uint32_t>(value));
        u32(static_cast<std::uint32_t>(value >> 32));
    }
    void f32(float value) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        u32(bits);
    }
    void f64(double value) {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        u64(bits);
    }
    void bytes(const void *data, std::size_t size) {
        if (size == 0) {
            return;
        }
        const auto *bytes = static_cast<const std::uint8_t *>(data);
        data_.insert(data_.end(), bytes, bytes + size);
    }
    void cstr(const std::string &value) {
        bytes(value.data(), value.size());
        u8(0);
    }
    void wstr(const std::u16string &value) {
        for (char16_t ch : value) {
            u16(static_cast<std::uint16_t>(ch));
        }
        u16(0);
    }

    const std::vector<std::uint8_t> &data() const { return data_; }
    std::vector<std::uint8_t> take() { return std::move(data_); }

private:
    std::vector<std::uint8_t> data_;
};

namespace detail {

template <class T>
void write_value(const T &value, ByteWriter &out, WireStats &stats);

class DefVisitor {
public:
    DefVisitor(ByteWriter &out, WireStats &stats) : out_(out), stats_(stats) {}

    template <class F>
    void operator()(const char *name, F &field) {
        ByteWriter temp;
        WireStats field_stats;
        write_value(field, temp, field_stats);
        if (field_stats.unsupported != 0) {
            stats_.unsupported += field_stats.unsupported;
            return;
        }
        out_.u32(crc32(name));
        out_.bytes(temp.data().data(), temp.data().size());
        ++stats_.fields;
    }

    template <class F>
    void operator()(const char *name, ::fable::defs::detail::BitfieldRef<F> ref) {
        out_.u32(crc32(name));
        ++stats_.fields;
        out_.u8(ref.get() ? 1 : 0);
    }

private:
    ByteWriter &out_;
    WireStats &stats_;
};

class CompoundVisitor {
public:
    CompoundVisitor(ByteWriter &out, WireStats &stats) : out_(out), stats_(stats) {}

    template <class F>
    void operator()(const char *, F &field) {
        write_value(field, out_, stats_);
    }

    template <class F>
    void operator()(const char *, ::fable::defs::detail::BitfieldRef<F> ref) {
        out_.u8(ref.get() ? 1 : 0);
    }

private:
    ByteWriter &out_;
    WireStats &stats_;
};

template <class T>
void write_value(const T &value, ByteWriter &out, WireStats &stats) {
    using U = std::remove_cv_t<T>;
    if constexpr (std::is_same_v<U, bool>) {
        out.u8(value ? 1 : 0);
    } else if constexpr (std::is_same_v<U, std::uint8_t>) {
        out.u8(value);
    } else if constexpr (std::is_same_v<U, std::int8_t>) {
        out.i8(value);
    } else if constexpr (std::is_same_v<U, std::uint16_t>) {
        out.u16(value);
    } else if constexpr (std::is_same_v<U, std::int16_t>) {
        out.u16(static_cast<std::uint16_t>(value));
    } else if constexpr (std::is_same_v<U, std::uint32_t>) {
        out.u32(value);
    } else if constexpr (std::is_same_v<U, std::int32_t>) {
        out.i32(value);
    } else if constexpr (std::is_same_v<U, std::uint64_t>) {
        out.u64(value);
    } else if constexpr (std::is_same_v<U, std::int64_t>) {
        out.u64(static_cast<std::uint64_t>(value));
    } else if constexpr (std::is_same_v<U, float>) {
        out.f32(value);
    } else if constexpr (std::is_same_v<U, double>) {
        out.f64(value);
    } else if constexpr (std::is_same_v<U, CDefString>) {
        out.i32(value.TablePos);
    } else if constexpr (std::is_same_v<U, CDefIndex>) {
        out.i32(value.Val);
    } else if constexpr (std::is_enum_v<U>) {
        out.i32(static_cast<std::int32_t>(value));
    } else if constexpr (std::is_same_v<U, std::string>) {
        out.cstr(value);
    } else if constexpr (std::is_same_v<U, std::u16string>) {
        out.wstr(value);
    } else if constexpr (IsVector<U>::value) {
        out.u32(static_cast<std::uint32_t>(value.size()));
        if constexpr (std::is_same_v<typename IsVector<U>::value_type, bool>) {
            for (bool item : value) {
                write_value<bool>(item, out, stats);
            }
        } else {
            for (const auto &item : value) {
                write_value(item, out, stats);
            }
        }
    } else if constexpr (IsList<U>::value || IsSet<U>::value) {
        out.u32(static_cast<std::uint32_t>(value.size()));
        for (const auto &item : value) {
            write_value(item, out, stats);
        }
    } else if constexpr (IsMap<U>::value) {
        out.u32(static_cast<std::uint32_t>(value.size()));
        for (const auto &[key, mapped] : value) {
            write_value(key, out, stats);
            write_value(mapped, out, stats);
        }
    } else if constexpr (IsMultiMap<U>::value) {
        out.u32(static_cast<std::uint32_t>(value.size()));
        for (const auto &[key, mapped] : value) {
            write_value(key, out, stats);
            write_value(mapped, out, stats);
        }
    } else if constexpr (IsPair<U>::value) {
        write_value(value.first, out, stats);
        write_value(value.second, out, stats);
    } else if constexpr (std::is_array_v<U>) {
        for (const auto &item : value) {
            write_value(item, out, stats);
        }
    } else if constexpr (std::is_same_v<U, ::fable::defs::CReactionMatchEntry>) {
        out.u32(value.Tag);
        out.i32(value.ReactionType);
        switch (value.Tag) {
        case 0:
            out.i32(value.Axis);
            out.f32(value.LowerBoundOnAxis);
            out.f32(value.InvIntervalOnAxis);
            out.f32(value.MShiftZero);
            out.f32(value.MShiftWeight);
            out.f32(value.RShiftZero);
            out.f32(value.RShiftWeight);
            break;
        case 1:
            out.i32(value.Axis);
            out.f32(value.CentreOnAxis);
            out.f32(value.RadiusOnAxis);
            out.f32(value.MShiftZero);
            out.f32(value.MShiftWeight);
            out.f32(value.RShiftZero);
            out.f32(value.RShiftWeight);
            break;
        case 2:
            out.i32(value.XAxisOpinionType);
            out.i32(value.YAxisOpinionType);
            out.f32(value.XCentre);
            out.f32(value.NegInvXRadius);
            out.f32(value.PosInvXRadius);
            out.f32(value.YCentre);
            out.f32(value.NegInvYRadius);
            out.f32(value.PosInvYRadius);
            break;
        case 3:
            out.i32(value.XAxisOpinionType);
            out.i32(value.YAxisOpinionType);
            out.f32(value.XCentre);
            out.f32(value.NegInvXRadius);
            out.f32(value.PosInvXRadius);
            out.f32(value.YCentre);
            out.f32(value.NegInvYRadius);
            out.f32(value.PosInvYRadius);
            out.f32(value.ScarinessShift);
            out.f32(value.AgreeablenessShift);
            out.f32(value.AttractivenessShift);
            break;
        default:
            break;
        }
    } else if constexpr (std::is_same_v<U, ::fable::defs::CAnimationComponent>) {
        out.u32(value.Tag);
        switch (value.Tag) {
        case 0:
            out.i32(value.Flags);
            break;
        case 1:
            out.i32(value.TransitionInTime);
            break;
        case 2:
            out.i32(value.Delay);
            break;
        case 3:
            out.i32(value.StartHandedness);
            out.i32(value.EndHandedness);
            break;
        case 4:
            out.i32(value.ComboStage);
            out.i32(value.ComboId);
            break;
        case 5:
            out.i32(value.RecoilAnimIndex);
            break;
        case 6:
            out.u8(value.MeleeFlourish ? 1 : 0);
            out.u8(value.MeleeKnockdown ? 1 : 0);
            break;
        case 7:
            out.u8(value.TargetOffset ? 1 : 0);
            out.f32(value.TargetOffsetVectorX);
            out.f32(value.TargetOffsetVectorY);
            break;
        case 8:
            out.i32(value.NextAnimName.TablePos);
            break;
        case 9:
            out.f32(value.AnimationSpeedMultiplier);
            break;
        case 10:
            out.i32(value.Mode);
            break;
        case 11:
            out.i32(value.NextFilter.TablePos);
            break;
        case 12:
            out.u32(static_cast<std::uint32_t>(value.ResponseAnimName.size()));
            out.bytes(value.ResponseAnimName.data(), value.ResponseAnimName.size());
            break;
        default:
            break;
        }
    } else if constexpr (std::is_same_v<U, ::fable::defs::CReactionFrequencyTraits>) {
        out.u32(value.Type);
        switch (value.Type) {
        case 1:
            out.f32(value.MinWait);
            out.f32(value.WaitRange);
            break;
        case 2:
            out.f32(value.LowFreqPerMin);
            out.f32(value.HighLowFreqInterval);
            out.u32(value.FramesPerMinute);
            out.u32(value.RampUpTimeFrames);
            out.u32(value.CutoffIntervalFrames);
            break;
        case 3:
            out.u8(value.AllowIndividualRepeats ? 1 : 0);
            out.u32(value.MinGapFrames);
            out.f32(value.CountRechargePerFrame);
            out.f32(value.CurrentAvailableCount);
            out.f32(value.MaxCount);
            out.f32(value.InvMaxCount);
            break;
        default:
            break;
        }
    } else if constexpr (std::is_same_v<U, ::fable::defs::CPhysicalPrimitiveInit>) {
        out.u32(value.Type);
        if (value.Type == 1 || value.Type == 2) {
            out.i32(value.BaseName.TablePos);
            out.f32(value.Radius);
        }
        if (value.Type == 2) {
            out.f32(value.Height);
        }
    } else if constexpr (::fable::defs::is_model_class<U>::value) {
        // `DefStruct` values carry field controls even when nested (e.g. a
        // `CUIStateDef` inside a vector); `WireStruct` values are bare members
        // (e.g. `C2DVector`). Transfer metadata is the discriminator we have:
        // DefStruct classes have a lifted `Transfer`, WireStructs do not.
        auto &mutable_value = const_cast<U &>(value);
        if constexpr (::fable::defs::has_transfer<U>::value) {
            DefVisitor visitor(out, stats);
            ::fable::defs::visit_model_fields(mutable_value, visitor);
        } else {
            CompoundVisitor visitor(out, stats);
            ::fable::defs::visit_model_fields(mutable_value, visitor);
        }
    } else if constexpr (IsSharedPtr<U>::value) {
        ++stats.unsupported;
    } else {
        ++stats.unsupported;
    }
}

} // namespace detail

namespace detail {

} // namespace detail

/// Serialize a def class as `(u32 crc32(field name), value)` controls.
template <class T>
void serialize_def(T &value, ByteWriter &out, WireStats &stats) {
    detail::DefVisitor visitor(out, stats);
    ::fable::defs::visit_model_fields(value, visitor);
}

} // namespace fable::defs::compiler
