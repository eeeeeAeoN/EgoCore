//! Clean decompiler spellings out of a C++ declaration.
//!
//! The PDB declarations are exact but noisy: MSVC calling conventions,
//! `enum`/`class` type prefixes, MSVC's `long` (32-bit), and the default
//! allocator/comparator template arguments. The cleaner removes that noise and
//! leaves a portable C++17 spelling.

/// Clean a type or declaration fragment.
pub fn clean_decl(decl: &str) -> String {
    let mut s = decl.to_owned();
    for word in [
        "__thiscall",
        "__fastcall",
        "__cdecl",
        "__stdcall",
        "__vectorcall",
        "__single_inheritance",
    ] {
        s = replace_word(&s, word, "");
    }
    for word in ["enum", "class", "struct"] {
        s = remove_type_keyword(&s, word);
    }
    s = clean_scalars(&s);
    s = clean_templates(&s);
    s = normalize_spaces(&s);
    s
}

fn clean_scalars(s: &str) -> String {
    let mut s = s.to_owned();
    for (from, to) in [
        ("unsigned long long", "std::uint64_t"),
        ("long long", "std::int64_t"),
        ("__int64", "std::int64_t"),
        ("__int32", "std::int32_t"),
        ("unsigned long", "std::uint32_t"),
        ("unsigned int", "std::uint32_t"),
        ("unsigned short", "std::uint16_t"),
        ("unsigned char", "std::uint8_t"),
        ("long", "std::int32_t"),
        ("int", "std::int32_t"),
        ("short", "std::int16_t"),
    ] {
        s = replace_word(&s, from, to);
    }
    s
}

/// Remove a type keyword wherever it elides a type (`enum EThing` → `EThing`).
fn remove_type_keyword(s: &str, word: &str) -> String {
    let needle = format!("{word} ");
    let mut out = String::with_capacity(s.len());
    let mut i = 0usize;
    while i < s.len() {
        if s[i..].starts_with(&needle) && boundary_before(s, i) {
            i += needle.len();
        } else {
            let ch = s[i..].chars().next().unwrap();
            out.push(ch);
            i += ch.len_utf8();
        }
    }
    out
}

fn replace_word(s: &str, from: &str, to: &str) -> String {
    if from.is_empty() {
        return s.to_owned();
    }
    let mut out = String::with_capacity(s.len());
    let mut i = 0usize;
    while i < s.len() {
        if s[i..].starts_with(from) && boundary_before(s, i) && boundary_after(s, i + from.len()) {
            out.push_str(to);
            i += from.len();
        } else {
            let ch = s[i..].chars().next().unwrap();
            out.push(ch);
            i += ch.len_utf8();
        }
    }
    out
}

fn boundary_before(s: &str, i: usize) -> bool {
    s[..i]
        .chars()
        .next_back()
        .is_none_or(|ch| !(ch.is_alphanumeric() || ch == '_'))
}

fn boundary_after(s: &str, i: usize) -> bool {
    s[i..]
        .chars()
        .next()
        .is_none_or(|ch| !(ch.is_alphanumeric() || ch == '_'))
}

/// Recursively remove default allocator/comparator arguments.
pub fn clean_templates(s: &str) -> String {
    let mut s = s.to_owned();
    for base in [
        "std::vector",
        "std::list",
        "std::deque",
        "std::stack",
        "std::queue",
        "std::map",
        "std::multimap",
        "std::unordered_map",
        "std::unordered_multimap",
        "std::set",
        "std::multiset",
        "std::unordered_set",
        "std::unordered_multiset",
        "std::pair",
        "std::shared_ptr",
        "std::unique_ptr",
        "CVectorMap",
        "CArray",
        "CCountedPointer",
        "CDefPointer",
        "CIVCountedPointer",
        "CIntelligentPointer",
        "CSmallVector",
        "CBasicString",
        "CKeyPairCompareLess",
        "boost::scoped_ptr",
    ] {
        s = clean_template_at(&s, base);
    }
    s
}

fn clean_template_at(s: &str, base: &str) -> String {
    let needle = format!("{base}<");
    let mut out = String::with_capacity(s.len());
    let mut i = 0usize;
    while let Some(found) = s[i..].find(&needle) {
        let start = i + found;
        out.push_str(&s[i..start]);
        let open = start + base.len();
        let Some(close) = find_matching(s, open) else {
            out.push_str(&s[start..]);
            return out;
        };
        let mut args: Vec<String> = split_args(&s[open + 1..close])
            .into_iter()
            .map(|arg| clean_templates(&arg))
            .collect();
        if matches!(
            base,
            "std::vector" | "std::list" | "std::deque" | "std::stack" | "std::queue"
        ) && args.len() == 2
            && args[1].starts_with("std::allocator<")
        {
            args.truncate(1);
        } else if matches!(
            base,
            "std::map" | "std::multimap" | "std::unordered_map" | "std::unordered_multimap"
        ) && args.len() >= 3
        {
            args.truncate(2);
        } else if matches!(
            base,
            "std::set" | "std::multiset" | "std::unordered_set" | "std::unordered_multiset"
        ) && args.len() >= 2
        {
            args.truncate(1);
        }
        out.push_str(base);
        out.push('<');
        out.push_str(&args.join(","));
        out.push('>');
        i = close + 1;
    }
    out.push_str(&s[i..]);
    out
}

fn find_matching(s: &str, open: usize) -> Option<usize> {
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

fn split_args(s: &str) -> Vec<String> {
    let mut out = Vec::new();
    let mut current = String::new();
    let mut depth = 0i32;
    for ch in s.chars() {
        match ch {
            '<' | '(' | '[' => depth += 1,
            '>' | ')' | ']' => depth -= 1,
            _ => {}
        }
        if ch == ',' && depth == 0 {
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

fn normalize_spaces(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    let mut space = false;
    for ch in s.chars() {
        if ch.is_whitespace() {
            space = true;
        } else {
            if space && !out.is_empty() {
                out.push(' ');
            }
            space = false;
            out.push(ch);
        }
    }
    let mut out = out.trim().to_owned();
    for (from, to) in [(" *", "*"), ("* ", "*"), (" &", "&"), ("& ", "&")] {
        out = out.replace(from, to);
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn removes_msvc_noise_and_default_arguments() {
        assert_eq!(clean_decl("unsigned long Value"), "std::uint32_t Value");
        assert_eq!(
            clean_decl("std::vector<long,std::allocator<long> > Items"),
            "std::vector<std::int32_t> Items"
        );
        assert_eq!(
            clean_decl(
                "std::map<CCharString,long,std::less<CCharString>,std::allocator<std::pair<CCharString const ,long> > > PhonemeAnim"
            ),
            "std::map<CCharString,std::int32_t> PhonemeAnim"
        );
        assert_eq!(clean_decl("enum EThing Value"), "EThing Value");
        assert_eq!(
            clean_decl("void __thiscall (*Callback)(void)"),
            "void (*Callback)(void)"
        );
    }
}
