//! Lowering: convert parsed text Definitions into typed binary def structs.
//!
//! Three composable primitives:
//! - [`Evaluator`] — evaluates a single [`Expr`] to a value using a [`SymbolTable`]
//! - [`Args`] — reads positional values from a constructor or method-call argument list
//! - [`DefReader`] — scans [`Statement`]s by name, consuming fields and producing values or sub-readers
//!
//! These replace the old `FieldReader` (which was never fully defined). The design
//! follows the spec laid out for text→binary compilation: depth-based path matching,
//! consumed-entry tracking, and recursive composition for indexed sub-structs and
//! tagged blocks.

use defs::def::text::{
    Call, Expr, PathSegment, Span, Spanned, Statement, SymbolTable, number_is_float,
};

/// The property path a statement is addressed by: a field's own path, or the
/// object a method is called on. Tagged blocks are addressed by tag rather than
/// by path, so they have none.
///
/// Spelled out rather than wildcarded on purpose: a fourth [`Statement`] variant
/// should have to decide what its path is, not inherit "none" silently from
/// three separate copies of this match.
pub(crate) fn statement_path(stmt: &Spanned<Statement>) -> Option<&[PathSegment]> {
    match &stmt.value {
        Statement::Field(field) => Some(&field.path.segments),
        Statement::MethodCall(mc) => Some(&mc.object.segments),
        Statement::TaggedBlock(_) => None,
    }
}

/// If `stmt` is a leaf field whose path is exactly `name` at `depth` (i.e. ends there), return its
/// value expression. Used by the by-name leaf accessors so the path-matching lives in one place.
fn leaf_field<'a>(
    stmt: &'a Spanned<Statement>,
    depth: usize,
    name: &str,
) -> Option<&'a Spanned<Expr>> {
    let Statement::Field(field) = &stmt.value else {
        return None;
    };
    let segments = &field.path.segments;
    match segments.get(depth) {
        Some(PathSegment::Field(n)) if n == name && segments.len() == depth + 1 => {
            Some(&field.expr)
        }
        _ => None,
    }
}

/// The bare `NULL` token is the def language's "no reference" sentinel
/// (`OnDeathObject NULL;`, `PrimaryEffect NULL;`, …). It is not a header
/// `#define`, so it reaches the evaluator as an unresolved symbol; the game
/// compiler lowers it to 0 — verified against retail (`on_death_object == 0` on
/// 2857/2858 `CThingObjectDef` entries). Handling it here (the single source of
/// truth for expression evaluation) covers every field kind uniformly:
/// def-references, enums, and the hand-written lowerings alike.
pub(crate) fn is_null_ref(name: &str) -> bool {
    name == "NULL"
}

/// Parse an integer-shaped [`Expr::Number`] source (`-?[0-9]+`) as `i64`. The
/// lexer already validated the shape, so this only fails on `i64` overflow.
fn parse_number_i64(s: &str) -> Result<i64, EvalError> {
    s.parse::<i64>()
        .map_err(|_| EvalError::InvalidNumber(s.to_string()))
}

/// Parse a float-shaped [`Expr::Number`] source as `f32`, stripping a trailing
/// `f` first (`4.2f` → `4.2`), matching the old parser's `Expr::Float` handling.
fn parse_number_f32(s: &str) -> Result<f32, EvalError> {
    s.trim_end_matches('f')
        .parse::<f32>()
        .map_err(|_| EvalError::InvalidNumber(s.to_string()))
}

/// A short human name for an expression's shape, for the `found` side of a
/// [`EvalError::TypeMismatch`] message.
fn expr_kind_name(expr: &Expr) -> &'static str {
    match expr {
        Expr::Number(_) => "a number",
        Expr::Bool(_) => "a boolean",
        Expr::String(_) => "a string literal",
        Expr::Symbol(_) => "a symbol",
        Expr::Constructor(_) => "a constructor",
        Expr::BitOr(_) => "a bitwise-or expression",
        Expr::Add(_) => "an additive expression",
    }
}

// ── Evaluator ────────────────────────────────────────────────────────────────

/// Evaluates parsed [`Expr`] values against a [`SymbolTable`].
///
/// This is the single source of truth for expression evaluation. The old
/// `Expr::eval_*` methods from `text/mod.rs` have been removed — all evaluation
/// lives here.
#[derive(Clone, Copy)]
pub struct Evaluator<'s> {
    symbols: &'s SymbolTable,
}

#[derive(Debug)]
pub enum EvalError {
    UnknownSymbol(String),
    OutOfRange(i64),
    Overflow,
    /// A numeric literal that failed to parse at evaluation time (e.g. an
    /// integer literal that overflows `i64`). The lexer validates number *shape*,
    /// so this only fires on out-of-`i64`-range integers.
    InvalidNumber(String),
    /// The value's shape doesn't match what the field's type accepts (e.g. a
    /// string literal for a numeric field). Both sides are named for the message.
    TypeMismatch {
        expected: &'static str,
        found: &'static str,
    },
    ExpectedConstructor {
        found: &'static str,
    },
    WrongConstructor {
        expected: String,
        found: String,
    },
}

