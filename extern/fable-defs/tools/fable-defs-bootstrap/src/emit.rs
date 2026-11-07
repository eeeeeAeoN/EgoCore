//! Select the def classes and their dependencies, then emit one clean header.

use std::collections::{BTreeMap, BTreeSet};

use crate::Result;
use crate::clean::clean_decl;
use crate::tree::{Class, Enum, Tree, TypeRef};

/// What was emitted.
#[derive(Debug, Default, Clone, Copy)]
pub struct Stats {
    pub def_classes: usize,
    pub classes: usize,
    pub nested: usize,
    pub enums: usize,
    pub members: usize,
}

/// Names that are replaced by small hand-written aliases, not emitted.
const ALIASES: &[&str] = &[
    "CCharString",
    "CWideString",
    "CDefString",
    "CDefIndex",
    "CCountedPointer",
    "CDefPointer",
    "CArray",
    "CVectorMap",
    "CKeyPairCompareLess",
    "CSmallVector",
    "CBasicString",
    "CIVCountedPointer",
    "CIntelligentPointer",
];

/// Runtime bases that carry no def data (or no data at all).
const RUNTIME_BASES: &[&str] = &[
    "CDefClassBase",
    "CSubDefClassBase",
    "CParentDefClassBase",
    "CDefPointeeBase",
    "CIVCountedPointeeBase",
    "CBaseClass",
    "CBaseClassNonCopyable",
    "CInitBaseClass",
    "CFailedAllocationHandler",
];

const PTR_WRAPPERS: &[&str] = &[
    "CCountedPointer",
    "CDefPointer",
    "CIVCountedPointer",
    "CIntelligentPointer",
    "std::shared_ptr",
    "std::unique_ptr",
    "boost::scoped_ptr",
];

pub fn generate(tree: &Tree) -> Result<(String, Stats)> {
    let mut selector = Selector::new(tree);
    selector.select_def_classes();
    selector.select_referenced_enums();

    let mut stats = Stats {
        def_classes: selector.def_classes,
        ..Stats::default()
    };
    let order = selector.order();
    for qname in &order {
        if let Some(class) = tree.class(qname) {
            stats.classes += 1;
            stats.members += class.members.len();
        }
    }
    for qname in &selector.selected {
        if let Some(class) = tree.class(qname)
            && class.parent.is_some()
        {
            stats.nested += 1;
            stats.members += class.members.len();
        }
    }
    stats.enums = selector.enums.len();

    let mut out = String::new();
    preamble(&mut out);
    emit_enums(&mut out, tree, &selector);
    emit_forward_declarations(&mut out, tree, &selector);
    for qname in &order {
        if let Some(class) = tree.class(qname)
            && class.parent.is_none()
        {
            emit_class(&mut out, tree, &selector.selected, class, "");
        }
    }
    out.push_str("\n#endif /* FABLE_DEFS_HPP */\n");
    Ok((out, stats))
}

// ── selection ───────────────────────────────────────────────────────────────

struct Selector<'a> {
    tree: &'a Tree,
    selected: BTreeSet<String>,
    enums: BTreeSet<String>,
    simple_classes: BTreeMap<String, String>,
    simple_enums: BTreeMap<String, String>,
    def_classes: usize,
}

impl<'a> Selector<'a> {
    fn new(tree: &'a Tree) -> Self {
        Self {
            tree,
            selected: BTreeSet::new(),
            enums: BTreeSet::new(),
            simple_classes: simple_index(tree.classes.keys()),
            simple_enums: simple_index(tree.enums.keys()),
            def_classes: 0,
        }
    }

    fn select_def_classes(&mut self) {
        let defs: Vec<String> = self
            .tree
            .classes
            .keys()
            .filter(|qname| self.is_def(qname, &mut BTreeSet::new()))
            .cloned()
            .collect();
        self.def_classes = defs.len();
        for qname in defs {
            let simple = simple_name(&qname);
            if matches!(
                simple,
                "CDefClassBase" | "CSubDefClassBase" | "CParentDefClassBase"
            ) {
                continue;
            }
            self.select(&qname);
        }
    }

