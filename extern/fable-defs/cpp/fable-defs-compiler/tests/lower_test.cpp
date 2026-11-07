#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <fable/defs/visit.hpp>

#include "fable/defs/compiler/definitions.hpp"
#include "fable/defs/compiler/lower.hpp"
#include "fable/defs/compiler/names.hpp"
#include "fable/defs/compiler/parser.hpp"
#include "fable/defs/compiler/symbols.hpp"

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

SourceAst parse_ast(std::string_view source) {
    ParseResult result = parse_source(source, FileId{1});
    if (auto *error = std::get_if<ParseError>(&result)) {
        std::cerr << "parse error: " << error->message << "\n";
        ++failures;
        return {};
    }
    return std::move(std::get<SourceAst>(result));
}

Definition parse_def(std::string_view body, std::string_view type = "TEST") {
    std::string source = "#definition " + std::string(type) + " T\n" +
                         std::string(body) + "\n#end_definition";
    ParseResult result = parse_source(source, FileId{1});
    if (auto *error = std::get_if<ParseError>(&result)) {
        std::cerr << "parse error: " << error->message << "\n";
        ++failures;
        return {};
    }
    const SourceAst &ast = std::get<SourceAst>(result);
    return ast.items[0].definition.value;
}

SymbolTable parse_symbols(std::string_view source) {
    SymbolTable table;
    ParseResult result = parse_source(source, FileId{2});
    if (auto *error = std::get_if<ParseError>(&result)) {
        std::cerr << "symbol parse error: " << error->message << "\n";
        ++failures;
        return table;
    }
    std::vector<Redefinition> redefined;
    if (auto error = table.evaluate_items(std::get<SourceAst>(result).items, redefined)) {
        std::cerr << "symbol error: " << error->message << "\n";
        ++failures;
    }
    return table;
}

void test_scalar() {
    CAICreatureWillPowerIndicatorDef value{};
    LowerStats stats;
    lower_definition(parse_def("RechargeTime 2.5;"), value, SymbolTable{}, stats);
    CHECK(stats.errors.empty());
    CHECK(value.RechargeTime == 2.5f);
    CHECK(stats.applied == 1);
}

void test_inherited_field() {
    CThingObjectDef value{};
    LowerStats stats;
    lower_definition(parse_def("Health 5.5; RotationTime 2;"), value, SymbolTable{},
                     stats);
    CHECK(stats.errors.empty());
    CHECK(value.Health == 5.5f);
    CHECK(value.RotationTime == 2.0f);
    CHECK(stats.applied == 2);
}

void test_bitfield_and_strings() {
    CCreatureDef value{};
    LowerStats stats;
    lower_definition(parse_def("Short TRUE; Inventory 7; BattleCrySound 42;"),
                     value, SymbolTable{}, stats);
    CHECK(stats.errors.empty());
    CHECK(value.Short);
    CHECK(value.Inventory == 7);
    CHECK(value.BattleCrySound.TablePos == 42);
    CHECK(stats.applied == 3);

    CAIScratchpadDef scratch{};
    LowerStats string_stats;
    lower_definition(parse_def("Action \"Walk\";"), scratch, SymbolTable{},
                     string_stats);
    CHECK(string_stats.errors.empty());
    CHECK(scratch.Action == "Walk");

    NUISystem::CUIDef ui{};
    LowerStats ui_stats;
    lower_definition(parse_def("TextValue \"Hi\";"), ui, SymbolTable{}, ui_stats);
    CHECK(ui_stats.errors.empty());
    CHECK(ui.TextValue == u"Hi");
}

void test_enum_and_symbols() {
    SymbolTable symbols = parse_symbols("enum EHeroAbility { HERO_ABILITY_TEST = 3 };");
    CAbilityDef value{};
    LowerStats stats;
    lower_definition(parse_def("Ability HERO_ABILITY_TEST;"), value, symbols, stats);
    CHECK(stats.errors.empty());
    CHECK(static_cast<int>(value.Ability) == 3);
}

