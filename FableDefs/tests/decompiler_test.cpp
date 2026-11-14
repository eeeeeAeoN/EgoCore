#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "fable/defs/compiler/build.hpp"
#include "fable/defs/compiler/decompiler.hpp"
#include "fable/defs/compiler/inflate.hpp"

namespace fs = std::filesystem;
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

void write_file(const fs::path &path, const std::string &text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << text;
}

void test_inflate() {
    const std::array<std::uint8_t, 16> compressed = {
        120, 1, 203, 72, 205, 201, 201, 87, 200, 64, 39, 1, 104, 3, 8, 177};
    std::vector<std::uint8_t> output;
    std::string error;
    CHECK(zlib_inflate(compressed.data(), compressed.size(), output, error));
    CHECK(std::string(output.begin(), output.end()) == "hello hello hello hello");
}

void test_build_and_decompile() {
    const fs::path corpus = fs::temp_directory_path() / "fable-defs-decompiler-corpus";
    const fs::path out_a = fs::temp_directory_path() / "fable-defs-decompiler-out-a";
    const fs::path out_b = fs::temp_directory_path() / "fable-defs-decompiler-out-b";
    fs::remove_all(corpus);
    fs::remove_all(out_a);
    fs::remove_all(out_b);
    write_file(corpus / "test.h",
               "enum EEngineGraphicType { ENGINE_GRAPHIC_STATIC_MESH = 1 };\n");
    write_file(corpus / "objects.def",
               "#definition OBJECT TEST_OBJECT\n"
               "    Health 5;\n"
               "    Graphic.Type ENGINE_GRAPHIC_STATIC_MESH;\n"
               "    Graphic.BankIndex 7;\n"
               "#end_definition\n");
    const auto report = build_directory(corpus, out_a);
    CHECK(report.ok);
    if (!report.ok) {
        std::cerr << report.log;
        return;
    }

    std::string text;
    std::string error;
    DecompileStats stats;
    CHECK(decompile_binary_to_text(out_a.string(), "game", corpus.string(), text, stats,
                                   error));
    CHECK(stats.definitions == 1);
    CHECK(text.find("#definition OBJECT TEST_OBJECT") != std::string::npos);
    CHECK(text.find("Health 5") != std::string::npos);
    CHECK(text.find("Graphic.Type ENGINE_GRAPHIC_STATIC_MESH") != std::string::npos);
    CHECK(text.find("Graphic.BankIndex 7") != std::string::npos);

    // A second build with one changed field must be detected by the diff.
    write_file(corpus / "objects.def",
               "#definition OBJECT TEST_OBJECT\n"
               "    Health 6;\n"
               "    Graphic.Type ENGINE_GRAPHIC_STATIC_MESH;\n"
               "    Graphic.BankIndex 7;\n"
               "#end_definition\n");
    const auto changed = build_directory(corpus, out_b);
    CHECK(changed.ok);
    if (!changed.ok) {
        std::cerr << changed.log;
        return;
    }

    std::string diff;
    DecompileStats diff_stats;
    CHECK(diff_binaries_to_text(out_a.string(), out_b.string(), corpus.string(), diff,
                                diff_stats, error));
    CHECK(diff_stats.modified == 1);
    CHECK(diff.find("Health 6") != std::string::npos);
    CHECK(diff.find("Health 5") == std::string::npos);

    fs::remove_all(corpus);
    fs::remove_all(out_a);
    fs::remove_all(out_b);
}