    fn is_def(&self, qname: &str, seen: &mut BTreeSet<String>) -> bool {
        if !seen.insert(qname.to_owned()) {
            return false;
        }
        let Some(class) = self.tree.class(qname) else {
            return false;
        };
        for base in &class.bases {
            let simple = simple_name(base);
            if matches!(
                simple,
                "CDefClassBase" | "CSubDefClassBase" | "CParentDefClassBase"
            ) {
                return true;
            }
            if let Some(base) = self.resolve_class(base)
                && self.is_def(&base, seen)
            {
                return true;
            }
        }
        false
    }

    fn select(&mut self, qname: &str) {
        if self.selected.contains(qname) || !self.tree.classes.contains_key(qname) {
            return;
        }
        if ALIASES.contains(&simple_name(qname)) {
            return;
        }
        self.selected.insert(qname.to_owned());
        let Some(class) = self.tree.class(qname).cloned() else {
            return;
        };
        if let Some(parent) = &class.parent {
            self.select(parent);
        }
        for base in &class.bases {
            if RUNTIME_BASES.contains(&simple_name(base)) {
                continue;
            }
            if let Some(base) = self.resolve_class(base) {
                self.select(&base);
            }
        }
        for member in &class.members {
            for ident in self.type_idents(&member.decl) {
                if self.tree.classes.contains_key(&ident) {
                    self.select(&ident);
                } else if self.tree.enums.contains_key(&ident) {
                    self.enums.insert(ident);
                }
            }
        }
        for nested in &class.nested {
            match nested {
                TypeRef::Class(child) => self.select(child),
                TypeRef::Enum(child) => {
                    self.enums.insert(child.clone());
                }
            }
        }
    }

    fn select_referenced_enums(&mut self) {
        let classes: Vec<String> = self.selected.iter().cloned().collect();
        for qname in classes {
            let Some(class) = self.tree.class(&qname) else {
                continue;
            };
            for member in &class.members {
                for ident in self.type_idents(&member.decl) {
                    if self.tree.enums.contains_key(&ident) {
                        self.enums.insert(ident);
                    }
                }
            }
            for nested in &class.nested {
                if let TypeRef::Enum(child) = nested {
                    self.enums.insert(child.clone());
                }
            }
        }
        // A selected nested enum requires its parent to be emitted; the parent
        // is already selected by construction, but keep this defensive.
        for qname in self.enums.clone() {
            if !self.tree.enums.contains_key(&qname)
                && let Some(parent) = qname.rsplit_once("::").map(|(p, _)| p)
                && self.tree.classes.contains_key(parent)
            {
                self.select(parent);
            }
        }
    }

    fn type_idents(&self, text: &str) -> BTreeSet<String> {
        let mut out = BTreeSet::new();
        let bytes = text.as_bytes();
        let mut i = 0usize;
        while i < bytes.len() {
            if bytes[i].is_ascii_alphabetic() || bytes[i] == b'_' {
                let start = i;
                i += 1;
                while i < bytes.len() && (bytes[i].is_ascii_alphanumeric() || bytes[i] == b'_') {
                    i += 1;
                }
                while i + 1 < bytes.len() && bytes[i] == b':' && bytes[i + 1] == b':' {
                    i += 2;
                    while i < bytes.len() && (bytes[i].is_ascii_alphanumeric() || bytes[i] == b'_')
                    {
                        i += 1;
                    }
                }
                let full = &text[start..i];
                if self.tree.classes.contains_key(full) || self.tree.enums.contains_key(full) {
                    out.insert(full.to_owned());
                } else if let Some(qname) = self.simple_classes.get(simple_name(full)) {
                    out.insert(qname.clone());
                } else if let Some(qname) = self.simple_enums.get(simple_name(full)) {
                    out.insert(qname.clone());
                }
            } else {
                i += 1;
            }
        }
        out
    }

    fn resolve_class(&self, base: &str) -> Option<String> {
        if self.tree.classes.contains_key(base) {
            return Some(base.to_owned());
        }
        self.simple_classes.get(simple_name(base)).cloned()
    }

    fn order(&self) -> Vec<String> {
        let mut order = Vec::new();
        let mut temp = BTreeSet::new();
        let mut done = BTreeSet::new();
        let top: Vec<&String> = self
            .selected
            .iter()
            .filter(|qname| {
                self.tree
                    .class(qname)
                    .is_some_and(|class| class.parent.is_none())
            })
            .collect();
        for qname in top {
            self.visit(qname, &mut temp, &mut done, &mut order);
        }
        order
    }

