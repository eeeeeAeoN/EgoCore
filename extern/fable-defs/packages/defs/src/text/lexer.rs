//! A unified tokenizer for the text-def and header grammars (§11.4).
//!
//! The lexer *classifies and delimits*; it never interprets or owns payloads
//! (§11.2). Numbers stay raw (`TokenKind::Number`, value = the source slice) and
//! are interpreted later per-field during evaluation; strings keep their quotes
//! (`TokenKind::Str`) and are unquoted in the parser. Trivia — whitespace
//! (including `\r`), `//` line comments, `/* */` block comments, and decorative
//! *banner* lines (`/*****…` / `*****…` dividers with no closer, used as section
//! separators between defs) — is consumed and never emitted.
//!
//! One lexer serves both grammars: the def grammar (`#definition …`) and the
//! header grammar (`enum`/`#define`/`namespace`/`#ifdef`). The token set is
//! [`TokenKind`] (§11.3): a flat, `Copy`, payload-free enum. Contextual words —
//! `specialises`, `TRUE`/`FALSE`/`BTRUE`/`BFALSE`, `NULL` — stay [`TokenKind::Ident`]
//! and are recognized by the parser/evaluator, not here.
//!
//! The full header-directive set (`#define`/`#ifdef`/`#ifndef`/`#else`/`#endif`
//! /`#pragma`) is represented in the token set — added in Phase 3 for the
//! header-grammar merge.

use super::base::{FileId, ParseError, Span, Spanned};
use derive_more::{Display, Error};
use std::borrow::Cow;

/// A lexical token kind. Flat, `Copy`, payload-free — the raw text lives in
/// [`Token::source`] and the span in [`Token::span`] (§11.3).
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum TokenKind {
    Ident,
    Number,
    Str, // value tokens = raw source slice
    Definition,
    DefinitionTemplate,
    EndDefinition, // def directives (flat)
    Define,
    Ifdef,
    Ifndef,
    Else,
    Endif,
    Pragma,
    Namespace,
    Enum, // header directives/keywords
    Dot,
    LBracket,
    RBracket,
    LParen,
    RParen,
    LBrace,
    RBrace,
    Lt,
    Gt,
    Backslash,
    Pipe,
    Plus,
    Shl,
    Eq,
    Comma,
    Semi, // punctuation (Shl = `<<`, header)
    Eof,
}

/// A token: its kind, its byte span, and the exact source slice it covers.
///
/// `source` is `&input[span.start..span.end]`, so `Number`/`Str` slices
/// reproduce their source byte-for-byte. `Str`'s `source` includes the quotes
/// (the parser strips them).
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub struct Token<'a> {
    pub kind: TokenKind,
    pub span: Span,
    pub source: &'a str,
}

