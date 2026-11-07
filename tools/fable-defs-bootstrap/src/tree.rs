//! Parse the decomposed C++ source tree written by `decomp-cli tree`.
//!
//! The decompiler emits one header per original file, with class, struct and
//! enum declarations in Allman style and method declarations one per line.
//! We only need the *data*: class names, bases, fields, nested types, and
//! enums. This module extracts that shape without a full C++ parser.

use std::collections::BTreeMap;
use std::fs;
use std::path::{Path, PathBuf};

use crate::{Error, Result};

/// One data member of a class.
#[derive(Debug, Clone)]
pub struct Member {
    /// The declaration text before the member name.
    pub decl: String,
    pub name: String,
    /// Array suffixes and bit-field width (`"[4]"`, `" : 1"`).
    pub suffix: String,
    /// A function-pointer member, kept as one raw declaration.
    pub special: bool,
}

/// A class or struct definition.
#[derive(Debug, Clone)]
pub struct Class {
    pub name: String,
    pub qname: String,
    /// Enclosing namespace, `""` at global scope.
    pub namespace: String,
    /// Enclosing class, if this type is nested.
    pub parent: Option<String>,
    pub bases: Vec<String>,
    pub members: Vec<Member>,
    pub nested: Vec<TypeRef>,
    pub file: String,
}

#[derive(Debug, Clone)]
pub enum TypeRef {
    Class(String),
    Enum(String),
}

/// An enum definition.
#[derive(Debug, Clone)]
pub struct Enum {
    pub name: String,
    pub qname: String,
    pub namespace: String,
    pub parent: Option<String>,
    pub values: Vec<(String, Option<String>)>,
}

#[derive(Debug, Default)]
pub struct Tree {
    pub classes: BTreeMap<String, Class>,
    pub enums: BTreeMap<String, Enum>,
}

impl Tree {
    /// The namespace-qualified name of a top-level or nested class.
    pub fn class(&self, qname: &str) -> Option<&Class> {
        self.classes.get(qname)
    }

    pub fn enum_(&self, qname: &str) -> Option<&Enum> {
        self.enums.get(qname)
    }
}

/// Parse every `.hpp` under `root`.
pub fn parse_tree(root: &Path) -> Result<Tree> {
    let files = hpp_files(root)?;
    if files.is_empty() {
        return Err(Error::new(format!(
            "{}: no .hpp files (is this a `decomp-cli tree` output?)",
            root.display()
        )));
    }
    let mut tree = Tree::default();
    for path in files {
        let text = fs::read_to_string(&path)
            .map_err(|e| Error::new(format!("{}: {e}", path.display())))?;
        let file = path
            .strip_prefix(root)
            .unwrap_or(&path)
            .to_string_lossy()
            .replace('\\', "/");
        parse_text(&text, &file, &mut tree)?;
    }
    if tree.classes.is_empty() {
        return Err(Error::new(format!("{}: parsed no classes", root.display())));
    }
    Ok(tree)
}

fn hpp_files(root: &Path) -> Result<Vec<PathBuf>> {
    let mut out = Vec::new();
    let mut stack = vec![root.to_path_buf()];
    while let Some(dir) = stack.pop() {
        let entries =
            fs::read_dir(&dir).map_err(|e| Error::new(format!("{}: {e}", dir.display())))?;
        for entry in entries {
            let entry = entry.map_err(|e| Error::new(format!("{}: {e}", dir.display())))?;
            let path = entry.path();
            if path.is_dir() {
                stack.push(path);
            } else if path.extension().is_some_and(|ext| ext == "hpp") {
                out.push(path);
            }
        }
    }
    out.sort();
    Ok(out)
}

#[derive(Debug)]
enum Frame {
    Namespace,
    Class(String),
    Enum(String),
}

