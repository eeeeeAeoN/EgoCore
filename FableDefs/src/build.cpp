#include "fable/defs/compiler/build.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "embedded_headers.hpp"
#include "fable/defs/compiler/diagnostic.hpp"
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

bool is_source_file(const fs::path &path) {
    const std::string name = path.filename().string();
    if (name.find("_deprecated.") != std::string::npos) {
        return false;
    }
    const std::string ext = path.extension().string();
    return ext == ".def" || ext == ".tpl" || ext == ".h";
}

std::vector<fs::path> collect_files(const fs::path &root, std::string &error) {
    std::vector<fs::path> files;
    std::error_code ec;
    for (fs::recursive_directory_iterator it(root, ec), end; !ec && it != end;
         it.increment(ec)) {
        if (it->is_regular_file(ec) && is_source_file(it->path())) {
            files.push_back(it->path());
        }
    }
    if (ec) {
        error = ec.message();
        return {};
    }
    std::sort(files.begin(), files.end());
    return files;
}

void append_line(std::ostringstream &log, const std::string &line) {
    log << line;
    if (line.empty() || line.back() != '\n') {
        log << '\n';
    }
}

std::string render_parse_error(const ParseError &error, const std::string &text,
                               const fs::path &path) {
    std::ostringstream out;
    out << render_error(error, text, path.string());
    return out.str();
}

} // namespace

namespace fable::defs::compiler {

std::size_t inject_embedded_headers(
    SymbolTable &symbols, const std::vector<fs::path> &disk_files,
    std::vector<Redefinition> &redefined, std::uint32_t &next_file,
    std::string &error_log) {
    std::set<std::string> seen_disk_headers;
    for (const fs::path &path : disk_files) {
        if (path.extension().string() == ".h") {
            std::string lower_name = path.filename().string();
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            seen_disk_headers.insert(std::move(lower_name));
        }
    }

    std::ostringstream log;
    std::size_t errors = 0;
    std::size_t embedded_count = 0;
    const EmbeddedHeader *embedded = embedded_non_retail_headers(embedded_count);
    for (std::size_t i = 0; i < embedded_count; ++i) {
        std::string lower_name(embedded[i].name);
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (seen_disk_headers.count(lower_name) != 0) {
            continue;
        }
        std::string text(embedded[i].content);
        ParseResult result = parse_source(text, FileId{next_file++});
        if (auto *parse_error = std::get_if<ParseError>(&result)) {
            ++errors;
            append_line(log, "error: embedded header " + std::string(embedded[i].name) +
                                 ": " + render_parse_error(*parse_error, text,
                                                           fs::path(embedded[i].name)));
            continue;
        }
        SourceAst &ast = std::get<SourceAst>(result);
        if (auto symbol_error = symbols.evaluate_items(ast.items, redefined)) {
            ++errors;
            append_line(log, "error: embedded header " + std::string(embedded[i].name) +
                                 ": " + render_parse_error(
                                     ParseError{symbol_error->span,
                                                symbol_error->message},
                                     text, fs::path(embedded[i].name)));
        }
    }
    error_log += log.str();
    return errors;
}

DirectoryBuild build_directory(const fs::path &input, const fs::path &output) {
    DirectoryBuild report;
    std::ostringstream log;

    std::error_code ec;
    if (input.empty() || !fs::exists(input, ec) || !fs::is_directory(input, ec)) {
        append_line(log, "error: input directory does not exist: " + input.string());
        report.log = log.str();
        return report;
    }
    if (!fs::exists(output, ec)) {
        fs::create_directories(output, ec);
        if (ec) {
            append_line(log, "error: cannot create output directory: " +
                                 output.string() + ": " + ec.message());
            report.log = log.str();
            return report;
        }
    } else if (!fs::is_directory(output, ec)) {
        append_line(log, "error: output path is not a directory: " + output.string());
        report.log = log.str();
        return report;
    }

    std::string collect_error;
    const std::vector<fs::path> files = collect_files(input, collect_error);
    if (!collect_error.empty()) {
        append_line(log, "error: cannot scan " + input.string() + ": " +
                             collect_error);
        report.log = log.str();
        return report;
    }
    if (files.empty()) {
        append_line(log, "error: no .def/.tpl/.h sources found under " +
                             input.string());
        report.log = log.str();
        return report;
    }

    SymbolTable symbols;
    std::vector<Redefinition> redefined;
    std::vector<ParsedFile> parsed;
    std::size_t errors = 0;
    std::uint32_t next_file = 1;
    parsed.reserve(files.size());
    for (const fs::path &path : files) {
        std::string text = read_file(path);
        if (text.empty() && !fs::exists(path, ec)) {
            ++errors;
            append_line(log, "error: " + path.string() + ": cannot read file");
            continue;
        }
        ParseResult result = parse_source(text, FileId{next_file++});
        if (auto *parse_error = std::get_if<ParseError>(&result)) {
            ++errors;
            append_line(log, "error: " + path.string() + ": " +
                                 render_parse_error(*parse_error, text, path));
            continue;
        }
        SourceAst &ast = std::get<SourceAst>(result);
        if (auto symbol_error = symbols.evaluate_items(ast.items, redefined)) {
            ++errors;
            append_line(log, "error: " + path.string() + ": " +
                                 render_parse_error(
                                     ParseError{symbol_error->span,
                                                symbol_error->message},
                                     text, path));
        }
        std::string normalized = path.string();
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        parsed.push_back(ParsedFile{std::move(normalized), std::move(ast)});
    }

    std::string embedded_log;
    errors += inject_embedded_headers(symbols, files, redefined, next_file, embedded_log);
    if (!embedded_log.empty()) {
        append_line(log, embedded_log);
    }

    if (errors != 0) {
        append_line(log, "error: cannot build with " + std::to_string(errors) +
                             " parse/symbol error(s)");
        report.log = log.str();
        return report;
    }
    inject_engine_enums(symbols);

    const std::vector<BinaryKind> kinds = {BinaryKind::Game, BinaryKind::Frontend,
                                           BinaryKind::Script};
    report.result = build_binaries(parsed, symbols, output, kinds);
    for (const BinBuildStats &bin : report.result.bins) {
        append_line(log, bin.label + ": " + std::to_string(bin.entries) +
                             " entries (" + std::to_string(bin.null_defs) +
                             " null, " + std::to_string(bin.named) + " named, " +
                             std::to_string(bin.lowered) + " lowered)");
        for (const std::string &error : bin.errors) {
            append_line(log, "error: " + bin.label + ": " + error);
        }
    }
    append_line(log, "names: " + std::to_string(report.result.names));
    if (!report.result.error.empty()) {
        append_line(log, "error: build: " + report.result.error);
    }
    if (!report.result.ok) {
        append_line(log, "error: build completed with errors");
    } else {
        append_line(log, "build succeeded");
    }
    report.ok = report.result.ok;
    report.log = log.str();
    return report;
}

} // namespace fable::defs::compiler