void test_specialises_parent_diff() {
    const fs::path corpus = fs::temp_directory_path() / "fable-defs-decompiler-specialises-corpus";
    const fs::path out_dir = fs::temp_directory_path() / "fable-defs-decompiler-specialises-out";
    fs::remove_all(corpus);
    fs::remove_all(out_dir);
    write_file(corpus / "test.h",
               "enum EMaterialType { MATERIAL_ROCK = 1 };\n");
    write_file(corpus / "buildings.tpl",
               "#definition_template BUILDING TEST_BUILDING_PARENT\n"
               "    Health 15000;\n"
               "    ReverbEnvironmentType REVERB_ENVIRONMENT_SMALL_ROOM;\n"
               "    Components.Add(\"CTCPhysicsStandard\");\n"
               "    Components.Add(\"CTCVillageMember\");\n"
               "    <CVillageMemberDef>\n"
               "        NeedsVillageToFunctionCorrectly TRUE;\n"
               "    <\\CVillageMemberDef>\n"
               "#end_definition\n"
               "\n"
               "#definition BUILDING TEST_BUILDING_CHILD specialises TEST_BUILDING_PARENT\n"
               "    FloorMaterial MATERIAL_ROCK;\n"
               "    Components.Add(\"CTCSmokeGenerator\");\n"
               "    <CVillageMemberDef>\n"
               "        NeedsVillageToFunctionCorrectly FALSE;\n"
               "    <\\CVillageMemberDef>\n"
               "#end_definition\n");

    const auto report = build_directory(corpus, out_dir);
    CHECK(report.ok);
    if (!report.ok) {
        std::cerr << report.log;
        return;
    }

    std::string text;
    std::string error;
    DecompileStats stats;
    CHECK(decompile_binary_to_text(out_dir.string(), "game", corpus.string(), text, stats,
                                   error));
    CHECK(text.find("REVERB_ENVIRONMENT_SMALL_ROOM") != std::string::npos);

    const std::size_t child_pos =
        text.find("#definition BUILDING TEST_BUILDING_CHILD specialises TEST_BUILDING_PARENT");
    CHECK(child_pos != std::string::npos);
    if (child_pos != std::string::npos) {
        const std::string child_block = text.substr(child_pos);
        CHECK(child_block.find("Health 15000") == std::string::npos);
        CHECK(child_block.find("Components.Add(\"CTCPhysicsStandard\")") == std::string::npos);
        CHECK(child_block.find("Components.Add(\"CTCSmokeGenerator\")") != std::string::npos);
        CHECK(child_block.find("NeedsVillageToFunctionCorrectly FALSE") != std::string::npos);
    }

    fs::remove_all(corpus);
    fs::remove_all(out_dir);
}

