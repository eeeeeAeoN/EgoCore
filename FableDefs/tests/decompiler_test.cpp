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
    write_file(corpus / "objects.def",
               "#definition OBJECT TEST_OBJECT\n"
               "    Health 5;\n"
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
    CHECK(decompile_binary_to_text(out_a.string(), "game", text, stats, error));
    CHECK(stats.definitions == 1);
    CHECK(text.find("#definition OBJECT TEST_OBJECT") != std::string::npos);
    CHECK(text.find("Health 5") != std::string::npos);
    CHECK(text.find("Graphic.BankIndex 7") != std::string::npos);

    // A second build with one changed field must be detected by the diff.
    write_file(corpus / "objects.def",
               "#definition OBJECT TEST_OBJECT\n"
               "    Health 6;\n"
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
    CHECK(diff_binaries_to_text(out_a.string(), out_b.string(), diff, diff_stats,
                                error));
    CHECK(diff_stats.modified == 1);
    CHECK(diff.find("Health 6") != std::string::npos);
    CHECK(diff.find("Health 5") == std::string::npos);

    fs::remove_all(corpus);
    fs::remove_all(out_a);
    fs::remove_all(out_b);
}

} // namespace

int main() {
    test_inflate();
    test_build_and_decompile();
    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "all decompiler tests passed\n";
    return 0;
}
