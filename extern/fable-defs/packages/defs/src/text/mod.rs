pub mod base;
pub mod lexer;
pub mod symbols;

pub use self::base::{FileId, ParseContext, Span, Spanned};
pub use self::lexer::{LexError, LexErrorKind, Lexer, TextParseErrorKind, Token, TokenKind, lex};
pub use self::symbols::SymbolTable;

use self::base::ParseError;
use self::lexer::{Cursor, lex_error_to_parse_error};
use std::collections::HashMap;
/// One top-level construct. Both `.def`/`.tpl` and `.h` files are sequences of
/// these — the split between "def grammar" and "header grammar" is
/// conventional, not structural.
///
/// The corpus proves it: `engine_local_detail.def` declares two `enum`s whose
/// 31 symbols appear in no header and are referenced ~1,800 times, so a `.def`
/// legitimately carries declarations. Nothing in a `.h` carries definitions
/// today, but nothing rules it out either.
#[derive(Debug, Clone)]
pub enum Item {
    Definition(Spanned<Definition>),
    Enum(EnumDecl),
    Define(Define),
    Namespace(Namespace),
    /// `#ifdef` / `#ifndef` … `#else` … `#endif`.
    Conditional(IfDef),
    /// `#pragma once`. Kept as an item rather than silently skipped so the
    /// top-level loop has no "recognized but discarded" category; it
    /// contributes no symbols.
    PragmaOnce(Span),
}

/// A parsed source file: `.def`, `.tpl`, or `.h`.
#[derive(Debug, Clone, Default)]
pub struct SourceAst {
    pub items: Vec<Item>,
    /// Byte ranges that matched no item and were skipped, coalesced into runs.
    ///
    /// **Nothing reports these today, by design.** The canonical corpus is the
    /// ground state and must compile clean, and it contains four such runs — the
    /// leftover body of two commented-out definitions, a stray identifier, and a
    /// duplicate `#end_definition`. Warning about them would mean a stock build
    /// is never quiet, which trains everyone to ignore the output.
    ///
    /// The spans are recorded anyway because the parser has to skip these tokens
    /// regardless, so keeping the range costs nothing, and a future verbose mode
    /// wants exactly this: a way to catalogue the corpus's oddities without
    /// touching the parser again. That mode should surface them as notes, not
    /// warnings.
    ///
    /// Not an error channel either — the strict one-error-per-file policy (§11)
    /// is unchanged.
    pub ignored: Vec<Span>,
}

impl SourceAst {
    /// The definitions this file declares, in source order.
    pub fn definitions(&self) -> impl Iterator<Item = &Spanned<Definition>> {
        self.items.iter().filter_map(|i| match i {
            Item::Definition(d) => Some(d),
            _ => None,
        })
    }

    /// Index of each definition name into [`Self::definitions`] order. Later
    /// duplicates within one file overwrite earlier ones, matching the
    /// whole-body-replace semantics the builder applies across files.
    pub fn definitions_by_name(&self) -> HashMap<&str, usize> {
        self.definitions()
            .enumerate()
            .map(|(i, d)| (d.value.name.as_str(), i))
            .collect()
    }
}

#[derive(Debug, Clone)]
pub struct Definition {
    pub is_template: bool,
    pub def_type: String,
    pub name: String,
    pub specializes: Option<String>,
    pub specializes_span: Option<Span>,
    pub body: Vec<Spanned<Statement>>,
}

#[derive(Debug, Clone)]
pub enum Statement {
    Field(Field),
    MethodCall(MethodCall),
    TaggedBlock(TaggedBlock),
}

#[derive(Debug, Clone)]
pub struct Field {
    pub path: PropertyPath,
    pub expr: Spanned<Expr>,
}

#[derive(Debug, Clone)]
pub struct MethodCall {
    pub object: PropertyPath,
    pub call: Call,
}

#[derive(Debug, Clone)]
pub struct TaggedBlock {
    pub tag: String,
    pub body: Vec<Spanned<Statement>>,
}

#[derive(Debug, Clone)]
pub struct PropertyPath {
    pub segments: Vec<PathSegment>,
}

impl PropertyPath {
    pub fn simple(name: impl Into<String>) -> Self {
        Self {
            segments: vec![PathSegment::Field(name.into())],
        }
    }
}

impl std::fmt::Display for PropertyPath {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for (i, seg) in self.segments.iter().enumerate() {
            match seg {
                PathSegment::Field(name) => {
                    if i > 0 {
                        f.write_str(".")?;
                    }
                    f.write_str(name)?;
                }
                PathSegment::Index(expr) => write!(f, "[{}]", expr.value)?,
            }
        }
        Ok(())
    }
}

impl std::fmt::Display for Expr {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Expr::Number(s) => f.write_str(s),
            Expr::Bool(b) => f.write_str(if *b { "TRUE" } else { "FALSE" }),
            Expr::String(s) => write!(f, "\"{s}\""),
            Expr::Symbol(s) => f.write_str(s),
            Expr::Constructor(c) => {
                write!(f, "{}(", c.name)?;
                fmt_separated(f, &c.arguments, ", ")?;
                f.write_str(")")
            }
            Expr::BitOr(terms) => fmt_separated(f, terms, " | "),
            Expr::Add(terms) => fmt_separated(f, terms, " + "),
        }
    }
}

fn fmt_separated(
    f: &mut std::fmt::Formatter<'_>,
    terms: &[Spanned<Expr>],
    sep: &str,
) -> std::fmt::Result {
    for (i, term) in terms.iter().enumerate() {
        if i > 0 {
            f.write_str(sep)?;
        }
        write!(f, "{}", term.value)?;
    }
    Ok(())
}

#[derive(Debug, Clone)]
pub enum PathSegment {
    Field(String),
    Index(Spanned<Expr>),
}

#[derive(Debug, Clone, PartialEq)]
pub enum Expr {
    /// A numeric literal, kept as its raw source slice (optional leading `-`,
    /// digits, optional `.frac`, optional trailing `f`). Interpretation — int
    /// vs float, truncation, range — is deferred to evaluation (§11.2), where it
    /// is type-specific per field. Use [`number_is_float`] to classify the shape.
    Number(String),
    Bool(bool),
    String(String),
    Symbol(String),
    Constructor(Call),
    BitOr(Vec<Spanned<Expr>>),
    Add(Vec<Spanned<Expr>>),
}

/// Whether a [`Expr::Number`] literal is float-shaped — it contains a `.` or a
/// trailing `f` — mirroring the old parser's int-vs-float split (`has_dot ||
/// has_f_suffix`). Integer-shaped literals parse as integers; float-shaped ones
/// parse as `f32` (after stripping the `f`).
pub fn number_is_float(s: &str) -> bool {
    s.contains('.') || s.ends_with('f')
}

#[derive(Debug, Clone, PartialEq)]
pub struct Call {
    pub name: String,
    pub arguments: Vec<Spanned<Expr>>,
}

// ── Declaration items ─────────────────────────────────────────────────────────
//
// Equally at home in a `.def` and a `.h`: `engine_local_detail.def` declares two
// `enum`s whose symbols exist nowhere else. Nested bodies hold `Item`, so there
// is one item type for the whole grammar.