void test_roundtrip_fidelity_fixes() {
    const fs::path corpus = fs::temp_directory_path() / "fable-defs-decompiler-fidelity-corpus";
    const fs::path out_a = fs::temp_directory_path() / "fable-defs-decompiler-fidelity-out-a";
    const fs::path corpus_rt = fs::temp_directory_path() / "fable-defs-decompiler-fidelity-rt";
    const fs::path out_b = fs::temp_directory_path() / "fable-defs-decompiler-fidelity-out-b";
    fs::remove_all(corpus);
    fs::remove_all(out_a);
    fs::remove_all(corpus_rt);
    fs::remove_all(out_b);

    write_file(corpus / "test.h",
               "enum EBodyPartType { BODY_PART_HEAD = 0, BODY_PART_TORSO = 1 };\n"
               "enum EHeroMorphType { HERO_MORPH_WILL = 0, HERO_MORPH_GOOD = 1 };\n"
               "enum EMeshLocationPoint { MESH_LOCATION_POS = 0, MESH_LOCATION_DUMMY = 1 };\n");
    write_file(
        corpus / "defs.def",
        "#definition_template OBJECT BASE_OBJ\n"
        "    Components.Add(\"CTCPhysicsStandard\");\n"
        "#end_definition\n\n"
        "#definition BUILDING CHILD_BLDG specialises BASE_OBJ\n"
        "    Components.Add(\"CTCPhysicsStandard\");\n"
        "    Components.Add(\"CTCDegradableBarrier\");\n"
        "    <CDegradableDef>\n"
        "        Degradable TRUE;\n"
        "        GraphicType 5;\n"
        "        Degradations.Add(0.6, 42, 9, TRUE);\n"
        "    <\\CDegradableDef>\n"
        "#end_definition\n\n"
        "#definition OPINION_REACTION_MANAGER OPINION_REACTION_MANAGER_INSTANCE\n"
        "    Matches.Add(20, MRBlob(-0.2, 0.6, -1.9, -0.1, -0.25, -0.5, -0.5));\n"
        "    Matches.Add(22, Pyramid(4, -0.6, -0.1, 0.2, 3, 0.8, 0.95, 1.0));\n"
        "    Frequency.Set(29, ControlledCount(1, 5.0, 61.0, FALSE));\n"
        "    Frequency.Set(57, Linear(0.0, 120.0));\n"
        "#end_definition\n\n"
        "#definition_template CREATURE BASE_CREATURE\n"
        "    <CAppearanceDef>\n"
        "        Animation.Add(\"ST_RUN\", 1);\n"
        "    <\\CAppearanceDef>\n"
        "    <CHeroMorphDef>\n"
        "        ParticleMorphs.Add(HERO_MORPH_GOOD, 5, 0.7, 1.1, MESH_LOCATION_POS, \"\", 0);\n"
        "    <\\CHeroMorphDef>\n"
        "    <CCreatureDef>\n"
        "        RandomAppearanceMorph.AddBodyPart(BODY_PART_TORSO, 1, 10);\n"
        "    <\\CCreatureDef>\n"
        "#end_definition\n\n"
        "#definition CREATURE CHILD_CREATURE specialises BASE_CREATURE\n"
        "    <CAppearanceDef>\n"
        "        Animation.Remove(\"ST_RUN\");\n"
        "        Animation.Add(\"ST_RUN\", 2);\n"
        "    <\\CAppearanceDef>\n"
        "    <CHeroMorphDef>\n"
        "        ParticleMorphs.AddBlend(HERO_MORPH_WILL, 6, 7, 0.3, 1.0, MESH_LOCATION_DUMMY, \"WEAPON_FOCUS_01\", 0);\n"
        "    <\\CHeroMorphDef>\n"
        "    <CCreatureDef>\n"
        "        RandomAppearanceMorph.AddBodyPart(BODY_PART_HEAD, 1, 11);\n"
        "    <\\CCreatureDef>\n"
        "#end_definition\n\n"
        "#definition SOUND_SETUP SOUND_SETUP\n"
        "    ReverbEnvironmentEntries[0] CReverbEnvironmentEntry(-10000, -500, 0, 2.0);\n"
        "#end_definition\n");

    const auto report_a = build_directory(corpus, out_a);
    CHECK(report_a.ok);
    if (!report_a.ok) {
        std::cerr << report_a.log;
        return;
    }

    std::string text;
    std::string error;
    DecompileStats stats;
    CHECK(decompile_binary_to_text(out_a.string(), "game", corpus.string(), text, stats, error));
    CHECK(text.find("Animation.Remove(\"ST_RUN\")") != std::string::npos);
    CHECK(text.find("ParticleMorphs.AddBlend(HERO_MORPH_WILL") != std::string::npos);
    CHECK(text.find("Degradations.Add(0.6") != std::string::npos);
    CHECK(text.find("Frequency.Set(") != std::string::npos);

    write_file(corpus_rt / "test.h",
               "enum EBodyPartType { BODY_PART_HEAD = 0, BODY_PART_TORSO = 1 };\n"
               "enum EHeroMorphType { HERO_MORPH_WILL = 0, HERO_MORPH_GOOD = 1 };\n"
               "enum EMeshLocationPoint { MESH_LOCATION_POS = 0, MESH_LOCATION_DUMMY = 1 };\n");
    write_file(corpus_rt / "decompiled.def", text);

    const auto report_b = build_directory(corpus_rt, out_b);
    CHECK(report_b.ok);
    if (!report_b.ok) {
        std::cerr << report_b.log;
        return;
    }

    std::string diff;
    DecompileStats diff_stats;
    CHECK(diff_binaries_to_text(out_a.string(), out_b.string(), corpus.string(), diff,
                                diff_stats, error));
    if (diff_stats.modified != 0 || diff_stats.added != 0) {
        std::cerr << "fidelity diff:\n" << diff << "\n";
    }
    CHECK(diff_stats.modified == 0);
    CHECK(diff_stats.added == 0);

    fs::remove_all(corpus);
    fs::remove_all(out_a);
    fs::remove_all(corpus_rt);
    fs::remove_all(out_b);
}

