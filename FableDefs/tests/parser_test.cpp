#include <cstdint>
#include <iostream>
#include <string>
#include <filesystem>
#include <type_traits>
#include <variant>
#include <vector>

#include <fable/defs.hpp>

#include "fable/defs/compiler/lexer.hpp"
#include "fable/defs/compiler/build_index.hpp"
#include "fable/defs/compiler/build_pipeline.hpp"
#include "fable/defs/compiler/parser.hpp"
#include "fable/defs/compiler/symbols.hpp"
#include "fable/defs/compiler/type_registry.hpp"

using namespace fable::defs::compiler;

static_assert(std::is_same<decltype(fable::defs::CCreatureDef::Inventory),
                           std::int32_t>::value,
              "the compiler links the C++ def data model");

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

SourceAst parse_ok(std::string_view text) {
    ParseResult result = parse_source(text, FileId{1});
    if (auto *error = std::get_if<ParseError>(&result)) {
        std::cerr << "unexpected parse error: " << error->message << "\n";
        ++failures;
        return {};
    }
    return std::move(std::get<SourceAst>(result));
}

void test_lexing() {
    LexResult result = lex("#definition OBJECT T\nHealth -4.2f;", FileId{1});
    CHECK(std::holds_alternative<std::vector<Token>>(result));
    if (!std::holds_alternative<std::vector<Token>>(result)) {
        return;
    }
    const auto &tokens = std::get<std::vector<Token>>(result);
    CHECK(tokens.size() == 7);
    CHECK(tokens[0].kind == TokenKind::Definition);
    CHECK(tokens[1].kind == TokenKind::Ident);
    CHECK(tokens[1].text == "OBJECT");
    CHECK(tokens[2].kind == TokenKind::Ident);
    CHECK(tokens[2].text == "T");
    CHECK(tokens[3].kind == TokenKind::Ident);
    CHECK(tokens[3].text == "Health");
    CHECK(tokens[4].kind == TokenKind::Number);
    CHECK(tokens[4].text == "-4.2f");
    CHECK(tokens[5].kind == TokenKind::Semi);
    CHECK(tokens[6].kind == TokenKind::Eof);
}

void test_definition() {
    SourceAst ast = parse_ok(
        "#definition CREATURE C\n"
        "Health 100;\n"
        "Controls.Add(CActionInputControl(1, 2));\n"
        "<CThing> Value 1; <\\CThing>\n"
        "#end_definition\n");
    CHECK(ast.items.size() == 1);
    if (ast.items.empty()) {
        return;
    }
    const auto &item = ast.items[0];
    CHECK(item.kind == Item::Kind::Definition);
    CHECK(item.definition.value.def_type == "CREATURE");
    CHECK(item.definition.value.name == "C");
    CHECK(item.definition.value.body.size() == 3);
    if (item.definition.value.body.size() != 3) {
        return;
    }
    CHECK(item.definition.value.body[0].kind == Statement::Kind::Field);
    CHECK(item.definition.value.body[0].field.path.segments.size() == 1);
    CHECK(item.definition.value.body[0].field.path.segments[0].field == "Health");
    CHECK(item.definition.value.body[1].kind == Statement::Kind::MethodCall);
    CHECK(item.definition.value.body[1].method_call.method == "Add");
    CHECK(item.definition.value.body[1].method_call.object.segments.size() == 1);
    CHECK(item.definition.value.body[1].method_call.object.segments[0].field ==
          "Controls");
    CHECK(item.definition.value.body[1].method_call.arguments.size() == 1);
    CHECK(item.definition.value.body[2].kind == Statement::Kind::TaggedBlock);
    CHECK(item.definition.value.body[2].tagged_block);
    if (item.definition.value.body[2].tagged_block) {
        CHECK(item.definition.value.body[2].tagged_block->tag == "CThing");
        CHECK(item.definition.value.body[2].tagged_block->body.size() == 1);
    }
}

void test_enum_and_symbols() {
    SourceAst ast = parse_ok(
        "enum E\n"
        "{\n"
        "    A = 1,\n"
        "    B,\n"
        "    C = A << 3 | 2,\n"
        "};\n");
    SymbolTable table;
    std::vector<Redefinition> redefined;
    CHECK(!table.evaluate_items(ast.items, redefined));
    CHECK(table.lookup("A") == 1);
    CHECK(table.lookup("B") == 2);
    CHECK(table.lookup("C") == 10);
    CHECK(redefined.empty());
}