void test_specialization_and_references() {
    SourceAst base = parse_ast("#definition TEST BASE\nHealth 1;\n#end_definition");
    SourceAst child =
        parse_ast("#definition TEST CHILD specialises BASE\nMinHealth 2;\n"
                  "#end_definition");
    SourceAst group = parse_ast("#definition GROUP_X GROUP_X\n#end_definition");

    DefinitionIndex index;
    index.add(base);
    index.add(child);
    index.add(group);

    auto chain_result = specialization_chain(index, "CHILD");
    CHECK(std::holds_alternative<Chain>(chain_result));
    if (!std::holds_alternative<Chain>(chain_result)) {
        return;
    }
    const Chain &chain = std::get<Chain>(chain_result);
    CHECK(chain.size() == 2);
    CHECK(chain[0]->name == "BASE");
    CHECK(chain[1]->name == "CHILD");

    Definition flattened = flatten_chain(chain);
    CHECK(flattened.body.size() == 2);

    CThingObjectDef value{};
    LowerStats stats;
    lower_definition(flattened, value, SymbolTable{}, stats, nullptr, &index);
    CHECK(stats.errors.empty());
    CHECK(value.Health == 1.0f);
    CHECK(value.MinHealth == 2.0f);

    CThingObjectDef references{};
    LowerStats reference_stats;
    lower_definition(parse_def("GroupDef GROUP_X;"), references, SymbolTable{},
                     reference_stats, nullptr, &index);
    CHECK(reference_stats.errors.empty());
    CHECK(index.index_of("GROUP_X").has_value());
    CHECK(references.GroupDef == *index.index_of("GROUP_X"));
}

void test_subpaths_and_containers() {
    CThingObjectDef thing{};
    LowerStats thing_stats;
    lower_definition(parse_def("Graphic.Type 3; Graphic.BankIndex 42;"), thing,
                     SymbolTable{}, thing_stats);
    CHECK(thing_stats.errors.empty());
    CHECK(thing.Graphic.Type == 3);
    CHECK(thing.Graphic.BankIndex == 42);

    NUISystem::CUIDef ui{};
    LowerStats ui_stats;
    lower_definition(
        parse_def("States[0].UpdateTime 1.5; States[0].LinearChange TRUE;"
                  "Sprites[0] 7; Children.Add(5);"),
        ui, SymbolTable{}, ui_stats);
    CHECK(ui_stats.errors.empty());
    CHECK(ui.States.size() == 1);
    CHECK(ui.States[0].UpdateTime == 1.5f);
    CHECK(ui.States[0].LinearChange);
    CHECK(ui.Sprites[NUISystem::ETableSprites(0)] == 7);
    CHECK(ui.Children.size() == 1);
    CHECK(ui.Children[0] == 5);
}

void test_transfer_names_and_defaults() {
    NUISystem::CUIStateDef state{};
    LowerStats state_stats;
    lower_definition(parse_def("PositionX 1.5; PositionY 2.5; ColourR 0.25;"), state,
                     SymbolTable{}, state_stats);
    CHECK(state_stats.errors.empty());
    CHECK(state.Position.X == 1.5f);
    CHECK(state.Position.Y == 2.5f);
    CHECK(state.Colour.R == 0.25f);

    CThingObjectDef thing{};
    LowerStats thing_stats;
    lower_definition(parse_def(""), thing, SymbolTable{}, thing_stats);
    CHECK(thing_stats.errors.empty());
    CHECK(thing.AvailableInEditor);
    CHECK(thing.AlwaysFrameUpdate);
    CHECK(thing.DrawWeaponTrails);
    CHECK(thing.RenderFadeDistance == 64.0f);
    CHECK(thing.MeshHeight == 2.0f);
    CHECK(thing.ApproxMaxMeshHeight == 2.0f);
    CHECK(thing.MinHealth == -0.1f);

    CCreatureDef creature{};
    LowerStats creature_stats;
    lower_definition(parse_def(""), creature, SymbolTable{}, creature_stats);
    CHECK(creature_stats.errors.empty());
    CHECK(creature.AllowedInProtectedTowns); // Transfer default: true
    CHECK(creature.Inventory == 0);
}