impl std::fmt::Display for EvalError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            EvalError::UnknownSymbol(s) => write!(f, "unknown constant `{s}`"),
            EvalError::OutOfRange(n) => write!(f, "value {n} out of range"),
            EvalError::Overflow => f.write_str("arithmetic overflow"),
            EvalError::InvalidNumber(s) => write!(f, "invalid number {s}"),
            EvalError::TypeMismatch { expected, found } => {
                write!(f, "expected {expected}, found {found}")
            }
            EvalError::ExpectedConstructor { found } => {
                write!(f, "expected a constructor, found {found}")
            }
            EvalError::WrongConstructor { expected, found } => {
                write!(f, "expected constructor {expected}, found {found}")
            }
        }
    }
}

impl<'s> Evaluator<'s> {
    pub fn new(symbols: &'s SymbolTable) -> Self {
        Self { symbols }
    }

    pub fn i32(&self, expr: &Spanned<Expr>) -> Result<i32, EvalError> {
        use EvalError as E;
        match &expr.value {
            // A numeric literal, interpreted per-type here (§11.2): a float-shaped
            // literal in an int context truncates via `f32 as i32` (matching the
            // old `Expr::Float(n) => *n as i32`); an integer-shaped one parses as
            // `i64` then range-checks into `i32`.
            Expr::Number(s) if number_is_float(s) => Ok(parse_number_f32(s)? as i32),
            Expr::Number(s) => {
                let n = parse_number_i64(s)?;
                i32::try_from(n).map_err(|_| E::OutOfRange(n))
            }
            Expr::Bool(b) => Ok(*b as i32),
            Expr::Symbol(name) if is_null_ref(name) => Ok(0),
            Expr::Symbol(name) => {
                let n = self
                    .symbols
                    .lookup(name)
                    .ok_or_else(|| E::UnknownSymbol(name.clone()))?;
                i32::try_from(n).map_err(|_| E::OutOfRange(n))
            }
            Expr::BitOr(parts) => parts.iter().try_fold(0i32, |acc, p| Ok(acc | self.i32(p)?)),
            Expr::Add(parts) => parts.iter().try_fold(0i32, |acc, p| {
                acc.checked_add(self.i32(p)?).ok_or(E::Overflow)
            }),
            other => Err(E::TypeMismatch {
                expected: "a number",
                found: expr_kind_name(other),
            }),
        }
    }

    pub fn u32(&self, expr: &Spanned<Expr>) -> Result<u32, EvalError> {
        use EvalError as E;
        match &expr.value {
            // Integer-shaped only — a float-shaped literal has no `u32`
            // interpretation (the old parser produced no `Expr::Float` arm here),
            // so it falls through to `TypeMismatch`.
            Expr::Number(s) if !number_is_float(s) => {
                let n = parse_number_i64(s)?;
                u32::try_from(n).map_err(|_| E::OutOfRange(n))
            }
            Expr::Bool(b) => Ok(*b as u32),
            Expr::Symbol(name) if is_null_ref(name) => Ok(0),
            Expr::Symbol(name) => {
                let n = self
                    .symbols
                    .lookup(name)
                    .ok_or_else(|| E::UnknownSymbol(name.clone()))?;
                u32::try_from(n).map_err(|_| E::OutOfRange(n))
            }
            Expr::BitOr(parts) => parts.iter().try_fold(0u32, |acc, p| Ok(acc | self.u32(p)?)),
            Expr::Add(parts) => parts.iter().try_fold(0u32, |acc, p| {
                acc.checked_add(self.u32(p)?).ok_or(E::Overflow)
            }),
            other => Err(E::TypeMismatch {
                expected: "a number",
                found: expr_kind_name(other),
            }),
        }
    }

    pub fn f32(&self, expr: &Spanned<Expr>) -> Result<f32, EvalError> {
        use EvalError as E;
        match &expr.value {
            // Float-shaped parses as `f32` (after stripping a trailing `f`);
            // integer-shaped parses as `i64` then widens (matching the old
            // `Expr::Integer(n) => *n as f32`).
            Expr::Number(s) if number_is_float(s) => parse_number_f32(s),
            Expr::Number(s) => Ok(parse_number_i64(s)? as f32),
            Expr::Symbol(name) => self
                .symbols
                .lookup(name)
                .map(|v| v as f32)
                .ok_or_else(|| E::UnknownSymbol(name.clone())),
            Expr::Add(parts) => parts.iter().try_fold(0f32, |acc, p| Ok(acc + self.f32(p)?)),
            other => Err(E::TypeMismatch {
                expected: "a number",
                found: expr_kind_name(other),
            }),
        }
    }

    pub fn bool(&self, expr: &Spanned<Expr>) -> Result<bool, EvalError> {
        match &expr.value {
            Expr::Bool(b) => Ok(*b),
            Expr::Symbol(s) if s == "true" || s == "TRUE" => Ok(true),
            Expr::Symbol(s) if s == "false" || s == "FALSE" => Ok(false),
            other => Err(EvalError::TypeMismatch {
                expected: "a boolean",
                found: expr_kind_name(other),
            }),
        }
    }