/// What went wrong, without the location.
#[derive(Copy, Clone, Debug, PartialEq, Eq, Display, Error)]
pub enum LexErrorKind {
    #[display("unterminated string")]
    UnterminatedString,
    #[display("unterminated block comment")]
    UnterminatedBlockComment,
    #[display("unexpected character {_0:?}")]
    UnexpectedChar(#[error(not(source))] char),
}

/// A lex error, always spanned.
#[derive(Copy, Clone, Debug, PartialEq, Eq, Display, Error)]
#[display("{kind}")]
pub struct LexError {
    pub kind: LexErrorKind,
    pub span: Span,
}

/// A unified parse-error kind covering both the def and header grammars
/// (P1.3 — merged `DefParseErrorKind` and `HeaderParseErrorKind`).
#[derive(Debug, Display)]
pub enum TextParseErrorKind {
    /// `expected` names what the grammar wanted here; `found` is filled in
    /// uniformly by the `Display` impl, so no construction site formats its own
    /// "found …" half. `Cow` because most sites are literals and two are not.
    #[display("expected {expected}, found {}", describe(*found))]
    UnexpectedToken {
        expected: Cow<'static, str>,
        found: TokenKind,
    },
    #[display("missing #end_definition")]
    MissingEndDefinition,
    /// `Foo[0](args)` — the path ends in an index, so there is no name to call.
    /// Not an `UnexpectedToken`: the token under the cursor (`(`) is fine, it is
    /// the path in front of it that is wrong.
    #[display("method call has no method name: the path ends in an index")]
    MethodNameIsIndex,
    #[display("mismatched tag: opened <{opened}>, closed <\\{closed}>")]
    MismatchedTag { opened: String, closed: String },
    #[display("unterminated string")]
    UnterminatedString,
    #[display("unterminated block comment")]
    UnterminatedBlockComment,
    #[display("unexpected character {_0:?}")]
    UnexpectedChar(char),
    #[display("unterminated namespace")]
    UnterminatedNamespace,
    #[display("unterminated #ifdef")]
    UnterminatedIfDef,
    #[display("unterminated enum")]
    UnterminatedEnum,
    #[display("unknown item")]
    UnknownItem,
    #[display("invalid number")]
    InvalidNumber,
}

/// Convert a lex error to the corresponding [`TextParseErrorKind`].
pub fn lex_error_to_parse_error(e: LexError) -> (Span, TextParseErrorKind) {
    let kind = match e.kind {
        LexErrorKind::UnterminatedString => TextParseErrorKind::UnterminatedString,
        LexErrorKind::UnterminatedBlockComment => TextParseErrorKind::UnterminatedBlockComment,
        LexErrorKind::UnexpectedChar(c) => TextParseErrorKind::UnexpectedChar(c),
    };
    (e.span, kind)
}

/// A short human name for a token kind, for "expected …, found …" messages.
pub fn describe(kind: TokenKind) -> &'static str {
    match kind {
        TokenKind::Ident => "identifier",
        TokenKind::Number => "number",
        TokenKind::Str => "string",
        TokenKind::Definition => "#definition",
        TokenKind::DefinitionTemplate => "#definition_template",
        TokenKind::EndDefinition => "#end_definition",
        TokenKind::Define => "#define",
        TokenKind::Ifdef => "#ifdef",
        TokenKind::Ifndef => "#ifndef",
        TokenKind::Else => "#else",
        TokenKind::Endif => "#endif",
        TokenKind::Pragma => "#pragma",
        TokenKind::Namespace => "namespace",
        TokenKind::Enum => "enum",
        TokenKind::Dot => ".",
        TokenKind::LBracket => "[",
        TokenKind::RBracket => "]",
        TokenKind::LParen => "(",
        TokenKind::RParen => ")",
        TokenKind::LBrace => "{",
        TokenKind::RBrace => "}",
        TokenKind::Lt => "<",
        TokenKind::Gt => ">",
        TokenKind::Backslash => "\\",
        TokenKind::Pipe => "|",
        TokenKind::Plus => "+",
        TokenKind::Shl => "<<",
        TokenKind::Eq => "=",
        TokenKind::Comma => ",",
        TokenKind::Semi => ";",
        TokenKind::Eof => "end of input",
    }
}

/// A shared token cursor for the def and header parsers (P1.1).
///
/// Owns the flat token stream produced by [`lex`] and exposes cursor verbs both
/// parsers previously hand-rolled: `peek`, `peek_at`, `bump`, `at`, `at_ident`,
/// `prev_end`, `expect`, `expect_ident`.
pub struct Cursor<'a> {
    tokens: Vec<Token<'a>>,
    pos: usize,
}

impl<'a> Cursor<'a> {
    pub fn new(tokens: Vec<Token<'a>>) -> Self {
        Self { tokens, pos: 0 }
    }