void test_defines_and_conditionals() {
    SourceAst ast = parse_ok(
        "#define GUARD\n"
        "#define VALUE 7\n"
        "#ifdef _WINDOWS\n"
        "enum E { A = 5 };\n"
        "#else\n"
        "enum E { A = 9 };\n"
        "#endif\n"
        "namespace N { enum F { B = 3 }; }\n");
    SymbolTable table;
    std::vector<Redefinition> redefined;
    CHECK(!table.evaluate_items(ast.items, redefined));
    CHECK(!table.lookup("GUARD").has_value());
    CHECK(table.lookup("VALUE") == 7);
    CHECK(table.lookup("A") == 5);
    CHECK(table.lookup("B") == 3);
}

void test_missing_end_definition() {
    ParseResult result = parse_source("#definition OBJECT T\nHealth 1;", FileId{1});
    CHECK(std::holds_alternative<ParseError>(result));
    if (auto *error = std::get_if<ParseError>(&result)) {
        CHECK(error->message == "missing #end_definition");
    }
}

void test_build_index() {
    CHECK(is_allowed_type(BinaryKind::Game, "CREATURE"));
    CHECK(!is_allowed_type(BinaryKind::Game, "NOT_A_DEF_TYPE"));

    SourceAst a = parse_ok(
        "#definition CREATURE A\n#end_definition\n"
        "#definition FOO B\n#end_definition\n");
    SourceAst b = parse_ok(
        "#definition OBJECT C\n#end_definition\n");
    std::vector<SourceAst> asts;
    asts.push_back(std::move(a));
    asts.push_back(std::move(b));
    BuildIndex index = BuildIndex::build(BinaryKind::Game, asts);
    CHECK(index.nulldef_count() == 249);
    CHECK(index.named_count() == 2);
    CHECK(index.index_of("A") == 249);
    CHECK(index.index_of("C") == 250);
    CHECK(!index.index_of("B").has_value());
}

void test_binary_pipeline() {
    std::vector<ParsedFile> files;
    files.push_back(ParsedFile{"/tmp/fable-defs-test/creature.def",
                               parse_ok("#definition CREATURE TEST_CREATURE\n"
                                        "  Health 50.0;\n"
                                        "#end_definition\n")});
    SymbolTable symbols;
    const auto out = std::filesystem::temp_directory_path() / "fable-defs-build-test";
    std::error_code ec;
    std::filesystem::remove_all(out, ec);
    BuildResult result = build_binaries(files, symbols, out, {BinaryKind::Game});
    CHECK(result.ok);
    CHECK(result.bins.size() == 1);
    CHECK(result.bins[0].entries == 250); // 249 NULLDEFs + TEST_CREATURE
    CHECK(std::filesystem::exists(out / "game.bin"));
    CHECK(std::filesystem::exists(out / "names.bin"));
    std::filesystem::remove_all(out, ec);
}

void test_type_registry() {
    CHECK(std::string(class_name_for_type("CREATURE")) == "CThingCreatureDef");
    CHECK(std::string(class_name_for_type("UI")) == "NUISystem::CUIDef");
    bool called = false;
    bool found = with_class_for_type("CREATURE", [&](auto value) {
        using T = decltype(value);
        called = std::is_same_v<T, fable::defs::CThingCreatureDef>;
    });
    CHECK(found);
    CHECK(called);
}

void test_expression() {
    auto result = parse_expr_text("CActionInputControl(1, TRUE, 4 + 5)");
    CHECK(std::holds_alternative<Expr>(result));
    if (auto *expr = std::get_if<Expr>(&result)) {
        CHECK(expr->kind == Expr::Kind::Constructor);
        CHECK(expr->text == "CActionInputControl");
        CHECK(expr->arguments.size() == 3);
        CHECK(expr->arguments[1].kind == Expr::Kind::Bool);
        CHECK(expr->arguments[1].boolean);
        CHECK(expr->arguments[2].kind == Expr::Kind::Add);
    }
}

} // namespace

int main() {
    test_lexing();
    test_definition();
    test_enum_and_symbols();
    test_defines_and_conditionals();
    test_missing_end_definition();
    test_build_index();
    test_binary_pipeline();
    test_type_registry();
    test_expression();
    if (failures) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "all parser tests passed\n";
    return 0;
}