    pub fn string<'e>(&self, expr: &'e Spanned<Expr>) -> Result<&'e str, EvalError> {
        match &expr.value {
            // A bare symbol in a string field is its own text — the corpus
            // writes plain identifiers where a name is wanted.
            Expr::String(s) | Expr::Symbol(s) => Ok(s),
            other => Err(EvalError::TypeMismatch {
                expected: "a string",
                found: expr_kind_name(other),
            }),
        }
    }

    /// Evaluate an expression as a `usize` — used for index expressions like `[0]`.
    pub fn usize(&self, expr: &Spanned<Expr>) -> Result<usize, EvalError> {
        let n = self.i32(expr)?;
        usize::try_from(n).map_err(|_| EvalError::OutOfRange(n as i64))
    }

    /// Validate that `expr` is a `Constructor` with the expected name and return
    /// a reference to its [`Call`] (name + argument list).
    pub fn call<'e>(&self, expr: &'e Spanned<Expr>, name: &str) -> Result<&'e Call, EvalError> {
        match &expr.value {
            Expr::Constructor(call) if call.name == name => Ok(call),
            Expr::Constructor(call) => {
                let found = call.name.clone();
                Err(EvalError::WrongConstructor {
                    expected: name.to_string(),
                    found,
                })
            }
            _ => Err(EvalError::ExpectedConstructor {
                found: "not a constructor",
            }),
        }
    }
}

impl<'s> Evaluator<'s> {
    /// Fallible positional-arg accessor: eval arg `idx` as i32, default on error.
    pub fn arg_i32_or(&self, args: &[Spanned<Expr>], idx: usize, default: i32) -> i32 {
        args.get(idx)
            .and_then(|e| self.i32(e).ok())
            .unwrap_or(default)
    }

    /// Fallible positional-arg accessor: eval arg `idx` as f32, default on error.
    pub fn arg_f32_or(&self, args: &[Spanned<Expr>], idx: usize, default: f32) -> f32 {
        args.get(idx)
            .and_then(|e| self.f32(e).ok())
            .unwrap_or(default)
    }

    /// Fallible positional-arg accessor: eval arg `idx` as u32, default on error.
    pub fn arg_u32_or(&self, args: &[Spanned<Expr>], idx: usize, default: u32) -> u32 {
        args.get(idx)
            .and_then(|e| self.u32(e).ok())
            .unwrap_or(default)
    }

    /// Fallible positional-arg accessor: eval arg `idx` as bool, default on error.
    pub fn arg_bool_or(&self, args: &[Spanned<Expr>], idx: usize, default: bool) -> bool {
        args.get(idx)
            .and_then(|e| self.bool(e).ok())
            .unwrap_or(default)
    }

    /// Fallible positional-arg accessor: eval arg `idx` as String, default on error.
    pub fn arg_string_or(&self, args: &[Spanned<Expr>], idx: usize, default: &str) -> String {
        args.get(idx)
            .and_then(|e| self.string(e).ok())
            .map(String::from)
            .unwrap_or_else(|| default.to_string())
    }

    /// Fallible positional-arg accessor: eval arg `idx` as String, None on missing/discordant.
    pub fn arg_string_opt(&self, args: &[Spanned<Expr>], idx: usize) -> Option<String> {
        args.get(idx)
            .and_then(|e| self.string(e).ok())
            .map(String::from)
    }
}

impl<'s> Evaluator<'s> {
    pub fn eval_i32(&self, expr: &Spanned<Expr>) -> Result<i32, DefReaderError> {
        let span = expr.span;
        self.i32(expr).map_err(|e| DefReaderError::Eval(e, span))
    }
    pub fn eval_u32(&self, expr: &Spanned<Expr>) -> Result<u32, DefReaderError> {
        let span = expr.span;
        self.u32(expr).map_err(|e| DefReaderError::Eval(e, span))
    }
    pub fn eval_f32(&self, expr: &Spanned<Expr>) -> Result<f32, DefReaderError> {
        let span = expr.span;
        self.f32(expr).map_err(|e| DefReaderError::Eval(e, span))
    }
    pub fn eval_bool(&self, expr: &Spanned<Expr>) -> Result<bool, DefReaderError> {
        let span = expr.span;
        self.bool(expr).map_err(|e| DefReaderError::Eval(e, span))
    }
    pub fn eval_string(&self, expr: &Spanned<Expr>) -> Result<String, DefReaderError> {
        let span = expr.span;
        self.string(expr)
            .map(|s| s.to_string())
            .map_err(|e| DefReaderError::Eval(e, span))
    }
    pub fn eval_usize(&self, expr: &Spanned<Expr>) -> Result<usize, DefReaderError> {
        let span = expr.span;
        self.usize(expr).map_err(|e| DefReaderError::Eval(e, span))
    }
}

// ── Args ─────────────────────────────────────────────────────────────────────

/// Positional reader over a constructor or method-call argument list.
///
/// Arguments are addressed by zero-based index. `Args` is immutable and shared
/// (positional reads don't consume), so all methods take `&self`.
#[derive(Clone)]
pub struct Args<'e, 's> {
    args: &'e [Spanned<Expr>],
    eval: Evaluator<'s>,
}