    fn visit(
        &self,
        qname: &str,
        temp: &mut BTreeSet<String>,
        done: &mut BTreeSet<String>,
        order: &mut Vec<String>,
    ) {
        if done.contains(qname) || temp.contains(qname) {
            return;
        }
        temp.insert(qname.to_owned());
        for dep in self.deps(qname) {
            self.visit(&dep, temp, done, order);
        }
        temp.remove(qname);
        done.insert(qname.to_owned());
        order.push(qname.to_owned());
    }

    /// Top-level classes needed before `qname` (including deps of nested types).
    fn deps(&self, qname: &str) -> BTreeSet<String> {
        let mut out = BTreeSet::new();
        self.collect_deps(qname, &mut out);
        out.remove(qname);
        out.into_iter()
            .filter_map(|dep| self.owner(&dep))
            .filter(|owner| owner != qname)
            .collect()
    }

    fn collect_deps(&self, qname: &str, out: &mut BTreeSet<String>) {
        let Some(class) = self.tree.class(qname) else {
            return;
        };
        for base in kept_bases(self.tree, &self.selected, class) {
            if let Some(base) = self.resolve_class(&base) {
                out.insert(base);
            }
        }
        for member in &class.members {
            let pointers = pointer_targets(&member.decl);
            for ident in self.type_idents(&member.decl) {
                if pointers.contains(&ident) {
                    continue;
                }
                if self.tree.classes.contains_key(&ident) && self.selected.contains(&ident) {
                    out.insert(ident);
                }
            }
        }
        for nested in &class.nested {
            if let TypeRef::Class(child) = nested {
                self.collect_deps(child, out);
            }
        }
    }

    fn owner(&self, qname: &str) -> Option<String> {
        let mut current = qname.to_owned();
        loop {
            let class = self.tree.class(&current)?;
            match &class.parent {
                Some(parent) => current = parent.clone(),
                None => return Some(current),
            }
        }
    }
}

fn simple_index<'a>(keys: impl Iterator<Item = &'a String>) -> BTreeMap<String, String> {
    let mut out = BTreeMap::new();
    for qname in keys {
        out.entry(simple_name(qname).to_owned())
            .or_insert_with(|| qname.clone());
    }
    out
}

fn simple_name(qname: &str) -> &str {
    qname.rsplit("::").next().unwrap_or(qname)
}

// ── pointer / base analysis ─────────────────────────────────────────────────

fn pointer_targets(decl: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for wrapper in PTR_WRAPPERS {
        let needle = format!("{wrapper}<");
        let mut i = 0usize;
        while let Some(found) = decl[i..].find(&needle) {
            let start = i + found + wrapper.len();
            let Some(end) = matching_angle(decl, start) else {
                break;
            };
            for ident in identifiers(&decl[start + 1..end]) {
                out.insert(ident);
            }
            i = end + 1;
        }
    }
    // Raw pointers and references: the identifier immediately before the `*`.
    let bytes = decl.as_bytes();
    for (i, ch) in bytes.iter().enumerate() {
        if *ch != b'*' && *ch != b'&' {
            continue;
        }
        let mut j = i;
        while j > 0 && bytes[j - 1].is_ascii_whitespace() {
            j -= 1;
        }
        let end = j;
        while j > 0 && (bytes[j - 1].is_ascii_alphanumeric() || bytes[j - 1] == b'_') {
            j -= 1;
        }
        if j < end {
            out.insert(decl[j..end].to_owned());
        }
    }
    out
}

fn identifiers(text: &str) -> Vec<String> {
    let bytes = text.as_bytes();
    let mut out = Vec::new();
    let mut i = 0usize;
    while i < bytes.len() {
        if bytes[i].is_ascii_alphabetic() || bytes[i] == b'_' {
            let start = i;
            i += 1;
            while i < bytes.len() && (bytes[i].is_ascii_alphanumeric() || bytes[i] == b'_') {
                i += 1;
            }
            out.push(text[start..i].to_owned());
        } else {
            i += 1;
        }
    }
    out
}

fn matching_angle(s: &str, open: usize) -> Option<usize> {
    let mut depth = 0i32;
    for (offset, ch) in s[open..].char_indices() {
        match ch {
            '<' => depth += 1,
            '>' => {
                depth -= 1;
                if depth == 0 {
                    return Some(open + offset);
                }
            }
            _ => {}
        }
    }
    None
}