void test_names_layout() {
    NamesBuilder names;
    CHECK(names.intern("A") == 4);
    CHECK(names.intern("BB") == 10); // 4-byte CRC + "A\0" + 4-byte CRC
    CHECK(names.intern("A") == 4);
    CHECK(names.size() == 2);
    std::array<std::uint8_t, 20> header{};
    auto bytes = names.to_bytes(header);
    CHECK(bytes.size() == 20 + (4 + 1 + 1) + (4 + 2 + 1));
    CHECK(bytes[20 + 4] == 'A');
    const auto read_u32 = [&](std::size_t at) {
        return static_cast<std::uint32_t>(bytes[at]) |
               (static_cast<std::uint32_t>(bytes[at + 1]) << 8) |
               (static_cast<std::uint32_t>(bytes[at + 2]) << 16) |
               (static_cast<std::uint32_t>(bytes[at + 3]) << 24);
    };
    CHECK(read_u32(8) == 2);
    CHECK(read_u32(12) == 4 + 13);
}

void test_names_interning() {
    CThingObjectDef thing{};
    NamesBuilder names;
    LowerStats stats;
    lower_definition(parse_def("DefaultScriptName \"test_script\";"), thing,
                     SymbolTable{}, stats, nullptr, nullptr, &names);
    CHECK(stats.errors.empty());
    CHECK(names.size() == 1);
    CHECK(names.find("test_script").has_value());
    CHECK(thing.DefaultScriptName.TablePos ==
          static_cast<std::int32_t>(*names.find("test_script")));
}

