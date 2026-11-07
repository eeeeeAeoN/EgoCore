use super::base::{Span, Spanned};
use super::{EnumDecl, EnumExpr, Item};
use derive_more::{Display, Error};
use std::collections::HashMap;

pub struct SymbolTable {
    map: HashMap<String, i64>,
}

impl Default for SymbolTable {
    fn default() -> Self {
        Self::new()
    }
}

impl SymbolTable {
    pub fn new() -> Self {
        Self {
            map: HashMap::new(),
        }
    }
    pub fn lookup(&self, name: &str) -> Option<i64> {
        self.map.get(name).copied()
    }
    pub fn len(&self) -> usize {
        self.map.len()
    }
    pub fn is_empty(&self) -> bool {
        self.map.is_empty()
    }
    pub fn iter(&self) -> impl Iterator<Item = (&str, i64)> {
        self.map.iter().map(|(k, v)| (k.as_str(), *v))
    }
}

/// A declaration that could not be evaluated into a symbol value.
///
/// Spanned, and with a real `Display`: these used to be rendered with `{e:?}`,
/// so a broken header reported `UnknownSymbol("FOO")` with no location.
#[derive(Debug, Display, Error)]
pub enum SymbolEvalError {
    #[display("unknown constant `{name}`")]
    UnknownSymbol { name: String, span: Span },
    #[display("shift amount {amount} is out of range (expected 0..64)")]
    InvalidShift { amount: i64, span: Span },
}

impl SymbolEvalError {
    pub fn span(&self) -> Span {
        match self {
            SymbolEvalError::UnknownSymbol { span, .. }
            | SymbolEvalError::InvalidShift { span, .. } => *span,
        }
    }
}

/// The source range an enum-value expression covers. Compound forms report the
/// first leaf, which is where a reader looks first.
fn enum_expr_span(expr: &EnumExpr) -> Span {
    match expr {
        EnumExpr::Int(n) => n.span,
        EnumExpr::Ident(name) => name.span,
        EnumExpr::Shift(terms) | EnumExpr::BitOr(terms) => {
            terms.first().map_or(Span::SYNTHETIC, enum_expr_span)
        }
    }
}

/// A symbol that was already defined when a later definition overwrote it.
///
/// A redefinition is **not** an error: the corpus legitimately ships variant
/// header sets, and overriding a stock symbol is a normal modding operation.
/// It is reported so an *unintended* shadow is visible rather than silent.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Redefinition {
    pub name: String,
    /// The value that was in the table, now replaced.
    pub previous: i64,
    /// The value that won.
    pub value: i64,
    /// Where the winning definition is written.
    pub span: Span,
}