fn kept_bases(tree: &Tree, selected: &BTreeSet<String>, class: &Class) -> Vec<String> {
    let mut out = Vec::new();
    for base in &class.bases {
        let simple = simple_name(base);
        if base.starts_with("std::")
            || RUNTIME_BASES.contains(&simple)
            || simple.starts_with("CArray")
            || simple.starts_with("CVectorMap")
            || simple.starts_with("CCompare")
        {
            continue;
        }
        let resolved = if tree.classes.contains_key(base) {
            Some(base.clone())
        } else {
            tree.classes
                .keys()
                .find(|qname| simple_name(qname) == simple)
                .cloned()
        };
        if let Some(resolved) = resolved
            && selected.contains(&resolved)
        {
            out.push(base.clone());
        }
    }
    out
}

// ── emission ────────────────────────────────────────────────────────────────

fn preamble(out: &mut String) {
    out.push_str(
        r#"/* Fable: The Lost Chapters — def data types (cleaned C++ header).
 *
 * Generated from the game's PDB class declarations, recovered with
 * `decomp-cli tree`. The declarations are the original def classes: original
 * class and member names, original inheritance, and original field order.
 *
 * Methods, constructors, static members and compiler-specific calling
 * conventions are removed. Runtime bases (`CDefClassBase` and friends) are
 * dropped; data-bearing bases (`CThingBaseDef`, `CCombatAbilityData`, …) are
 * kept. Scalars are normalized to <cstdint>; STL default allocator/comparator
 * arguments are removed. Custom game containers are aliases to standard ones.
 *
 * This is a declaration model, not a wire layout. A def body on disk is a
 * sequence of (uint32_t crc32(field name), value) controls.
 */

#ifndef FABLE_DEFS_HPP
#define FABLE_DEFS_HPP

#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using CCharString = std::string;
using CWideString = std::u16string;

struct CDefString {
    std::int32_t TablePos;
    bool operator<(const CDefString &other) const { return TablePos < other.TablePos; }
    bool operator==(const CDefString &other) const { return TablePos == other.TablePos; }
};

struct CDefIndex {
    std::int32_t Val;
    bool operator<(const CDefIndex &other) const { return Val < other.Val; }
    bool operator==(const CDefIndex &other) const { return Val == other.Val; }
};

template<class T> using CCountedPointer = std::shared_ptr<T>;
template<class T> using CDefPointer = T *;
template<class T> using CIVCountedPointer = std::shared_ptr<T>;
template<class T> using CIntelligentPointer = T *;
template<class T> using CArray = std::vector<T>;
template<class K, class V, class Cmp> using CVectorMap = std::vector<std::pair<K, V>>;
template<class K, class V> struct CKeyPairCompareLess {};
template<class T> struct CCompareString {};
template<class T, int N> using CSmallVector = std::vector<T>;
template<class T> using CBasicString = std::string;

"#,
    );
}

fn emit_enums(out: &mut String, tree: &Tree, selector: &Selector<'_>) {
    let mut wrote = false;
    for qname in &selector.enums {
        let Some(item) = tree.enum_(qname) else {
            continue;
        };
        if item.parent.is_some() {
            continue;
        }
        if !wrote {
            out.push_str(
                "/* ── Enums ───────────────────────────────────────────────────────────── */\n\n",
            );
            wrote = true;
        }
        emit_enum(out, item, "");
    }
}

fn emit_enum(out: &mut String, item: &Enum, indent: &str) {
    let mut body = String::new();
    body.push_str(&format!("{indent}enum {}\n{indent}{{\n", item.name));
    for (name, value) in &item.values {
        match value {
            Some(value) => body.push_str(&format!("{indent}\t{name} = {},\n", clean_decl(value))),
            None => body.push_str(&format!("{indent}\t{name},\n")),
        }
    }
    body.push_str(&format!("{indent}}};\n"));
    if item.parent.is_none() && !item.namespace.is_empty() {
        out.push_str(&format!("namespace {} {{\n", item.namespace));
        out.push_str(&body);
        out.push_str("}\n");
    } else {
        out.push_str(&body);
    }
    out.push('\n');
}