impl<'e, 's> Args<'e, 's> {
    pub fn new(args: &'e [Spanned<Expr>], eval: Evaluator<'s>) -> Self {
        Self { args, eval }
    }

    pub fn len(&self) -> usize {
        self.args.len()
    }

    pub fn is_empty(&self) -> bool {
        self.args.is_empty()
    }

    fn get(&self, idx: usize) -> Result<&'e Spanned<Expr>, DefReaderError> {
        self.args
            .get(idx)
            .ok_or(DefReaderError::MissingArg(idx, Span::SYNTHETIC))
    }

    pub fn i32(&self, idx: usize) -> Result<i32, DefReaderError> {
        let expr = self.get(idx)?;
        let span = expr.span;
        self.eval
            .i32(expr)
            .map_err(|e| DefReaderError::Eval(e, span))
    }

    pub fn u32(&self, idx: usize) -> Result<u32, DefReaderError> {
        let expr = self.get(idx)?;
        let span = expr.span;
        self.eval
            .u32(expr)
            .map_err(|e| DefReaderError::Eval(e, span))
    }

    pub fn f32(&self, idx: usize) -> Result<f32, DefReaderError> {
        let expr = self.get(idx)?;
        let span = expr.span;
        self.eval
            .f32(expr)
            .map_err(|e| DefReaderError::Eval(e, span))
    }

    pub fn bool(&self, idx: usize) -> Result<bool, DefReaderError> {
        let expr = self.get(idx)?;
        let span = expr.span;
        self.eval
            .bool(expr)
            .map_err(|e| DefReaderError::Eval(e, span))
    }

    pub fn string(&self, idx: usize) -> Result<String, DefReaderError> {
        let expr = self.get(idx)?;
        let span = expr.span;
        self.eval
            .string(expr)
            .map(|s| s.to_string())
            .map_err(|e| DefReaderError::Eval(e, span))
    }

    pub fn opt(&self, idx: usize) -> Option<&'e Spanned<Expr>> {
        self.args.get(idx)
    }

    pub fn ctor(&self, idx: usize, name: &'static str) -> Result<Args<'e, 's>, DefReaderError> {
        let expr = self.get(idx)?;
        let span = expr.span;
        let call = self
            .eval
            .call(expr, name)
            .map_err(|e| DefReaderError::Eval(e, span))?;
        Ok(Args::new(&call.arguments, self.eval))
    }
}

// ── DefReaderError ────────────────────────────────────────────────────────────

#[derive(Debug)]
pub enum DefReaderError {
    MissingField(&'static str, Span),
    UnexpectedStatement(Spanned<Statement>),
    MissingArg(usize, Span),
    Eval(EvalError, Span),
    Semantic(&'static str, Option<Span>),
}

impl std::fmt::Display for DefReaderError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            DefReaderError::MissingField(name, _) => write!(f, "missing required field {name}"),
            // Name the field. "unexpected statement" left the reader to work
            // out which of a group's statements was the problem.
            DefReaderError::UnexpectedStatement(stmt) => match &stmt.value {
                Statement::Field(field) => write!(f, "unknown field {}", field.path),
                Statement::MethodCall(mc) => {
                    write!(f, "unknown method {}.{}", mc.object, mc.call.name)
                }
                Statement::TaggedBlock(tb) => write!(f, "unexpected <{}> block", tb.tag),
            },
            DefReaderError::MissingArg(i, _) => write!(f, "missing argument at index {i}"),
            DefReaderError::Eval(e, _) => write!(f, "{e}"),
            DefReaderError::Semantic(msg, _) => f.write_str(msg),
        }
    }
}

// ── Body ─────────────────────────────────────────────────────────────────────

/// A definition body: a sequence of statements the compiler reads, **without
/// requiring that they be contiguous in memory**.
///
/// Lowering never indexes or slices a body — it only iterates one. Keeping that
/// the only requirement is what lets a specialization chain be read in place:
/// flattening one used to clone 681,479 statements across the corpus (7× the
/// definitions' own bodies) purely so [`DefReader`] could borrow them back.
///
/// [`Body::Refs`] is the form the bespoke arms' filters produce — selecting a
/// subset copies pointers rather than statements.
#[derive(Clone, Copy)]
pub enum Body<'a> {
    /// One contiguous run — a single definition's own body.
    Flat(&'a [Spanned<Statement>]),
    /// Runs read back to back: a specialization chain most-distant-first, or a
    /// tagged block merged across one.
    Chain(&'a [&'a [Spanned<Statement>]]),
    /// An explicit selection, e.g. what remains after a bespoke arm takes the
    /// statements it handles itself.
    Refs(&'a [&'a Spanned<Statement>]),
}

impl<'a> Body<'a> {
    pub const EMPTY: Body<'static> = Body::Flat(&[]);