// Names and leaf values are `Spanned` so failures found *after* parsing — a
// symbol that will not evaluate, a redefinition worth reporting — can point at
// the source instead of naming the file and leaving the reader to grep.

#[derive(Debug, Clone)]
pub struct EnumDecl {
    pub name: Option<Spanned<String>>,
    pub variants: Vec<EnumVariant>,
}

#[derive(Debug, Clone)]
pub struct EnumVariant {
    pub name: Spanned<String>,
    pub value: Option<EnumExpr>,
}

#[derive(Debug, Clone)]
pub enum EnumExpr {
    Int(Spanned<i64>),
    Ident(Spanned<String>),
    Shift(Vec<EnumExpr>),
    BitOr(Vec<EnumExpr>),
}

#[derive(Debug, Clone)]
pub struct Define {
    pub name: Spanned<String>,
    /// `None` for a valueless `#define NAME`, which C treats as "defined, empty
    /// expansion" rather than a number. The corpus has 46 of these (every
    /// include guard) against 25 with values.
    ///
    /// A valueless define contributes **no symbol**: using it where a number is
    /// wanted is an error in C too, and keeping it out of the table means the
    /// ~46 guard names never shadow anything.
    pub value: Option<i64>,
}

#[derive(Debug, Clone)]
pub struct Namespace {
    pub name: String,
    pub items: Vec<Item>,
}

#[derive(Debug, Clone)]
pub struct IfDef {
    pub condition: String,
    pub if_branch: Vec<Item>,
    pub else_branch: Option<Vec<Item>>,
    pub inverted: bool,
}

pub type DefParseError = ParseError<TextParseErrorKind>;

/// Whether `kind` can never appear inside a def body — a directive or header
/// keyword, or EOF. Hitting one before `#end_definition` is the precise "missing
/// `#end_definition`" error (§11.5). `EndDefinition` is *not* here: the body loop
/// matches it first as the valid closer.
fn is_body_terminator(kind: TokenKind) -> bool {
    matches!(
        kind,
        TokenKind::Definition
            | TokenKind::DefinitionTemplate
            | TokenKind::Define
            | TokenKind::Ifdef
            | TokenKind::Ifndef
            | TokenKind::Else
            | TokenKind::Endif
            | TokenKind::Pragma
            | TokenKind::Namespace
            | TokenKind::Enum
            | TokenKind::Eof
    )
}

// ── Public entry points ───────────────────────────────────────────────────────

/// Parse any file under `Defs/` — `.def`, `.tpl`, or `.h`.
///
/// There is one grammar. The extension decides where a file's *contents* are
/// used (header-set variant resolution, definition emission), never how it is
/// parsed. The two-parser split it replaced gave the same text different
/// meanings depending on the extension: `#ifdef` was a real conditional in a
/// `.h` and silently-skipped junk in a `.def`, so a guarded `#define` in a
/// `.def` leaked its symbol unconditionally.
pub fn parse_source(input: &str, file: FileId) -> Result<SourceAst, DefParseError> {
    let tokens = lex(input, file).map_err(|e| {
        let (span, kind) = lex_error_to_parse_error(e);
        ParseError::new(span, kind)
    })?;
    let mut cursor = Cursor::new(tokens);
    parse_file(&mut cursor)
}

/// Parse a single expression from `input`. Used by tests that need to evaluate
/// expressions without going through a full definition.
pub fn parse_expr_str(input: &str) -> Result<Spanned<Expr>, DefParseError> {
    let tokens = lex(input, FileId::ANONYMOUS).map_err(|e| {
        let (span, kind) = lex_error_to_parse_error(e);
        ParseError::new(span, kind)
    })?;
    let mut cursor = Cursor::new(tokens);
    parse_expr(&mut cursor)
}

// ── Productions on &mut Cursor ────────────────────────────────────────────────

/// The one top-level loop, for every file kind.
///
/// A file is a sequence of items: definitions and declarations. Anything else
/// is junk — recorded in [`SourceAst::ignored`] and skipped, never a hard error.
/// The stock corpus contains four such runs (leftovers from commented-out
/// definitions, a stray identifier, a duplicate `#end_definition`), so erroring
/// would fail a stock build on pre-existing dead text. The builder warns
/// instead, which is what a modder whose edit landed outside a definition needs
/// to hear.
fn parse_file(cursor: &mut Cursor<'_>) -> Result<SourceAst, ParseError<TextParseErrorKind>> {
    let mut file = SourceAst::default();
    loop {
        let kind = cursor.peek().kind;
        if kind == TokenKind::Eof {
            break;
        }
        if matches!(kind, TokenKind::Definition | TokenKind::DefinitionTemplate) {
            file.items.push(Item::Definition(parse_definition(cursor)?));
        } else if starts_declaration(kind) {
            file.items.push(parse_declaration(cursor)?);
        } else {
            // Coalesce a run of junk into one span so a commented-out
            // definition's leftover body is a single diagnostic, not one per
            // token.
            let start = cursor.peek().span.start;
            let mut end = cursor.bump().span.end;
            loop {
                let k = cursor.peek().kind;
                if k == TokenKind::Eof
                    || matches!(k, TokenKind::Definition | TokenKind::DefinitionTemplate)
                    || starts_declaration(k)
                {
                    break;
                }
                end = cursor.bump().span.end;
            }
            file.ignored.push(Span::new(cursor.file(), start, end));
        }
    }
    Ok(file)
}

fn parse_definition(
    cursor: &mut Cursor<'_>,
) -> Result<Spanned<Definition>, ParseError<TextParseErrorKind>> {
    let header_tok = cursor.peek();
    let def_start = header_tok.span.start;
    let is_template = match header_tok.kind {
        TokenKind::DefinitionTemplate => {
            cursor.bump();
            true
        }
        TokenKind::Definition => {
            cursor.bump();
            false
        }
        _ => {
            return Err(cursor.err_at(
                header_tok.span,
                TextParseErrorKind::UnexpectedToken {
                    expected: "#definition or #definition_template".into(),
                    found: header_tok.kind,
                },
            ));
        }
    };

    let def_type = cursor.expect_ident("definition type")?;
    let name_span = cursor.peek().span;
    let name = cursor.expect_ident("definition name")?;

    // Everything from here to `#end_definition` is "in definition `NAME`".
    // The span is the name, not the whole header line, so the secondary label
    // underlines what identifies the def — matching how lowering errors render.
    let def_ctx = ParseContext::new("definition", Some(name.clone()), name_span);

    let specializes = if cursor.at_ident("specialises") {
        let spec_kw = cursor.bump();
        let parent_span_start = cursor.peek().span.start;
        let parent = cursor.expect_ident("specialised parent")?;
        let spec_span = Span::new(
            cursor.file(),
            spec_kw.span.start,
            parent_span_start + parent.len(),
        );
        Some((parent, spec_span))
    } else {
        None
    };

    let mut body = Vec::new();
    let def_end = loop {
        let tk = cursor.peek().kind;
        if tk == TokenKind::EndDefinition {
            let mut end = cursor.bump().span.end;
            if cursor.at(TokenKind::Semi) {
                end = cursor.bump().span.end;
            }
            break end;
        }
        if is_body_terminator(tk) {
            let err = cursor
                .err(TextParseErrorKind::MissingEndDefinition)
                .within(&def_ctx);
            return Err(err);
        }
        body.push(parse_statement(cursor).map_err(|e| e.within(&def_ctx))?);
    };

    let (specializes, specializes_span) = match specializes {
        Some((name, span)) => (Some(name), Some(span)),
        None => (None, None),
    };

    Ok(Spanned {
        span: Span::new(cursor.file(), def_start, def_end),
        value: Definition {
            is_template,
            def_type,
            name,
            specializes,
            specializes_span,
            body,
        },
    })
}