fn emit_forward_declarations(out: &mut String, tree: &Tree, selector: &Selector<'_>) {
    out.push_str(
        "/* ── Forward declarations ────────────────────────────────────────────── */\n\n",
    );
    for qname in &selector.selected {
        let Some(class) = tree.class(qname) else {
            continue;
        };
        if class.parent.is_some() {
            continue;
        }
        if class.namespace.is_empty() {
            out.push_str(&format!("class {};\n", class.name));
        } else {
            out.push_str(&format!(
                "namespace {} {{ class {}; }}\n",
                class.namespace, class.name
            ));
        }
    }
    out.push('\n');
}

fn emit_class(
    out: &mut String,
    tree: &Tree,
    selected: &BTreeSet<String>,
    class: &Class,
    indent: &str,
) {
    let mut body = String::new();
    let kept = kept_bases(tree, selected, class);
    let base_text = if kept.is_empty() {
        String::new()
    } else {
        format!(
            " : {}",
            kept.iter()
                .map(|base| format!("public {}", clean_decl(base)))
                .collect::<Vec<_>>()
                .join(", ")
        )
    };
    body.push_str(&format!(
        "{indent}class {}{}\n{indent}{{\n",
        class.name, base_text
    ));
    body.push_str(&format!("{indent}public:\n"));
    for nested in &class.nested {
        match nested {
            TypeRef::Enum(qname) => {
                if let Some(item) = tree.enum_(qname) {
                    emit_enum(&mut body, item, &format!("{indent}\t"));
                }
            }
            TypeRef::Class(qname) => {
                if let Some(child) = tree.class(qname) {
                    emit_class(&mut body, tree, selected, child, &format!("{indent}\t"));
                }
            }
        }
    }
    for member in &class.members {
        if member.special {
            body.push_str(&format!("{indent}\t{};\n", clean_decl(&member.decl)));
        } else {
            body.push_str(&format!(
                "{indent}\t{} {}{};\n",
                clean_decl(&member.decl),
                member.name,
                member.suffix
            ));
        }
    }
    body.push_str(&format!("{indent}}};\n"));
    if class.parent.is_none() && !class.namespace.is_empty() {
        out.push_str(&format!("namespace {} {{\n", class.namespace));
        out.push_str(&body);
        out.push_str("}\n\n");
    } else {
        out.push_str(&body);
        out.push('\n');
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::tree::{Class, Member, TypeRef};

    fn class(name: &str, qname: &str, parent: Option<&str>, bases: &[&str]) -> Class {
        Class {
            name: name.to_owned(),
            qname: qname.to_owned(),
            namespace: String::new(),
            parent: parent.map(str::to_owned),
            bases: bases.iter().map(|b| (*b).to_owned()).collect(),
            members: Vec::new(),
            nested: Vec::new(),
            file: "test.hpp".to_owned(),
        }
    }

    fn member(decl: &str, name: &str) -> Member {
        Member {
            decl: decl.to_owned(),
            name: name.to_owned(),
            suffix: String::new(),
            special: false,
        }
    }

    #[test]
    fn emits_original_names_and_nested_types() {
        let mut tree = Tree::default();
        let mut parent = class("CFooDef", "CFooDef", None, &["CSubDefClassBase"]);
        parent
            .members
            .push(member("std::vector<long,std::allocator<long> >", "Items"));
        parent.members.push(member("EThing", "Thing"));
        parent
            .nested
            .push(TypeRef::Class("CFooDef::CEntry".to_owned()));
        let mut child = class("CEntry", "CFooDef::CEntry", Some("CFooDef"), &[]);
        child.members.push(member("long", "Value"));
        tree.classes.insert(parent.qname.clone(), parent);
        tree.classes.insert(child.qname.clone(), child);
        tree.enums.insert(
            "EThing".to_owned(),
            Enum {
                name: "EThing".to_owned(),
                qname: "EThing".to_owned(),
                namespace: String::new(),
                parent: None,
                values: vec![("THING_A".to_owned(), Some("0".to_owned()))],
            },
        );
        let (text, stats) = generate(&tree).unwrap();
        assert_eq!(stats.def_classes, 1);
        assert!(text.contains("class CFooDef"));
        assert!(text.contains("std::vector<std::int32_t> Items;"));
        assert!(text.contains("class CEntry"));
        assert!(text.contains("enum EThing"));
    }
}