fn parse_text(text: &str, file: &str, tree: &mut Tree) -> Result<()> {
    let lines: Vec<&str> = text.lines().collect();
    let mut ns_stack: Vec<String> = Vec::new();
    let mut frames: Vec<Frame> = Vec::new();
    let mut i = 0usize;

    while i < lines.len() {
        let line = lines[i].trim();
        i += 1;

        if line.is_empty() || line.starts_with('#') || line.starts_with("//") {
            continue;
        }

        if let Some(name) = parse_namespace(line) {
            frames.push(Frame::Namespace);
            ns_stack.push(name);
            continue;
        }

        if line == "}" || line == "};" {
            if let Some(Frame::Namespace) = frames.pop() {
                ns_stack.pop();
            }
            continue;
        }

        // A class template definition is not a def type; skip its whole body.
        // A template forward declaration (`template<...> class X;`) is just a
        // line; do not let it swallow the next class definition.
        if line.starts_with("template") {
            if line.ends_with(';') {
                continue;
            }
            i = skip_template_body(&lines, i);
            continue;
        }

        if let Some((name, bases_text)) = parse_class_header(line) {
            if next_is_open(&lines, i) {
                i = consume_open(&lines, i);
                let parent = current_class(&frames);
                let namespace = ns_stack.join("::");
                let qname = qualify(&namespace, parent.as_deref(), &name);
                let class = Class {
                    name: name.clone(),
                    qname: qname.clone(),
                    namespace,
                    parent: parent.clone(),
                    bases: parse_bases(bases_text),
                    members: Vec::new(),
                    nested: Vec::new(),
                    file: file.to_owned(),
                };
                if let Some(parent) = &parent
                    && let Some(parent_class) = tree.classes.get_mut(parent)
                {
                    parent_class.nested.push(TypeRef::Class(qname.clone()));
                }
                tree.classes.insert(qname.clone(), class);
                frames.push(Frame::Class(qname));
                continue;
            }
            continue;
        }

        if let Some(name) = parse_enum_header(line) {
            if next_is_open(&lines, i) {
                i = consume_open(&lines, i);
                let parent = current_class(&frames);
                let namespace = ns_stack.join("::");
                let qname = qualify(&namespace, parent.as_deref(), &name);
                let item = Enum {
                    name,
                    qname: qname.clone(),
                    namespace,
                    parent: parent.clone(),
                    values: Vec::new(),
                };
                if let Some(parent) = &parent
                    && let Some(parent_class) = tree.classes.get_mut(parent)
                {
                    parent_class.nested.push(TypeRef::Enum(qname.clone()));
                }
                tree.enums.insert(qname.clone(), item);
                frames.push(Frame::Enum(qname));
                continue;
            }
            continue;
        }

        match frames.last() {
            Some(Frame::Enum(qname)) => {
                if let Some(item) = tree.enums.get_mut(qname) {
                    parse_enum_values(line, &mut item.values);
                }
            }
            Some(Frame::Class(qname)) => {
                if let Some(class) = tree.classes.get_mut(qname) {
                    parse_member(line, class);
                }
            }
            _ => {}
        }
    }
    Ok(())
}

fn parse_namespace(line: &str) -> Option<String> {
    let rest = line.strip_prefix("namespace ")?;
    let rest = rest.trim_end_matches('{').trim();
    let name = rest.split_whitespace().next()?;
    if name.is_empty() {
        None
    } else {
        Some(name.to_owned())
    }
}

/// `class Foo : public Bar` → `("Foo", Some("public Bar"))`.
fn parse_class_header(line: &str) -> Option<(String, Option<&str>)> {
    let rest = line
        .strip_prefix("class ")
        .or_else(|| line.strip_prefix("struct "))?;
    let rest = rest.trim();
    if rest.starts_with("__") {
        return None;
    }
    let end = rest
        .find(|ch: char| ch.is_whitespace() || ch == ':' || ch == '{' || ch == ';')
        .unwrap_or(rest.len());
    let name = &rest[..end];
    if name.is_empty() {
        return None;
    }
    let after = rest[end..].trim_start();
    let bases = after.strip_prefix(':').map(str::trim);
    Some((name.to_owned(), bases))
}

fn parse_enum_header(line: &str) -> Option<String> {
    let rest = line.strip_prefix("enum ")?;
    let rest = rest.trim();
    let rest = rest.strip_prefix("class ").unwrap_or(rest);
    let name = rest
        .split(|ch: char| ch.is_whitespace() || ch == ':' || ch == '{' || ch == ';')
        .next()?;
    if name.is_empty() || name.starts_with("__") {
        None
    } else {
        Some(name.to_owned())
    }
}

fn next_is_open(lines: &[&str], mut i: usize) -> bool {
    while i < lines.len() {
        let line = lines[i].trim();
        if line.is_empty() || line.starts_with("//") {
            i += 1;
        } else {
            return line == "{";
        }
    }
    false
}