    /// The current token. Always valid: the stream ends in exactly one `Eof` and
    /// [`bump`](Self::bump) never advances past it.
    pub fn peek(&self) -> Token<'a> {
        self.tokens[self.pos]
    }

    /// The token `n` ahead, saturating at the trailing `Eof`.
    pub fn peek_at(&self, n: usize) -> Token<'a> {
        *self
            .tokens
            .get(self.pos + n)
            .unwrap_or_else(|| self.tokens.last().expect("stream ends in Eof"))
    }

    /// Return the current token and advance (never past `Eof`).
    pub fn bump(&mut self) -> Token<'a> {
        let tok = self.tokens[self.pos];
        if tok.kind != TokenKind::Eof {
            self.pos += 1;
        }
        tok
    }

    pub fn at(&self, kind: TokenKind) -> bool {
        self.peek().kind == kind
    }

    /// Whether the current token is an `Ident` whose text is exactly `name`
    /// (contextual keywords like `specialises`).
    pub fn at_ident(&self, name: &str) -> bool {
        let t = self.peek();
        t.kind == TokenKind::Ident && t.source == name
    }

    /// End offset of the most recently consumed token — used for node spans.
    pub fn prev_end(&self) -> usize {
        self.tokens[self.pos.saturating_sub(1)].span.end
    }

    /// The span of the token just consumed — used to anchor a [`ParseContext`]
    /// on the keyword that opened the construct.
    pub fn prev_span(&self) -> Span {
        self.tokens[self.pos.saturating_sub(1)].span
    }

    /// The file being parsed. Every token carries it, including the trailing
    /// `Eof`, so this is always available.
    pub fn file(&self) -> FileId {
        self.peek().span.file
    }

    /// An error about the token the cursor is sitting on. The error span is the
    /// whole token, so the caret underlines it.
    pub fn err(&self, kind: TextParseErrorKind) -> ParseError<TextParseErrorKind> {
        ParseError::new(self.peek().span, kind)
    }

    pub fn err_at(&self, at: Span, kind: TextParseErrorKind) -> ParseError<TextParseErrorKind> {
        ParseError::new(at, kind)
    }

    pub fn expect(&mut self, kind: TokenKind) -> Result<Token<'a>, ParseError<TextParseErrorKind>> {
        if self.at(kind) {
            Ok(self.bump())
        } else {
            Err(self.unexpected(describe(kind)))
        }
    }

    pub fn expect_ident(
        &mut self,
        what: &'static str,
    ) -> Result<String, ParseError<TextParseErrorKind>> {
        Ok(self.expect_ident_spanned(what)?.value)
    }

    /// [`expect_ident`](Self::expect_ident) keeping the name's span, for AST
    /// nodes whose names have to be pointed at after parsing.
    pub fn expect_ident_spanned(
        &mut self,
        what: &'static str,
    ) -> Result<Spanned<String>, ParseError<TextParseErrorKind>> {
        let t = self.peek();
        if t.kind == TokenKind::Ident {
            self.bump();
            Ok(Spanned {
                span: t.span,
                value: t.source.to_string(),
            })
        } else {
            Err(self.unexpected(what))
        }
    }

    /// An `UnexpectedToken` about the current token, describing what the
    /// grammar wanted. The "found …" half is added by the `Display` impl.
    pub fn unexpected(
        &self,
        expected: impl Into<Cow<'static, str>>,
    ) -> ParseError<TextParseErrorKind> {
        self.err(TextParseErrorKind::UnexpectedToken {
            expected: expected.into(),
            found: self.peek().kind,
        })
    }
}

/// The tokenizer. Borrows the source for the token slices' lifetime.
pub struct Lexer<'a> {
    input: &'a str,
    pos: usize,
    /// Stamped onto every span this lexer produces, so a token knows its file
    /// for as long as it survives — including after specialization copies it
    /// into a definition in a different file.
    file: FileId,
}

impl<'a> Lexer<'a> {
    pub fn new(input: &'a str, file: FileId) -> Self {
        Self {
            input,
            pos: 0,
            file,
        }
    }