void test_standalone_folder_and_parent_recovery() {
    const fs::path base = fs::temp_directory_path();
    const fs::path corpus = base / "fable_defs_folder_corpus";
    const fs::path bin_a = base / "fable_defs_folder_bin_a";
    const fs::path headers_only = base / "fable_defs_folder_headers";
    const fs::path folder_out = base / "fable_defs_folder_out";
    const fs::path bin_b = base / "fable_defs_folder_bin_b";
    fs::remove_all(corpus);
    fs::remove_all(bin_a);
    fs::remove_all(headers_only);
    fs::remove_all(folder_out);
    fs::remove_all(bin_b);

    const std::string mesh_h =
        "enum EMeshType2 { MESH_MOD_TEST_SWORD = 9001 };\n";
    write_file(corpus / "RetailHeaders" / "meshdata.h", mesh_h);
    write_file(headers_only / "RetailHeaders" / "meshdata.h", mesh_h);

    write_file(
        corpus / "game_test.def",
        "#definition_template BUILDING TEST_BLDG_BASE\n"
        "    <CVillageMemberDef>\n"
        "        Village \"Oakvale\";\n"
        "    <\\CVillageMemberDef>\n"
        "    <COccupiableDef>\n"
        "        MaxNumberOfResidents 4;\n"
        "    <\\COccupiableDef>\n"
        "#end_definition\n\n"
        "#definition BUILDING TEST_BLDG_CHILD specialises TEST_BLDG_BASE\n"
        "    <COccupiableDef>\n"
        "        MaxNumberOfResidents 8;\n"
        "    <\\COccupiableDef>\n"
        "#end_definition\n\n"
        "#definition OBJECT TEST_CUSTOM_SWORD\n"
        "    Graphic.Type ENGINE_GRAPHIC_STATIC_MESH;\n"
        "    Graphic.BankIndex MESH_MOD_TEST_SWORD;\n"
        "    <CWeaponDef>\n"
        "        Class WC_LIGHT;\n"
        "        DamageType DAMAGE_FIRE;\n"
        "    <\\CWeaponDef>\n"
        "#end_definition\n");

    write_file(corpus / "FrontEndDefs" / "fe_test.def",
               "#definition UI TEST_UI_ELEM\n"
               "    Type UI_TYPE_SPRITE;\n"
               "#end_definition\n");

    write_file(corpus / "ScriptDefs" / "script_test.def",
               "#definition CScriptDef TEST_SCRIPT_NODE\n"
               "    <CScriptDef>\n"
               "        KN_CreatureType CREATURE_GROUP_HERO;\n"
               "    <\\CScriptDef>\n"
               "#end_definition\n");

    const auto report_a = build_directory(corpus, bin_a);
    CHECK(report_a.ok);
    if (!report_a.ok) {
        std::cerr << report_a.log;
        return;
    }

    FolderDecompileStats folder_stats;
    std::string error;
    CHECK(decompile_directory_to_folder(
        bin_a.string(), headers_only.string(), folder_out.string(),
        folder_stats, error));
    CHECK(error.empty());
    CHECK(fs::exists(folder_out / "building.tpl"));
    CHECK(fs::exists(folder_out / "buildings.def"));
    CHECK(fs::exists(folder_out / "objects.def"));
    CHECK(fs::exists(folder_out / "FrontEndDefs" / "ui.def"));
    CHECK(fs::exists(folder_out / "ScriptDefs" / "scripts.def"));

    auto read_all = [](const fs::path &p) {
        std::ifstream in(p, std::ios::binary);
        return std::string{std::istreambuf_iterator<char>(in),
                           std::istreambuf_iterator<char>()};
    };
    const std::string buildings_txt = read_all(folder_out / "buildings.def");
    CHECK(buildings_txt.find("#definition BUILDING TEST_BLDG_CHILD specialises TEST_BLDG_BASE") !=
          std::string::npos);

    const std::string objects_txt = read_all(folder_out / "objects.def");
    CHECK(objects_txt.find("WC_LIGHT") != std::string::npos);
    CHECK(objects_txt.find("DAMAGE_FIRE") != std::string::npos);
    CHECK(objects_txt.find("MESH_MOD_TEST_SWORD") != std::string::npos);

    write_file(folder_out / "RetailHeaders" / "meshdata.h", mesh_h);
    const auto report_b = build_directory(folder_out, bin_b);
    CHECK(report_b.ok);
    if (!report_b.ok) {
        std::cerr << report_b.log;
        return;
    }

    std::string diff;
    DecompileStats diff_stats;
    CHECK(diff_binaries_to_text(bin_a.string(), bin_b.string(),
                                headers_only.string(), diff, diff_stats, error));
    CHECK(diff_stats.modified == 0);
    CHECK(diff_stats.added == 0);

    fs::remove_all(corpus);
    fs::remove_all(bin_a);
    fs::remove_all(headers_only);
    fs::remove_all(folder_out);
    fs::remove_all(bin_b);
}

} // namespace

int main() {
    test_inflate();
    test_build_and_decompile();
    test_specialises_parent_diff();
    test_roundtrip_fidelity_fixes();
    test_standalone_folder_and_parent_recovery();
    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "all decompiler tests passed\n";
    return 0;
}