fn parse_statement(
    cursor: &mut Cursor<'_>,
) -> Result<Spanned<Statement>, ParseError<TextParseErrorKind>> {
    let stmt_start = cursor.peek().span.start;

    // Tagged block: `<` not followed by `\` (a `<\` opens a *close* tag).
    if cursor.at(TokenKind::Lt) && cursor.peek_at(1).kind != TokenKind::Backslash {
        let tb = parse_tagged_block(cursor)?;
        return Ok(Spanned {
            span: Span::new(cursor.file(), stmt_start, cursor.prev_end()),
            value: Statement::TaggedBlock(tb),
        });
    }

    let path = parse_property_path(cursor)?;

    // Method call: the path is followed by an argument list.
    if cursor.at(TokenKind::LParen) {
        let (object, method) = split_method_path(cursor, path)?;
        let call = parse_call_with_name(cursor, method)?;
        if cursor.at(TokenKind::Semi) {
            cursor.bump();
        }
        return Ok(Spanned {
            span: Span::new(cursor.file(), stmt_start, cursor.prev_end()),
            value: Statement::MethodCall(MethodCall { object, call }),
        });
    }

    // Field assignment: `path expr`.
    let expr = parse_expr(cursor)?;
    if cursor.at(TokenKind::Semi) {
        cursor.bump();
    }
    Ok(Spanned {
        span: Span::new(cursor.file(), stmt_start, cursor.prev_end()),
        value: Statement::Field(Field { path, expr }),
    })
}

fn parse_tagged_block(
    cursor: &mut Cursor<'_>,
) -> Result<TaggedBlock, ParseError<TextParseErrorKind>> {
    cursor.expect(TokenKind::Lt)?;
    let tag_span = cursor.peek().span;
    let tag = cursor.expect_ident("tag name")?;
    cursor.expect(TokenKind::Gt)?;
    // Innermost wins, so an error inside the block reports the block rather
    // than the enclosing definition.
    let ctx = ParseContext::new("tagged block", Some(tag.clone()), tag_span);
    let mut body = Vec::new();
    loop {
        let tk = cursor.peek().kind;
        if tk == TokenKind::Lt && cursor.peek_at(1).kind == TokenKind::Backslash {
            let close_start = cursor.peek().span.start;
            cursor.bump(); // `<`
            cursor.bump(); // `\`
            let close_tag = cursor.expect_ident("closing tag name")?;
            cursor.expect(TokenKind::Gt)?;
            if close_tag != tag {
                // Underline the whole `<\Tag>` closer, which is what disagrees
                // with the opener.
                return Err(ParseError::new(
                    Span::new(cursor.file(), close_start, cursor.prev_end()),
                    TextParseErrorKind::MismatchedTag {
                        opened: tag,
                        closed: close_tag,
                    },
                ));
            }
            break;
        }
        // A directive/keyword, EOF, or `#end_definition` inside the block
        // means it was never closed (§11.5, strict).
        if is_body_terminator(tk) || tk == TokenKind::EndDefinition {
            return Err(cursor.unexpected(format!("<\\{tag}>")).within(&ctx));
        }
        body.push(parse_statement(cursor).map_err(|e| e.within(&ctx))?);
    }
    Ok(TaggedBlock { tag, body })
}

fn parse_property_path(
    cursor: &mut Cursor<'_>,
) -> Result<PropertyPath, ParseError<TextParseErrorKind>> {
    let mut segments = vec![PathSegment::Field(cursor.expect_ident("field name")?)];
    loop {
        if cursor.at(TokenKind::Dot) {
            cursor.bump();
            segments.push(PathSegment::Field(cursor.expect_ident("field name")?));
        } else if cursor.at(TokenKind::LBracket) {
            cursor.bump();
            let idx = parse_expr(cursor)?;
            cursor.expect(TokenKind::RBracket)?;
            segments.push(PathSegment::Index(idx));
        } else {
            break;
        }
    }
    Ok(PropertyPath { segments })
}

fn split_method_path(
    cursor: &Cursor<'_>,
    path: PropertyPath,
) -> Result<(PropertyPath, String), ParseError<TextParseErrorKind>> {
    let mut segments = path.segments;
    if let Some(PathSegment::Field(method)) = segments.pop() {
        Ok((PropertyPath { segments }, method))
    } else {
        Err(cursor.err(TextParseErrorKind::MethodNameIsIndex))
    }
}

pub fn parse_expr(
    cursor: &mut Cursor<'_>,
) -> Result<Spanned<Expr>, ParseError<TextParseErrorKind>> {
    parse_bitor_expr(cursor)
}

fn parse_bitor_expr(
    cursor: &mut Cursor<'_>,
) -> Result<Spanned<Expr>, ParseError<TextParseErrorKind>> {
    let start = cursor.peek().span.start;
    let first = parse_add_expr(cursor)?;
    let mut terms = vec![first];
    while cursor.at(TokenKind::Pipe) {
        cursor.bump();
        terms.push(parse_add_expr(cursor)?);
    }
    if terms.len() == 1 {
        Ok(terms.pop().unwrap())
    } else {
        Ok(Spanned {
            span: Span::new(cursor.file(), start, cursor.prev_end()),
            value: Expr::BitOr(terms),
        })
    }
}

fn parse_add_expr(
    cursor: &mut Cursor<'_>,
) -> Result<Spanned<Expr>, ParseError<TextParseErrorKind>> {
    let start = cursor.peek().span.start;
    let first = parse_leaf_expr(cursor)?;
    let mut terms = vec![first];
    while cursor.at(TokenKind::Plus) {
        cursor.bump();
        terms.push(parse_leaf_expr(cursor)?);
    }
    if terms.len() == 1 {
        Ok(terms.pop().unwrap())
    } else {
        Ok(Spanned {
            span: Span::new(cursor.file(), start, cursor.prev_end()),
            value: Expr::Add(terms),
        })
    }
}

fn parse_leaf_expr(
    cursor: &mut Cursor<'_>,
) -> Result<Spanned<Expr>, ParseError<TextParseErrorKind>> {
    let tok = cursor.peek();
    match tok.kind {
        TokenKind::Str => {
            cursor.bump();
            let unquoted = tok.source[1..tok.source.len() - 1].to_string();
            Ok(Spanned {
                span: tok.span,
                value: Expr::String(unquoted),
            })
        }
        TokenKind::Number => {
            cursor.bump();
            Ok(Spanned {
                span: tok.span,
                value: Expr::Number(tok.source.to_string()),
            })
        }
        TokenKind::Ident => {
            cursor.bump();
            match tok.source {
                "TRUE" | "BTRUE" => Ok(Spanned {
                    span: tok.span,
                    value: Expr::Bool(true),
                }),
                "FALSE" | "BFALSE" => Ok(Spanned {
                    span: tok.span,
                    value: Expr::Bool(false),
                }),
                ident => {
                    if cursor.at(TokenKind::LParen) {
                        let call = parse_call_with_name(cursor, ident.to_string())?;
                        Ok(Spanned {
                            span: Span::new(cursor.file(), tok.span.start, cursor.prev_end()),
                            value: Expr::Constructor(call),
                        })
                    } else {
                        Ok(Spanned {
                            span: tok.span,
                            value: Expr::Symbol(ident.to_string()),
                        })
                    }
                }
            }
        }
        _ => Err(cursor.unexpected("expression")),
    }
}