fn consume_open(lines: &[&str], mut i: usize) -> usize {
    while i < lines.len() {
        let line = lines[i].trim();
        if line == "{" {
            return i + 1;
        }
        i += 1;
    }
    i
}

fn skip_template_body(lines: &[&str], mut i: usize) -> usize {
    while i < lines.len() && lines[i].trim() != "{" {
        i += 1;
    }
    if i == lines.len() {
        return i;
    }
    let mut depth = 1i32;
    i += 1;
    while i < lines.len() && depth > 0 {
        for ch in lines[i].chars() {
            match ch {
                '{' => depth += 1,
                '}' => depth -= 1,
                _ => {}
            }
        }
        i += 1;
    }
    i
}

fn current_class(frames: &[Frame]) -> Option<String> {
    frames.iter().rev().find_map(|frame| match frame {
        Frame::Class(qname) => Some(qname.clone()),
        _ => None,
    })
}

fn qualify(namespace: &str, parent: Option<&str>, name: &str) -> String {
    if let Some(parent) = parent {
        format!("{parent}::{name}")
    } else if namespace.is_empty() {
        name.to_owned()
    } else {
        format!("{namespace}::{name}")
    }
}

fn parse_bases(bases: Option<&str>) -> Vec<String> {
    let Some(bases) = bases else {
        return Vec::new();
    };
    split_top(bases, ',')
        .into_iter()
        .map(|base| {
            let mut base = base.trim().to_owned();
            for keyword in ["public ", "protected ", "private ", "virtual "] {
                base = base.replace(keyword, "");
            }
            base.trim().to_owned()
        })
        .filter(|base| !base.is_empty())
        .collect()
}

fn parse_enum_values(line: &str, values: &mut Vec<(String, Option<String>)>) {
    let line = line.split("//").next().unwrap_or(line);
    for part in split_top(line, ',') {
        let part = part.trim().trim_end_matches([',', ';']).trim();
        if part.is_empty() || part == "}" {
            continue;
        }
        if let Some((name, value)) = part.split_once('=') {
            values.push((name.trim().to_owned(), Some(value.trim().to_owned())));
        } else if is_identifier(part) {
            values.push((part.to_owned(), None));
        }
    }
}

fn parse_member(line: &str, class: &mut Class) {
    if matches!(
        line.trim_end_matches(':').trim(),
        "public" | "protected" | "private"
    ) {
        return;
    }
    if line.starts_with("typedef ")
        || line.starts_with("using ")
        || line.starts_with("static ")
        || line.starts_with("friend ")
        || line.starts_with("virtual ")
        || line.starts_with("template")
        || line.starts_with("class ")
        || line.starts_with("struct ")
        || line.starts_with("enum ")
    {
        return;
    }
    if !line.ends_with(';') {
        return;
    }
    if is_method(line) {
        return;
    }
    if let Some(member) = split_member(line) {
        class.members.push(member);
    }
}

fn is_method(line: &str) -> bool {
    if line.contains("(*") {
        return false;
    }
    let mut depth = 0i32;
    for ch in line.chars() {
        match ch {
            '<' => depth += 1,
            '>' => depth -= 1,
            '(' if depth == 0 => return true,
            _ => {}
        }
    }
    false
}

/// Split `TYPE NAME[4] : 1;` into (type, name, suffix).
fn split_member(line: &str) -> Option<Member> {
    let mut s = line.trim().trim_end_matches(';').trim().to_owned();

    if let Some(pos) = s.find("(*") {
        let tail = &s[pos..];
        let close = tail.find(')')?;
        let ptr = &tail[..close];
        let name: String = ptr
            .chars()
            .rev()
            .skip_while(|ch| *ch == '*' || ch.is_whitespace())
            .take_while(|ch| ch.is_alphanumeric() || *ch == '_')
            .collect::<String>()
            .chars()
            .rev()
            .collect();
        if name.is_empty() {
            return None;
        }
        return Some(Member {
            decl: s,
            name,
            suffix: String::new(),
            special: true,
        });
    }

    let mut suffix = String::new();
    if let Some(pos) = top_level_colon(&s) {
        let bit = s[pos..].trim();
        if bit.len() > 1 && bit[1..].trim().chars().all(|ch| ch.is_ascii_digit()) {
            suffix = format!(" {bit}");
            s.truncate(pos);
            s = s.trim_end().to_owned();
        }
    }
    loop {
        let trimmed = s.trim_end();
        let Some(open) = trimmed.rfind('[') else {
            break;
        };
        if !trimmed.ends_with(']') {
            break;
        }
        suffix = format!("{}{}", &trimmed[open..], suffix);
        s.truncate(open);
        s = s.trim_end().to_owned();
    }

    let (start, end) = last_identifier(&s)?;
    let name = s[start..end].to_owned();
    let decl = s[..start].trim_end().to_owned();
    if decl.is_empty() {
        return None;
    }
    Some(Member {
        decl,
        name,
        suffix,
        special: false,
    })
}

