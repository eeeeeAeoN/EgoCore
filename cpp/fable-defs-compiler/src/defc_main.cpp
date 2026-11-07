// fable-defc — C++ def compiler front end.
//
// The first milestone is the text front end: parse the corpus, evaluate
// header symbols and report what was found. Lowering and binary emission
// plug into the same AST next.

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "fable/defs/compiler/build_index.hpp"
#include "fable/defs/compiler/build_pipeline.hpp"
#include "fable/defs/compiler/definitions.hpp"
#include "fable/defs/compiler/diagnostic.hpp"
#include "fable/defs/compiler/lower.hpp"
#include "fable/defs/compiler/model_registry.hpp"
#include "fable/defs/compiler/names.hpp"
#include "fable/defs/compiler/parser.hpp"
#include "fable/defs/compiler/symbols.hpp"
#include "fable/defs/compiler/type_registry.hpp"
#include "fable/defs/compiler/wire.hpp"

namespace fs = std::filesystem;
using namespace fable::defs::compiler;

namespace {

struct Stats {
    std::size_t files = 0;
    std::size_t def_files = 0;
    std::size_t header_files = 0;
    std::size_t definitions = 0;
    std::size_t enums = 0;
    std::size_t defines = 0;
    std::size_t namespaces = 0;
    std::size_t conditionals = 0;
    std::size_t ignored_runs = 0;
    std::size_t parse_errors = 0;
    std::size_t symbol_errors = 0;
    std::size_t redefinitions = 0;
};

void usage(std::ostream &out) {
    out << "Usage: fable-defc <command> <Defs-dir> [options]\n"
           "\n"
           "Commands:\n"
           "  parse <dir>     Parse every .def/.tpl/.h and evaluate header symbols\n"
           "  symbols <dir>   Parse, then print the evaluated symbol table\n"
           "  build <dir> <out-dir> [--binary all|game|frontend|script]\n"
           "                  Compile the whole corpus to the retail binaries\n"
           "  lower <dir> <class|auto> <definition> [--binary B] [--dump]\n"
           "                  Lower one definition into a model class\n"
           "                  (auto picks the class from the def type;\n"
           "                   --binary game|frontend|script selects the manifest;\n"
           "                   --dump also serializes the wire body)\n"
           "\n"
           "Options:\n"
           "  --quiet         Only print errors\n"
           "  --files         Print per-file definition counts\n"
           "  --limit N       Stop after N files (debugging)\n"
           "  --help          Show this help\n";
}

std::string read_file(const fs::path &path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool is_source_file(const fs::path &path) {
    std::string name = path.filename().string();
    if (name.find("_deprecated.") != std::string::npos) {
        return false;
    }
    std::string ext = path.extension().string();
    return ext == ".def" || ext == ".tpl" || ext == ".h";
}

void count_item(const Item &item, Stats &stats) {
    switch (item.kind) {
    case Item::Kind::Definition:
        ++stats.definitions;
        break;
    case Item::Kind::Enum:
        ++stats.enums;
        break;
    case Item::Kind::Define:
        ++stats.defines;
        break;
    case Item::Kind::Namespace:
        ++stats.namespaces;
        if (item.namespace_) {
            for (const auto &child : item.namespace_->items) {
                count_item(child, stats);
            }
        }
        break;
    case Item::Kind::Conditional:
        ++stats.conditionals;
        if (item.conditional) {
            for (const auto &child : item.conditional->if_branch) {
                count_item(child, stats);
            }
            if (item.conditional->else_branch) {
                for (const auto &child : *item.conditional->else_branch) {
                    count_item(child, stats);
                }
            }
        }
        break;
    case Item::Kind::PragmaOnce:
        break;
    }
}

std::vector<fs::path> collect_files(const fs::path &root,
                                    std::optional<std::size_t> limit) {
    std::vector<fs::path> files;
    std::error_code error;
    for (fs::recursive_directory_iterator it(root, error), end;
         !error && it != end; it.increment(error)) {
        if (it->is_regular_file(error) && is_source_file(it->path())) {
            files.push_back(it->path());
        }
    }
    if (error) {
        std::cerr << "fable-defc: " << root << ": " << error.message() << "\n";
        return {};
    }
    std::sort(files.begin(), files.end());
    if (limit && files.size() > *limit) {
        files.resize(*limit);
    }
    return files;
}

int run_build(const fs::path &root, const fs::path &out_dir,
              const std::vector<BinaryKind> &kinds) {
    std::vector<fs::path> files = collect_files(root, std::nullopt);
    if (files.empty()) {
        return 1;
    }

    SymbolTable symbols;
    std::vector<Redefinition> redefined;
    std::vector<ParsedFile> parsed;
    std::size_t errors = 0;
    std::uint32_t next_file = 1;
    for (const auto &path : files) {
        std::string text = read_file(path);
        ParseResult result = parse_source(text, FileId{next_file++});
        if (auto *parse_error = std::get_if<ParseError>(&result)) {
            ++errors;
            std::cerr << "parse error: "
                      << render_error(*parse_error, text, path.string()) << "\n";
            continue;
        }
        SourceAst &ast = std::get<SourceAst>(result);
        if (auto symbol_error = symbols.evaluate_items(ast.items, redefined)) {
            ++errors;
            std::cerr << "symbol error: "
                      << render_error(ParseError{symbol_error->span,
                                                 symbol_error->message},
                                      text, path.string())
                      << "\n";
        }
        std::string normalized = path.string();
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        parsed.push_back(ParsedFile{std::move(normalized), std::move(ast)});
    }
    if (errors != 0) {
        std::cerr << "cannot build with " << errors << " parse/symbol error(s)\n";
        return 1;
    }
    inject_engine_enums(symbols);

    BuildResult report = build_binaries(parsed, symbols, out_dir, kinds);
    for (const BinBuildStats &bin : report.bins) {
        std::cout << bin.label << ": "
                  << bin.entries << " entries (" << bin.null_defs << " null, "
                  << bin.named << " named, " << bin.lowered << " lowered)"
                  << " unsupported " << bin.unsupported << "\n";
        for (const std::string &error : bin.errors) {
            std::cerr << bin.label << ": " << error << "\n";
        }
    }
    std::cout << "names: " << report.names << "\n";
    if (!report.error.empty()) {
        std::cerr << "build: " << report.error << "\n";
    }
    if (!report.ok) {
        std::cerr << "build completed with errors\n";
    }
    return report.ok ? 0 : 1;
}

int run_lower(const fs::path &root, const std::string &class_name,
              const std::string &definition_name, std::optional<std::size_t> limit,
              bool dump, BinaryKind binary) {
    std::vector<fs::path> files = collect_files(root, limit);
    if (files.empty()) {
        return 1;
    }

    SymbolTable symbols;
    std::vector<SourceAst> asts;
    std::vector<Redefinition> redefined;
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
        if (auto symbol_error = symbols.evaluate_items(ast.items, redefined)) {
            ++errors;
            std::cerr << "symbol error: "
                      << render_error(
                             ParseError{symbol_error->span, symbol_error->message},
                             text, path.string())
                      << "\n";
        }
        asts.push_back(std::move(ast));
    }
    if (errors != 0) {
        std::cerr << "cannot lower with " << errors << " parse/symbol error(s)\n";
        return 1;
    }
    inject_engine_enums(symbols);

    DefinitionIndex index;
    for (const auto &ast : asts) {
        index.add(ast);
    }
    BuildIndex build_index = BuildIndex::build(binary, asts);
    index.apply_indices(build_index.indices());
    auto chain_result = specialization_chain(index, definition_name);
    if (auto *error = std::get_if<ChainError>(&chain_result)) {
        std::cerr << "fable-defc: " << error->message << "\n";
        return 1;
    }
    const Chain &chain = std::get<Chain>(chain_result);
    Definition definition = flatten_chain(chain);

    LowerStats stats;
    NamesBuilder names;
    WireStats wire_stats;
    std::vector<std::uint8_t> wire_bytes;
    std::vector<std::string> applied;
    auto apply = [&](auto value) {
        lower_definition(definition, value, symbols, stats, &applied, &index, &names);
        if (dump) {
            ByteWriter writer;
            serialize_def(value, writer, wire_stats);
            wire_bytes = writer.take();
        }
    };
    bool found = false;
    std::string resolved = class_name;
    if (class_name == "auto") {
        found = with_class_for_type(definition.def_type, apply);
        if (const char *name = class_name_for_type(definition.def_type)) {
            resolved = name;
        }
    } else {
        found = with_class(class_name, apply);
    }
    if (!found) {
        if (class_name == "auto") {
            std::cerr << "fable-defc: no model class for def type '"
                      << definition.def_type << "'\n";
        } else {
            std::cerr << "fable-defc: unknown model class '" << class_name << "'\n";
        }
        return 1;
    }

    std::cout << "definition:   " << definition_name << "\n";
    std::cout << "chain:        ";
    for (std::size_t i = 0; i < chain.size(); ++i) {
        if (i != 0) {
            std::cout << " -> ";
        }
        std::cout << chain[i]->name;
    }
    std::cout << "\n";
    std::cout << "class:        " << resolved << "\n";
    std::cout << "index:        " << build_index.nulldef_count() << " null + "
              << build_index.named_count() << " named\n";
    std::cout << "applied:      " << stats.applied << "\n";
    std::cout << "absent:       " << stats.absent << "\n";
    std::cout << "unsupported:  " << stats.unsupported << "\n";
    std::cout << "errors:       " << stats.errors.size() << "\n";
    std::cout << "names:        " << names.size() << "\n";
    if (!applied.empty()) {
        std::cout << "fields:";
        for (const auto &name : applied) {
            std::cout << " " << name;
        }
        std::cout << "\n";
    }
    if (dump) {
        std::cout << "wire fields:   " << wire_stats.fields << "\n";
        std::cout << "wire unsupported: " << wire_stats.unsupported << "\n";
        std::cout << "wire bytes:    " << wire_bytes.size() << "\n";
        const std::size_t show = wire_bytes.size() < 256 ? wire_bytes.size() : 256;
        for (std::size_t i = 0; i < show; ++i) {
            if (i % 16 == 0) {
                std::cout << "  ";
            }
            const char hex[] = "0123456789abcdef";
            std::uint8_t byte = wire_bytes[i];
            std::cout << hex[byte >> 4] << hex[byte & 0xF];
            if (i % 16 == 15 || i + 1 == show) {
                std::cout << "\n";
            } else {
                std::cout << ' ';
            }
        }
        if (show < wire_bytes.size()) {
            std::cout << "  ...\n";
        }
    }
    for (const auto &error : stats.errors) {
        std::cerr << "lower error: " << error.message << "\n";
    }
    return stats.errors.empty() ? 0 : 1;
}

int run(std::string_view command, const fs::path &root, bool quiet,
        bool list_files, std::optional<std::size_t> limit) {
    std::vector<fs::path> files = collect_files(root, limit);
    if (files.empty()) {
        return 1;
    }

    Stats stats;
    SymbolTable symbols;
    std::vector<Redefinition> redefinitions;

    std::uint32_t next_file = 1;
    for (const auto &path : files) {
        std::string text = read_file(path);
        FileId file{next_file++};
        ParseResult parsed = parse_source(text, file);
        if (auto *parse_error = std::get_if<ParseError>(&parsed)) {
            ++stats.parse_errors;
            std::cerr << "parse error: "
                      << render_error(*parse_error, text, path.string()) << "\n";
            continue;
        }
        SourceAst &ast = std::get<SourceAst>(parsed);
        if (list_files) {
            std::cout << path.string() << "\t" << ast.definitions().size() << "\n";
        }
        ++stats.files;
        std::string ext = path.extension().string();
        if (ext == ".def" || ext == ".tpl") {
            ++stats.def_files;
        } else {
            ++stats.header_files;
        }
        stats.ignored_runs += ast.ignored.size();
        for (const auto &item : ast.items) {
            count_item(item, stats);
        }
        std::size_t before = redefinitions.size();
        if (auto symbol_error = symbols.evaluate_items(ast.items, redefinitions)) {
            ++stats.symbol_errors;
            std::cerr << "symbol error: "
                      << render_error(ParseError{symbol_error->span,
                                                 symbol_error->message},
                                      text, path.string())
                      << "\n";
        }
        stats.redefinitions += redefinitions.size() - before;
    }

    if (!quiet) {
        std::cout << "files:        " << stats.files << " (" << stats.def_files
                  << " def/tpl, " << stats.header_files << " h)\n";
        std::cout << "definitions:  " << stats.definitions << "\n";
        std::cout << "enums:        " << stats.enums << "\n";
        std::cout << "defines:      " << stats.defines << "\n";
        std::cout << "namespaces:   " << stats.namespaces << "\n";
        std::cout << "conditionals: " << stats.conditionals << "\n";
        std::cout << "ignored runs: " << stats.ignored_runs << "\n";
        std::cout << "symbols:      " << symbols.size() << "\n";
        std::cout << "redefinitions:" << stats.redefinitions << "\n";
        std::cout << "parse errors: " << stats.parse_errors << "\n";
        std::cout << "symbol errors:" << stats.symbol_errors << "\n";
    }

    if (command == "symbols") {
        for (const auto &[name, value] : symbols.symbols()) {
            std::cout << name << "\t" << value << "\n";
        }
    }

    return stats.parse_errors == 0 && stats.symbol_errors == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char **argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        usage(std::cout);
        return args.empty() ? 1 : 0;
    }
    std::string command = args[0];
    if (command == "build") {
        if (args.size() < 3) {
            std::cerr << "fable-defc: build needs <Defs-dir> <out-dir>\n";
            usage(std::cerr);
            return 1;
        }
        std::vector<BinaryKind> kinds;
        for (std::size_t i = 3; i < args.size(); ++i) {
            if (args[i] == "--binary" && i + 1 < args.size()) {
                std::string kind = args[++i];
                if (kind == "all") {
                    kinds.clear();
                } else if (kind == "game") {
                    kinds = {BinaryKind::Game};
                } else if (kind == "frontend") {
                    kinds = {BinaryKind::Frontend};
                } else if (kind == "script") {
                    kinds = {BinaryKind::Script};
                } else {
                    std::cerr << "fable-defc: unknown binary '" << kind << "'\n";
                    return 1;
                }
            } else {
                std::cerr << "fable-defc: unknown option '" << args[i] << "'\n";
                return 1;
            }
        }
        return run_build(args[1], args[2], kinds);
    }
    if (command == "lower") {
        if (args.size() < 4) {
            std::cerr << "fable-defc: lower needs <Defs-dir> <class> <definition>\n";
            usage(std::cerr);
            return 1;
        }
        bool dump = false;
        BinaryKind binary = BinaryKind::Game;
        for (std::size_t i = 4; i < args.size(); ++i) {
            if (args[i] == "--dump") {
                dump = true;
            } else if (args[i] == "--binary" && i + 1 < args.size()) {
                std::string kind = args[++i];
                if (kind == "game") {
                    binary = BinaryKind::Game;
                } else if (kind == "frontend") {
                    binary = BinaryKind::Frontend;
                } else if (kind == "script") {
                    binary = BinaryKind::Script;
                } else {
                    std::cerr << "fable-defc: unknown binary '" << kind << "'\n";
                    return 1;
                }
            } else {
                std::cerr << "fable-defc: unknown option '" << args[i] << "'\n";
                return 1;
            }
        }
        return run_lower(args[1], args[2], args[3], std::nullopt, dump, binary);
    }
    if (command != "parse" && command != "symbols") {
        std::cerr << "fable-defc: unknown command '" << command << "'\n";
        usage(std::cerr);
        return 1;
    }
    if (args.size() < 2) {
        std::cerr << "fable-defc: " << command << " needs a Defs directory\n";
        return 1;
    }
    fs::path root = args[1];
    bool quiet = false;
    bool list_files = false;
    std::optional<std::size_t> limit;
    for (std::size_t i = 2; i < args.size(); ++i) {
        if (args[i] == "--quiet") {
            quiet = true;
        } else if (args[i] == "--files") {
            list_files = true;
        } else if (args[i] == "--limit" && i + 1 < args.size()) {
            limit = static_cast<std::size_t>(std::stoull(args[++i]));
        } else if (args[i] == "--help" || args[i] == "-h") {
            usage(std::cout);
            return 0;
        } else {
            std::cerr << "fable-defc: unknown option '" << args[i] << "'\n";
            return 1;
        }
    }
    return run(command, root, quiet, list_files, limit);
}