fn parse_call_with_name(
    cursor: &mut Cursor<'_>,
    name: String,
) -> Result<Call, ParseError<TextParseErrorKind>> {
    cursor.expect(TokenKind::LParen)?;
    let arguments = parse_arguments(cursor)?;
    cursor.expect(TokenKind::RParen)?;
    Ok(Call { name, arguments })
}

fn parse_arguments(
    cursor: &mut Cursor<'_>,
) -> Result<Vec<Spanned<Expr>>, ParseError<TextParseErrorKind>> {
    let mut args = Vec::new();
    if cursor.at(TokenKind::RParen) {
        return Ok(args);
    }
    loop {
        args.push(parse_expr(cursor)?);
        if cursor.at(TokenKind::Comma) {
            cursor.bump();
        } else {
            break;
        }
    }
    Ok(args)
}

// ── Declaration productions ───────────────────────────────────────────────────

/// Whether `kind` starts a declaration item.
pub(crate) fn starts_declaration(kind: TokenKind) -> bool {
    use TokenKind as T;
    matches!(
        kind,
        T::Enum | T::Define | T::Namespace | T::Ifdef | T::Ifndef | T::Pragma
    )
}

/// Parse one declaration item (`enum` / `#define` / `namespace` /
/// `#ifdef` / `#ifndef` / `#pragma once`) at the cursor.
///
/// Precondition: [`starts_declaration`] holds for the current token.
pub(crate) fn parse_declaration(
    cursor: &mut Cursor<'_>,
) -> Result<Item, ParseError<TextParseErrorKind>> {
    use TokenKind as T;
    match cursor.peek().kind {
        T::Enum => {
            cursor.bump();
            Ok(Item::Enum(parse_enum_body(cursor)?))
        }
        T::Define => {
            cursor.bump();
            Ok(Item::Define(parse_define_body(cursor)?))
        }
        T::Namespace => {
            cursor.bump();
            Ok(Item::Namespace(parse_namespace_body(cursor)?))
        }
        T::Ifdef | T::Ifndef => {
            let inverted = cursor.peek().kind == T::Ifndef;
            cursor.bump();
            Ok(Item::Conditional(parse_if_def_body(cursor, inverted)?))
        }
        T::Pragma => {
            let span = cursor.bump().span;
            // `#pragma once` is the only pragma the corpus uses; anything else
            // would be a name we do not know how to honor.
            if cursor.at_ident("once") {
                cursor.bump();
            }
            Ok(Item::PragmaOnce(span))
        }
        _ => Err(cursor.err(TextParseErrorKind::UnknownItem)),
    }
}

fn parse_enum_body(cursor: &mut Cursor<'_>) -> Result<EnumDecl, ParseError<TextParseErrorKind>> {
    // `enum` was already bumped by the caller; anchor on it so an anonymous
    // enum still has something to point at.
    let keyword_span = cursor.prev_span();
    let name = if cursor.at(TokenKind::Ident) {
        Some(cursor.expect_ident_spanned("enum name")?)
    } else {
        None
    };
    let ctx = ParseContext::new(
        "enum",
        name.as_ref().map(|n| n.value.clone()),
        name.as_ref().map_or(keyword_span, |n| n.span),
    );
    cursor
        .expect(TokenKind::LBrace)
        .map_err(|e| e.within(&ctx))?;
    let variants = parse_enum_variants(cursor).map_err(|e| e.within(&ctx))?;
    cursor
        .expect(TokenKind::RBrace)
        .map_err(|e| e.within(&ctx))?;
    if cursor.at(TokenKind::Semi) {
        cursor.bump();
    }
    Ok(EnumDecl { name, variants })
}

fn parse_enum_variants(
    cursor: &mut Cursor<'_>,
) -> Result<Vec<EnumVariant>, ParseError<TextParseErrorKind>> {
    let mut variants = Vec::new();
    loop {
        if cursor.at(TokenKind::Eof) {
            return Err(cursor.err(TextParseErrorKind::UnterminatedEnum));
        }
        if cursor.at(TokenKind::RBrace) {
            break;
        }
        let variant = parse_enum_variant(cursor)?;
        let has_value = variant.value.is_some();
        variants.push(variant);
        if cursor.at(TokenKind::Comma) {
            cursor.bump();
        } else if cursor.at(TokenKind::RBrace) {
            break;
        } else {
            // Report here rather than breaking and letting the caller's
            // `expect(RBrace)` fail, which would claim `}` was the only option.
            //
            // The expected set depends on how far the variant got. If it has no
            // value yet, `parse_enum_variant` peeked for `=` at *this* byte and
            // did not find it, so `=` is still a legal continuation and must be
            // listed. Once a value has been read, only a separator can follow.
            //
            // This is also what a variant name split by a stray token looks
            // like — `FOO BAR = 1` parses `FOO`, then lands here on `BAR` —
            // without the message having to guess at that interpretation.
            let expected = if has_value {
                "`,` or `}`"
            } else {
                "`=`, `,` or `}`"
            };
            return Err(cursor.unexpected(expected));
        }
    }
    Ok(variants)
}

fn parse_enum_variant(
    cursor: &mut Cursor<'_>,
) -> Result<EnumVariant, ParseError<TextParseErrorKind>> {
    let name = cursor.expect_ident_spanned("identifier")?;
    let value = if cursor.at(TokenKind::Eq) {
        cursor.bump();
        Some(parse_enum_expr(cursor)?)
    } else {
        None
    };
    Ok(EnumVariant { name, value })
}

fn parse_enum_expr(cursor: &mut Cursor<'_>) -> Result<EnumExpr, ParseError<TextParseErrorKind>> {
    parse_enum_bitor(cursor)
}

fn parse_enum_bitor(cursor: &mut Cursor<'_>) -> Result<EnumExpr, ParseError<TextParseErrorKind>> {
    let first = parse_enum_shift(cursor)?;
    let mut terms = vec![first];
    while cursor.at(TokenKind::Pipe) {
        cursor.bump();
        terms.push(parse_enum_shift(cursor)?);
    }
    Ok(if terms.len() == 1 {
        terms.pop().unwrap()
    } else {
        EnumExpr::BitOr(terms)
    })
}

fn parse_enum_shift(cursor: &mut Cursor<'_>) -> Result<EnumExpr, ParseError<TextParseErrorKind>> {
    let first = parse_enum_leaf(cursor)?;
    let mut terms = vec![first];
    while cursor.at(TokenKind::Shl) {
        cursor.bump();
        terms.push(parse_enum_leaf(cursor)?);
    }
    Ok(if terms.len() == 1 {
        terms.pop().unwrap()
    } else {
        EnumExpr::Shift(terms)
    })
}

