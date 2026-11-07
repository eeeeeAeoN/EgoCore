//! Semantic verification — compare our compiled output to retail.
//!
//! Default mode (`--ledger`): reference-resolving full-ledger comparison,
//! classifying every named entry and anonymous sub-def as Reproduced, Bug, etc.
//!
//! Sub-def dump mode (`--dump-subdef <parent> [<tag>]`): pretty-print the
//! SemVal trees of a specific parent's sub-defs from both builds.
//!
//! Usage:
//!   cargo run -p def-compiler --example verify -- <ours_dir> <retail_dir>
//!   cargo run -p def-compiler --example verify -- <ours_dir> <retail_dir> --dump-subdef <Parent> [<Tag>]

use std::collections::HashMap;
use std::path::Path;

use defs::binary::DefBody;
use defs::crc32::crc;
use defs::def::binary::{DefBinary, SubDefRecord};
use defs::names::Names;
use defs::visit::{FieldRef, FieldVisitor, StructSlot};

// ═══════════════════════════════════════════════════════════════════════════════
// SemVal: reference-resolved semantic value tree
// ═══════════════════════════════════════════════════════════════════════════════

#[derive(Debug, Clone, PartialEq)]
enum SemVal {
    F32(u32),
    Int(i64),
    Bool(bool),
    Str(String),
    WStr(String),
    DefRef(String),
    DefStr(String),
    PString(Vec<u8>),
    List(Vec<SemVal>),
    Map(Vec<(SemVal, SemVal)>),
    Struct(Vec<(&'static str, SemVal)>),
    Variant(u32, Vec<(&'static str, SemVal)>),
    Opaque(&'static str),
}

impl SemVal {
    fn contains_opaque(&self) -> bool {
        match self {
            SemVal::Opaque(_) => true,
            SemVal::List(xs) => xs.iter().any(SemVal::contains_opaque),
            SemVal::Map(xs) => xs
                .iter()
                .any(|(k, v)| k.contains_opaque() || v.contains_opaque()),
            SemVal::Struct(xs) | SemVal::Variant(_, xs) => {
                xs.iter().any(|(_, v)| v.contains_opaque())
            }
            _ => false,
        }
    }
}

struct Resolvers<'a> {
    def_index_name: &'a dyn Fn(i32) -> Option<String>,
    def_string_value: &'a dyn Fn(i32) -> Option<String>,
}

fn field_to_semval(field: FieldRef<'_>, r: &Resolvers) -> SemVal {
    match field {
        FieldRef::F32(x) => SemVal::F32(x.to_bits()),
        FieldRef::I32(x) => SemVal::Int(*x as i64),
        FieldRef::U32(x) => SemVal::Int(*x as i64),
        FieldRef::Bool(x) => SemVal::Bool(*x),
        FieldRef::Str(x) => SemVal::Str(x.clone()),
        FieldRef::WStr(x) => SemVal::WStr(x.0.clone()),
        FieldRef::Enum(s) => SemVal::Int(s.get_i32() as i64),
        FieldRef::Flags(s) => SemVal::Int(s.get_i32() as i64),
        FieldRef::DefString(ds) => {
            let off = ds.0;
            SemVal::DefStr((r.def_string_value)(off).unwrap_or_else(|| format!("off:{off}")))
        }
        FieldRef::DefIndex(di) => {
            let idx = di.0;
            SemVal::DefRef((r.def_index_name)(idx).unwrap_or_else(|| format!("idx:{idx}")))
        }
        FieldRef::U8(x) => SemVal::Int(*x as i64),
        FieldRef::U16(x) => SemVal::Int(*x as i64),
        FieldRef::U64(x) => SemVal::Int(*x as i64),
        FieldRef::I8(x) => SemVal::Int(*x as i64),
        FieldRef::I16(x) => SemVal::Int(*x as i64),
        FieldRef::PString(p) => SemVal::PString(p.0.clone()),
        FieldRef::Vec(slot) => {
            let n = slot.len();
            let mut out = Vec::with_capacity(n);
            for i in 0..n {
                out.push(field_to_semval(slot.element(i), r));
            }
            SemVal::List(out)
        }
        FieldRef::Map(slot) => {
            let mut out = Vec::new();
            slot.for_each_pair(&mut |k, v| {
                out.push((field_to_semval(k, r), field_to_semval(v, r)));
            });
            SemVal::Map(out)
        }
        FieldRef::Struct(slot) => SemVal::Struct(read_members(slot, r)),
        FieldRef::Variant(slot) => {
            let tag = slot.tag();
            let n = slot.member_count();
            let mut out = Vec::with_capacity(n);
            for i in 0..n {
                let name = slot.member_name(i).unwrap_or("?");
                if let Some(m) = slot.member(i) {
                    out.push((name, field_to_semval(m, r)));
                }
            }
            SemVal::Variant(tag, out)
        }
        FieldRef::Array(slot) => {
            let n = slot.len();
            let mut out = Vec::with_capacity(n);
            for i in 0..n {
                out.push(field_to_semval(slot.element(i), r));
            }
            SemVal::List(out)
        }
        FieldRef::Complex(s) => SemVal::Opaque(s),
    }
}

fn read_members(slot: &mut dyn StructSlot, r: &Resolvers) -> Vec<(&'static str, SemVal)> {
    let n = slot.member_count();
    let mut out = Vec::with_capacity(n);
    for i in 0..n {
        let name = slot.member_name(i).unwrap_or("?");
        if let Some(m) = slot.member(i) {
            out.push((name, field_to_semval(m, r)));
        }
    }
    out
}

struct Collector<'a, 'b> {
    r: &'a Resolvers<'b>,
    out: Vec<(&'static str, SemVal)>,
}

impl FieldVisitor for Collector<'_, '_> {
    fn field(&mut self, name: &'static str, field: FieldRef<'_>) {
        self.out.push((name, field_to_semval(field, self.r)));
    }
}

fn game_body_to_semval(b: &mut DefBody, r: &Resolvers) -> SemVal {
    let mut c = Collector { r, out: Vec::new() };
    b.visit_active(&mut c);
    SemVal::Struct(c.out)
}

#[derive(Clone, Copy, Default)]
struct DiffPolicy {
    unordered_maps: bool,
    unordered_lists: bool,
}

impl DiffPolicy {
    fn strict() -> Self {
        DiffPolicy {
            unordered_maps: false,
            unordered_lists: false,
        }
    }
    fn unordered() -> Self {
        DiffPolicy {
            unordered_maps: true,
            unordered_lists: true,
        }
    }
}

fn sem_eq(a: &SemVal, b: &SemVal, policy: DiffPolicy) -> bool {
    match (a, b) {
        (SemVal::Map(xa), SemVal::Map(xb)) => {
            xa.len() == xb.len()
                && if policy.unordered_maps {
                    multiset_eq_pairs(xa, xb, policy)
                } else {
                    xa.iter().zip(xb).all(|((ka, va), (kb, vb))| {
                        sem_eq(ka, kb, policy) && sem_eq(va, vb, policy)
                    })
                }
        }
        (SemVal::List(xa), SemVal::List(xb)) => {
            xa.len() == xb.len()
                && if policy.unordered_lists {
                    multiset_eq(xa, xb, policy)
                } else {
                    xa.iter().zip(xb).all(|(x, y)| sem_eq(x, y, policy))
                }
        }
        (SemVal::Struct(xa), SemVal::Struct(xb)) => {
            xa.len() == xb.len()
                && xa
                    .iter()
                    .zip(xb)
                    .all(|((na, va), (nb, vb))| na == nb && sem_eq(va, vb, policy))
        }
        (SemVal::Variant(ta, xa), SemVal::Variant(tb, xb)) => {
            ta == tb
                && xa.len() == xb.len()
                && xa
                    .iter()
                    .zip(xb)
                    .all(|((na, va), (nb, vb))| na == nb && sem_eq(va, vb, policy))
        }
        _ => a == b,
    }
}

fn multiset_eq(a: &[SemVal], b: &[SemVal], policy: DiffPolicy) -> bool {
    let mut used = vec![false; b.len()];
    for x in a {
        let Some(j) = b
            .iter()
            .enumerate()
            .position(|(j, y)| !used[j] && sem_eq(x, y, policy))
        else {
            return false;
        };
        used[j] = true;
    }
    true
}

fn multiset_eq_pairs(a: &[(SemVal, SemVal)], b: &[(SemVal, SemVal)], policy: DiffPolicy) -> bool {
    let mut used = vec![false; b.len()];
    for (ka, va) in a {
        let Some(j) = b
            .iter()
            .enumerate()
            .position(|(j, (kb, vb))| !used[j] && sem_eq(ka, kb, policy) && sem_eq(va, vb, policy))
        else {
            return false;
        };
        used[j] = true;
    }
    true
}

fn first_diff(a: &SemVal, b: &SemVal, policy: DiffPolicy) -> Option<Diff> {
    let mut path = String::new();
    first_diff_at(a, b, policy, &mut path)
}

struct Diff {
    path: String,
    ours: String,
    theirs: String,
}

fn short(v: &SemVal) -> String {
    match v {
        SemVal::List(xs) => format!("List(len={})", xs.len()),
        SemVal::Map(xs) => format!("Map(len={})", xs.len()),
        SemVal::Struct(xs) => format!("Struct(fields={})", xs.len()),
        SemVal::Variant(t, xs) => format!("Variant(tag={t},fields={})", xs.len()),
        other => format!("{other:?}"),
    }
}

fn diff_here(a: &SemVal, b: &SemVal, path: &str) -> Diff {
    Diff {
        path: path.to_string(),
        ours: short(a),
        theirs: short(b),
    }
}

fn diff_members(
    a: &SemVal,
    b: &SemVal,
    xa: &[(&'static str, SemVal)],
    xb: &[(&'static str, SemVal)],
    policy: DiffPolicy,
    path: &mut String,
) -> Option<Diff> {
    if xa.len() != xb.len() {
        return Some(diff_here(a, b, path));
    }
    for ((na, va), (nb, vb)) in xa.iter().zip(xb) {
        if na != nb {
            return Some(diff_here(a, b, path));
        }
        let len = path.len();
        if !path.is_empty() {
            path.push('.');
        }
        path.push_str(na);
        if let Some(d) = first_diff_at(va, vb, policy, path) {
            return Some(d);
        }
        path.truncate(len);
    }
    None
}

fn first_diff_at(a: &SemVal, b: &SemVal, policy: DiffPolicy, path: &mut String) -> Option<Diff> {
    match (a, b) {
        (SemVal::Struct(xa), SemVal::Struct(xb)) => diff_members(a, b, xa, xb, policy, path),
        (SemVal::Variant(ta, xa), SemVal::Variant(tb, xb)) => {
            if ta != tb {
                return Some(diff_here(a, b, path));
            }
            diff_members(a, b, xa, xb, policy, path)
        }
        (SemVal::List(xa), SemVal::List(xb)) => {
            // Under the unordered policy, positional comparison reports pure
            // reordering as a spurious `[0]` diff and masks the real divergence.
            // Reconcile as a multiset instead: skip if equal, else pinpoint the
            // element that actually differs (presence or nearest near-match).
            if policy.unordered_lists {
                return unordered_seq_diff(xa, xb, policy, path);
            }
            if xa.len() != xb.len() {
                return Some(diff_here(a, b, path));
            }
            for (i, (x, y)) in xa.iter().zip(xb).enumerate() {
                let len = path.len();
                path.push_str(&format!("[{i}]"));
                if let Some(d) = first_diff_at(x, y, policy, path) {
                    return Some(d);
                }
                path.truncate(len);
            }
            None
        }
        (SemVal::Map(xa), SemVal::Map(xb)) => {
            if policy.unordered_maps {
                return unordered_pair_diff(xa, xb, policy, path);
            }
            if xa.len() != xb.len() {
                return Some(diff_here(a, b, path));
            }
            for (i, ((ka, va), (kb, vb))) in xa.iter().zip(xb).enumerate() {
                let len = path.len();
                path.push_str(&format!("[{i}].key"));
                if let Some(d) = first_diff_at(ka, kb, policy, path) {
                    return Some(d);
                }
                path.truncate(len);
                path.push_str(&format!("[{i}]"));
                if let Some(d) = first_diff_at(va, vb, policy, path) {
                    return Some(d);
                }
                path.truncate(len);
            }
            None
        }
        _ if sem_eq(a, b, policy) => None,
        _ => Some(diff_here(a, b, path)),
    }
}

/// Count how structurally similar two values are (matching member/leaf pairs),
/// used to pair up leftover elements when diffing unordered lists.
fn similarity(a: &SemVal, b: &SemVal) -> usize {
    match (a, b) {
        (SemVal::Struct(xa), SemVal::Struct(xb))
        | (SemVal::Variant(_, xa), SemVal::Variant(_, xb)) => xa
            .iter()
            .zip(xb)
            .filter(|((na, va), (nb, vb))| na == nb && va == vb)
            .count(),
        _ => usize::from(a == b),
    }
}

/// Diff two lists as multisets under `policy`: remove exact matches greedily,
/// then report the first genuinely-unmatched element — either as a presence
/// difference or, when both sides have leftovers, by descending into the most
/// similar pair to pinpoint the differing field.
fn unordered_seq_diff(
    xa: &[SemVal],
    xb: &[SemVal],
    policy: DiffPolicy,
    path: &str,
) -> Option<Diff> {
    let mut used_b = vec![false; xb.len()];
    let mut rem_a: Vec<&SemVal> = Vec::new();
    for x in xa {
        match xb
            .iter()
            .enumerate()
            .position(|(j, y)| !used_b[j] && sem_eq(x, y, policy))
        {
            Some(j) => used_b[j] = true,
            None => rem_a.push(x),
        }
    }
    let rem_b: Vec<&SemVal> = xb
        .iter()
        .enumerate()
        .filter(|(j, _)| !used_b[*j])
        .map(|(_, y)| y)
        .collect();
    diff_leftovers(&rem_a, &rem_b, policy, path)
}

/// Same as [`unordered_seq_diff`] but for map (key, value) pairs.
fn unordered_pair_diff(
    xa: &[(SemVal, SemVal)],
    xb: &[(SemVal, SemVal)],
    policy: DiffPolicy,
    path: &str,
) -> Option<Diff> {
    let pair =
        |(k, v): &(SemVal, SemVal)| SemVal::Struct(vec![("key", k.clone()), ("value", v.clone())]);
    let a: Vec<SemVal> = xa.iter().map(pair).collect();
    let b: Vec<SemVal> = xb.iter().map(pair).collect();
    unordered_seq_diff(&a, &b, policy, path)
}

fn diff_leftovers(
    rem_a: &[&SemVal],
    rem_b: &[&SemVal],
    policy: DiffPolicy,
    path: &str,
) -> Option<Diff> {
    match (rem_a.first(), rem_b.first()) {
        (None, None) => None,
        (Some(x), None) => Some(Diff {
            path: format!("{path}[+ours({} extra)]", rem_a.len()),
            ours: short(x),
            theirs: "<absent>".into(),
        }),
        (None, Some(y)) => Some(Diff {
            path: format!("{path}[+theirs({} extra)]", rem_b.len()),
            ours: "<absent>".into(),
            theirs: short(y),
        }),
        (Some(x), Some(_)) => {
            let best = rem_b.iter().max_by_key(|y| similarity(x, y)).unwrap();
            let mut p = format!("{path}[~]");
            first_diff_at(x, best, policy, &mut p).or_else(|| Some(diff_here(x, best, &p)))
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Ledger
// ═══════════════════════════════════════════════════════════════════════════════

#[derive(PartialEq, Eq, Hash, Clone, Copy, Debug)]
enum Class {
    Reproduced,
    AcceptSort,
    OpaqueOnly,
    AcceptArtifact,
    Bug,
    NonGame,
    Missing,
}

const ARTIFACT_FRAGMENTS: &[&str] = &[
    "trailing_u32",
    "next_filter",
    "RandomAppearanceMorph",
    "AugmentationParticles",
    "FlourishParticles",
    // CDegradableInfo trailing struct-padding (25-byte struct, 4-byte tail): 0
    // for every entry except CREATURE_GENERATOR_WASP_NEST_01, whose element
    // caught a stale heap value (0x01AB6CE3, the 0x01AB6Cxx heap-sentinel family
    // as trailing_u32=0x01AB6CCE). Uninitialized memory, 1 entry — unreproducible.
    "skip",
];

fn is_artifact_leaf(path: &str) -> bool {
    if std::env::var_os("STRICT").is_some() {
        return false;
    }
    ARTIFACT_FRAGMENTS.iter().any(|f| path.contains(f))
}

#[derive(Default)]
struct BinLedger {
    tally: HashMap<Class, usize>,
    bug_paths: HashMap<String, usize>,
    samples: Vec<(Class, String, String, String)>,
    subdef_tally: HashMap<Class, usize>,
    subdef_bug_tags: HashMap<String, usize>,
    subdef_class_tags: HashMap<Class, HashMap<String, usize>>,
    subdef_samples: Vec<(Class, String, String, String)>,
    subdef_count_mismatch: usize,
    subdef_artifact_paths: HashMap<String, usize>,
    // Diagnostic: for OpaqueOnly entries, histogram of `type.opaque_path` — i.e.
    // exactly which fixed-array field the differ is BLIND to (always-equal).
    opaque_paths: HashMap<String, usize>,
}

/// Path to the first `Opaque` leaf in a decoded body — the field the differ
/// cannot see into. Used to quantify the differ's blind spots.
fn first_opaque_path(v: &SemVal, path: &mut String) -> Option<String> {
    match v {
        SemVal::Opaque(_) => Some(path.clone()),
        SemVal::List(xs) => xs.iter().enumerate().find_map(|(i, x)| {
            let len = path.len();
            path.push_str(&format!("[{i}]"));
            let r = first_opaque_path(x, path);
            path.truncate(len);
            r
        }),
        SemVal::Map(xs) => xs.iter().find_map(|(_, x)| {
            let len = path.len();
            path.push_str("[v]");
            let r = first_opaque_path(x, path);
            path.truncate(len);
            r
        }),
        SemVal::Struct(xs) | SemVal::Variant(_, xs) => xs.iter().find_map(|(n, x)| {
            let len = path.len();
            if !path.is_empty() {
                path.push('.');
            }
            path.push_str(n);
            let r = first_opaque_path(x, path);
            path.truncate(len);
            r
        }),
        _ => None,
    }
}

impl BinLedger {
    fn record(&mut self, class: Class, ty: &str, name: &str, detail: Option<String>) {
        *self.tally.entry(class).or_default() += 1;
        if class == Class::Bug
            && let Some(d) = &detail
        {
            let field = d.split(" :: ").next().unwrap_or(d);
            *self.bug_paths.entry(format!("{ty}.{field}")).or_default() += 1;
        }
        let cap = if class == Class::Bug { 50 } else { 6 };
        if self.samples.iter().filter(|s| s.0 == class).count() < cap {
            self.samples
                .push((class, ty.into(), name.into(), detail.unwrap_or_default()));
        }
    }

    fn record_subdef(&mut self, class: Class, tag: &str, parent: &str, detail: Option<String>) {
        *self.subdef_tally.entry(class).or_default() += 1;
        if class == Class::Bug || class == Class::AcceptArtifact || class == Class::AcceptSort {
            *self
                .subdef_class_tags
                .entry(class)
                .or_default()
                .entry(tag.to_string())
                .or_default() += 1;
        }
        if class == Class::AcceptArtifact
            && let Some(ref d) = detail
        {
            let field = d.split(" :: ").next().unwrap_or(d);
            *self
                .subdef_artifact_paths
                .entry(format!("{tag}.{field}"))
                .or_default() += 1;
        }
        let cap = if class == Class::Bug { 100000 } else { 4 };
        if self.subdef_samples.iter().filter(|s| s.0 == class).count() < cap {
            self.subdef_samples.push((
                class,
                tag.into(),
                parent.into(),
                detail.unwrap_or_default(),
            ));
        }
    }

    fn count(&self, class: Class) -> usize {
        self.tally.get(&class).copied().unwrap_or(0)
    }
    fn subdef_count(&self, class: Class) -> usize {
        self.subdef_tally.get(&class).copied().unwrap_or(0)
    }

    fn render(&self, title: &str) -> String {
        use std::fmt::Write;
        let mut s = String::new();
        let _ = writeln!(s, "=== VERIFY LEDGER ({title}) ===");
        let order = [
            Class::Reproduced,
            Class::AcceptSort,
            Class::OpaqueOnly,
            Class::AcceptArtifact,
            Class::Bug,
            Class::NonGame,
            Class::Missing,
        ];
        let total: usize = self.tally.values().sum();
        for c in order {
            let _ = writeln!(s, "  {:<16} {}", format!("{c:?}"), self.count(c));
        }
        let _ = writeln!(s, "  {:<16} {}", "TOTAL", total);
        if !self.bug_paths.is_empty() {
            let _ = writeln!(s, "  -- BUG diff paths (field -> count):");
            let mut v: Vec<_> = self.bug_paths.iter().collect();
            v.sort_by_key(|(_, n)| std::cmp::Reverse(**n));
            for (path, n) in v.iter().take(30) {
                let _ = writeln!(s, "       {n:4}  {path}");
            }
        }
        if !self.opaque_paths.is_empty() {
            let _ = writeln!(
                s,
                "  -- OpaqueOnly blind spots (type.field the differ can't see -> count):"
            );
            let mut v: Vec<_> = self.opaque_paths.iter().collect();
            v.sort_by_key(|(_, n)| std::cmp::Reverse(**n));
            for (path, n) in v.iter().take(30) {
                let _ = writeln!(s, "       {n:5}  {path}");
            }
        }
        let bug_samples: Vec<_> = self.samples.iter().filter(|s| s.0 == Class::Bug).collect();
        if !bug_samples.is_empty() {
            let _ = writeln!(s, "  -- BUG samples:");
            for (_c, ty, name, detail) in bug_samples.iter().take(20) {
                let _ = writeln!(s, "     {ty} {name} :: {detail}");
            }
        }
        let sub_total: usize = self.subdef_tally.values().sum();
        if sub_total > 0 {
            let _ = writeln!(s, "  -- SUB-DEFS (by parent+tag): ");
            for c in [
                Class::Reproduced,
                Class::AcceptSort,
                Class::OpaqueOnly,
                Class::AcceptArtifact,
                Class::Bug,
            ] {
                let _ = writeln!(s, "     {:<14} {}", format!("{c:?}"), self.subdef_count(c));
            }
            let _ = writeln!(
                s,
                "     (of which count-mismatch: {})",
                self.subdef_count_mismatch
            );
            if !self.subdef_bug_tags.is_empty() {
                let mut v: Vec<_> = self.subdef_bug_tags.iter().collect();
                v.sort_by_key(|(_, n)| std::cmp::Reverse(**n));
                let _ = writeln!(s, "     -- sub-def Bug tags:");
                for (tag, n) in v.iter().take(20) {
                    let _ = writeln!(s, "        {n:4}  {tag}");
                }
            }
            for &cc in [Class::AcceptArtifact, Class::AcceptSort].iter() {
                if let Some(tags) = self.subdef_class_tags.get(&cc) {
                    let mut v: Vec<_> = tags.iter().collect();
                    v.sort_by_key(|(_, n)| std::cmp::Reverse(**n));
                    let _ = writeln!(s, "     -- sub-def {cc:?} tags:");
                    for (tag, n) in v.iter().take(20) {
                        let _ = writeln!(s, "        {n:4}  {tag}");
                    }
                }
            }
            if !self.subdef_artifact_paths.is_empty() {
                let mut v: Vec<_> = self.subdef_artifact_paths.iter().collect();
                v.sort_by_key(|(_, n)| std::cmp::Reverse(**n));
                let _ = writeln!(s, "     -- sub-def AcceptArtifact paths (field -> count):");
                for (path, n) in v.iter().take(20) {
                    let _ = writeln!(s, "        {n:4}  {path}");
                }
            }
            for &cc in [Class::AcceptArtifact, Class::AcceptSort].iter() {
                let aa_samples: Vec<_> = self.subdef_samples.iter().filter(|s| s.0 == cc).collect();
                if !aa_samples.is_empty() {
                    let _ = writeln!(s, "     -- sub-def {cc:?} samples:");
                    for (_c, tag, parent, detail) in aa_samples.iter().take(10) {
                        let _ = writeln!(s, "        {parent} <{tag}> :: {detail}");
                    }
                }
            }
            let sub_bugs: Vec<_> = self
                .subdef_samples
                .iter()
                .filter(|s| s.0 == Class::Bug)
                .collect();
            if !sub_bugs.is_empty() {
                let mut hist: std::collections::BTreeMap<String, (usize, String)> = Default::default();
                for (_c, tag, parent, detail) in sub_bugs.iter() {
                    let path = detail.split(" :: ").next().unwrap_or("").to_string();
                    let key = format!("<{tag}> {}", path.split('[').next().unwrap_or(""));
                    let e = hist.entry(key).or_insert((0, format!("{parent}: {detail}")));
                    e.0 += 1;
                }
                let mut v: Vec<_> = hist.into_iter().collect();
                v.sort_by_key(|(_, (n, _))| std::cmp::Reverse(*n));
                let _ = writeln!(s, "     -- sub-def Bug HISTOGRAM:");
                for (k, (n, ex)) in v {
                    let _ = writeln!(s, "        {n:4} {k}\n              e.g. {ex}");
                }
                let _ = writeln!(s, "     -- sub-def Bug samples:");
                for (_c, tag, parent, detail) in sub_bugs.iter() {
                    let _ = writeln!(s, "        {parent} <{tag}> :: {detail}");
                }
            }
        }
        s
    }
}

fn resolver_maps(bin: &DefBinary, names: &Names) -> (HashMap<i32, String>, HashMap<i32, String>) {
    let mut idx_to_name = HashMap::new();
    for e in bin.entries(names) {
        if let Some(fname) = e.file_name
            && !fname.starts_with("NULLDEF_")
        {
            idx_to_name.insert(e.global_index as i32, fname.to_string());
        }
    }
    let off_to_str: HashMap<i32, String> = names
        .map
        .iter()
        .map(|(off, entry)| (*off as i32, entry.string.clone()))
        .collect();
    (idx_to_name, off_to_str)
}

fn verify_binary(
    ours: &DefBinary,
    ours_names: &Names,
    retail: &DefBinary,
    retail_names: &Names,
) -> BinLedger {
    let (ours_idx, ours_off) = resolver_maps(ours, ours_names);
    let (retail_idx, retail_off) = resolver_maps(retail, retail_names);
    let ours_res = Resolvers {
        def_index_name: &|i| ours_idx.get(&i).cloned(),
        def_string_value: &|o| ours_off.get(&o).cloned(),
    };
    let retail_res = Resolvers {
        def_index_name: &|i| retail_idx.get(&i).cloned(),
        def_string_value: &|o| retail_off.get(&o).cloned(),
    };

    let retail_by_name: HashMap<&str, (&str, &DefBody)> = retail
        .entries(retail_names)
        .filter_map(|e| match e.file_name {
            Some(f) if !f.starts_with("NULLDEF_") => {
                Some((f, (e.def_name.unwrap_or("?"), &e.record.body)))
            }
            _ => None,
        })
        .collect();
    let ours_names_set: std::collections::HashSet<&str> = ours
        .entries(ours_names)
        .filter_map(|e| e.file_name.filter(|f| !f.starts_with("NULLDEF_")))
        .collect();

    let mut ledger = BinLedger::default();
    for (&fname, &(ty, _)) in &retail_by_name {
        if !ours_names_set.contains(fname) {
            ledger.record(Class::Missing, ty, fname, None);
        }
    }
    for e in ours.entries(ours_names) {
        let Some(fname) = e.file_name else { continue };
        if fname.starts_with("NULLDEF_") {
            continue;
        }
        let ty = e.def_name.unwrap_or("?");
        let Some(&(_, theirs)) = retail_by_name.get(fname) else {
            continue;
        };
        let (Some(mut a), Some(mut b)) = (Some(e.record.body.clone()), Some(theirs.clone())) else {
            ledger.record(Class::NonGame, ty, fname, None);
            continue;
        };
        let sa = game_body_to_semval(&mut a, &ours_res);
        let sb = game_body_to_semval(&mut b, &retail_res);
        let class = if sem_eq(&sa, &sb, DiffPolicy::strict()) {
            if sa.contains_opaque() {
                Class::OpaqueOnly
            } else {
                Class::Reproduced
            }
        } else if sem_eq(&sa, &sb, DiffPolicy::unordered()) {
            Class::AcceptSort
        } else {
            match first_diff(&sa, &sb, DiffPolicy::unordered()) {
                Some(d) if is_artifact_leaf(&d.path) => Class::AcceptArtifact,
                _ => Class::Bug,
            }
        };
        let detail = if class == Class::Bug {
            first_diff(&sa, &sb, DiffPolicy::unordered())
                .map(|d| format!("{} :: ours={} theirs={}", d.path, d.ours, d.theirs))
        } else {
            None
        };
        if class == Class::OpaqueOnly
            && let Some(p) = first_opaque_path(&sa, &mut String::new())
        {
            *ledger.opaque_paths.entry(format!("{ty}.{p}")).or_default() += 1;
        }
        ledger.record(class, ty, fname, detail);
    }
    verify_subdefs(
        ours,
        ours_names,
        retail,
        retail_names,
        &ours_res,
        &retail_res,
        &mut ledger,
    );
    ledger
}

fn index_bodies(bin: &DefBinary, names: &Names) -> HashMap<u32, (String, DefBody)> {
    bin.entries(names)
        .map(|e| {
            (
                e.global_index as u32,
                (e.def_name.unwrap_or("?").to_string(), e.record.body.clone()),
            )
        })
        .collect()
}

fn verify_subdefs(
    ours: &DefBinary,
    ours_names: &Names,
    retail: &DefBinary,
    retail_names: &Names,
    ours_res: &Resolvers,
    retail_res: &Resolvers,
    ledger: &mut BinLedger,
) {
    let ours_bodies = index_bodies(ours, ours_names);
    let retail_bodies = index_bodies(retail, retail_names);
    let retail_subs: HashMap<&str, &Vec<SubDefRecord>> = retail
        .entries(retail_names)
        .filter_map(|e| match (e.file_name, e.record.sub_defs.as_ref()) {
            (Some(f), Some(sd)) if !f.starts_with("NULLDEF_") && !sd.is_empty() => Some((f, sd)),
            _ => None,
        })
        .collect();

    let group = |sd: &[SubDefRecord],
                 bodies: &HashMap<u32, (String, DefBody)>,
                 res: &Resolvers|
     -> (HashMap<u32, Vec<SemVal>>, HashMap<u32, String>) {
        let mut by_crc: HashMap<u32, Vec<SemVal>> = HashMap::new();
        let mut label = HashMap::new();
        for rec in sd {
            if let Some((cls, body)) = bodies.get(&rec.def_index) {
                let mut b = body.clone();
                by_crc
                    .entry(rec.name_crc)
                    .or_default()
                    .push(game_body_to_semval(&mut b, res));
                label.entry(rec.name_crc).or_insert_with(|| cls.clone());
            }
        }
        (by_crc, label)
    };

    for e in ours.entries(ours_names) {
        let Some(fname) = e.file_name else { continue };
        if fname.starts_with("NULLDEF_") {
            continue;
        }
        let Some(ours_sd) = e.record.sub_defs.as_ref().filter(|s| !s.is_empty()) else {
            continue;
        };
        let Some(&retail_sd) = retail_subs.get(fname) else {
            continue;
        };
        let (og, ol) = group(ours_sd, &ours_bodies, ours_res);
        let (rg, rl) = group(retail_sd, &retail_bodies, retail_res);
        let crcs: std::collections::HashSet<u32> = og.keys().chain(rg.keys()).copied().collect();
        for crc in crcs {
            let mut o = og.get(&crc).cloned().unwrap_or_default();
            let mut r = rg.get(&crc).cloned().unwrap_or_default();
            let tag = ol
                .get(&crc)
                .or_else(|| rl.get(&crc))
                .cloned()
                .unwrap_or_default();
            if o.len() != r.len() {
                ledger.subdef_count_mismatch += 1;
                ledger.record_subdef(
                    Class::Bug,
                    &tag,
                    fname,
                    Some(format!("count ours={} theirs={}", o.len(), r.len())),
                );
                continue;
            }
            o.sort_by_cached_key(|s| format!("{s:?}"));
            r.sort_by_cached_key(|s| format!("{s:?}"));
            for (sa, sb) in o.iter().zip(&r) {
                let (class, detail) = classify_pair(sa, sb);
                ledger.record_subdef(class, &tag, fname, detail);
            }
        }
    }
}

fn classify_pair(sa: &SemVal, sb: &SemVal) -> (Class, Option<String>) {
    if sem_eq(sa, sb, DiffPolicy::strict()) {
        return (
            if sa.contains_opaque() {
                Class::OpaqueOnly
            } else {
                Class::Reproduced
            },
            None,
        );
    }
    if sem_eq(sa, sb, DiffPolicy::unordered()) {
        return (Class::AcceptSort, None);
    }
    match first_diff(sa, sb, DiffPolicy::unordered()) {
        Some(d) if is_artifact_leaf(&d.path) => (Class::AcceptArtifact, Some(d.path)),
        Some(d) => (
            Class::Bug,
            Some(format!("{} :: ours={} theirs={}", d.path, d.ours, d.theirs)),
        ),
        None => (Class::Bug, None),
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Sub-def dump
// ═══════════════════════════════════════════════════════════════════════════════

fn pp(v: &SemVal, indent: usize, out: &mut String) {
    let pad = "  ".repeat(indent);
    match v {
        SemVal::List(xs) => {
            out.push_str("[\n");
            for (i, x) in xs.iter().enumerate() {
                out.push_str(&format!("{pad}  [{i}] "));
                pp(x, indent + 1, out);
                out.push('\n');
            }
            out.push_str(&format!("{pad}]"));
        }
        SemVal::Struct(xs) | SemVal::Variant(_, xs) => {
            out.push_str("{\n");
            for (name, x) in xs {
                out.push_str(&format!("{pad}  {name}: "));
                pp(x, indent + 1, out);
                out.push('\n');
            }
            out.push_str(&format!("{pad}}}"));
        }
        SemVal::Map(xs) => {
            out.push_str("map{\n");
            for (k, x) in xs {
                out.push_str(&format!("{pad}  "));
                pp(k, indent + 1, out);
                out.push_str(" => ");
                pp(x, indent + 1, out);
                out.push('\n');
            }
            out.push_str(&format!("{pad}}}"));
        }
        other => out.push_str(&format!("{other:?}")),
    }
}

fn dump_subdef(
    _dir: &str,
    _bin: &str,
    parent: &str,
    tag_crc: Option<u32>,
    names: &Names,
    b: &DefBinary,
) -> Vec<(u32, String)> {
    let (idx, off) = resolver_maps(b, names);
    let res = Resolvers {
        def_index_name: &|i| idx.get(&i).cloned(),
        def_string_value: &|o| off.get(&o).cloned(),
    };
    let bodies = index_bodies(b, names);
    let mut result = Vec::new();
    for e in b.entries(names) {
        if e.file_name != Some(parent) {
            continue;
        }
        let Some(sd) = e.record.sub_defs.as_ref() else {
            continue;
        };
        for rec in sd.iter() {
            if let Some(tc) = tag_crc
                && tc != rec.name_crc
            {
                continue;
            }
            if let Some(body) = bodies.get(&rec.def_index) {
                let mut bd = body.1.clone();
                let sv = game_body_to_semval(&mut bd, &res);
                let mut s = String::new();
                pp(&sv, 0, &mut s);
                result.push((rec.name_crc, s));
            }
        }
    }
    result
}

// ═══════════════════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════════════════

fn load(dir: &str, bin: &str) -> (DefBinary, Names) {
    let names = Names::load(&Path::new(dir).join("names.bin")).unwrap();
    let b = defs::wire::with_lenient_control_ids(|| {
        DefBinary::load_with_names(&Path::new(dir).join(format!("{bin}.bin")), &names)
    }).unwrap();
    (b, names)
}

fn run_ledger(ours_dir: &str, retail_dir: &str) {
    let (on, rn) = (
        Names::load(&Path::new(ours_dir).join("names.bin")),
        Names::load(&Path::new(retail_dir).join("names.bin")),
    );
    let (Ok(on), Ok(rn)) = (on, rn) else {
        eprintln!("cannot load names.bin");
        return;
    };
    for (file, title) in [
        ("game", "game"),
        ("frontend", "frontend"),
        ("script", "script"),
    ] {
        let (ob, rb) = (load(ours_dir, file), load(retail_dir, file));
        let ledger = verify_binary(&ob.0, &on, &rb.0, &rn);
        eprint!("{}", ledger.render(title));
    }
}

fn run_dump_subdef(ours_dir: &str, retail_dir: &str, parent: &str, tag: Option<&str>) {
    let tag_crc = tag.map(|t| crc(t.as_bytes()));
    for dir in [ours_dir, retail_dir] {
        let (_on, _) = (Names::load(&Path::new(dir).join("names.bin")).unwrap(), ());
        for bin in ["game", "frontend", "script"] {
            let (b, names) = load(dir, bin);
            let d = dump_subdef(dir, bin, parent, tag_crc, &names, &b);
            if d.is_empty() {
                continue;
            }
            println!("=== {dir}/{bin}.bin: {} sub-defs ===", d.len());
            for (crc, s) in &d {
                println!("-- tag crc {crc:#010x} --\n{s}\n");
            }
        }
    }
}

/// Sub-def bodies of `parent` (optionally filtered to one tag), as reference-
/// resolved SemVals, from one build directory.
fn subdef_semvals(
    parent: &str,
    tag_crc: Option<u32>,
    names: &Names,
    b: &DefBinary,
) -> Vec<(u32, SemVal)> {
    let (idx, off) = resolver_maps(b, names);
    let res = Resolvers {
        def_index_name: &|i| idx.get(&i).cloned(),
        def_string_value: &|o| off.get(&o).cloned(),
    };
    let bodies = index_bodies(b, names);
    let mut out = Vec::new();
    for e in b.entries(names) {
        if e.file_name != Some(parent) {
            continue;
        }
        let Some(sd) = e.record.sub_defs.as_ref() else {
            continue;
        };
        for rec in sd.iter() {
            if let Some(tc) = tag_crc
                && tc != rec.name_crc
            {
                continue;
            }
            if let Some(body) = bodies.get(&rec.def_index) {
                let mut bd = body.1.clone();
                out.push((rec.name_crc, game_body_to_semval(&mut bd, &res)));
            }
        }
    }
    out
}

/// Multiset leftovers: elements of `a` with no exact (unordered) match in `b`,
/// and vice versa. The core of a "what actually differs" comparison for the
/// duplicate-key / reordered lists where positional diffs are pure noise.
fn multiset_leftovers<'a>(a: &'a [SemVal], b: &'a [SemVal]) -> (Vec<&'a SemVal>, Vec<&'a SemVal>) {
    let policy = DiffPolicy::unordered();
    let mut used_b = vec![false; b.len()];
    let mut only_a = Vec::new();
    for x in a {
        match b
            .iter()
            .enumerate()
            .position(|(j, y)| !used_b[j] && sem_eq(x, y, policy))
        {
            Some(j) => used_b[j] = true,
            None => only_a.push(x),
        }
    }
    let only_b = b
        .iter()
        .enumerate()
        .filter(|(j, _)| !used_b[*j])
        .map(|(_, y)| y)
        .collect();
    (only_a, only_b)
}

/// Recurse structs/variants to the first diverging container, then print every
/// ours-only and theirs-only element in full — the clear way to see a
/// duplicate-key/reordered list's real content difference.
fn print_setdiff(sa: &SemVal, sb: &SemVal, path: &str) {
    let policy = DiffPolicy::unordered();
    match (sa, sb) {
        (SemVal::Struct(xa), SemVal::Struct(xb))
        | (SemVal::Variant(_, xa), SemVal::Variant(_, xb))
            if xa.len() == xb.len() =>
        {
            for ((na, va), (_, vb)) in xa.iter().zip(xb) {
                if !sem_eq(va, vb, policy) {
                    print_setdiff(va, vb, &format!("{path}.{na}"));
                }
            }
        }
        (SemVal::List(xa), SemVal::List(xb)) => {
            let (only_a, only_b) = multiset_leftovers(xa, xb);
            println!(
                "=== {path}: len ours={} theirs={} | {} ours-only, {} theirs-only ===",
                xa.len(),
                xb.len(),
                only_a.len(),
                only_b.len()
            );
            for x in &only_a {
                let mut s = String::new();
                pp(x, 0, &mut s);
                println!("  [OURS-ONLY] {s}");
            }
            for y in &only_b {
                let mut s = String::new();
                pp(y, 0, &mut s);
                println!("  [THEIRS-ONLY] {s}");
            }
        }
        (SemVal::Map(xa), SemVal::Map(xb)) => {
            let pair = |(k, v): &(SemVal, SemVal)| {
                SemVal::Struct(vec![("key", k.clone()), ("value", v.clone())])
            };
            let a: Vec<SemVal> = xa.iter().map(pair).collect();
            let bb: Vec<SemVal> = xb.iter().map(pair).collect();
            let (only_a, only_b) = multiset_leftovers(&a, &bb);
            println!(
                "=== {path}: len ours={} theirs={} | {} ours-only, {} theirs-only ===",
                xa.len(),
                xb.len(),
                only_a.len(),
                only_b.len()
            );
            for x in &only_a {
                let mut s = String::new();
                pp(x, 0, &mut s);
                println!("  [OURS-ONLY] {s}");
            }
            for y in &only_b {
                let mut s = String::new();
                pp(y, 0, &mut s);
                println!("  [THEIRS-ONLY] {s}");
            }
        }
        _ => println!("=== {path}: {} vs {} ===", short(sa), short(sb)),
    }
}

fn run_setdiff(ours_dir: &str, retail_dir: &str, parent: &str, tag: Option<&str>) {
    let tag_crc = tag.map(|t| crc(t.as_bytes()));
    for bin in ["game", "frontend", "script"] {
        let (ob, on) = load(ours_dir, bin);
        let (rb, rn) = load(retail_dir, bin);
        let ours = subdef_semvals(parent, tag_crc, &on, &ob);
        let retail = subdef_semvals(parent, tag_crc, &rn, &rb);
        if ours.is_empty() && retail.is_empty() {
            continue;
        }
        // Pair by tag crc in order of appearance.
        for ((crc_o, sa), (_, sb)) in ours.iter().zip(&retail) {
            if sem_eq(sa, sb, DiffPolicy::unordered()) {
                continue;
            }
            println!("### {parent} <tag crc {crc_o:#010x}> ({bin}.bin) ###");
            print_setdiff(sa, sb, "body");
        }
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 3 {
        eprintln!(
            "usage: verify <ours_dir> <retail_dir> [--dump-subdef|--setdiff <Parent> [<Tag>]]"
        );
        std::process::exit(2);
    }
    let (ours_dir, retail_dir) = (&args[1], &args[2]);
    match args.get(3).map(|s| s.as_str()) {
        Some("--dump-subdef") => {
            let parent = args.get(4).expect("--dump-subdef requires <Parent>");
            run_dump_subdef(
                ours_dir,
                retail_dir,
                parent,
                args.get(5).map(|s| s.as_str()),
            );
        }
        Some("--setdiff") => {
            let parent = args.get(4).expect("--setdiff requires <Parent>");
            run_setdiff(
                ours_dir,
                retail_dir,
                parent,
                args.get(5).map(|s| s.as_str()),
            );
        }
        _ => run_ledger(ours_dir, retail_dir),
    }
}
