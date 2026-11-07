#include "fable/defs/compiler/build.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using fable::defs::compiler::build_directory;

namespace {

void write_file(const fs::path &path, const std::string &text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << text;
}

} // namespace

int main() {
    const fs::path missing =
        fs::temp_directory_path() / "fable-defs-build-test-missing";
    const fs::path out = fs::temp_directory_path() / "fable-defs-build-test-out";
    fs::remove_all(missing);
    fs::remove_all(out);

    const auto absent = build_directory(missing, out);
    if (absent.ok || absent.log.find("input directory") == std::string::npos) {
        return 1;
    }

    fs::create_directories(missing);
    const auto empty = build_directory(missing, out);
    fs::remove_all(missing);
    fs::remove_all(out);
    if (empty.ok || empty.log.find("no .def") == std::string::npos) {
        return 2;
    }

    // Frontend and script defs must not leak into game.bin. Each source is a
    // distinct name, so a game pass without the scope filter would report
    // three named entries; the correct count is one.
    const fs::path corpus =
        fs::temp_directory_path() / "fable-defs-build-test-scope";
    fs::remove_all(corpus);
    write_file(corpus / "root.def",
               "#definition OBJECT ROOT_OBJECT\nHealth 1;\n#end_definition\n");
    write_file(corpus / "FrontEndDefs" / "fe.def",
               "#definition OBJECT FRONTEND_OBJECT\nHealth 2;\n#end_definition\n");
    write_file(corpus / "ScriptDefs" / "sc.def",
               "#definition OBJECT SCRIPT_OBJECT\nHealth 3;\n#end_definition\n");
    const auto scoped = build_directory(corpus, out);
    fs::remove_all(corpus);
    fs::remove_all(out);
    if (!scoped.ok ||
        scoped.log.find("game: 250 entries (249 null, 1 named") ==
            std::string::npos) {
        return 3;
    }
    return 0;
}