    pub fn iter(&self) -> BodyIter<'a> {
        match *self {
            Body::Flat(s) => BodyIter::Flat(s.iter()),
            Body::Chain(runs) => BodyIter::Chain {
                runs: runs.iter(),
                run: [].iter(),
            },
            Body::Refs(r) => BodyIter::Refs(r.iter()),
        }
    }

    pub fn len(&self) -> usize {
        match *self {
            Body::Flat(s) => s.len(),
            Body::Chain(runs) => runs.iter().map(|r| r.len()).sum(),
            Body::Refs(r) => r.len(),
        }
    }

    pub fn is_empty(&self) -> bool {
        match *self {
            Body::Flat(s) => s.is_empty(),
            Body::Chain(runs) => runs.iter().all(|r| r.is_empty()),
            Body::Refs(r) => r.is_empty(),
        }
    }
}

impl<'a> From<&'a [Spanned<Statement>]> for Body<'a> {
    fn from(body: &'a [Spanned<Statement>]) -> Self {
        Body::Flat(body)
    }
}

impl<'a> IntoIterator for Body<'a> {
    type Item = &'a Spanned<Statement>;
    type IntoIter = BodyIter<'a>;

    fn into_iter(self) -> BodyIter<'a> {
        self.iter()
    }
}

pub enum BodyIter<'a> {
    Flat(std::slice::Iter<'a, Spanned<Statement>>),
    Chain {
        runs: std::slice::Iter<'a, &'a [Spanned<Statement>]>,
        run: std::slice::Iter<'a, Spanned<Statement>>,
    },
    Refs(std::slice::Iter<'a, &'a Spanned<Statement>>),
}

impl<'a> Iterator for BodyIter<'a> {
    type Item = &'a Spanned<Statement>;

    fn next(&mut self) -> Option<&'a Spanned<Statement>> {
        match self {
            BodyIter::Flat(it) => it.next(),
            BodyIter::Refs(it) => it.next().copied(),
            BodyIter::Chain { runs, run } => loop {
                if let Some(stmt) = run.next() {
                    return Some(stmt);
                }
                *run = runs.next()?.iter();
            },
        }
    }
}

// ── DefReader ────────────────────────────────────────────────────────────────

struct Entry<'a> {
    stmt: &'a Spanned<Statement>,
    consumed: bool,
}

pub struct DefReader<'a, 's> {
    entries: Vec<Entry<'a>>,
    depth: usize,
    eval: Evaluator<'s>,
}

impl<'a, 's> DefReader<'a, 's> {
    pub fn new(body: impl Into<Body<'a>>, symbols: &'s SymbolTable) -> Self {
        Self {
            entries: body
                .into()
                .iter()
                .map(|s| Entry {
                    stmt: s,
                    consumed: false,
                })
                .collect(),
            depth: 0,
            eval: Evaluator::new(symbols),
        }
    }

    fn new_with_depth(entries: Vec<Entry<'a>>, depth: usize, eval: Evaluator<'s>) -> Self {
        Self {
            entries,
            depth,
            eval,
        }
    }

    fn find_opt_leaf(&mut self, name: &'static str) -> Option<&'a Spanned<Expr>> {
        let mut found: Option<&'a Spanned<Expr>> = None;
        for entry in self.entries.iter_mut() {
            if entry.consumed {
                continue;
            }
            if let Some(expr) = leaf_field(entry.stmt, self.depth, name) {
                entry.consumed = true;
                found = Some(expr);
            }
        }
        found
    }

    /// Consume the last matching leaf field `name` and return its raw expression,
    /// or `None` if no matching statement exists. Callers that need typed access
    /// can evaluate the expression with [`Evaluator`] methods.
    pub fn opt_expr(&mut self, name: &'static str) -> Option<&'a Spanned<Expr>> {
        self.find_opt_leaf(name)
    }

    /// Leaf lookup matched by *normalized* name (case-insensitive, underscores
    /// stripped): text-def member spellings follow the C++ member names
    /// (`BankIndex`) while Rust fields are snake_case (`bank_index`). Used for
    /// compound (`WireStruct`) members, whose text form is `name.member`.
    pub fn opt_expr_normalized(&mut self, name: &str) -> Option<&'a Spanned<Expr>> {
        let want = defs::def::visit::normalize_member_name(name);
        let mut found: Option<&'a Spanned<Expr>> = None;
        for entry in self.entries.iter_mut() {
            if entry.consumed {
                continue;
            }
            if let Statement::Field(field) = &entry.stmt.value {
                let segs = &field.path.segments;
                if segs.len() == self.depth + 1
                    && matches!(segs.get(self.depth), Some(PathSegment::Field(n))
                        if defs::def::visit::normalize_member_name(n) == want)
                {
                    entry.consumed = true;
                    found = Some(&field.expr);
                }
            }
        }
        found
    }

    pub fn eval(&self) -> Evaluator<'s> {
        self.eval
    }

    // ── group (compound paths) ───────────────────────────────────────────────