fn top_level_colon(s: &str) -> Option<usize> {
    let bytes = s.as_bytes();
    let mut angle = 0i32;
    let mut found = None;
    for (i, &ch) in bytes.iter().enumerate() {
        match ch {
            b'<' => angle += 1,
            b'>' => angle -= 1,
            b':' if angle == 0 => {
                let prev = i.checked_sub(1).and_then(|j| bytes.get(j)).copied();
                let next = bytes.get(i + 1).copied();
                if prev != Some(b':') && next != Some(b':') {
                    found = Some(i);
                }
            }
            _ => {}
        }
    }
    found
}

fn last_identifier(s: &str) -> Option<(usize, usize)> {
    let bytes = s.as_bytes();
    let mut angle = 0i32;
    let mut spans = Vec::new();
    let mut i = 0usize;
    while i < bytes.len() {
        let ch = bytes[i];
        if ch == b'<' {
            angle += 1;
            i += 1;
            continue;
        }
        if ch == b'>' {
            angle -= 1;
            i += 1;
            continue;
        }
        if ch.is_ascii_alphabetic() || ch == b'_' {
            let start = i;
            while i < bytes.len() && (bytes[i].is_ascii_alphanumeric() || bytes[i] == b'_') {
                i += 1;
            }
            if angle == 0 {
                let before = s[..start].trim_end();
                let after = s[i..].trim_start();
                if !before.ends_with("::") && !after.starts_with("::") {
                    spans.push((start, i));
                }
            }
            continue;
        }
        i += 1;
    }
    spans.pop()
}

fn split_top(s: &str, sep: char) -> Vec<String> {
    let mut out = Vec::new();
    let mut current = String::new();
    let mut depth = 0i32;
    for ch in s.chars() {
        match ch {
            '<' | '(' | '[' => depth += 1,
            '>' | ')' | ']' => depth -= 1,
            _ => {}
        }
        if ch == sep && depth == 0 {
            out.push(current.trim().to_owned());
            current.clear();
        } else {
            current.push(ch);
        }
    }
    if !current.trim().is_empty() {
        out.push(current.trim().to_owned());
    }
    out
}

fn is_identifier(s: &str) -> bool {
    let mut chars = s.chars();
    let Some(first) = chars.next() else {
        return false;
    };
    (first == '_' || first.is_ascii_alphabetic())
        && chars.all(|ch| ch == '_' || ch.is_ascii_alphanumeric())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_nested_types_and_members() {
        let text = r#"
namespace N {
class CFoo : public CBar
{
public:
    class CEntry
    {
    public:
        long Value;
    };
    std::vector<long,std::allocator<long> > Items;
    bool Flag : 1;
    void Method();
};
enum EThing
{
    THING_A = 0,
    THING_B = 1,
};
}
"#;
        let mut tree = Tree::default();
        parse_text(text, "test.hpp", &mut tree).unwrap();
        let foo = tree.class("N::CFoo").unwrap();
        assert_eq!(foo.bases, vec!["CBar"]);
        assert!(
            foo.nested
                .iter()
                .any(|n| matches!(n, TypeRef::Class(q) if q == "N::CFoo::CEntry"))
        );
        assert_eq!(foo.members.len(), 2);
        assert_eq!(foo.members[0].name, "Items");
        assert_eq!(foo.members[1].name, "Flag");
        assert_eq!(foo.members[1].suffix, " : 1");
        let nested = tree.class("N::CFoo::CEntry").unwrap();
        assert_eq!(nested.members[0].name, "Value");
        let thing = tree.enum_("N::EThing").unwrap();
        assert_eq!(thing.values.len(), 2);
    }
}