void test_random_appearance_morph() {
    NamesBuilder names;
    CCreatureDef creature{};
    LowerStats stats;
    lower_definition(
        parse_def(
            "RandomAppearanceMorph.AddBodyPart(0, 1, 10, 11);"
            "RandomAppearanceMorph.AddTextureToMesh(10, 3, 20650, 20750);"
            "RandomAppearanceMorph.AddTextureToMesh(10, 2, 20600, 20700, 20800);"
            "RandomAppearanceMorph.AddTextureToAll(100, 101, 102);"
            "RandomAppearanceMorph.AddSkeletalMorph(1, \"bs_male_short.bncfg\", TRUE);"),
        creature, SymbolTable{}, stats, nullptr, nullptr, &names);
    CHECK(stats.errors.empty());

    const CRandomAppearanceMorph &ram = *creature.PRandomAppearanceMorph;
    CHECK(ram.BodyParts0.Meshes.size() == 2);
    CHECK(ram.FinalTrailing == 3);

    const auto read_i32 = [](const std::uint8_t *bytes, std::size_t at) {
        std::int32_t value = 0;
        std::memcpy(&value, bytes + at, 4);
        return value;
    };
    const CRandomAppearanceMorph::CBodyPart0Mesh &mesh = ram.BodyParts0.Meshes.at(0);
    CHECK(mesh.MeshId == 10);
    CHECK(mesh.TextureId == 1);
    CHECK(mesh.TextureMorphs.Trailing == 3);
    CHECK(mesh.TextureMorphs.Entries.size() == 3);
    // Groups follow authored order: 20650 before 20600, not sorted by id.
    CHECK(read_i32(mesh.TextureMorphs.Entries[0].second.Morph, 0) == 20650);
    CHECK(read_i32(mesh.TextureMorphs.Entries[0].second.Morph, 4) == 20750);
    CHECK(read_i32(mesh.TextureMorphs.Entries[1].second.Morph, 0) == 20600);
    CHECK(read_i32(mesh.TextureMorphs.Entries[1].second.Morph, 4) == 20700);
    CHECK(read_i32(mesh.TextureMorphs.Entries[2].second.Morph, 0) == 20600);
    CHECK(read_i32(mesh.TextureMorphs.Entries[2].second.Morph, 4) == 20800);

    CHECK(ram.GeneralTextureMorphs.Entries.size() == 2);
    CHECK(ram.GeneralTextureMorphs.Trailing == 0);
    CHECK(ram.GeneralTextureMorphs.Dirty == 0);
    CHECK(read_i32(ram.GeneralTextureMorphs.Entries[0].second.Morph, 0) == 100);
    CHECK(read_i32(ram.GeneralTextureMorphs.Entries[0].second.Morph, 4) == 101);
    CHECK(read_i32(ram.GeneralTextureMorphs.Entries[1].second.Morph, 0) == 100);
    CHECK(read_i32(ram.GeneralTextureMorphs.Entries[1].second.Morph, 4) == 102);

    CHECK(ram.SkeletalMorphs.size() == 1);
    CHECK(ram.SkeletalMorphs[0].Key == 1);
    CHECK(ram.SkeletalMorphs[0].MorphId == 1);
    CHECK(ram.SkeletalMorphs[0].BoneOrMorphIndex ==
          static_cast<std::int32_t>(*names.find("bs_male_short.bncfg")));
    CHECK(ram.SkeletalMorphs[0].AllowVariation);

    // Defaults restored by the EgoCore patch: these structs carry the
    // container-size sentinel used by the retail compiler, not zero.
    CHECK(creature.PExpressions->TrailingU32 == 28011826);
    CHECK(creature.PWoundMorphs->TrailingU32 == 28011758);
    CHeroMorphDef hero{};
    LowerStats hero_stats;
    lower_definition(parse_def(""), hero, SymbolTable{}, hero_stats);
    CHECK(hero_stats.errors.empty());
    CHECK(hero.TextureMorphs.TrailingU32 == 28011707);
    CHECK(hero.ParticleMorphs.TrailingU32 == 28011708);
    CHECK(hero.IdleParticleMorphs.TrailingU32 == 28011708);
}

void test_reference_fields() {
    // The EgoCore patch retyped the VillagePeople creature slots as references.
    // The C++ model keeps them as integers, so a bare definition name must
    // resolve through the definition index exactly like a CDefIndex field.
    SourceAst group = parse_ast("#definition GROUP_X GROUP_X\n#end_definition");
    DefinitionIndex index;
    index.add(group);
    CVillagePeopleDef people{};
    LowerStats stats;
    lower_definition(
        parse_def("MaleShopKeeper GROUP_X; FemaleBarman GROUP_X;"), people,
        SymbolTable{}, stats, nullptr, &index);
    CHECK(stats.errors.empty());
    CHECK(people.MaleShopKeeper == *index.index_of("GROUP_X"));
    CHECK(people.FemaleBarman == *index.index_of("GROUP_X"));
}

void test_unsupported_and_unknown() {
    CCreatureDef value{};
    LowerStats stats;
    lower_definition(parse_def("InitialActions 1;"), value, SymbolTable{}, stats);
    CHECK(stats.errors.empty());
    CHECK(stats.unsupported == 1);

    CAIScratchpadDef scratch{};
    LowerStats unknown;
    lower_definition(parse_def("Action NOPE;"), scratch, SymbolTable{}, unknown);
    CHECK(unknown.errors.size() == 1);
}

} // namespace

int main() {
    test_scalar();
    test_inherited_field();
    test_bitfield_and_strings();
    test_enum_and_symbols();
    test_specialization_and_references();
    test_subpaths_and_containers();
    test_transfer_names_and_defaults();
    test_names_layout();
    test_names_interning();
    test_random_appearance_morph();
    test_reference_fields();
    test_unsupported_and_unknown();
    if (failures) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "all lowering tests passed\n";
    return 0;
}