fn parse_enum_leaf(cursor: &mut Cursor<'_>) -> Result<EnumExpr, ParseError<TextParseErrorKind>> {
    use TokenKind;
    match cursor.peek().kind {
        TokenKind::Number => {
            let t = cursor.bump();
            let n = t
                .source
                .parse::<i64>()
                .map_err(|_| ParseError::new(t.span, TextParseErrorKind::InvalidNumber))?;
            Ok(EnumExpr::Int(Spanned {
                span: t.span,
                value: n,
            }))
        }
        TokenKind::Ident => {
            let t = cursor.bump();
            Ok(EnumExpr::Ident(Spanned {
                span: t.span,
                value: t.source.to_string(),
            }))
        }
        _ => Err(cursor.unexpected("number or identifier")),
    }
}

fn parse_define_body(cursor: &mut Cursor<'_>) -> Result<Define, ParseError<TextParseErrorKind>> {
    let name = cursor.expect_ident_spanned("identifier")?;
    // The value is optional: `#define __FOO_H__` is an include guard, not a
    // malformed constant.
    if !cursor.at(TokenKind::Number) {
        return Ok(Define { name, value: None });
    }
    let t = cursor.bump();
    let value = t
        .source
        .parse::<i64>()
        .map_err(|_| ParseError::new(t.span, TextParseErrorKind::InvalidNumber))?;
    Ok(Define {
        name,
        value: Some(value),
    })
}

fn parse_namespace_body(
    cursor: &mut Cursor<'_>,
) -> Result<Namespace, ParseError<TextParseErrorKind>> {
    let name_span = cursor.peek().span;
    let name = cursor.expect_ident("identifier")?;
    let ctx = ParseContext::new("namespace", Some(name.clone()), name_span);
    cursor
        .expect(TokenKind::LBrace)
        .map_err(|e| e.within(&ctx))?;
    let mut items = Vec::new();
    loop {
        if cursor.at(TokenKind::Eof) {
            return Err(cursor
                .err(TextParseErrorKind::UnterminatedNamespace)
                .within(&ctx));
        }
        if cursor.at(TokenKind::RBrace) {
            break;
        }
        items.push(parse_declaration(cursor).map_err(|e| e.within(&ctx))?);
    }
    cursor
        .expect(TokenKind::RBrace)
        .map_err(|e| e.within(&ctx))?;
    if cursor.at(TokenKind::Semi) {
        cursor.bump();
    }
    Ok(Namespace { name, items })
}

fn parse_if_def_body(
    cursor: &mut Cursor<'_>,
    inverted: bool,
) -> Result<IfDef, ParseError<TextParseErrorKind>> {
    let directive = if inverted { "#ifndef" } else { "#ifdef" };
    let cond_span = cursor.peek().span;
    let condition = cursor.expect_ident("identifier")?;
    let ctx = ParseContext::new(directive, Some(condition.clone()), cond_span);
    let mut if_branch = Vec::new();
    loop {
        if cursor.at(TokenKind::Eof) {
            return Err(cursor
                .err(TextParseErrorKind::UnterminatedIfDef)
                .within(&ctx));
        }
        if cursor.at(TokenKind::Else) || cursor.at(TokenKind::Endif) {
            break;
        }
        if_branch.push(parse_declaration(cursor).map_err(|e| e.within(&ctx))?);
    }
    let else_branch = if cursor.at(TokenKind::Else) {
        cursor.bump();
        let mut else_branch = Vec::new();
        loop {
            if cursor.at(TokenKind::Eof) {
                return Err(cursor
                    .err(TextParseErrorKind::UnterminatedIfDef)
                    .within(&ctx));
            }
            if cursor.at(TokenKind::Endif) {
                break;
            }
            else_branch.push(parse_declaration(cursor).map_err(|e| e.within(&ctx))?);
        }
        Some(else_branch)
    } else {
        None
    };
    cursor
        .expect(TokenKind::Endif)
        .map_err(|e| e.within(&ctx))?;
    // `#endif __GUARD_NAME__` — the pre-C99 habit of naming the condition on
    // the closer. One occurrence in the corpus, and it is a real C idiom, so
    // accept it rather than leaving a trailing identifier to be reported as
    // stray text.
    if cursor.at(TokenKind::Ident) {
        cursor.bump();
    }
    Ok(IfDef {
        condition,
        if_branch,
        else_branch,
        inverted,
    })
}

#[cfg(test)]
mod tests {
    use super::lexer::TextParseErrorKind;
    use super::*;

    fn parse_def(body: &str) -> Spanned<Definition> {
        let input = format!("#definition OBJECT T\n{body}\n#end_definition");
        parse_source(&input, FileId::ANONYMOUS)
            .unwrap()
            .definitions()
            .next()
            .unwrap()
            .clone()
    }

    fn parse_first_def(input: &str) -> Spanned<Definition> {
        parse_source(input, FileId::ANONYMOUS)
            .unwrap()
            .definitions()
            .next()
            .unwrap()
            .clone()
    }

    fn parse_err(input: &str) -> TextParseErrorKind {
        parse_source(input, FileId::ANONYMOUS).unwrap_err().inner
    }

    fn parse_stmt(stmt: &str) -> Spanned<Statement> {
        parse_def(stmt).value.body.pop().unwrap()
    }

    fn parse_expr_test(value: &str) -> Spanned<Expr> {
        match &parse_stmt(&format!("X {value};")).value {
            Statement::Field(f) => f.expr.clone(),
            other => panic!("expected Field, got {other:?}"),
        }
    }

    fn parse_path(path: &str) -> PropertyPath {
        let Spanned {
            value: Statement::Field(f),
            ..
        } = parse_stmt(&format!("{path} 0;"))
        else {
            panic!()
        };
        f.path
    }

    fn number(value: &str) -> String {
        match parse_expr_test(value).value {
            Expr::Number(s) => s,
            other => panic!("expected Number, got {other:?}"),
        }
    }

    // --- numbers stay raw (interpretation is deferred to evaluation) -----------

    #[test]
    fn integer() {
        assert_eq!(number("42"), "42");
        assert_eq!(number("42282949"), "42282949");
    }

    #[test]
    fn negative_integer() {
        assert_eq!(number("-42"), "-42");
        assert_eq!(number("-42282949"), "-42282949");
    }

    #[test]
    fn float_keeps_source() {
        // Every float form is preserved verbatim — no `Float` node any more.
        assert_eq!(number("4.2"), "4.2");
        assert_eq!(number("4.2f"), "4.2f");
        assert_eq!(number("4."), "4.");
        assert_eq!(number("-4.2"), "-4.2");
        assert_eq!(number("-4.2f"), "-4.2f");
        assert_eq!(number("-4."), "-4.");
    }

    #[test]
    fn number_shape_classification() {
        assert!(!number_is_float("42"));
        assert!(!number_is_float("-42"));
        assert!(number_is_float("4.2"));
        assert!(number_is_float("4."));
        assert!(number_is_float("4.2f"));
    }

    #[test]
    fn string() {
        let Expr::String(s) = parse_expr_test(r#""Hello, World!""#).value else {
            panic!()
        };
        assert_eq!(s, "Hello, World!");
    }

