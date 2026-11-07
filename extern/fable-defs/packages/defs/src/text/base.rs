use derive_more::{Display, Error};

/// Which source file a [`Span`] came from.
///
/// Opaque to the parser: it is assigned by whoever owns the file registry
/// (`def-compiler`'s builder) and handed to `parse_source`. The parser never
/// interprets it, only propagates it.
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct FileId(pub u32);

impl FileId {
    /// Text parsed outside any file registry — `parse_expr_str`, unit tests,
    /// and the synthetic spans lowering invents for generated values.
    pub const ANONYMOUS: FileId = FileId(u32::MAX);
}

/// A byte range in a specific source file (half-open: `start..end`).
///
/// Byte offsets, not char offsets — `&source[start..end]` reproduces the text.
///
/// **The file is part of the span**, because spans outlive the file they were
/// read from. A template's statements are read by every definition that
/// inherits them, spans included, so by the time an error is raised the span
/// may belong to a different file than the definition being compiled. Without the file id there is no way to recover which, and a
/// diagnostic that interprets the offset against the wrong text points
/// confidently at unrelated code.
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub struct Span {
    pub file: FileId,
    pub start: usize,
    pub end: usize,
}

impl Span {
    pub fn new(file: FileId, start: usize, end: usize) -> Self {
        Self { file, start, end }
    }

    /// For values the compiler synthesizes, which correspond to no source text.
    pub const SYNTHETIC: Span = Span {
        file: FileId::ANONYMOUS,
        start: 0,
        end: 0,
    };

    /// Whether `other` lies within this range, in the same file.
    ///
    /// The file check is the point: two spans in different files overlap
    /// numerically all the time, so containment without it is meaningless.
    pub fn contains(self, other: Span) -> bool {
        self.file == other.file && other.start >= self.start && other.end <= self.end
    }
}

// Deliberately no `join`/`merge`: combining two spans is only meaningful when
// they share a file, and nothing here can guarantee that — taking one file id
// and discarding the other silently produces a span that points into the wrong
// text. Productions that need a range spanning several tokens build it from the
// cursor (`Span::new(cursor.file(), start, end)`), where the file comes from the
// cursor rather than from either endpoint, so there is nothing to reconcile.

/// A value annotated with its source span.
#[derive(Copy, Clone, Debug)]
pub struct Spanned<T> {
    pub span: Span,
    pub value: T,
}

impl<T: PartialEq> PartialEq for Spanned<T> {
    fn eq(&self, other: &Self) -> bool {
        self.value == other.value
    }
}

/// The construct a parse error happened *inside*, rendered as the diagnostic's
/// secondary label ("in enum `ESoundNames`").
///
/// One context, not a stack: the innermost production to see the error wins,
/// because it is the first to run on the way out. That is the frame a reader
/// needs — the enclosing enum, definition, or tagged block — and the file name
/// supplies the rest.
#[derive(Clone, Debug)]
pub struct ParseContext {
    /// What kind of construct: "definition", "enum", "namespace", "#ifdef",
    /// "tagged block".
    pub what: &'static str,
    /// Its name, when it has one (`UI_TABLE`, `ESoundNames`).
    pub name: Option<String>,
    /// The name if present, else the opening keyword — never the whole header
    /// line, so the label underlines what identifies the construct.
    pub span: Span,
}

impl ParseContext {
    pub fn new(what: &'static str, name: Option<String>, span: Span) -> Self {
        Self { what, name, span }
    }

    /// The secondary-label text: "in enum `ESoundNames`" / "in this enum".
    pub fn label(&self) -> String {
        match &self.name {
            Some(name) => format!("in {} `{name}`", self.what),
            None => format!("in this {}", self.what),
        }
    }
}

#[derive(Debug, Display, Error)]
#[display("{inner}")]
pub struct ParseError<InnerError> {
    /// The source range the error is *about* — normally the offending token, so
    /// the rendered caret covers it rather than pointing at a single byte.
    pub span: Span,
    /// Boxed because it is large (a name, two spans) and usually absent —
    /// unboxed it dominates the size of every `Result` in the parser.
    pub context: Option<Box<ParseContext>>,
    pub inner: InnerError,
}

impl<T> ParseError<T> {
    pub(crate) fn new(span: Span, inner: T) -> Self {
        Self {
            span,
            context: None,
            inner,
        }
    }

    /// Attach the enclosing construct. Innermost wins: an outer production
    /// never overwrites context an inner one already set.
    pub(crate) fn within(mut self, context: &ParseContext) -> Self {
        if self.context.is_none() {
            self.context = Some(Box::new(context.clone()));
        }
        self
    }
}