    /// Collect statements whose path descends into `name` (`name.member …`)
    /// into a sub-reader at `depth + 1`. Only deeper field segments match —
    /// indexed forms (`name[i] …`) belong to [`DefReader::indexed_sparse`].
    pub fn group(&mut self, name: &str) -> Option<DefReader<'a, 's>> {
        let mut indices = Vec::new();
        for (i, entry) in self.entries.iter().enumerate() {
            if entry.consumed {
                continue;
            }
            if let Statement::Field(field) = &entry.stmt.value {
                let segs = &field.path.segments;
                if segs.len() > self.depth + 1
                    && matches!(segs.get(self.depth), Some(PathSegment::Field(n)) if n == name)
                    && matches!(segs.get(self.depth + 1), Some(PathSegment::Field(_)))
                {
                    indices.push(i);
                }
            }
        }
        if indices.is_empty() {
            return None;
        }
        for &i in &indices {
            self.entries[i].consumed = true;
        }
        let entries: Vec<Entry<'a>> = indices
            .iter()
            .map(|&i| Entry {
                stmt: self.entries[i].stmt,
                consumed: false,
            })
            .collect();
        Some(DefReader::new_with_depth(
            entries,
            self.depth + 1,
            self.eval,
        ))
    }

    // ── calls ───────────────────────────────────────────────────────────────

    pub fn calls(&mut self, object: &str, method: &str) -> Vec<Args<'a, 's>> {
        let mut results = Vec::new();
        for entry in self.entries.iter_mut() {
            if entry.consumed {
                continue;
            }
            if let Statement::MethodCall(mc) = &entry.stmt.value {
                // Depth-aware: the object path must end at `object` at this
                // reader's depth, so nested `Field[i].Attitudes.Add(…)` matches
                // in the sub-reader for `Field[i]` (depth > 0).
                let segs = &mc.object.segments;
                let matches_obj = segs.len() == self.depth + 1
                    && matches!(segs.get(self.depth), Some(PathSegment::Field(n)) if n == object);
                if matches_obj && mc.call.name == method {
                    entry.consumed = true;
                    results.push(Args::new(&mc.call.arguments, self.eval));
                }
            }
        }
        results
    }

    // ── indexed (sparse) ────────────────────────────────────────────────────

    /// Like [`DefReader::indexed`] but without the contiguity requirement:
    /// returns `(index, reader)` pairs sorted by index. Used for fields like
    /// `States[..]` where the game resizes the vector to the highest index and
    /// default-fills any gaps.
    pub fn indexed_sparse(
        &mut self,
        name: &str,
    ) -> Result<Vec<(usize, DefReader<'a, 's>)>, DefReaderError> {
        use std::collections::BTreeMap;

        let mut groups: BTreeMap<usize, Vec<usize>> = BTreeMap::new();

        for (i, entry) in self.entries.iter().enumerate() {
            if entry.consumed {
                continue;
            }
            // Both leaf sub-fields (`Reaction[0].Animation …`) and method calls
            // on a sub-field (`Reaction[0].Attitudes.Add(…)`) belong to the
            // indexed element — group by the object/path segments of either.
            let Some(segs) = statement_path(entry.stmt) else {
                continue;
            };
            if segs.len() >= self.depth + 2
                && matches!(segs.get(self.depth), Some(PathSegment::Field(n)) if n == name)
                && let Some(PathSegment::Index(idx_expr)) = segs.get(self.depth + 1)
            {
                let idx = self.eval.eval_usize(idx_expr)?;
                groups.entry(idx).or_default().push(i);
            }
        }

        let mut readers = Vec::with_capacity(groups.len());
        for (idx, entry_indices) in groups {
            for &i in &entry_indices {
                self.entries[i].consumed = true;
            }
            let entries: Vec<Entry<'a>> = entry_indices
                .iter()
                .map(|&i| Entry {
                    stmt: self.entries[i].stmt,
                    consumed: false,
                })
                .collect();
            readers.push((
                idx,
                DefReader::new_with_depth(entries, self.depth + 2, self.eval),
            ));
        }

        Ok(readers)
    }

    // ── keyed (map-like) ────────────────────────────────────────────────────

    /// Collect `Name[key] value;` statements where `key` is an arbitrary
    /// expression (symbol, integer, or string) used as a map key. Groups by the
    /// key expression's textual form, in first-occurrence order. The caller
    /// evaluates the key; the value is read from the group reader (last
    /// occurrence wins on duplicate keys).
    pub fn keyed(&mut self, name: &str) -> Vec<(&'a Spanned<Expr>, DefReader<'a, 's>)> {
        let mut order: Vec<(String, &'a Spanned<Expr>, Vec<usize>)> = Vec::new();

        for (i, entry) in self.entries.iter().enumerate() {
            if entry.consumed {
                continue;
            }
            if let Statement::Field(field) = &entry.stmt.value {
                let segs = &field.path.segments;
                if segs.len() >= self.depth + 2
                    && matches!(segs.get(self.depth), Some(PathSegment::Field(n)) if n == name)
                    && let Some(PathSegment::Index(key_expr)) = segs.get(self.depth + 1)
                {
                    let key_text = key_expr.value.to_string();
                    match order.iter_mut().find(|(k, _, _)| *k == key_text) {
                        Some((_, _, indices)) => indices.push(i),
                        None => order.push((key_text, key_expr, vec![i])),
                    }
                }
            }
        }

        let mut result = Vec::with_capacity(order.len());
        for (_, key_expr, entry_indices) in order {
            for &i in &entry_indices {
                self.entries[i].consumed = true;
            }
            let entries: Vec<Entry<'a>> = entry_indices
                .iter()
                .map(|&i| Entry {
                    stmt: self.entries[i].stmt,
                    consumed: false,
                })
                .collect();
            result.push((
                key_expr,
                DefReader::new_with_depth(entries, self.depth + 2, self.eval),
            ));
        }

        result
    }

    // ── any (nameless values) ───────────────────────────────────────────────

    /// Peek the (nameless) value at this reader's depth without consuming it
    /// (last-wins, matching [`any_expr`]). Used to distinguish a bare
    /// constructor value (`Field[i] CFoo(a, b)`) from member-path statements.
    pub fn peek_any_expr(&self) -> Option<&'a Spanned<Expr>> {
        let mut found = None;
        for entry in self.entries.iter() {
            if entry.consumed {
                continue;
            }
            if let Statement::Field(field) = &entry.stmt.value
                && field.path.segments.len() == self.depth
            {
                found = Some(&field.expr);
            }
        }
        found
    }