    /// Tokenize the whole input, ending with a single [`TokenKind::Eof`].
    /// The first lex error aborts and is returned.
    pub fn tokenize(&mut self) -> Result<Vec<Token<'a>>, LexError> {
        let mut tokens = Vec::new();
        loop {
            self.skip_trivia()?;
            if self.pos >= self.input.len() {
                tokens.push(self.token(TokenKind::Eof, self.pos, self.pos));
                return Ok(tokens);
            }
            tokens.push(self.next_token()?);
        }
    }

    fn rest(&self) -> &'a str {
        &self.input[self.pos..]
    }

    fn peek(&self) -> Option<char> {
        self.rest().chars().next()
    }

    fn peek_at(&self, offset: usize) -> Option<char> {
        self.rest().chars().nth(offset)
    }

    fn token(&self, kind: TokenKind, start: usize, end: usize) -> Token<'a> {
        Token {
            kind,
            span: Span::new(self.file, start, end),
            source: &self.input[start..end],
        }
    }

    /// Consume whitespace (incl. `\r`), `//` line comments, `/* */` block
    /// comments, and decorative *banner* lines (all-separator dividers, see
    /// [`line_is_decorative`](Self::line_is_decorative)). A genuinely
    /// unterminated block comment (one with real content) is a spanned error.
    fn skip_trivia(&mut self) -> Result<(), LexError> {
        loop {
            while let Some(c) = self.peek() {
                if c.is_whitespace() {
                    self.pos += c.len_utf8();
                } else {
                    break;
                }
            }
            let rest = self.rest();
            if rest.starts_with("//") {
                self.skip_to_end_of_line();
                continue;
            }
            if let Some(after_open) = rest.strip_prefix("/*") {
                // Close at the first `*/` after the opener.
                if let Some(off) = after_open.find("*/") {
                    self.pos += 2 + off + 2;
                    continue;
                }
                // No closer. A decorative rule (`/*****…` with nothing but
                // separator chars on the line) is a banner, not a comment —
                // some corpus files use them as section dividers between defs
                // (there is no `*/` anywhere in those files). Skip the line and
                // keep the defs that follow. A `/*` with real content and no
                // closer is a genuine error.
                if self.line_is_decorative() {
                    self.skip_to_end_of_line();
                    continue;
                }
                // Anchor on the `/*` that was never closed, not on the rest of
                // the file: the opener is the thing the reader has to go fix.
                return Err(LexError {
                    kind: LexErrorKind::UnterminatedBlockComment,
                    span: Span::new(self.file, self.pos, (self.pos + 2).min(self.input.len())),
                });
            }
            // A bare decorative banner (`*****…` with no leading `/`). `*` and
            // `/` never begin a valid token in either grammar, so a line of only
            // separator chars is unambiguously a divider.
            if matches!(self.peek(), Some('*' | '/')) && self.line_is_decorative() {
                self.skip_to_end_of_line();
                continue;
            }
            return Ok(());
        }
    }

    /// Advance past the next `\n` (or to EOF if none remains).
    fn skip_to_end_of_line(&mut self) {
        while let Some(c) = self.peek() {
            self.pos += c.len_utf8();
            if c == '\n' {
                break;
            }
        }
    }

    /// Whether the rest of the current line (up to `\n`/EOF) is a decorative
    /// separator: only `*`, `/`, and horizontal whitespace. Precondition: the
    /// next char is `*` or `/`, so the line is non-empty.
    fn line_is_decorative(&self) -> bool {
        for c in self.rest().chars() {
            match c {
                '\n' => break,
                '*' | '/' | ' ' | '\t' | '\r' => {}
                _ => return false,
            }
        }
        true
    }

    /// Dispatch on the next non-trivia character. Precondition: not at EOF.
    fn next_token(&mut self) -> Result<Token<'a>, LexError> {
        let c = self.peek().expect("next_token called at EOF");
        match c {
            '"' => self.lex_string(),
            '#' => self.lex_directive(),
            '0'..='9' => self.lex_number(),
            '-' if self.peek_at(1).is_some_and(|d| d.is_ascii_digit()) => self.lex_number(),
            c if c == '_' || c.is_ascii_alphabetic() => Ok(self.lex_ident_or_keyword()),
            _ => self.lex_punct(),
        }
    }

    /// Raw number slice: optional leading `-`, digits, optional `.frac`,
    /// optional trailing `f`. No interpretation (§11.2) — `kind = Number`,
    /// meaning = `source`. Precondition: a digit is present (guaranteed by the
    /// dispatch in [`next_token`](Self::next_token)).
    fn lex_number(&mut self) -> Result<Token<'a>, LexError> {
        let start = self.pos;
        if self.peek() == Some('-') {
            self.pos += 1;
        }
        while self.peek().is_some_and(|c| c.is_ascii_digit()) {
            self.pos += 1;
        }
        if self.peek() == Some('.') {
            self.pos += 1;
            while self.peek().is_some_and(|c| c.is_ascii_digit()) {
                self.pos += 1;
            }
        }
        if self.peek() == Some('f') {
            self.pos += 1;
        }
        Ok(self.token(TokenKind::Number, start, self.pos))
    }

    /// Raw string slice including the quotes. Unterminated → spanned error.
    /// Precondition: positioned on the opening `"`.
    fn lex_string(&mut self) -> Result<Token<'a>, LexError> {
        let start = self.pos;
        self.pos += 1; // opening quote
        while let Some(c) = self.peek() {
            if c == '"' {
                self.pos += 1; // closing quote
                return Ok(self.token(TokenKind::Str, start, self.pos));
            }
            self.pos += c.len_utf8();
        }
        // Anchor on the opening quote. The scan ran to EOF, but the byte the
        // reader has to go fix is where the string started.
        Err(LexError {
            kind: LexErrorKind::UnterminatedString,
            span: Span::new(self.file, start, (start + 1).min(self.input.len())),
        })
    }

    /// `[A-Za-z_][A-Za-z0-9_]*`; `namespace`/`enum` map to keyword kinds,
    /// everything else is `Ident`. Precondition: positioned on `_` or a letter.
    fn lex_ident_or_keyword(&mut self) -> Token<'a> {
        let start = self.pos;
        self.pos += 1; // first char (letter or `_`)
        while self
            .peek()
            .is_some_and(|c| c.is_ascii_alphanumeric() || c == '_')
        {
            self.pos += 1;
        }
        let kind = match &self.input[start..self.pos] {
            "namespace" => TokenKind::Namespace,
            "enum" => TokenKind::Enum,
            _ => TokenKind::Ident,
        };
        self.token(kind, start, self.pos)
    }

    /// `#` + a `[A-Za-z0-9_]*` run, matched whole against the known directives.
    /// Reading the entire word makes the match longest — `#definition_template`
    /// wins over `#definition`, `#definition` over `#define` — and enforces the
    /// word boundary (`#definitionX` matches nothing). Precondition: on `#`.
    fn lex_directive(&mut self) -> Result<Token<'a>, LexError> {
        let start = self.pos;
        let mut end = start + 1; // past '#'
        let bytes = self.input.as_bytes();
        while end < bytes.len() && (bytes[end] == b'_' || bytes[end].is_ascii_alphanumeric()) {
            end += 1;
        }
        let kind = match &self.input[start..end] {
            "#definition_template" => TokenKind::DefinitionTemplate,
            "#definition" => TokenKind::Definition,
            "#end_definition" => TokenKind::EndDefinition,
            "#define" => TokenKind::Define,
            "#ifdef" => TokenKind::Ifdef,
            "#ifndef" => TokenKind::Ifndef,
            "#else" => TokenKind::Else,
            "#endif" => TokenKind::Endif,
            "#pragma" => TokenKind::Pragma,
            _ => {
                return Err(LexError {
                    kind: LexErrorKind::UnexpectedChar('#'),
                    span: Span::new(self.file, start, start + 1),
                });
            }
        };
        self.pos = end;
        Ok(self.token(kind, start, end))
    }

    /// Single-char punctuation, plus greedy `<<` → `Shl` (§11.3: safe — def
    /// bodies never contain `<<`; tagged blocks are single `<` and `<\`).
    fn lex_punct(&mut self) -> Result<Token<'a>, LexError> {
        let start = self.pos;
        let c = self.peek().expect("lex_punct called at EOF");
        if c == '<' {
            if self.peek_at(1) == Some('<') {
                self.pos += 2;
                return Ok(self.token(TokenKind::Shl, start, self.pos));
            }
            self.pos += 1;
            return Ok(self.token(TokenKind::Lt, start, self.pos));
        }
        let kind = match c {
            '.' => TokenKind::Dot,
            '[' => TokenKind::LBracket,
            ']' => TokenKind::RBracket,
            '(' => TokenKind::LParen,
            ')' => TokenKind::RParen,
            '{' => TokenKind::LBrace,
            '}' => TokenKind::RBrace,
            '>' => TokenKind::Gt,
            '\\' => TokenKind::Backslash,
            '|' => TokenKind::Pipe,
            '+' => TokenKind::Plus,
            '=' => TokenKind::Eq,
            ',' => TokenKind::Comma,
            ';' => TokenKind::Semi,
            other => {
                return Err(LexError {
                    kind: LexErrorKind::UnexpectedChar(other),
                    span: Span::new(self.file, start, start + other.len_utf8()),
                });
            }
        };
        self.pos += c.len_utf8(); // all matched punctuation is single-byte ASCII
        Ok(self.token(kind, start, self.pos))
    }
}