impl SymbolTable {
    /// Evaluate a parsed file's declarations into the table, returning every
    /// symbol it redefined.
    ///
    /// Definitions are skipped: this walk is only interested in what a file
    /// *declares*. Both `.h` and `.def` go through here — a `.def` carrying
    /// `enum`s (`engine_local_detail.def`) contributes symbols exactly as a
    /// header does, and both are merged into the one global table.
    pub fn evaluate_items(&mut self, items: &[Item]) -> Result<Vec<Redefinition>, SymbolEvalError> {
        let mut redefined = Vec::new();
        for item in items {
            self.evaluate_item(item, &mut redefined)?;
        }
        Ok(redefined)
    }
    fn evaluate_item(
        &mut self,
        item: &Item,
        redefined: &mut Vec<Redefinition>,
    ) -> Result<(), SymbolEvalError> {
        use Item as I;
        match item {
            I::Enum(decl) => self.evaluate_enum(decl, redefined),
            I::Define(d) => {
                // A valueless `#define NAME` (every include guard) binds no
                // number, so it contributes nothing to the table.
                if let Some(value) = d.value {
                    self.define(&d.name, value, redefined);
                }
                Ok(())
            }
            I::Namespace(ns) => {
                for item in &ns.items {
                    self.evaluate_item(item, redefined)?;
                }
                Ok(())
            }
            I::Conditional(ifdef) => {
                let taken = self.is_defined(&ifdef.condition) ^ ifdef.inverted;
                let branch = if taken {
                    &ifdef.if_branch
                } else {
                    ifdef.else_branch.as_deref().unwrap_or(&[])
                };
                for item in branch {
                    self.evaluate_item(item, redefined)?;
                }
                Ok(())
            }
            I::Definition(_) | I::PragmaOnce(_) => Ok(()),
        }
    }
    fn evaluate_enum(
        &mut self,
        decl: &EnumDecl,
        redefined: &mut Vec<Redefinition>,
    ) -> Result<(), SymbolEvalError> {
        let mut last_value: Option<i64> = None;
        for variant in &decl.variants {
            let value = match &variant.value {
                Some(expr) => self.evaluate_enum_expr(expr)?,
                None => last_value.map_or(0, |v| v + 1),
            };
            self.define(&variant.name, value, redefined);
            // Advance the auto-increment cursor from the value this variant
            // declared, whether or not it displaced an earlier symbol —
            // numbering follows the enum being read, not the table.
            last_value = Some(value);
        }
        Ok(())
    }
    /// Insert, recording a redefinition if the name was already bound.
    fn define(&mut self, name: &Spanned<String>, value: i64, redefined: &mut Vec<Redefinition>) {
        if let Some(previous) = self.insert(&name.value, value) {
            redefined.push(Redefinition {
                name: name.value.clone(),
                previous,
                value,
                span: name.span,
            });
        }
    }
    /// Bind `name` to `value`, returning the value it replaced, if any.
    ///
    /// Last definition wins, matching both the C preprocessor and the retail
    /// tooling's `m_SymbolMap[name] = value`. An earlier design made a
    /// duplicate an error that aborted the *rest of the header file*, which
    /// silently discarded every symbol below the first collision.
    pub fn insert(&mut self, name: &str, value: i64) -> Option<i64> {
        self.map.insert(name.to_string(), value)
    }
    fn is_defined(&self, cond: &str) -> bool {
        cond == "_WINDOWS"
    }
    fn evaluate_enum_expr(&self, expr: &EnumExpr) -> Result<i64, SymbolEvalError> {
        use EnumExpr as E;
        match expr {
            E::Int(n) => Ok(n.value),
            E::Ident(name) => {
                self.lookup(&name.value)
                    .ok_or_else(|| SymbolEvalError::UnknownSymbol {
                        name: name.value.clone(),
                        span: name.span,
                    })
            }
            E::Shift(terms) => {
                let mut iter = terms.iter();
                let first = self.evaluate_enum_expr(iter.next().unwrap())?;
                iter.try_fold(first, |acc, term| {
                    let n = self.evaluate_enum_expr(term)?;
                    if !(0..64).contains(&n) {
                        return Err(SymbolEvalError::InvalidShift {
                            amount: n,
                            span: enum_expr_span(term),
                        });
                    }
                    Ok(acc << n)
                })
            }
            E::BitOr(terms) => terms
                .iter()
                .map(|t| self.evaluate_enum_expr(t))
                .try_fold(0i64, |acc, v| Ok(acc | v?)),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::super::{FileId, parse_source};
    use super::*;

    fn eval(table: &mut SymbolTable, src: &str) -> Vec<Redefinition> {
        let header = parse_source(src, FileId::ANONYMOUS).expect("header parses");
        table
            .evaluate_items(&header.items)
            .expect("header evaluates")
    }

    #[test]
    fn last_definition_wins() {
        let mut t = SymbolTable::new();
        eval(&mut t, "enum E { A = 1 };");
        let redefined = eval(&mut t, "enum E { A = 2 };");
        assert_eq!(t.lookup("A"), Some(2));
        assert_eq!(
            redefined,
            vec![Redefinition {
                name: "A".into(),
                previous: 1,
                value: 2,
                span: redefined[0].span,
            }]
        );
    }

    /// The regression this module was rewritten for: a duplicate used to abort
    /// evaluation of the whole file, silently dropping every symbol below it.
    /// A mod that re-declares an enum to append symbols lost exactly the
    /// symbols it was adding.
    #[test]
    fn duplicate_does_not_discard_the_rest_of_the_file() {
        let mut t = SymbolTable::new();
        eval(&mut t, "enum EMeshType2 { MESH_A = 1, MESH_B = 2 };");
        eval(
            &mut t,
            "enum EMeshType2 { MESH_A = 1, MESH_B = 2, MESH_NEW = 3 };",
        );
        assert_eq!(t.lookup("MESH_NEW"), Some(3), "symbol after the duplicate");
        assert_eq!(t.lookup("MESH_A"), Some(1));
        assert_eq!(t.lookup("MESH_B"), Some(2));
    }

    /// Redefinition must not be reported for a fresh name, or every additive
    /// header would drown the log.
    #[test]
    fn new_symbols_are_not_redefinitions() {
        let mut t = SymbolTable::new();
        let redefined = eval(&mut t, "enum E { A = 1, B = 2 };");
        assert!(redefined.is_empty());
    }

    /// Auto-increment numbering follows the enum being read, not the table, so
    /// a redefinition mid-enum does not shift the variants after it.
    #[test]
    fn auto_increment_unaffected_by_redefinition() {
        let mut t = SymbolTable::new();
        eval(&mut t, "enum E { A = 40 };");
        eval(&mut t, "enum F { A = 1, B, C };");
        assert_eq!(t.lookup("A"), Some(1));
        assert_eq!(t.lookup("B"), Some(2));
        assert_eq!(t.lookup("C"), Some(3));
    }

    #[test]
    fn unknown_symbol_in_enum_expression_is_still_an_error() {
        let mut t = SymbolTable::new();
        let header = parse_source("enum E { A = NOPE };", FileId::ANONYMOUS).unwrap();
        assert!(matches!(
            t.evaluate_items(&header.items),
            Err(SymbolEvalError::UnknownSymbol { name, .. }) if name == "NOPE"
        ));
    }

    #[test]
    fn insert_reports_the_replaced_value() {
        let mut t = SymbolTable::new();
        assert_eq!(t.insert("A", 1), None);
        assert_eq!(t.insert("A", 2), Some(1));
        assert_eq!(t.lookup("A"), Some(2));
    }
}