    /// Value of the (nameless) statement at this reader's depth. Consumes every
    /// such statement; the LAST wins (override semantics).
    pub fn any_expr(&mut self) -> Result<&'a Spanned<Expr>, DefReaderError> {
        let mut found: Option<&'a Spanned<Expr>> = None;
        for entry in self.entries.iter_mut() {
            if entry.consumed {
                continue;
            }
            if let Statement::Field(field) = &entry.stmt.value {
                let segs = &field.path.segments;
                if segs.len() == self.depth {
                    entry.consumed = true;
                    found = Some(&field.expr);
                }
            }
        }
        found.ok_or(DefReaderError::MissingField("(any)", Span::SYNTHETIC))
    }

    pub fn any_i32(&mut self) -> Result<i32, DefReaderError> {
        let expr = self.any_expr()?;
        self.eval.eval_i32(expr)
    }

    pub fn any_u32(&mut self) -> Result<u32, DefReaderError> {
        let expr = self.any_expr()?;
        self.eval.eval_u32(expr)
    }

    pub fn any_f32(&mut self) -> Result<f32, DefReaderError> {
        let expr = self.any_expr()?;
        self.eval.eval_f32(expr)
    }

    pub fn any_string(&mut self) -> Result<String, DefReaderError> {
        let expr = self.any_expr()?;
        self.eval.eval_string(expr)
    }

    pub fn any_bool(&mut self) -> Result<bool, DefReaderError> {
        let expr = self.any_expr()?;
        self.eval.eval_bool(expr)
    }

    // ── finish ──────────────────────────────────────────────────────────────

    // There was once a `remaining_statements()` returning every unconsumed
    // statement, so `lower_def` could hand them back for the builder to warn
    // about. It was removed because the question it answers is the wrong one:
    // it reports what *this reader* did not consume, but consumption is spread
    // across several readers and several pipeline stages. Tagged blocks are
    // consumed by the sub-def pass, `Components.Add` by `build_thing_components`,
    // and the bespoke UI/CONTROL_SCHEME arms run a second full-body reader over
    // statements the generic pass already took. Measured on the stock corpus it
    // produced 103,467 "unconsumed" statements of which 10 were real — so the
    // warning was never turned on, and the channel sat dead.
    //
    // Reviving it needs a build-scoped consumption ledger keyed on statement
    // identity and checked once at the end, not a `Vec` returned per reader.
    //
    // `finish()` below is a different thing and is live: it validates a *group*
    // sub-reader (a Vec element, map value, or indexed sub-struct), where the
    // reader really does own every statement it was handed.

    pub fn finish(self) -> Result<(), DefReaderError> {
        match self.entries.iter().find(|e| !e.consumed) {
            Some(entry) => Err(DefReaderError::UnexpectedStatement(entry.stmt.clone())),
            None => Ok(()),
        }
    }
}

#[cfg(test)]
mod eval_tests {
    use super::Evaluator;
    use defs::def::text::{Expr, Spanned, SymbolTable, parse_expr_str};