/// Tokenize `input` into a `Vec` ending with a single `Eof`, or the first
/// [`LexError`].
pub fn lex(input: &str, file: FileId) -> Result<Vec<Token<'_>>, LexError> {
    Lexer::new(input, file).tokenize()
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Lex, dropping the trailing `Eof`, and collect just the kinds.
    fn kinds(input: &str) -> Vec<TokenKind> {
        let mut toks = lex(input, FileId::ANONYMOUS).expect("lex ok");
        assert_eq!(toks.last().map(|t| t.kind), Some(TokenKind::Eof));
        toks.pop();
        toks.into_iter().map(|t| t.kind).collect()
    }

    /// Lex, dropping the trailing `Eof`, and collect `(kind, source)` pairs.
    fn toks(input: &str) -> Vec<(TokenKind, &str)> {
        let mut toks = lex(input, FileId::ANONYMOUS).expect("lex ok");
        toks.pop(); // Eof
        toks.into_iter().map(|t| (t.kind, t.source)).collect()
    }

    fn lex_err(input: &str) -> LexErrorKind {
        lex(input, FileId::ANONYMOUS)
            .expect_err("expected lex error")
            .kind
    }

    use TokenKind::*;

    // --- numbers: raw slice, no interpretation --------------------------------

    #[test]
    fn integer() {
        assert_eq!(toks("42"), vec![(Number, "42")]);
        assert_eq!(toks("42282949"), vec![(Number, "42282949")]);
    }

    #[test]
    fn negative_integer() {
        assert_eq!(toks("-42"), vec![(Number, "-42")]);
        assert_eq!(toks("-42282949"), vec![(Number, "-42282949")]);
    }

    #[test]
    fn float_forms() {
        // Every float form keeps its exact source — interpretation is deferred.
        assert_eq!(toks("4.2"), vec![(Number, "4.2")]);
        assert_eq!(toks("4.2f"), vec![(Number, "4.2f")]);
        assert_eq!(toks("4."), vec![(Number, "4.")]);
        assert_eq!(toks("-4.2"), vec![(Number, "-4.2")]);
        assert_eq!(toks("-4.2f"), vec![(Number, "-4.2f")]);
        assert_eq!(toks("-4."), vec![(Number, "-4.")]);
    }

    #[test]
    fn number_stops_at_delimiter() {
        assert_eq!(
            toks("Time[0]"),
            vec![
                (Ident, "Time"),
                (LBracket, "["),
                (Number, "0"),
                (RBracket, "]")
            ]
        );
    }

    // --- strings: raw slice incl. quotes --------------------------------------

    #[test]
    fn string_keeps_quotes() {
        assert_eq!(
            toks(r#""Hello, World!""#),
            vec![(Str, r#""Hello, World!""#)]
        );
    }

    #[test]
    fn string_source_reproduces_exactly() {
        let input = r#"Name "Test""#;
        let t = &lex(input, FileId::ANONYMOUS).unwrap()[1];
        assert_eq!(t.kind, Str);
        assert_eq!(&input[t.span.start..t.span.end], t.source);
        assert_eq!(t.source, r#""Test""#);
    }

    // --- identifiers & contextual words stay Ident ----------------------------

    #[test]
    fn identifier() {
        assert_eq!(toks("GRAPHIC_NULL"), vec![(Ident, "GRAPHIC_NULL")]);
    }

    #[test]
    fn contextual_words_are_idents() {
        // TRUE/FALSE/BTRUE/BFALSE/NULL/specialises are recognized by the parser,
        // not the lexer (§11.3).
        for w in ["TRUE", "FALSE", "BTRUE", "BFALSE", "NULL", "specialises"] {
            assert_eq!(kinds(w), vec![Ident], "{w}");
        }
    }

    #[test]
    fn keywords() {
        assert_eq!(kinds("namespace"), vec![Namespace]);
        assert_eq!(kinds("enum"), vec![Enum]);
    }

    // --- paths & indices ------------------------------------------------------

    #[test]
    fn nested_path() {
        assert_eq!(kinds("Stats.ExperienceWorth"), vec![Ident, Dot, Ident]);
    }

    #[test]
    fn nested_field_and_index() {
        assert_eq!(
            kinds("Time[0].SkyTexture0"),
            vec![Ident, LBracket, Number, RBracket, Dot, Ident]
        );
    }

    // --- expressions ----------------------------------------------------------

    #[test]
    fn constructor_with_args() {
        assert_eq!(
            kinds("CRGBColour(255, 128, 64, 255)"),
            vec![
                Ident, LParen, Number, Comma, Number, Comma, Number, Comma, Number, RParen
            ]
        );
    }

    #[test]
    fn empty_constructor() {
        assert_eq!(kinds("CRGBColour()"), vec![Ident, LParen, RParen]);
    }

    #[test]
    fn add_and_bitor() {
        assert_eq!(kinds("1 + 2 + 3"), vec![Number, Plus, Number, Plus, Number]);
        assert_eq!(kinds("A | B | C"), vec![Ident, Pipe, Ident, Pipe, Ident]);
    }

    // --- tagged-block delimiters ----------------------------------------------

    #[test]
    fn tagged_block_open() {
        assert_eq!(kinds("<CCreatureDef>"), vec![Lt, Ident, Gt]);
    }

    #[test]
    fn tagged_block_close() {
        // `<\Tag>` is single `<` then `\` — never `Shl`.
        assert_eq!(kinds("<\\CCreatureDef>"), vec![Lt, Backslash, Ident, Gt]);
    }

    #[test]
    fn tagged_block_roundtrip() {
        assert_eq!(
            kinds("<A>\n  Health 100;\n<\\A>"),
            vec![Lt, Ident, Gt, Ident, Number, Semi, Lt, Backslash, Ident, Gt]
        );
    }

    // --- directives -----------------------------------------------------------

    #[test]
    fn def_directives() {
        assert_eq!(kinds("#definition"), vec![Definition]);
        assert_eq!(kinds("#definition_template"), vec![DefinitionTemplate]);
        assert_eq!(kinds("#end_definition"), vec![EndDefinition]);
    }

    #[test]
    fn directive_longest_match() {
        // `#definition_template` must win over the `#definition` prefix.
        assert_eq!(
            kinds("#definition_template OBJECT T"),
            vec![DefinitionTemplate, Ident, Ident]
        );
        assert_eq!(
            kinds("#definition OBJECT T"),
            vec![Definition, Ident, Ident]
        );
    }

    #[test]
    fn header_directives() {
        assert_eq!(kinds("#define"), vec![Define]);
        assert_eq!(kinds("#ifdef"), vec![Ifdef]);
        assert_eq!(kinds("#ifndef"), vec![Ifndef]);
        assert_eq!(kinds("#else"), vec![Else]);
        assert_eq!(kinds("#endif"), vec![Endif]);
        assert_eq!(kinds("#pragma"), vec![Pragma]);
    }

    #[test]
    fn def_header_line() {
        assert_eq!(
            kinds("#definition OBJECT CHILD specialises PARENT"),
            vec![Definition, Ident, Ident, Ident, Ident]
        );
    }

    #[test]
    fn unknown_directive_errors() {
        assert_eq!(lex_err("#definitionX"), LexErrorKind::UnexpectedChar('#'));
    }

    // --- header operators -----------------------------------------------------

    #[test]
    fn enum_decl() {
        assert_eq!(
            kinds("enum EFoo { A = 1, B = 2 };"),
            vec![
                Enum, Ident, LBrace, Ident, Eq, Number, Comma, Ident, Eq, Number, RBrace, Semi
            ]
        );
    }

    #[test]
    fn shift_is_greedy() {
        assert_eq!(kinds("1 << 2"), vec![Number, Shl, Number]);
        assert_eq!(toks("<<"), vec![(Shl, "<<")]);
    }

    #[test]
    fn define_line() {
        assert_eq!(kinds("#define FOO 3"), vec![Define, Ident, Number]);
    }

    // --- trivia ---------------------------------------------------------------

    #[test]
    fn line_comment_consumed() {
        assert_eq!(
            kinds("// just a comment\nHealth 100;"),
            vec![Ident, Number, Semi]
        );
    }

    #[test]
    fn block_comment_consumed() {
        assert_eq!(kinds("/* block */\nHealth 100;"), vec![Ident, Number, Semi]);
    }

    #[test]
    fn inline_block_comment() {
        assert_eq!(
            kinds(r#"Name /* inline */ "Test";"#),
            vec![Ident, Str, Semi]
        );
    }

    #[test]
    fn multiline_block_comment() {
        assert_eq!(kinds("/* multi\n line */\nX 1;"), vec![Ident, Number, Semi]);
    }

    #[test]
    fn slash_star_banner_is_trivia() {
        // A `/*****…` decorative rule with no `*/` is a section separator, not
        // an (unterminated) comment. The def that follows still lexes.
        let input = "#end_definition\n/*******************************\n#definition OBJECT T";
        assert_eq!(kinds(input), vec![EndDefinition, Definition, Ident, Ident]);
    }

    #[test]
    fn bare_star_banner_is_trivia() {
        let input = "#end_definition\n*******************************\n#definition OBJECT T";
        assert_eq!(kinds(input), vec![EndDefinition, Definition, Ident, Ident]);
    }

    #[test]
    fn banner_keeps_following_tokens() {
        assert_eq!(kinds("/*****\nHealth 100;"), vec![Ident, Number, Semi]);
        assert_eq!(kinds("*****\nHealth 100;"), vec![Ident, Number, Semi]);
    }

    #[test]
    fn crlf_whitespace_and_spans() {
        // `\r` is whitespace; spans are byte offsets into the CRLF source.
        let input = "Health\r\n100";
        let t = toks(input);
        assert_eq!(t, vec![(Ident, "Health"), (Number, "100")]);
        let full = lex(input, FileId::ANONYMOUS).unwrap();
        assert_eq!(full[1].span, Span::new(FileId::ANONYMOUS, 8, 11));
    }

    #[test]
    fn only_trivia_yields_eof() {
        for input in ["", "   \n\t  \r\n  ", "// line\n/* block */\n"] {
            assert_eq!(
                lex(input, FileId::ANONYMOUS)
                    .unwrap()
                    .into_iter()
                    .map(|t| t.kind)
                    .collect::<Vec<_>>(),
                vec![Eof]
            );
        }
    }

    // --- errors ---------------------------------------------------------------

    #[test]
    fn unterminated_string() {
        assert_eq!(
            lex_err("Name \"no close\n"),
            LexErrorKind::UnterminatedString
        );
    }

    #[test]
    fn unterminated_block_comment() {
        // A `/*` with real content and no `*/` is a genuine error — only an
        // all-separator line is treated as a decorative banner.
        assert_eq!(
            lex_err("Health /* never closes"),
            LexErrorKind::UnterminatedBlockComment
        );
        assert_eq!(
            lex_err("/* real comment, no closer\nX 1;"),
            LexErrorKind::UnterminatedBlockComment
        );
    }

    #[test]
    fn unexpected_char() {
        assert_eq!(lex_err("Health @ 1"), LexErrorKind::UnexpectedChar('@'));
        // A bare `-` not followed by a digit is not a number start.
        assert_eq!(lex_err("- 1"), LexErrorKind::UnexpectedChar('-'));
    }

    #[test]
    fn error_span_points_at_offender() {
        let e = lex("Health @", FileId::ANONYMOUS).unwrap_err();
        assert_eq!(e.kind, LexErrorKind::UnexpectedChar('@'));
        assert_eq!(e.span, Span::new(FileId::ANONYMOUS, 7, 8));
    }

    // --- eof ------------------------------------------------------------------

    #[test]
    fn always_ends_with_eof() {
        let toks = lex("Health 100;", FileId::ANONYMOUS).unwrap();
        let last = toks.last().unwrap();
        assert_eq!(last.kind, Eof);
        assert_eq!(last.source, "");
        assert_eq!(last.span, Span::new(FileId::ANONYMOUS, 11, 11));
    }
}
