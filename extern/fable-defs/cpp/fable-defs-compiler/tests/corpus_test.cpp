// Corpus smoke test. Skips unless FABLE_DEFS_CORPUS points at a Defs/ tree.
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "fable/defs/compiler/parser.hpp"
#include "fable/defs/compiler/symbols.hpp"

namespace fs = std::filesystem;
using namespace fable::defs::compiler;

namespace {

std::string read_file(const fs::path &path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool is_source(const fs::path &path) {
    std::string name = path.filename().string();
    if (name.find("_deprecated.") != std::string::npos) {
        return false;
    }
    std::string ext = path.extension().string();
    return ext == ".def" || ext == ".tpl" || ext == ".h";
}

} // namespace

int main() {
    const char *root = std::getenv("FABLE_DEFS_CORPUS");
    if (root == nullptr) {
        std::cout << "SKIP: set FABLE_DEFS_CORPUS to a Defs/ directory\n";
        return 0;
    }

    std::vector<fs::path> files;
    std::error_code error;
    for (fs::recursive_directory_iterator it(root, error), end;
         !error && it != end; it.increment(error)) {
        if (it->is_regular_file(error) && is_source(it->path())) {
            files.push_back(it->path());
        }
    }
    if (error) {
        std::cerr << root << ": " << error.message() << "\n";
        return 1;
    }
    std::sort(files.begin(), files.end());

    SymbolTable symbols;
    std::vector<Redefinition> redefined;
    std::size_t definitions = 0;
    std::size_t errors = 0;
    std::uint32_t next_file = 1;
    for (const auto &path : files) {
        std::string text = read_file(path);
        ParseResult parsed = parse_source(text, FileId{next_file++});
        if (auto *parse_error = std::get_if<ParseError>(&parsed)) {
            ++errors;
            std::cerr << "parse error: "
                      << render_error(*parse_error, text, path.string()) << "\n";
            continue;
        }
        SourceAst &ast = std::get<SourceAst>(parsed);
        definitions += ast.definitions().size();
        if (auto symbol_error = symbols.evaluate_items(ast.items, redefined)) {
            ++errors;
            std::cerr << "symbol error: "
                      << render_error(
                             ParseError{symbol_error->span, symbol_error->message},
                             text, path.string())
                      << "\n";
        }
    }

    std::cout << "parsed " << files.size() << " files, " << definitions
              << " definitions, " << symbols.size() << " symbols\n";
    if (errors != 0) {
        std::cerr << errors << " error(s)\n";
        return 1;
    }
    if (definitions < 10'000) {
        std::cerr << "definition count is suspiciously low\n";
        return 1;
    }
    return 0;
}