    #[test]
    fn bool_test() {
        assert!(matches!(parse_expr_test("TRUE").value, Expr::Bool(true)));
        assert!(matches!(parse_expr_test("FALSE").value, Expr::Bool(false)));
    }

    #[test]
    fn bool_b_prefix() {
        assert!(matches!(parse_expr_test("BTRUE").value, Expr::Bool(true)));
        assert!(matches!(parse_expr_test("BFALSE").value, Expr::Bool(false)));
    }

    #[test]
    fn add_n_ary() {
        let Expr::Add(terms) = &parse_expr_test("1 + 2 + 3").value else {
            panic!()
        };
        assert_eq!(terms.len(), 3);
    }

    #[test]
    fn bitor_n_ary() {
        let Expr::BitOr(terms) = &parse_expr_test("A | B | C").value else {
            panic!()
        };
        assert_eq!(terms.len(), 3);
    }

    #[test]
    fn bitor_precedence_lower_than_add() {
        let Expr::BitOr(terms) = &parse_expr_test("A | B + C").value else {
            panic!()
        };
        assert_eq!(terms.len(), 2);
        assert!(matches!(&terms[0].value, Expr::Symbol(s) if s == "A"));
        let Expr::Add(add_terms) = &terms[1].value else {
            panic!()
        };
        assert_eq!(add_terms.len(), 2);
    }

    #[test]
    fn constructor_with_args() {
        let Expr::Constructor(c) = &parse_expr_test("CRGBColour(255, 128, 64, 255)").value else {
            panic!()
        };
        assert_eq!(c.name, "CRGBColour");
        assert_eq!(c.arguments.len(), 4);
    }

    #[test]
    fn empty_constructor() {
        let Expr::Constructor(c) = &parse_expr_test("CRGBColour()").value else {
            panic!()
        };
        assert!(c.arguments.is_empty());
    }

    #[test]
    fn identifier() {
        let Expr::Symbol(s) = parse_expr_test("GRAPHIC_NULL").value else {
            panic!()
        };
        assert_eq!(s, "GRAPHIC_NULL");
    }

    #[test]
    fn simple_path() {
        let p = parse_path("Health");
        assert_eq!(p.segments.len(), 1);
        assert!(matches!(&p.segments[0], PathSegment::Field(s) if s == "Health"));
    }

    #[test]
    fn nested_path() {
        let p = parse_path("Stats.ExperienceWorth");
        assert_eq!(p.segments.len(), 2);
    }

    #[test]
    fn integer_index() {
        let p = parse_path("Time[0]");
        assert!(matches!(
            &p.segments[1],
            PathSegment::Index(spanned) if matches!(&spanned.value, Expr::Number(s) if s == "0")
        ));
    }

    #[test]
    fn negative_index() {
        let p = parse_path("Time[-1]");
        assert!(matches!(
            &p.segments[1],
            PathSegment::Index(spanned) if matches!(&spanned.value, Expr::Number(s) if s == "-1")
        ));
    }

    #[test]
    fn ident_index() {
        let p = parse_path("Foo[BAR_CONST]");
        let PathSegment::Index(spanned) = &p.segments[1] else {
            panic!()
        };
        let Expr::Symbol(s) = &spanned.value else {
            panic!()
        };
        assert_eq!(s, "BAR_CONST");
    }

    #[test]
    fn string_index() {
        let p = parse_path("Map[\"DAY\"]");
        let PathSegment::Index(spanned) = &p.segments[1] else {
            panic!()
        };
        let Expr::String(s) = &spanned.value else {
            panic!()
        };
        assert_eq!(s, "DAY");
    }

    #[test]
    fn expression_index() {
        let p = parse_path("States[STATE + 1]");
        assert!(matches!(
            &p.segments[1],
            PathSegment::Index(spanned) if matches!(spanned.value, Expr::Add(_))
        ));
    }

    #[test]
    fn nested_field_and_index() {
        let p = parse_path("Time[0].SkyTexture0");
        assert_eq!(p.segments.len(), 3);
        assert!(matches!(&p.segments[0], PathSegment::Field(s) if s == "Time"));
        assert!(matches!(
            &p.segments[1],
            PathSegment::Index(spanned) if matches!(&spanned.value, Expr::Number(s) if s == "0")
        ));
        assert!(matches!(&p.segments[2], PathSegment::Field(s) if s == "SkyTexture0"));
    }

    #[test]
    fn field_assignment() {
        let Spanned {
            value: Statement::Field(f),
            ..
        } = parse_stmt("Health 100;")
        else {
            panic!()
        };
        assert_eq!(f.path.segments.len(), 1);
        assert!(matches!(&f.expr.value, Expr::Number(s) if s == "100"));
    }

    #[test]
    fn method_call() {
        let Spanned {
            value: Statement::MethodCall(mc),
            ..
        } = parse_stmt("Components.Add(\"CTCPhysicsStandard\");")
        else {
            panic!()
        };
        assert_eq!(mc.call.name, "Add");
        assert_eq!(mc.call.arguments.len(), 1);
    }

    #[test]
    fn tagged_block() {
        let Spanned {
            value: Statement::TaggedBlock(tb),
            ..
        } = parse_stmt("<CCreatureDef>\n  Health 100;\n<\\CCreatureDef>")
        else {
            panic!()
        };
        assert_eq!(tb.tag, "CCreatureDef");
        assert_eq!(tb.body.len(), 1);
    }

    #[test]
    fn template_flag() {
        let def = parse_first_def("#definition_template OBJECT T\n#end_definition");
        assert!(def.value.is_template);
    }

    #[test]
    fn specialises() {
        let def = parse_first_def(
            "#definition OBJECT CHILD specialises PARENT\n  Health 50;\n#end_definition",
        );
        assert_eq!(def.value.specializes.as_deref(), Some("PARENT"));
    }

    #[test]
    fn end_definition_trailing_semicolon() {
        let file = parse_source(
            "#definition OBJECT T\n  Health 100;\n#end_definition;",
            FileId::ANONYMOUS,
        )
        .unwrap();
        assert_eq!(file.definitions().count(), 1);
    }

    #[test]
    fn multiple_definitions_preserve_order() {
        let file = parse_source(
            r#"
    #definition OBJECT FIRST
    #end_definition

    #definition OBJECT SECOND
    #end_definition
    "#,
            FileId::ANONYMOUS,
        )
        .unwrap();
        assert_eq!(file.definitions().count(), 2);
        assert_eq!(file.definitions().next().unwrap().value.name, "FIRST");
        assert_eq!(file.definitions().nth(1).unwrap().value.name, "SECOND");
        let by_name = file.definitions_by_name();
        assert_eq!(by_name["FIRST"], 0);
        assert_eq!(by_name["SECOND"], 1);
    }

    #[test]
    fn line_comment_in_body() {
        let def = parse_def("// just a comment\nHealth 100;");
        assert_eq!(def.value.body.len(), 1);
    }

    #[test]
    fn block_comment_in_body() {
        let def = parse_def("/* block comment */\nHealth 100;");
        assert_eq!(def.value.body.len(), 1);
    }

