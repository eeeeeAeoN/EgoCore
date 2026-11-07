#include "fable/defs/compiler/wire.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>

#include <fable/defs.hpp>

using namespace fable::defs;
using namespace fable::defs::compiler;

namespace {

int failures = 0;

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << #condition      \
                      << "\n";                                                  \
            ++failures;                                                         \
        }                                                                       \
    } while (false)

void test_crc() {
    CHECK(crc32("RechargeTime") == 0xe1df2cb9u);
    CHECK(crc32("WoundMorphs") == 0x288ef3ffu);
    CHECK(crc32("Inventory") == 0xd5d5dc3bu);
}

void test_simple_def() {
    CAICreatureWillPowerIndicatorDef value{};
    value.RechargeTime = 2.5f;
    ByteWriter out;
    WireStats stats;
    serialize_def(value, out, stats);
    CHECK(stats.fields == 1);
    CHECK(stats.unsupported == 0);
    CHECK(out.data().size() == 8);
    if (out.data().size() == 8) {
        // 0xe1df2cb9 little-endian, then f32 2.5.
        CHECK(out.data()[0] == 0xb9);
        CHECK(out.data()[1] == 0x2c);
        CHECK(out.data()[2] == 0xdf);
        CHECK(out.data()[3] == 0xe1);
        CHECK(out.data()[4] == 0x00);
        CHECK(out.data()[5] == 0x00);
        CHECK(out.data()[6] == 0x20);
        CHECK(out.data()[7] == 0x40);
    }
}

void test_containers() {
    NUISystem::CUIDef value{};
    value.Children.push_back(5);
    value.Sprites[NUISystem::ETableSprites(0)] = 7;
    value.TextValue = u"Hi";
    ByteWriter out;
    WireStats stats;
    serialize_def(value, out, stats);
    CHECK(!out.data().empty());
    CHECK(stats.fields > 0);
    CHECK(stats.unsupported == 0);
}

void test_transfer_wire_names() {
    NUISystem::CUIStateDef value{};
    value.Position.X = 1.5f;
    value.Position.Y = 2.5f;
    ByteWriter out;
    WireStats stats;
    serialize_def(value, out, stats);
    CHECK(stats.unsupported == 0);
    CHECK(stats.fields == 14);
    CHECK(out.data().size() >= 16);
    if (out.data().size() >= 16) {
        auto read_u32 = [&](std::size_t offset) {
            return static_cast<std::uint32_t>(out.data()[offset]) |
                   (static_cast<std::uint32_t>(out.data()[offset + 1]) << 8) |
                   (static_cast<std::uint32_t>(out.data()[offset + 2]) << 16) |
                   (static_cast<std::uint32_t>(out.data()[offset + 3]) << 24);
        };
        CHECK(read_u32(0) == crc32("GraphicIndex"));
        CHECK(read_u32(8) == crc32("PositionX"));
        CHECK(read_u32(16) == crc32("PositionY"));
    }
}

void test_compound_no_ids() {
    CThingObjectDef value{};
    value.Graphic.BankIndex = 8033;
    value.Graphic.Type = static_cast<EEngineGraphicType>(3);
    ByteWriter out;
    WireStats stats;
    serialize_def(value, out, stats);
    // Compound members are written without control ids; the def fields still
    // have them.
    CHECK(stats.fields > 0);
    CHECK(stats.unsupported == 0);
}

} // namespace

int main() {
    test_crc();
    test_simple_def();
    test_containers();
    test_transfer_wire_names();
    test_compound_no_ids();
    if (failures) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "all wire tests passed\n";
    return 0;
}