    fn syms() -> Evaluator<'static> {
        let table = SymbolTable::new();
        Evaluator::new(Box::leak(Box::new(table)))
    }

    fn populated_syms(symbols: &[(&str, i64)]) -> Evaluator<'static> {
        let mut table = SymbolTable::new();
        for &(name, value) in symbols {
            table.insert(name, value);
        }
        Evaluator::new(Box::leak(Box::new(table)))
    }

    fn parse_expr(s: &str) -> Spanned<Expr> {
        parse_expr_str(s).unwrap()
    }

    fn no_span<T>(value: T) -> Spanned<T> {
        Spanned {
            span: defs::def::text::Span::SYNTHETIC,
            value,
        }
    }

    fn expr_number(s: &str) -> Spanned<Expr> {
        no_span(Expr::Number(s.to_string()))
    }

    fn expr_symbol(s: &str) -> Spanned<Expr> {
        no_span(Expr::Symbol(s.to_string()))
    }

    fn expr_bool(b: bool) -> Spanned<Expr> {
        no_span(Expr::Bool(b))
    }

    fn expr_string(s: &str) -> Spanned<Expr> {
        no_span(Expr::String(s.to_string()))
    }

    #[test]
    fn eval_i32() {
        assert_eq!(syms().i32(&parse_expr("42")).unwrap(), 42);
        assert_eq!(syms().i32(&parse_expr("-42")).unwrap(), -42);
    }

    #[test]
    fn eval_u32_negative() {
        let e = parse_expr("-1");
        assert!(syms().u32(&e).is_err());
    }

    #[test]
    fn eval_f32() {
        assert_eq!(syms().f32(&parse_expr("64")).unwrap(), 64.0);
        assert_eq!(syms().f32(&parse_expr("3.25")).unwrap(), 3.25);
    }

    #[test]
    fn eval_bit_or() {
        let expr = parse_expr("1 | 2 | 4");
        assert_eq!(syms().u32(&expr).unwrap(), 7);
    }

    #[test]
    fn eval_bit_or_on_float_is_error() {
        assert!(syms().f32(&parse_expr("1 | 2")).is_err());
    }

    // ── Task P0.5: gap coverage ──────────────────────────────────────────────

    #[test]
    fn eval_bool_true() {
        assert!(syms().bool(&expr_bool(true)).unwrap());
    }

    #[test]
    fn eval_bool_false() {
        assert!(!syms().bool(&expr_bool(false)).unwrap());
    }

    #[test]
    fn eval_bool_true_symbol() {
        assert!(syms().bool(&expr_symbol("TRUE")).unwrap());
    }

    #[test]
    fn eval_bool_false_symbol() {
        assert!(!syms().bool(&expr_symbol("FALSE")).unwrap());
    }

    #[test]
    fn eval_bool_lowercase_true_symbol() {
        assert!(syms().bool(&expr_symbol("true")).unwrap());
    }

    #[test]
    fn eval_bool_lowercase_false_symbol() {
        assert!(!syms().bool(&expr_symbol("false")).unwrap());
    }

    #[test]
    fn eval_bool_btrue() {
        assert!(syms().bool(&parse_expr("BTRUE")).unwrap());
    }

    #[test]
    fn eval_bool_bfalse() {
        assert!(!syms().bool(&parse_expr("BFALSE")).unwrap());
    }

    #[test]
    fn eval_bool_number_is_error() {
        assert!(syms().bool(&expr_number("1")).is_err());
    }

    #[test]
    fn eval_string_literal() {
        assert_eq!(syms().string(&expr_string("hello")).unwrap(), "hello");
    }

    #[test]
    fn eval_string_symbol_returns_name_as_is() {
        assert_eq!(syms().string(&expr_symbol("MY_NAME")).unwrap(), "MY_NAME");
    }

    #[test]
    fn eval_string_error_on_number() {
        assert!(syms().string(&expr_number("42")).is_err());
    }

    #[test]
    fn eval_usize_positive() {
        assert_eq!(syms().usize(&expr_number("42")).unwrap(), 42);
    }

    #[test]
    fn eval_usize_null_to_zero() {
        assert_eq!(syms().usize(&expr_symbol("NULL")).unwrap(), 0);
    }

    #[test]
    fn eval_usize_error_on_negative() {
        assert!(syms().usize(&parse_expr("-1")).is_err());
    }

    #[test]
    fn eval_i32_symbol_resolution() {
        let e = populated_syms(&[("PLAYER", 421), ("MAX_HEALTH", 100)]);
        assert_eq!(e.i32(&expr_symbol("PLAYER")).unwrap(), 421);
        assert_eq!(e.i32(&expr_symbol("MAX_HEALTH")).unwrap(), 100);
    }

    #[test]
    fn eval_i32_unknown_symbol_error() {
        assert!(syms().i32(&expr_symbol("MISSING")).is_err());
    }

    #[test]
    fn eval_i32_null_to_zero() {
        assert_eq!(syms().i32(&expr_symbol("NULL")).unwrap(), 0);
    }

    #[test]
    fn eval_i32_float_truncation() {
        assert_eq!(syms().i32(&expr_number("3.14")).unwrap(), 3);
    }

    #[test]
    fn eval_i32_negative_float_truncation() {
        assert_eq!(syms().i32(&expr_number("-2.9")).unwrap(), -2);
    }

    #[test]
    fn eval_f32_trailing_f() {
        assert_eq!(syms().f32(&expr_number("1.0f")).unwrap(), 1.0f32);
    }

    #[test]
    fn eval_f32_rejects_null() {
        assert!(syms().f32(&expr_symbol("NULL")).is_err());
    }

    #[test]
    fn eval_f32_symbol_resolution() {
        let e = populated_syms(&[("SOME_VALUE", 7)]);
        assert_eq!(e.f32(&expr_symbol("SOME_VALUE")).unwrap(), 7.0f32);
    }

    #[test]
    fn eval_u32_null_to_zero() {
        assert_eq!(syms().u32(&expr_symbol("NULL")).unwrap(), 0);
    }

    #[test]
    fn eval_u32_symbol_resolution() {
        let e = populated_syms(&[("FLAG_ONE", 1), ("FLAG_TWO", 2)]);
        assert_eq!(e.u32(&expr_symbol("FLAG_ONE")).unwrap(), 1);
        assert_eq!(e.u32(&expr_symbol("FLAG_TWO")).unwrap(), 2);
    }
}