    #[test]
    fn block_comment_inline() {
        let def = parse_def("Name /* inline */ \"Test\";");
        assert_eq!(def.value.body.len(), 1);
        let Spanned {
            value: Statement::Field(f),
            ..
        } = &def.value.body[0]
        else {
            panic!()
        };
        let Expr::String(s) = &f.expr.value else {
            panic!()
        };
        assert_eq!(s, "Test");
    }

    #[test]
    fn block_comment_multiline() {
        let def = parse_def("/* multi\n   line\n   comment */\nHealth 100;");
        assert_eq!(def.value.body.len(), 1);
    }

    #[test]
    fn missing_semicolon_tolerated() {
        // The `;` terminator is optional (a clean grammar rule, not recovery) —
        // the only genuine tolerance the corpus relies on (§11.1).
        let def = parse_def("  Health 100");
        assert_eq!(def.value.body.len(), 1);
    }

    // --- strict errors (no recovery) -------------------------------------------

    #[test]
    fn err_unterminated_block_comment() {
        // A `/*` with real content and no closer is a lex error, surfaced as the
        // file's single parse error (§11.2, strict — no line-skip recovery).
        let kind = parse_err("#definition OBJECT T\n  Health /* never closes\n#end_definition");
        assert!(matches!(kind, TextParseErrorKind::UnterminatedBlockComment));
    }

    #[test]
    fn err_unterminated_string() {
        let kind = parse_err("#definition OBJECT T\n  Name \"no close\n#end_definition");
        assert!(matches!(kind, TextParseErrorKind::UnterminatedString));
    }

    #[test]
    fn err_mismatched_tag() {
        // A mismatched close tag is a hard error, not a dropped statement.
        let kind = parse_err("#definition OBJECT T\n  <A>\n  <\\B>\n#end_definition");
        assert!(matches!(kind, TextParseErrorKind::MismatchedTag { .. }));
    }

    #[test]
    fn err_missing_end_definition() {
        let kind = parse_err("#definition OBJECT T\n  Health 100;\n");
        assert!(matches!(kind, TextParseErrorKind::MissingEndDefinition));
    }

    #[test]
    fn missing_end_definition_does_not_swallow_next_def() {
        // The regression this rearchitecture targets: a def missing its
        // `#end_definition` must fail *precisely* on that def, not silently eat
        // the following one. The whole file errors (strict, one error per file);
        // the point is the error is `missing #end_definition`, anchored here.
        let input = concat!(
            "#definition OBJECT FIRST\n",
            "  Health 100;\n",
            "#definition OBJECT SECOND\n",
            "  Health 200;\n",
            "#end_definition\n",
        );
        let err = parse_source(input, FileId::ANONYMOUS).unwrap_err();
        assert!(matches!(
            err.inner,
            TextParseErrorKind::MissingEndDefinition
        ));
        // The error is anchored at the second `#definition` (the token that
        // revealed FIRST was never closed), not swallowed away.
        assert_eq!(
            err.span.start,
            input.find("#definition OBJECT SECOND").unwrap()
        );
    }

    #[test]
    fn empty_file() {
        let f = parse_source("", FileId::ANONYMOUS).unwrap();
        assert!(f.items.is_empty());
        assert!(f.ignored.is_empty());
    }

    #[test]
    fn whitespace_only() {
        assert!(
            parse_source("   \n\t  \n  ", FileId::ANONYMOUS)
                .unwrap()
                .definitions()
                .next()
                .is_none()
        );
    }

    #[test]
    fn comments_only() {
        let input = "// line comment\n/* block\n   comment */\n";
        assert!(
            parse_source(input, FileId::ANONYMOUS)
                .unwrap()
                .definitions()
                .next()
                .is_none()
        );
    }

    #[test]
    fn skips_between_def_junk() {
        // The lexer strips commented-out defs and decorative banner lines
        // (§11.4); the parser skips stray tokens between top-level items and
        // parses the file-local `enum` via the header bridge. Two defs survive.
        let input = r#"
    //#definition OBJECT COMMENTED_OUT_NEVER_PARSED
    //   Health 999;
    //#end_definition

    enum EFoo { A = 1, B = 2 };

    ****************************************

    #definition OBJECT FIRST
        Health 100;
    #end_definition

    stray_identifier;

    #definition OBJECT SECOND
        Health 200;
    #end_definition;
    "#;
        let file = parse_source(input, FileId::ANONYMOUS).unwrap();
        assert_eq!(file.definitions().count(), 2);
        assert_eq!(file.definitions().next().unwrap().value.name, "FIRST");
        assert_eq!(file.definitions().nth(1).unwrap().value.name, "SECOND");
        assert_eq!(
            file.items
                .iter()
                .filter(|i| matches!(i, Item::Enum(_)))
                .count(),
            1
        );
    }

    // ── Declaration items ─────────────────────────────────────────────────
    fn parse_h(input: &str) -> SourceAst {
        parse_source(input, FileId::ANONYMOUS).expect("header parse ok")
    }

    fn parse_h_err(input: &str) -> TextParseErrorKind {
        parse_source(input, FileId::ANONYMOUS).unwrap_err().inner
    }

    /// An include guard is ordinary structure, not a prologue to skip: a
    /// `#pragma once` item and an `#ifndef` conditional wrapping the file.
    ///
    /// It evaluates correctly on its own — the guard name is undefined, the
    /// condition is inverted, so the branch is taken — which is why the
    /// bespoke `skip_prologue`/`skip_epilogue` pair could be deleted. The
    /// guard's own valueless `#define` contributes no symbol, so the ~46 guard
    /// names in the corpus never reach the table.
    #[test]
    fn include_guard_is_an_ordinary_conditional() {
        let h = parse_h("#pragma once\n#ifndef __FOO_H__\n#define __FOO_H__\n#endif");
        assert_eq!(h.items.len(), 2);
        assert!(matches!(h.items[0], Item::PragmaOnce(_)));
        let Item::Conditional(ifdef) = &h.items[1] else {
            panic!("expected the guard to parse as a conditional")
        };
        assert!(ifdef.inverted);
        assert_eq!(ifdef.condition, "__FOO_H__");
        assert_eq!(ifdef.if_branch.len(), 1);
        assert!(h.ignored.is_empty());

        let mut t = super::SymbolTable::new();
        t.evaluate_items(&h.items).expect("guard evaluates");
        assert_eq!(
            t.lookup("__FOO_H__"),
            None,
            "valueless define binds nothing"
        );
    }

    #[test]
    fn named_enum() {
        let h = parse_h("enum EFoo { A = 1, B = 2 };");
        assert_eq!(h.items.len(), 1);
        let Item::Enum(decl) = &h.items[0] else {
            panic!()
        };
        assert_eq!(decl.name.as_ref().map(|n| n.value.as_str()), Some("EFoo"));
        assert_eq!(decl.variants.len(), 2);
        assert_eq!(decl.variants[0].name.value, "A");
        assert!(matches!(
            decl.variants[0].value,
            Some(EnumExpr::Int(Spanned { value: 1, .. }))
        ));
        assert_eq!(decl.variants[1].name.value, "B");
        assert!(matches!(
            decl.variants[1].value,
            Some(EnumExpr::Int(Spanned { value: 2, .. }))
        ));
    }

    #[test]
    fn anonymous_enum() {
        let h = parse_h("enum { A = 1, B = 2 };");
        let Item::Enum(decl) = &h.items[0] else {
            panic!()
        };
        assert!(decl.name.is_none());
        assert_eq!(decl.variants.len(), 2);
    }

    #[test]
    fn auto_increment() {
        let h = parse_h("enum EFoo { A = 1, B, C = 5, D };");
        let Item::Enum(decl) = &h.items[0] else {
            panic!()
        };
        assert!(matches!(
            decl.variants[0].value,
            Some(EnumExpr::Int(Spanned { value: 1, .. }))
        ));
        assert!(decl.variants[1].value.is_none()); // B = 2 (auto)
        assert!(matches!(
            decl.variants[2].value,
            Some(EnumExpr::Int(Spanned { value: 5, .. }))
        ));
        assert!(decl.variants[3].value.is_none()); // D = 6 (auto)
    }

    #[test]
    fn enum_with_ident_value() {
        let h = parse_h("enum EFoo { A = NO_SOUND_TYPES };");
        let Item::Enum(decl) = &h.items[0] else {
            panic!()
        };
        assert!(matches!(
            &decl.variants[0].value,
            Some(EnumExpr::Ident(s)) if s.value == "NO_SOUND_TYPES"
        ));
    }

    #[test]
    fn enum_with_bitor_expression() {
        let h = parse_h("enum EFoo { A = 1 | 2 | 4 };");
        let Item::Enum(decl) = &h.items[0] else {
            panic!()
        };
        assert!(
            matches!(&decl.variants[0].value, Some(EnumExpr::BitOr(terms)) if terms.len() == 3)
        );
    }

    #[test]
    fn enum_with_shift_expression() {
        let h = parse_h("enum EFoo { A = 1 << 0, B = 1 << 1 };");
        let Item::Enum(decl) = &h.items[0] else {
            panic!()
        };
        assert!(
            matches!(&decl.variants[0].value, Some(EnumExpr::Shift(terms)) if terms.len() == 2)
        );
    }

    #[test]
    fn enum_trailing_comma() {
        let h = parse_h("enum EFoo { A = 1, B = 2, };");
        let Item::Enum(decl) = &h.items[0] else {
            panic!()
        };
        assert_eq!(decl.variants.len(), 2);
    }

    #[test]
    fn enum_no_trailing_semicolon() {
        let h = parse_h("enum EFoo { A = 1 }");
        assert_eq!(h.items.len(), 1);
    }

    #[test]
    fn define_positive() {
        let h = parse_h("#define FOO 42");
        let Item::Define(d) = &h.items[0] else {
            panic!()
        };
        assert_eq!(d.name.value, "FOO");
        assert_eq!(d.value, Some(42));
    }

    #[test]
    fn define_negative() {
        let h = parse_h("#define FOO -42");
        let Item::Define(d) = &h.items[0] else {
            panic!()
        };
        assert_eq!(d.value, Some(-42));
    }

    #[test]
    fn namespace_with_enums() {
        let h = parse_h("namespace NFoo { enum EA { X = 1 }; }");
        let Item::Namespace(ns) = &h.items[0] else {
            panic!()
        };
        assert_eq!(ns.name, "NFoo");
        assert_eq!(ns.items.len(), 1);
    }

    #[test]
    fn ifdef_with_else() {
        let h = parse_h("#ifdef _WINDOWS\n#define FOO 1\n#else\n#define FOO 2\n#endif");
        let Item::Conditional(ifdef) = &h.items[0] else {
            panic!()
        };
        assert_eq!(ifdef.condition, "_WINDOWS");
        assert_eq!(ifdef.if_branch.len(), 1);
        assert_eq!(ifdef.else_branch.as_ref().unwrap().len(), 1);
        assert!(!ifdef.inverted);
    }

    #[test]
    fn ifndef_as_item() {
        // Put `#ifndef` after a namespace so it's not consumed by the prologue.
        let h = parse_h("namespace N { #ifndef _WINDOWS\n#define FOO 1\n#endif }");
        let Item::Namespace(ns) = &h.items[0] else {
            panic!()
        };
        assert_eq!(ns.items.len(), 1);
        let Item::Conditional(ifdef) = &ns.items[0] else {
            panic!()
        };
        assert!(ifdef.inverted);
        assert_eq!(ifdef.if_branch.len(), 1);
    }

    #[test]
    fn full_header_file() {
        let h = parse_h(
            "#pragma once\n\
             #ifndef __FOO_H__\n\
             #define __FOO_H__\n\
             #define MAX_THINGS 100\n\
             enum EFoo { A = 1, B = 2 };\n\
             #endif",
        );
        // Prologue consumed: #pragma, #ifndef, #define. Items: MAX_THINGS, EFoo.
        assert_eq!(h.items.len(), 2);
    }

    #[test]
    fn file_with_no_guard() {
        let h = parse_h("#define FOO 1\nenum EBar { A };\n");
        assert_eq!(h.items.len(), 2);
    }

    // --- error cases ---

    #[test]
    fn err_unterminated_enum() {
        let kind = parse_h_err("enum EFoo { A = 1");
        assert!(matches!(kind, TextParseErrorKind::UnexpectedToken { .. }));
    }

    #[test]
    fn err_unterminated_enum_eof_after_comma() {
        // EOF after a comma is a genuine UnterminatedEnum.
        let kind = parse_h_err("enum EFoo { A = 1,");
        assert!(matches!(kind, TextParseErrorKind::UnterminatedEnum));
    }

    #[test]
    fn err_unterminated_namespace() {
        let kind = parse_h_err("namespace NFoo { enum EA { X = 1 };");
        assert!(matches!(kind, TextParseErrorKind::UnterminatedNamespace));
    }

    #[test]
    fn err_unterminated_ifdef() {
        let kind = parse_h_err("#ifdef _WINDOWS\n#define FOO 1\n");
        assert!(matches!(kind, TextParseErrorKind::UnterminatedIfDef));
    }

    /// Text matching no item is recorded as ignored rather than erroring:
    /// the stock corpus contains four such runs (leftovers from commented-out
    /// definitions), so erroring would fail a stock build.
    #[test]
    fn unknown_top_level_text_is_ignored_not_an_error() {
        let h = parse_h("foo bar");
        assert!(h.items.is_empty());
        assert_eq!(h.ignored.len(), 1);
    }

    /// `#define FOO abc` is a non-numeric macro, which this grammar does not
    /// model (the corpus has none). `FOO` binds nothing and `abc` becomes
    /// ignored text the builder warns about — the same rule as any other
    /// unrecognized top-level token, rather than a special case.
    #[test]
    fn non_numeric_define_value_is_ignored_text() {
        let h = parse_h("#define FOO abc");
        let Item::Define(d) = &h.items[0] else {
            panic!()
        };
        assert_eq!(d.name.value, "FOO");
        assert_eq!(d.value, None);
        assert_eq!(h.ignored.len(), 1);
    }

    #[test]
    fn stray_guard_name_after_endif_consumed() {
        // `#endif __GUARD__` (no `//`) is consumed by the epilogue, matching
        // the old parser's `skip_to_end_of_line`.
        let h = parse_h("#define FOO 1\n#endif __GUARD_NAME__");
        assert_eq!(h.items.len(), 1); // FOO
    }
}
