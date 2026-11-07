//! Generic un-lowering: typed `DefBody` -> text AST statements / `Definition`.

use std::collections::{HashMap, HashSet};
use std::path::{Path, PathBuf};

use defs::binary::DefBody;
use defs::def::binary::EntryRecord;
use defs::def::visit::{FieldRef, FieldVisitor, StructSlot, VariantSlot};
use defs::text::{
    Call, Definition, EnumDecl, Expr, FileId, Item, PathSegment, PropertyPath, Span, Spanned,
    Statement, SymbolTable, parse_source,
};

fn syn<T>(value: T) -> Spanned<T> {
    Spanned {
        span: Span::SYNTHETIC,
        value,
    }
}

/// The following field names are declared as `DefIndex` in `defs` schema but hold
/// plain integer/text-id values (NOT def references).
pub const SCALAR_DEF_INDEX_FIELDS: &[&str] = &[
    "CardVal",
    "Mult",
    "Name",
    "InitialHawkingPhrase",
    "ReplyToRequestPhrase",
    "CustomerFollowPhrase",
    "ThankingPhrase",
    "MinGoldForDonation",
    "MaxGoldForDonation",
    "NumDaysToKeepSpecialItem",
    "MinAmountBeforeSpecialItem",
    "BedCost",
    "PointerParticleEffect",
    "Courses",
    "StartingCourse",
    "Rounds",
    "RenownGainedPerWitnessAhead",
    "EmoteIconForAvailableWitness",
    "SecondsBetweenEffectUsesAllowed",
    "Property",
    "TargetingRange",
];

/// Whether a `DefIndex` field holds a scalar value rather than a global def index.
pub fn is_scalar_def_index(name: &str) -> bool {
    SCALAR_DEF_INDEX_FIELDS
        .iter()
        .any(|&f| f.eq_ignore_ascii_case(name))
}

/// Reference-resolving semantic value tree for comparison.
#[derive(Debug, Clone, PartialEq)]
pub enum SemVal {
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

/// Known symbol domains from `RetailHeaders/` and root `.h` files.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum SymDomain {
    Anim,
    Mesh,
    Texture,
    Text,
    Particle,
    Sound,
    LipSync,
    Font,
    EngMesh,
    AnimFlags,
    Handedness,
    ExpressionType,
    BodyLocation,
    BodyPart,
    HeroMorph,
    MeshLocation,
    ObjectAugmentation,
    EngineGraphicType,
    Opinion,
    AttackHistoryType,
    DamageAttribute,
    Blend,
    EnumGroup(usize),
}

#[derive(Debug, Clone, Default)]
pub struct EnumGroupTable {
    pub val_to_sym: HashMap<i32, String>,
    pub bit_flags: Vec<(i32, String)>,
}

impl EnumGroupTable {
    pub fn insert(&mut self, val: i32, name: String) {
        match self.val_to_sym.get(&val) {
            None => {
                self.val_to_sym.insert(val, name.clone());
            }
            Some(existing) => {
                if is_sentinel_symbol(existing) && !is_sentinel_symbol(&name) {
                    self.val_to_sym.insert(val, name.clone());
                }
            }
        }
        if val > 0 && (val & (val - 1)) == 0 && !is_sentinel_symbol(&name) {
            if let Some(pos) = self.bit_flags.iter().position(|(v, _)| *v == val) {
                if is_sentinel_symbol(&self.bit_flags[pos].1) {
                    self.bit_flags[pos].1 = name;
                }
            } else {
                self.bit_flags.push((val, name));
            }
        }
    }

    pub fn resolve(&self, val: i32, as_bitflags: bool) -> Option<Expr> {
        if as_bitflags {
            if val == 0 {
                return self
                    .val_to_sym
                    .get(&0)
                    .map(|s| Expr::Symbol(s.clone()))
                    .or_else(|| Some(Expr::Number("0".to_string())));
            }
            if val > 0 && !self.bit_flags.is_empty() {
                let mut rem = val;
                let mut terms = Vec::new();
                for (bit, sym) in &self.bit_flags {
                    if *bit > 0 && (rem & *bit) == *bit {
                        rem &= !*bit;
                        terms.push(syn(Expr::Symbol(sym.clone())));
                    }
                }
                if rem == 0 && !terms.is_empty() {
                    if terms.len() == 1 {
                        return Some(terms.into_iter().next().unwrap().value);
                    } else {
                        return Some(Expr::BitOr(terms));
                    }
                }
            }
        }
        self.val_to_sym.get(&val).map(|s| Expr::Symbol(s.clone()))
    }
}

fn is_sentinel_symbol(name: &str) -> bool {
    name.starts_with("MAX_")
        || name.starts_with("NUM_")
        || name.starts_with("FIRST_")
        || name.starts_with("LAST_")
        || name.ends_with("_MAX")
        || name.ends_with("_LAST")
        || name.ends_with("_FIRST")
        || name.ends_with("_COUNT")
        || name.ends_with("_END")
        || name.ends_with("_TOTAL")
        || name.starts_with("DUMMY_")
}

/// Resolves integer IDs back to their symbolic names from `RetailHeaders/` and root `.h` files.
#[derive(Debug, Clone, Default)]
pub struct HeaderResolver {
    pub domains: HashMap<SymDomain, EnumGroupTable>,
    pub enum_groups: Vec<EnumGroupTable>,
    pub field_domains: HashMap<(String, String), (SymDomain, bool)>,
}

fn normalize_path(path: &Path) -> String {
    path.to_string_lossy().replace('\\', "/")
}

const HEADER_SET_ROOTS: &[&str] = &["RetailHeaders", "DevHeaders"];
const IGNORED_PLATFORM_DIRS: &[&str] = &["/xbox/"];

fn collect_retail_h_files(
    dir: &Path,
    root: &Path,
    selected_set: Option<&str>,
    out: &mut Vec<PathBuf>,
) {
    let Ok(entries) = std::fs::read_dir(dir) else {
        return;
    };
    let root_prefix = normalize_path(root);
    for entry in entries.filter_map(|e| e.ok()) {
        let path = entry.path();
        if path.is_dir() {
            collect_retail_h_files(&path, root, selected_set, out);
        } else if matches!(path.extension().and_then(|e| e.to_str()), Some("h")) {
            let full = normalize_path(&path);
            let rel = match full.strip_prefix(&root_prefix) {
                Some(r) => format!("/{}", r.trim_start_matches('/')),
                None => full.clone(),
            };
            if IGNORED_PLATFORM_DIRS.iter().any(|d| rel.contains(d))
                || rel.contains("scriptdialoguesnds2")
            {
                continue;
            }
            let losing_set = HEADER_SET_ROOTS
                .iter()
                .filter(|r| Some(**r) != selected_set)
                .any(|r| rel.contains(&format!("/{r}/")));
            if losing_set {
                continue;
            }
            out.push(path);
        }
    }
}

impl HeaderResolver {
    pub fn empty() -> Self {
        Self::default()
    }

    /// Build a `HeaderResolver` from `target_defs_dir`, using `RetailHeaders/` (never `DevHeaders/` or `xbox/`)
    /// and shared root `.h` files, plus corpus-trained field/argument mappings.
    pub fn build(target_defs_dir: &Path) -> Self {
        let selected_set = HEADER_SET_ROOTS
            .iter()
            .copied()
            .find(|root| target_defs_dir.join(root).is_dir());

        let mut h_files = Vec::new();
        collect_retail_h_files(target_defs_dir, target_defs_dir, selected_set, &mut h_files);
        h_files.sort();

        let def_files: Vec<PathBuf> = def_compiler::walk_def_files(target_defs_dir)
            .into_iter()
            .filter(|p| {
                !p.file_name().and_then(|n| n.to_str()).is_some_and(|n| {
                    n.starts_with("z_mod_decompiled") || n.contains("_deprecated.")
                })
            })
            .collect();

        let mut symbols = SymbolTable::new();
        let mut parsed_headers: Vec<(String, Vec<Item>)> = Vec::new();
        let root_prefix = normalize_path(target_defs_dir);

        for path in &h_files {
            if let Ok(text) = std::fs::read_to_string(path) {
                if let Ok(ast) = parse_source(&text, FileId::ANONYMOUS) {
                    let _ = symbols.evaluate_items(&ast.items);
                    let full = normalize_path(path);
                    let rel = full
                        .strip_prefix(&root_prefix)
                        .unwrap_or(&full)
                        .trim_start_matches('/')
                        .to_string();
                    parsed_headers.push((rel, ast.items));
                }
            }
        }

        let mut parsed_defs: Vec<Vec<Item>> = Vec::new();
        for path in &def_files {
            if let Ok(raw) = std::fs::read(path) {
                let text = String::from_utf8_lossy(&raw);
                if let Ok(ast) = parse_source(&text, FileId::ANONYMOUS) {
                    let _ = symbols.evaluate_items(&ast.items);
                    parsed_defs.push(ast.items);
                }
            }
        }

        // Fallback engine enums
        for (name, val) in [
            ("WATER_BUMP_PC", 0i64),
            ("BLEND_NULL", 0),
            ("BLEND_ADDITIVE", 1),
            ("BLEND_ALPHA", 2),
            ("BLEND_SOLID", 3),
            ("BLEND_MULTIPLY", 4),
        ] {
            if symbols.lookup(name).is_none() {
                symbols.insert(name, val);
            }
        }

        let mut resolver = HeaderResolver::default();
        let mut sym_to_groups: HashMap<String, Vec<usize>> = HashMap::new();

        // Populate blend domain
        {
            let mut blend_tbl = EnumGroupTable::default();
            for (name, val) in [
                ("BLEND_NULL", 0),
                ("BLEND_ADDITIVE", 1),
                ("BLEND_ALPHA", 2),
                ("BLEND_SOLID", 3),
                ("BLEND_MULTIPLY", 4),
            ] {
                blend_tbl.insert(val, name.to_string());
            }
            resolver.domains.insert(SymDomain::Blend, blend_tbl);
        }

        for (rel, items) in &parsed_headers {
            Self::collect_enum_items(
                rel,
                items,
                &symbols,
                &mut resolver,
                &mut sym_to_groups,
            );
        }
        for items in &parsed_defs {
            Self::collect_enum_items(
                "defs",
                items,
                &symbols,
                &mut resolver,
                &mut sym_to_groups,
            );
        }

        // Train field -> (SymDomain, is_bitflags) from all .def/.tpl ASTs in target_defs_dir
        let mut votes: HashMap<(String, String), HashMap<usize, usize>> = HashMap::new();
        let mut bitor_seen: HashSet<(String, String)> = HashSet::new();

        for items in &parsed_defs {
            for item in items {
                if let Item::Definition(def) = item {
                    let def_type = &def.value.def_type;
                    for stmt in &def.value.body {
                        Self::train_from_statement(
                            def_type,
                            &stmt.value,
                            &sym_to_groups,
                            &mut votes,
                            &mut bitor_seen,
                        );
                    }
                }
            }
        }

        for ((scope, field), group_counts) in votes {
            if let Some((&best_gid, _)) = group_counts.iter().max_by_key(|(_, count)| *count) {
                let is_bf = bitor_seen.contains(&(scope.clone(), field.clone()))
                    || bitor_seen.contains(&("*".to_string(), field.clone()));
                resolver
                    .field_domains
                    .insert((scope, field), (SymDomain::EnumGroup(best_gid), is_bf));
            }
        }

        resolver
    }

    fn collect_enum_items(
        rel_path: &str,
        items: &[Item],
        symbols: &SymbolTable,
        resolver: &mut HeaderResolver,
        sym_to_groups: &mut HashMap<String, Vec<usize>>,
    ) {
        for item in items {
            match item {
                Item::Enum(decl) => {
                    Self::record_enum(rel_path, decl, symbols, resolver, sym_to_groups);
                }
                Item::Namespace(ns) => {
                    Self::collect_enum_items(
                        rel_path,
                        &ns.items,
                        symbols,
                        resolver,
                        sym_to_groups,
                    );
                }
                Item::Conditional(ifdef) => {
                    let taken = (ifdef.condition == "_WINDOWS") ^ ifdef.inverted;
                    let branch = if taken {
                        &ifdef.if_branch
                    } else {
                        ifdef.else_branch.as_deref().unwrap_or(&[])
                    };
                    Self::collect_enum_items(rel_path, branch, symbols, resolver, sym_to_groups);
                }
                _ => {}
            }
        }
    }

    fn record_enum(
        rel_path: &str,
        decl: &EnumDecl,
        symbols: &SymbolTable,
        resolver: &mut HeaderResolver,
        sym_to_groups: &mut HashMap<String, Vec<usize>>,
    ) {
        let enum_name = decl
            .name
            .as_ref()
            .map(|s| s.value.as_str())
            .unwrap_or("");
        let mut group = EnumGroupTable::default();
        let gid = resolver.enum_groups.len();

        let rel_lower = rel_path.to_ascii_lowercase();
        let named_domain = match (rel_lower.as_str(), enum_name) {
            (p, "EAnimType2") if p.ends_with("meshdata.h") => Some(SymDomain::Anim),
            (p, "EMeshType2") if p.ends_with("meshdata.h") => Some(SymDomain::Mesh),
            (p, "EEngineGraphic" | "EGuiGraphicBank")
                if p.ends_with("textures.h") || p.ends_with("gui_bank.h") =>
            {
                Some(SymDomain::Texture)
            }
            (p, "EGameText" | "ETextGroup") if p.ends_with("text.h") => Some(SymDomain::Text),
            (p, "EParticleEmitter" | "EFrontEndParticleEmitter" | "EEngineGeneratedEffect")
                if p.ends_with("particles.h")
                    || p.ends_with("particles_frontend.h")
                    || p.ends_with("generated_effect.h") =>
            {
                Some(SymDomain::Particle)
            }
            (p, _)
                if p.ends_with("gamesnds.h")
                    || p.ends_with("dialoguesnds.h")
                    || p.ends_with("dialoguesnds2.h")
                    || p.ends_with("scriptdialoguesnds.h") =>
            {
                Some(SymDomain::Sound)
            }
            (p, _) if p.contains("lipsync") => Some(SymDomain::LipSync),
            (p, "EFont") if p.ends_with("fonts.h") => Some(SymDomain::Font),
            (p, "EEngineMeshType") if p.ends_with("engmesh.h") => Some(SymDomain::EngMesh),
            (_, "EAnimationFlags") => Some(SymDomain::AnimFlags),
            (_, "ECombatAnimationHandedness") => Some(SymDomain::Handedness),
            (_, "EExpressionType") => Some(SymDomain::ExpressionType),
            (_, "EBodyLocation") => Some(SymDomain::BodyLocation),
            (_, "EBodyPartType") => Some(SymDomain::BodyPart),
            (_, "EHeroMorphType") => Some(SymDomain::HeroMorph),
            (_, "EMeshLocationPoint") => Some(SymDomain::MeshLocation),
            (_, "EObjectAugmentationType") => Some(SymDomain::ObjectAugmentation),
            (_, "EEngineGraphicType") => Some(SymDomain::EngineGraphicType),
            (_, "EOpinion") => Some(SymDomain::Opinion),
            (_, "EAttackHistoryType") => Some(SymDomain::AttackHistoryType),
            (_, "EDamageAttribute") => Some(SymDomain::DamageAttribute),
            _ => None,
        };

        for variant in &decl.variants {
            let name = &variant.name.value;
            if !is_valid_ident(name) {
                continue;
            }
            let Some(val64) = symbols.lookup(name) else {
                continue;
            };
            let val = val64 as i32;
            group.insert(val, name.clone());
            sym_to_groups.entry(name.clone()).or_default().push(gid);

            if let Some(dom) = named_domain {
                resolver
                    .domains
                    .entry(dom)
                    .or_default()
                    .insert(val, name.clone());
            }
        }

        resolver.enum_groups.push(group);
    }

    fn collect_expr_symbols<'a>(expr: &'a Expr, out: &mut Vec<&'a str>, is_bitor: &mut bool) {
        match expr {
            Expr::Symbol(s) => {
                if !matches!(s.as_str(), "NULL" | "TRUE" | "FALSE" | "BTRUE" | "BFALSE") {
                    out.push(s.as_str());
                }
            }
            Expr::BitOr(terms) => {
                *is_bitor = true;
                for t in terms {
                    Self::collect_expr_symbols(&t.value, out, is_bitor);
                }
            }
            _ => {}
        }
    }

    fn record_symbol_votes(
        scope: &str,
        key: &str,
        expr: &Expr,
        sym_to_groups: &HashMap<String, Vec<usize>>,
        votes: &mut HashMap<(String, String), HashMap<usize, usize>>,
        bitor_seen: &mut HashSet<(String, String)>,
    ) {
        let mut syms = Vec::new();
        let mut is_bitor = false;
        Self::collect_expr_symbols(expr, &mut syms, &mut is_bitor);
        if syms.is_empty() {
            return;
        }
        if is_bitor {
            bitor_seen.insert((scope.to_string(), key.to_string()));
            bitor_seen.insert(("*".to_string(), key.to_string()));
        }
        for s in syms {
            if let Some(gids) = sym_to_groups.get(s) {
                for &gid in gids {
                    *votes
                        .entry((scope.to_string(), key.to_string()))
                        .or_default()
                        .entry(gid)
                        .or_insert(0) += 1;
                    *votes
                        .entry(("*".to_string(), key.to_string()))
                        .or_default()
                        .entry(gid)
                        .or_insert(0) += 1;
                }
            }
        }
    }

    fn train_from_statement(
        scope: &str,
        stmt: &Statement,
        sym_to_groups: &HashMap<String, Vec<usize>>,
        votes: &mut HashMap<(String, String), HashMap<usize, usize>>,
        bitor_seen: &mut HashSet<(String, String)>,
    ) {
        match stmt {
            Statement::Field(f) => {
                let mut last_field: Option<&str> = None;
                for seg in &f.path.segments {
                    match seg {
                        PathSegment::Field(name) => {
                            last_field = Some(name.as_str());
                        }
                        PathSegment::Index(idx) => {
                            if let Some(lf) = last_field {
                                let k = format!("{lf}[key]");
                                Self::record_symbol_votes(
                                    scope,
                                    &k,
                                    &idx.value,
                                    sym_to_groups,
                                    votes,
                                    bitor_seen,
                                );
                            }
                        }
                    }
                }
                if let Some(lf) = last_field {
                    Self::record_symbol_votes(
                        scope,
                        lf,
                        &f.expr.value,
                        sym_to_groups,
                        votes,
                        bitor_seen,
                    );
                }
            }
            Statement::MethodCall(mc) => {
                if let Some(PathSegment::Field(obj_name)) = mc.object.segments.last() {
                    if mc.call.name == "Add" && mc.call.arguments.len() == 1 {
                        Self::record_symbol_votes(
                            scope,
                            obj_name,
                            &mc.call.arguments[0].value,
                            sym_to_groups,
                            votes,
                            bitor_seen,
                        );
                    }
                }
            }
            Statement::TaggedBlock(tb) => {
                for s in &tb.body {
                    Self::train_from_statement(
                        &tb.tag,
                        &s.value,
                        sym_to_groups,
                        votes,
                        bitor_seen,
                    );
                }
            }
        }
    }

    pub fn resolve_domain(&self, domain: SymDomain, val: i32) -> Expr {
        let as_bitflags = matches!(
            domain,
            SymDomain::AnimFlags | SymDomain::ObjectAugmentation
        );
        match domain {
            SymDomain::EnumGroup(gid) => {
                if let Some(tbl) = self.enum_groups.get(gid) {
                    if let Some(expr) = tbl.resolve(val, false) {
                        return expr;
                    }
                }
                Expr::Number(val.to_string())
            }
            other => {
                if let Some(tbl) = self.domains.get(&other) {
                    if let Some(expr) = tbl.resolve(val, as_bitflags) {
                        return expr;
                    }
                }
                Expr::Number(val.to_string())
            }
        }
    }

    pub fn resolve_field(&self, scope: &str, field_name: &str, val: i32) -> Expr {
        if let Some(&(dom, is_bf)) = self
            .field_domains
            .get(&(scope.to_string(), field_name.to_string()))
            .or_else(|| {
                self.field_domains
                    .get(&("*".to_string(), field_name.to_string()))
            })
        {
            match dom {
                SymDomain::EnumGroup(gid) => {
                    if let Some(tbl) = self.enum_groups.get(gid) {
                        if let Some(expr) = tbl.resolve(val, is_bf) {
                            return expr;
                        }
                    }
                }
                other => {
                    if let Some(tbl) = self.domains.get(&other) {
                        if let Some(expr) = tbl.resolve(val, is_bf) {
                            return expr;
                        }
                    }
                }
            }
        }

        // Explicit field-name rules for asset bank IDs
        if field_name.eq_ignore_ascii_case("PhonemeAnim") {
            return self.resolve_domain(SymDomain::Anim, val);
        }
        if val > 0 {
            if matches!(
                field_name,
                "BodyParticleEffect"
                    | "DeadBodyDecayEffect"
                    | "StunnedParticle"
                    | "PointerParticleEffect"
            ) || field_name.ends_with("ParticleEffect")
                || field_name.ends_with("Particle")
                || field_name.ends_with("ParticleEmitter")
            {
                return self.resolve_domain(SymDomain::Particle, val);
            }
            if matches!(
                field_name,
                "ThankingPhrase"
                    | "IgnoringPhrase"
                    | "BeerRequestPhrase"
                    | "InitialHawkingPhrase"
                    | "ReplyToRequestPhrase"
                    | "CustomerFollowPhrase"
                    | "ItemName"
                    | "ItemDescription"
                    | "ItemDetails"
                    | "TitleText"
                    | "DescriptionText"
                    | "EmptySlotName"
            ) || field_name.ends_with("Phrase")
                || field_name.ends_with("TextID")
                || field_name.ends_with("TextTag")
            {
                return self.resolve_domain(SymDomain::Text, val);
            }
            if matches!(
                field_name,
                "PrimaryMesh" | "SecondaryMesh" | "MeshIndex" | "MeshID"
            ) {
                return self.resolve_domain(SymDomain::Mesh, val);
            }
            if field_name == "EmoteIconForAvailableWitness" {
                return self.resolve_domain(SymDomain::Texture, val);
            }
        }

        Expr::Number(val.to_string())
    }

    pub fn has_map_key_domain(&self, scope: &str, field_name: &str) -> bool {
        let key_lookup = format!("{field_name}[key]");
        self.field_domains
            .contains_key(&(scope.to_string(), key_lookup.clone()))
            || self
                .field_domains
                .contains_key(&("*".to_string(), key_lookup))
    }

    pub fn resolve_map_key(&self, scope: &str, field_name: &str, val: i32) -> Expr {
        let key_lookup = format!("{field_name}[key]");
        self.resolve_field(scope, &key_lookup, val)
    }
}

/// Context containing lookup tables needed to decompile and resolve def entries.
pub struct UnlowerCtx<'a> {
    pub idx_to_name: &'a HashMap<i32, String>,
    pub off_to_str: &'a HashMap<i32, String>,
    pub crc_to_str: &'a HashMap<u32, String>,
    pub nulldefs: &'a HashMap<String, DefBody>,
    pub all_entries: &'a HashMap<u32, (String, DefBody)>,
    pub headers: &'a HeaderResolver,
}

fn is_valid_ident(s: &str) -> bool {
    let mut chars = s.chars();
    match chars.next() {
        Some(c) if c.is_ascii_alphabetic() || c == '_' => {}
        _ => return false,
    }
    chars.all(|c| c.is_ascii_alphanumeric() || c == '_')
}

/// Convert a `FieldRef` into its `SemVal` for comparison against base.
pub fn field_to_semval(field: &mut FieldRef<'_>, name: &str, ctx: &UnlowerCtx) -> SemVal {
    match field {
        FieldRef::F32(x) => SemVal::F32(x.to_bits()),
        FieldRef::I32(x) => SemVal::Int(**x as i64),
        FieldRef::U32(x) => SemVal::Int(**x as i64),
        FieldRef::Bool(x) => SemVal::Bool(**x),
        FieldRef::Str(x) => SemVal::Str((*x).clone()),
        FieldRef::WStr(x) => SemVal::WStr(x.0.clone()),
        FieldRef::Enum(s) => SemVal::Int(s.get_i32() as i64),
        FieldRef::Flags(s) => SemVal::Int(s.get_i32() as i64),
        FieldRef::DefString(ds) => {
            let s = if ds.0 < 0 {
                String::new()
            } else if let Some(str_val) = ctx.off_to_str.get(&ds.0) {
                str_val.clone()
            } else {
                String::new()
            };
            SemVal::DefStr(s)
        }
        FieldRef::DefIndex(di) => {
            if is_scalar_def_index(name) {
                SemVal::Int(di.0 as i64)
            } else if di.0 == 0 {
                SemVal::DefRef("NULL".to_string())
            } else if let Some(n) = ctx.idx_to_name.get(&di.0) {
                SemVal::DefRef(n.clone())
            } else {
                SemVal::DefRef(format!("idx:{}", di.0))
            }
        }
        FieldRef::U8(x) => SemVal::Int(**x as i64),
        FieldRef::U16(x) => SemVal::Int(**x as i64),
        FieldRef::U64(x) => SemVal::Int(**x as i64),
        FieldRef::I8(x) => SemVal::Int(**x as i64),
        FieldRef::I16(x) => SemVal::Int(**x as i64),
        FieldRef::PString(p) => SemVal::PString(p.0.clone()),
        FieldRef::Vec(slot) => {
            let n = slot.len();
            let mut out = Vec::with_capacity(n);
            for i in 0..n {
                let mut el = slot.element(i);
                out.push(field_to_semval(&mut el, name, ctx));
            }
            SemVal::List(out)
        }
        FieldRef::Map(slot) => {
            let mut out = Vec::new();
            slot.for_each_pair(&mut |mut k, mut v| {
                out.push((
                    field_to_semval(&mut k, name, ctx),
                    field_to_semval(&mut v, name, ctx),
                ));
            });
            SemVal::Map(out)
        }
        FieldRef::Struct(slot) => {
            let tname = slot.type_name();
            let is_empty_aug_particle_set = if tname == "ObjectAugmentationParticleSet" {
                match slot.member(5) {
                    Some(FieldRef::Vec(v5)) => v5.len() == 0,
                    _ => false,
                }
            } else {
                false
            };
            let n = slot.member_count();
            let mut out = Vec::with_capacity(n);
            for i in 0..n {
                let mname = slot.member_name(i).unwrap_or("?");
                // Ignore uninitialized C++ struct tail/padding artifacts
                if matches!(mname, "trailing_u32" | "trailing" | "skip") {
                    continue;
                }
                if is_empty_aug_particle_set && (1..=4).contains(&i) {
                    out.push((mname, SemVal::Int(0)));
                    continue;
                }
                if let Some(mut m) = slot.member(i) {
                    out.push((mname, field_to_semval(&mut m, mname, ctx)));
                }
            }
            SemVal::Struct(out)
        }
        FieldRef::Variant(slot) => {
            let tag = slot.tag();
            let n = slot.member_count();
            let mut out = Vec::with_capacity(n);
            for i in 0..n {
                let mname = slot.member_name(i).unwrap_or("?");
                if let Some(mut m) = slot.member(i) {
                    out.push((mname, field_to_semval(&mut m, mname, ctx)));
                }
            }
            SemVal::Variant(tag, out)
        }
        FieldRef::Array(slot) => {
            let n = slot.len();
            let mut out = Vec::with_capacity(n);
            for i in 0..n {
                let mut el = slot.element(i);
                out.push(field_to_semval(&mut el, name, ctx));
            }
            SemVal::List(out)
        }
        FieldRef::Complex(_) => SemVal::Opaque(""),
    }
}

struct SemValCollector<'a, 'c> {
    ctx: &'a UnlowerCtx<'c>,
    out: Vec<(&'static str, SemVal)>,
}

impl FieldVisitor for SemValCollector<'_, '_> {
    fn field(&mut self, name: &'static str, mut field: FieldRef<'_>) {
        self.out.push((name, field_to_semval(&mut field, name, self.ctx)));
    }
}

/// Convert a whole `DefBody` into a semantic tree [`SemVal`].
pub fn body_to_semval(body: &mut DefBody, ctx: &UnlowerCtx) -> SemVal {
    let mut collector = SemValCollector {
        ctx,
        out: Vec::new(),
    };
    body.visit_active(&mut collector);
    SemVal::Struct(collector.out)
}

fn struct_to_ctor_expr(slot: &mut dyn StructSlot, scope: &str, ctx: &UnlowerCtx) -> Expr {
    let type_name = slot.type_name();
    match type_name {
        "RGBColour" => {
            if let Some(mut m0) = slot.member(0) {
                if let FieldRef::U32(packed) = &mut m0 {
                    let p = **packed;
                    let r = (p >> 16) & 0xff;
                    let g = (p >> 8) & 0xff;
                    let b = p & 0xff;
                    let a = (p >> 24) & 0xff;
                    let mut args = vec![
                        syn(Expr::Number(r.to_string())),
                        syn(Expr::Number(g.to_string())),
                        syn(Expr::Number(b.to_string())),
                    ];
                    if a != 255 {
                        args.push(syn(Expr::Number(a.to_string())));
                    }
                    return Expr::Constructor(Call {
                        name: "CRGBColour".to_string(),
                        arguments: args,
                    });
                }
            }
        }
        "BlendedParticleEffectSet" => {
            let m0 = slot
                .member(0)
                .and_then(|mut m| match &mut m {
                    FieldRef::I32(v) => Some(ctx.headers.resolve_domain(SymDomain::Particle, **v)),
                    _ => None,
                })
                .unwrap_or_else(|| Expr::Number("0".into()));
            let m2 = slot
                .member(2)
                .map(|mut m| field_to_expr_scoped(&mut m, scope, "m2", ctx))
                .unwrap_or_else(|| Expr::Number("0".into()));
            let m1 = slot
                .member(1)
                .and_then(|mut m| match &mut m {
                    FieldRef::I32(v) => Some(ctx.headers.resolve_domain(SymDomain::Particle, **v)),
                    _ => None,
                })
                .unwrap_or_else(|| Expr::Number("0".into()));
            let m3 = slot
                .member(3)
                .map(|mut m| field_to_expr_scoped(&mut m, scope, "m3", ctx))
                .unwrap_or_else(|| Expr::Number("0".into()));
            return Expr::Constructor(Call {
                name: "CBlendedParticleEffectSet".to_string(),
                arguments: vec![syn(m0), syn(m2), syn(m1), syn(m3)],
            });
        }
        "ObjectAugmentationParticleSet" => {
            let m0 = slot
                .member(0)
                .map(|mut m| field_to_expr_scoped(&mut m, scope, "m0", ctx))
                .unwrap_or_else(|| Expr::Number("0".into()));
            let pair_count = if let Some(FieldRef::Vec(v5)) = slot.member(5) {
                v5.len()
            } else {
                0
            };
            if pair_count == 0 {
                return Expr::Constructor(Call {
                    name: "CObjectAugmentationParticleSet".to_string(),
                    arguments: vec![syn(m0), syn(Expr::Number("0".into()))],
                });
            }
            let mut resolve_part = |idx: usize| -> Expr {
                slot.member(idx)
                    .map(|mut m| match &mut m {
                        FieldRef::I32(v) if **v > 0 => {
                            ctx.headers.resolve_domain(SymDomain::Particle, **v)
                        }
                        _ => field_to_expr_scoped(&mut m, scope, "part", ctx),
                    })
                    .unwrap_or_else(|| Expr::Number("0".into()))
            };
            let m1 = resolve_part(1);
            let m2 = resolve_part(2);
            let m3 = resolve_part(3);
            let m4 = resolve_part(4);
            let mut args = vec![
                syn(m0),
                syn(Expr::Number(pair_count.to_string())),
                syn(m1),
                syn(m2),
                syn(m3),
                syn(m4),
            ];
            for p in 0..pair_count {
                if let Some(FieldRef::Vec(v5)) = slot.member(5) {
                    let mut el5 = v5.element(p);
                    args.push(syn(field_to_expr_scoped(&mut el5, scope, "v5", ctx)));
                }
                if let Some(FieldRef::Vec(v6)) = slot.member(6) {
                    let mut el6 = v6.element(p);
                    args.push(syn(field_to_expr_scoped(&mut el6, scope, "v6", ctx)));
                }
            }
            return Expr::Constructor(Call {
                name: "CObjectAugmentationParticleSet".to_string(),
                arguments: args,
            });
        }
        "ExplosionRing" => {
            let m1 = slot
                .member(1)
                .map(|mut m| field_to_expr_scoped(&mut m, scope, "m1", ctx))
                .unwrap_or_else(|| Expr::Number("0".into()));
            let m0 = slot
                .member(0)
                .map(|mut m| field_to_expr_scoped(&mut m, scope, "m0", ctx))
                .unwrap_or_else(|| Expr::Number("0".into()));
            let m2 = slot
                .member(2)
                .map(|mut m| field_to_expr_scoped(&mut m, scope, "m2", ctx))
                .unwrap_or_else(|| Expr::Number("0".into()));
            let m3 = slot
                .member(3)
                .map(|mut m| field_to_expr_scoped(&mut m, scope, "m3", ctx))
                .unwrap_or_else(|| Expr::Number("0".into()));
            return Expr::Constructor(Call {
                name: "CExplosionRing".to_string(),
                arguments: vec![syn(m1), syn(m0), syn(m2), syn(m3)],
            });
        }
        "ParticleAttachmentInfo" => {
            let m1 = slot
                .member(1)
                .map(|mut m| field_to_expr_scoped(&mut m, scope, "m1", ctx))
                .unwrap_or_else(|| Expr::Number("0".into()));
            let m0 = slot
                .member(0)
                .map(|mut m| match &mut m {
                    FieldRef::I32(v) if **v > 0 => {
                        ctx.headers.resolve_domain(SymDomain::Particle, **v)
                    }
                    _ => field_to_expr_scoped(&mut m, scope, "m0", ctx),
                })
                .unwrap_or_else(|| Expr::Number("0".into()));
            let m2 = slot
                .member(2)
                .map(|mut m| field_to_expr_scoped(&mut m, scope, "m2", ctx))
                .unwrap_or_else(|| Expr::Number("0".into()));
            let m3 = slot
                .member(3)
                .map(|mut m| field_to_expr_scoped(&mut m, scope, "m3", ctx))
                .unwrap_or_else(|| Expr::Number("0".into()));
            return Expr::Constructor(Call {
                name: "CParticleAttachmentInfo".to_string(),
                arguments: vec![syn(m1), syn(m0), syn(m2), syn(m3)],
            });
        }
        "AttackHistoryCombo" => {
            let m1 = slot
                .member(1)
                .map(|mut m| field_to_expr_scoped(&mut m, scope, "m1", ctx))
                .unwrap_or_else(|| Expr::Number("1.0".into()));
            let mut args = vec![syn(m1)];
            if let Some(FieldRef::Vec(vslot)) = slot.member(0) {
                for i in 0..vslot.len() {
                    let mut el = vslot.element(i);
                    if let FieldRef::Struct(st) = &mut el {
                        let t = st
                            .member(0)
                            .and_then(|mut m| match &mut m {
                                FieldRef::I32(v) => Some(
                                    ctx.headers
                                        .resolve_domain(SymDomain::AttackHistoryType, **v),
                                ),
                                _ => None,
                            })
                            .unwrap_or_else(|| Expr::Number("0".into()));
                        let a = st
                            .member(1)
                            .and_then(|mut m| match &mut m {
                                FieldRef::I32(v) => Some(
                                    ctx.headers.resolve_domain(SymDomain::DamageAttribute, **v),
                                ),
                                _ => None,
                            })
                            .unwrap_or_else(|| Expr::Number("-1".into()));
                        args.push(syn(Expr::Constructor(Call {
                            name: "Add".to_string(),
                            arguments: vec![syn(t), syn(a)],
                        })));
                    }
                }
            }
            return Expr::Constructor(Call {
                name: "CAttackHistoryCombo".to_string(),
                arguments: args,
            });
        }
        _ => {}
    }

    let ctor_name = if type_name.starts_with('C') {
        type_name.to_string()
    } else {
        format!("C{type_name}")
    };
    let mut args = Vec::new();
    let n = slot.member_count();
    for i in 0..n {
        let member_name = slot.member_name(i).unwrap_or("");
        if let Some(mut m) = slot.member(i) {
            match &mut m {
                FieldRef::Vec(vslot) => {
                    for vi in 0..vslot.len() {
                        let mut vel = vslot.element(vi);
                        let expr = field_to_expr_scoped(&mut vel, scope, member_name, ctx);
                        args.push(syn(Expr::Constructor(Call {
                            name: "Add".to_string(),
                            arguments: vec![syn(expr)],
                        })));
                    }
                }
                FieldRef::Map(mslot) => {
                    mslot.for_each_pair(&mut |mut k, mut v| {
                        let k_expr = field_to_expr_scoped(&mut k, scope, member_name, ctx);
                        let v_expr = field_to_expr_scoped(&mut v, scope, member_name, ctx);
                        args.push(syn(Expr::Constructor(Call {
                            name: "Add".to_string(),
                            arguments: vec![syn(k_expr), syn(v_expr)],
                        })));
                    });
                }
                other => {
                    args.push(syn(field_to_expr_scoped(other, scope, member_name, ctx)));
                }
            }
        }
    }
    Expr::Constructor(Call {
        name: ctor_name,
        arguments: args,
    })
}

fn variant_to_ctor_expr(slot: &mut dyn VariantSlot, scope: &str, ctx: &UnlowerCtx) -> Expr {
    let type_name = slot.type_name();
    let tag = slot.tag();
    let ctor_name = match type_name {
        "PhysicalPrimitiveInitType" => match tag {
            0 => "CPhysicalPrimitiveNull",
            1 => "CPhysicalPrimitiveSphere",
            2 => "CPhysicalPrimitiveCylinder",
            3 => "CPhysicalPrimitiveMesh",
            _ => "CPhysicalPrimitiveNull",
        },
        "ReactionMatchListElementsEntry" => match tag {
            0 => "Secondary",
            1 => "SecondaryCentred",
            2 => "Pyramid",
            3 => "MRBlob",
            _ => "Secondary",
        },
        _ => "UnknownVariant",
    };
    let mut args = Vec::new();
    let n = slot.member_count();
    for i in 0..n {
        let mname = slot.member_name(i).unwrap_or("");
        if let Some(mut m) = slot.member(i) {
            args.push(syn(field_to_expr_scoped(&mut m, scope, mname, ctx)));
        }
    }
    Expr::Constructor(Call {
        name: ctor_name.to_string(),
        arguments: args,
    })
}

fn field_to_expr_scoped(
    field: &mut FieldRef<'_>,
    scope: &str,
    field_name: &str,
    ctx: &UnlowerCtx,
) -> Expr {
    match field {
        FieldRef::F32(v) => {
            let val = **v;
            if val.is_nan() || val.is_infinite() {
                Expr::Number("0.0".to_string())
            } else {
                let mut s = val.to_string();
                if !s.contains('.') && !s.contains('e') && !s.contains('E') {
                    s.push_str(".0");
                }
                Expr::Number(s)
            }
        }
        FieldRef::I32(v) => ctx.headers.resolve_field(scope, field_name, **v),
        FieldRef::U32(v) => ctx.headers.resolve_field(scope, field_name, **v as i32),
        FieldRef::Bool(v) => Expr::Bool(**v),
        FieldRef::Str(s) => Expr::String((*s).clone()),
        FieldRef::WStr(s) => Expr::String(s.0.clone()),
        FieldRef::Enum(s) => Expr::Symbol(s.symbol().to_string()),
        FieldRef::Flags(s) => {
            if let Some(syms) = s.decompose_symbols() {
                if syms.is_empty() {
                    Expr::Number("0".to_string())
                } else if syms.len() == 1 {
                    Expr::Symbol(syms[0].to_string())
                } else {
                    let terms = syms
                        .into_iter()
                        .map(|sym| syn(Expr::Symbol(sym.to_string())))
                        .collect();
                    Expr::BitOr(terms)
                }
            } else {
                Expr::Number(s.get_i32().to_string())
            }
        }
        FieldRef::DefString(ds) => {
            if ds.0 < 0 {
                Expr::String(String::new())
            } else if let Some(s) = ctx.off_to_str.get(&ds.0) {
                Expr::String(s.clone())
            } else {
                Expr::String(String::new())
            }
        }
        FieldRef::DefIndex(di) => {
            if is_scalar_def_index(field_name) {
                ctx.headers.resolve_field(scope, field_name, di.0)
            } else if di.0 == 0 {
                Expr::Symbol("NULL".to_string())
            } else if let Some(name) = ctx.idx_to_name.get(&di.0) {
                if is_valid_ident(name) {
                    Expr::Symbol(name.clone())
                } else {
                    Expr::String(name.clone())
                }
            } else {
                Expr::Number(di.0.to_string())
            }
        }
        FieldRef::U8(v) => Expr::Number((**v).to_string()),
        FieldRef::U16(v) => Expr::Number((**v).to_string()),
        FieldRef::U64(v) => Expr::Number((**v).to_string()),
        FieldRef::I8(v) => Expr::Number((**v).to_string()),
        FieldRef::I16(v) => Expr::Number((**v).to_string()),
        FieldRef::PString(p) => Expr::String(String::from_utf8_lossy(&p.0).into_owned()),
        FieldRef::Struct(slot) => struct_to_ctor_expr(*slot, scope, ctx),
        FieldRef::Variant(slot) => variant_to_ctor_expr(*slot, scope, ctx),
        FieldRef::Vec(_) | FieldRef::Map(_) | FieldRef::Array(_) | FieldRef::Complex(_) => {
            Expr::Number("0".to_string())
        }
    }
}

struct NamedFieldCollector<'a, 'c> {
    ctx: &'a UnlowerCtx<'c>,
    scope: &'a str,
    base_path: PropertyPath,
    base_fields: Option<HashMap<&'static str, SemVal>>,
    seen_fields: HashSet<&'static str>,
    out: &'a mut Vec<Spanned<Statement>>,
}

impl FieldVisitor for NamedFieldCollector<'_, '_> {
    fn field(&mut self, sub_name: &'static str, mut sub_field: FieldRef<'_>) {
        if matches!(sub_field, FieldRef::Complex(_)) {
            return;
        }
        if !self.seen_fields.insert(sub_name) {
            return;
        }
        let bval = self.base_fields.as_ref().and_then(|m| m.get(sub_name));
        if let Some(bv) = bval {
            let cur_sem = field_to_semval(&mut sub_field, sub_name, self.ctx);
            if &cur_sem == bv {
                return;
            }
        }
        let mut sub_path = self.base_path.clone();
        sub_path
            .segments
            .push(PathSegment::Field(sub_name.to_string()));
        unlower_field(
            self.scope,
            sub_path,
            sub_name,
            &mut sub_field,
            bval,
            self.ctx,
            self.out,
        );
    }
}

fn unlower_engine_graphic_members(
    base_path: &PropertyPath,
    slot: &mut dyn StructSlot,
    base_val: Option<&SemVal>,
    ctx: &UnlowerCtx,
    out: &mut Vec<Spanned<Statement>>,
) {
    let base_map: HashMap<&str, &SemVal> = match base_val {
        Some(SemVal::Struct(fields)) => fields.iter().map(|(k, v)| (*k, v)).collect(),
        _ => HashMap::new(),
    };

    let graphic_type_i32 = slot
        .member(0)
        .and_then(|m| match m {
            FieldRef::Enum(e) => Some(e.get_i32()),
            FieldRef::I32(v) => Some(*v),
            _ => None,
        })
        .unwrap_or(0);

    let member_specs: [(usize, &str, &str); 5] = [
        (0, "type_", "Type"),
        (1, "bank_index", "BankIndex"),
        (2, "anim_step", "AnimStep"),
        (3, "render_size_x", "RenderSizeX"),
        (4, "additive_alpha", "AdditiveAlpha"),
    ];

    for (idx, rust_name, script_name) in member_specs {
        let Some(mut m) = slot.member(idx) else {
            continue;
        };
        let cur_sem = field_to_semval(&mut m, rust_name, ctx);
        if let Some(&bsem) = base_map.get(rust_name) {
            if &cur_sem == bsem {
                continue;
            }
        }
        let expr = if idx == 1 {
            // BankIndex resolved by graphic_type_i32
            let bank_id = match &m {
                FieldRef::I32(v) => **v,
                _ => 0,
            };
            if bank_id > 0 {
                match graphic_type_i32 {
                    1 | 2 => ctx.headers.resolve_domain(SymDomain::Texture, bank_id),
                    3 => ctx.headers.resolve_domain(SymDomain::Particle, bank_id),
                    4 | 5 => ctx.headers.resolve_domain(SymDomain::Mesh, bank_id),
                    _ => Expr::Number(bank_id.to_string()),
                }
            } else {
                Expr::Number(bank_id.to_string())
            }
        } else {
            field_to_expr_scoped(&mut m, "EngineGraphic", script_name, ctx)
        };
        let mut mpath = base_path.clone();
        mpath
            .segments
            .push(PathSegment::Field(script_name.to_string()));
        out.push(syn(Statement::Field(defs::text::Field {
            path: mpath,
            expr: syn(expr),
        })));
    }
}

fn unlower_field(
    scope: &str,
    base_path: PropertyPath,
    field_name: &'static str,
    field: &mut FieldRef<'_>,
    base_val: Option<&SemVal>,
    ctx: &UnlowerCtx,
    out: &mut Vec<Spanned<Statement>>,
) {
    // Single-wrapper delegation for Struct
    if let FieldRef::Struct(slot) = field {
        if slot.member_count() == 1 {
            if let Some(mut m0) = slot.member(0) {
                if matches!(
                    m0,
                    FieldRef::Vec(_) | FieldRef::Map(_) | FieldRef::Variant(_)
                ) {
                    let base_inner = base_val.and_then(|sv| match sv {
                        SemVal::Struct(m) => m.first().map(|(_, v)| v),
                        _ => None,
                    });
                    unlower_field(scope, base_path, field_name, &mut m0, base_inner, ctx, out);
                    return;
                }
            }
        }
    }

    match field {
        FieldRef::Vec(slot) => {
            let base_len = match base_val {
                Some(SemVal::List(l)) => l.len(),
                _ => 0,
            };
            if base_len > 0 {
                out.push(syn(Statement::MethodCall(defs::text::MethodCall {
                    object: base_path.clone(),
                    call: Call {
                        name: "clear".to_string(),
                        arguments: Vec::new(),
                    },
                })));
            }
            for i in 0..slot.len() {
                let mut el = slot.element(i);
                let mut handled = false;
                if let FieldRef::Struct(st_slot) = &mut el {
                    if st_slot.type_name() == "SkeletalMorphsMorphsEntry" {
                        let m0 = st_slot
                            .member(0)
                            .map(|mut m| match &mut m {
                                FieldRef::U32(v) => {
                                    ctx.headers.resolve_domain(SymDomain::HeroMorph, **v as i32)
                                }
                                _ => field_to_expr_scoped(&mut m, scope, "morph_type", ctx),
                            })
                            .unwrap_or_else(|| Expr::Number("0".into()));
                        let m1 = st_slot
                            .member(1)
                            .map(|mut m| field_to_expr_scoped(&mut m, scope, "morph_name", ctx))
                            .unwrap_or_else(|| Expr::String(String::new()));
                        let m2 = st_slot
                            .member(2)
                            .map(|mut m| field_to_expr_scoped(&mut m, scope, "bone_index", ctx))
                            .unwrap_or_else(|| Expr::Number("0".into()));
                        let m3 = st_slot
                            .member(3)
                            .map(|mut m| field_to_expr_scoped(&mut m, scope, "mirror_over", ctx))
                            .unwrap_or_else(|| Expr::Bool(false));
                        out.push(syn(Statement::MethodCall(defs::text::MethodCall {
                            object: base_path.clone(),
                            call: Call {
                                name: "Add".to_string(),
                                arguments: vec![syn(m0), syn(m1), syn(m2), syn(m3)],
                            },
                        })));
                        continue;
                    }
                    let mut elem_path = base_path.clone();
                    elem_path
                        .segments
                        .push(PathSegment::Index(syn(Expr::Number(i.to_string()))));
                    let start_len = out.len();
                    let mut collector = NamedFieldCollector {
                        ctx,
                        scope,
                        base_path: elem_path.clone(),
                        base_fields: None,
                        seen_fields: HashSet::new(),
                        out,
                    };
                    if st_slot.visit_named(&mut collector) {
                        handled = true;
                        // Ensure at least one statement was emitted for index i so apply_vec_named grows the Vec
                        if out.len() == start_len && st_slot.member_count() > 0 {
                            let m0_name = st_slot.member_name(0).unwrap_or("Unknown");
                            if let Some(mut m0) = st_slot.member(0) {
                                let mut m0_path = elem_path;
                                m0_path
                                    .segments
                                    .push(PathSegment::Field(m0_name.to_string()));
                                let expr = field_to_expr_scoped(&mut m0, scope, m0_name, ctx);
                                out.push(syn(Statement::Field(defs::text::Field {
                                    path: m0_path,
                                    expr: syn(expr),
                                })));
                            }
                        }
                    }
                }
                if !handled {
                    let expr = field_to_expr_scoped(&mut el, scope, field_name, ctx);
                    out.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: base_path.clone(),
                        call: Call {
                            name: "Add".to_string(),
                            arguments: vec![syn(expr)],
                        },
                    })));
                }
            }
        }
        FieldRef::Map(slot) => {
            let base_len = match base_val {
                Some(SemVal::Map(m)) => m.len(),
                _ => 0,
            };
            if base_len > 0 {
                out.push(syn(Statement::MethodCall(defs::text::MethodCall {
                    object: base_path.clone(),
                    call: Call {
                        name: "clear".to_string(),
                        arguments: Vec::new(),
                    },
                })));
            }
            slot.for_each_pair(&mut |mut k, mut v| {
                let key_expr = match &mut k {
                    FieldRef::I32(kv) => ctx.headers.resolve_map_key(scope, field_name, **kv),
                    FieldRef::U32(kv) => {
                        ctx.headers.resolve_map_key(scope, field_name, **kv as i32)
                    }
                    FieldRef::Enum(s) if ctx.headers.has_map_key_domain(scope, field_name) => {
                        ctx.headers.resolve_map_key(scope, field_name, s.get_i32())
                    }
                    FieldRef::Flags(s) if ctx.headers.has_map_key_domain(scope, field_name) => {
                        ctx.headers.resolve_map_key(scope, field_name, s.get_i32())
                    }
                    other => field_to_expr_scoped(other, scope, field_name, ctx),
                };
                let mut elem_path = base_path.clone();
                elem_path
                    .segments
                    .push(PathSegment::Index(syn(key_expr.clone())));
                let mut handled = false;
                if let FieldRef::Struct(st_slot) = &mut v {
                    let mut collector = NamedFieldCollector {
                        ctx,
                        scope,
                        base_path: elem_path.clone(),
                        base_fields: None,
                        seen_fields: HashSet::new(),
                        out,
                    };
                    if st_slot.visit_named(&mut collector) {
                        handled = true;
                    }
                }
                if !handled {
                    let val_expr = field_to_expr_scoped(&mut v, scope, field_name, ctx);
                    out.push(syn(Statement::Field(defs::text::Field {
                        path: elem_path,
                        expr: syn(val_expr),
                    })));
                }
            });
        }
        FieldRef::Struct(slot) => {
            if slot.type_name() == "EngineGraphic" {
                unlower_engine_graphic_members(&base_path, *slot, base_val, ctx, out);
                return;
            }
            // If the struct is a nested DefStruct (e.g. CreatureStatsDef `Stats`),
            // emit member-path statements (`Stats.ExperienceWorth 5;`) comparing against base.
            let base_sub_fields = match base_val {
                Some(SemVal::Struct(_)) => {
                    // Build a wire-name keyed base map by visiting a default or using positional index
                    None
                }
                _ => None,
            };
            let mut collector = NamedFieldCollector {
                ctx,
                scope,
                base_path: base_path.clone(),
                base_fields: base_sub_fields,
                seen_fields: HashSet::new(),
                out,
            };
            if slot.visit_named(&mut collector) {
                return;
            }
            let expr = struct_to_ctor_expr(*slot, scope, ctx);
            out.push(syn(Statement::Field(defs::text::Field {
                path: base_path,
                expr: syn(expr),
            })));
        }
        FieldRef::Variant(slot) => {
            let expr = variant_to_ctor_expr(*slot, scope, ctx);
            out.push(syn(Statement::Field(defs::text::Field {
                path: base_path,
                expr: syn(expr),
            })));
        }
        FieldRef::Complex(_) => {
            // Opaque padding, ignore
        }
        _ => {
            let expr = field_to_expr_scoped(field, scope, field_name, ctx);
            out.push(syn(Statement::Field(defs::text::Field {
                path: base_path,
                expr: syn(expr),
            })));
        }
    }
}

struct UnlowerVisitorSkipping<'a, 'c> {
    ctx: &'a UnlowerCtx<'c>,
    scope: &'a str,
    base_fields: &'a HashMap<&'static str, SemVal>,
    skip_fields: &'a [&'a str],
    seen_wire_names: HashSet<&'static str>,
    stmts: &'a mut Vec<Spanned<Statement>>,
}

impl FieldVisitor for UnlowerVisitorSkipping<'_, '_> {
    fn field(&mut self, name: &'static str, mut field: FieldRef<'_>) {
        if self.skip_fields.contains(&name) || matches!(field, FieldRef::Complex(_)) {
            return;
        }
        if !self.seen_wire_names.insert(name) {
            return;
        }
        let field_sem = field_to_semval(&mut field, name, self.ctx);
        let base_val = self.base_fields.get(name);
        if let Some(bval) = base_val {
            if &field_sem == bval {
                return;
            }
        }
        unlower_field(
            self.scope,
            PropertyPath::simple(name),
            name,
            &mut field,
            base_val,
            self.ctx,
            self.stmts,
        );
    }
}

/// Generic un-lowerer skipping specified fields.
pub fn unlower_generic_skipping(
    scope: &str,
    body: &mut DefBody,
    base: &mut DefBody,
    ctx: &UnlowerCtx,
    skip_fields: &[&str],
) -> Vec<Spanned<Statement>> {
    let base_sem = body_to_semval(base, ctx);
    let base_fields: HashMap<&'static str, SemVal> = match base_sem {
        SemVal::Struct(fields) => fields.into_iter().collect(),
        _ => HashMap::new(),
    };

    let mut stmts = Vec::new();
    let mut visitor = UnlowerVisitorSkipping {
        ctx,
        scope,
        base_fields: &base_fields,
        skip_fields,
        seen_wire_names: HashSet::new(),
        stmts: &mut stmts,
    };
    body.visit_active(&mut visitor);
    stmts
}

/// Generic un-lowerer: compare `body` against `base` using `VisitFields` and emit
/// only statements for fields whose semantic value differs.
pub fn unlower_generic(
    body: &mut DefBody,
    base: &mut DefBody,
    ctx: &UnlowerCtx,
) -> Vec<Spanned<Statement>> {
    unlower_generic_skipping("*", body, base, ctx, &[])
}

fn unlower_animation_set(
    anim_set: &defs::def::values::AnimationSet,
    base_anim_set: &defs::def::values::AnimationSet,
    ctx: &UnlowerCtx,
    stmts: &mut Vec<Spanned<Statement>>,
) {
    if anim_set.default_delay != base_anim_set.default_delay {
        stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
            object: PropertyPath::simple("Animation"),
            call: Call {
                name: "SetDefaultDelay".to_string(),
                arguments: vec![syn(Expr::Number(anim_set.default_delay.to_string()))],
            },
        })));
    }
    if anim_set.default_group != base_anim_set.default_group && anim_set.default_group >= 0 {
        if let Some(grp_name) = ctx.off_to_str.get(&anim_set.default_group) {
            stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                object: PropertyPath::simple("Animation"),
                call: Call {
                    name: "SetDefaultGroupName".to_string(),
                    arguments: vec![syn(Expr::String(grp_name.clone()))],
                },
            })));
        }
    }
    let mut current_group: i32 = -1;
    for entry_wrap in &anim_set.anims {
        let entry = &entry_wrap.entry;
        if entry.group_name.0 != current_group {
            if current_group >= 0 {
                stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                    object: PropertyPath::simple("Animation"),
                    call: Call {
                        name: "EndGroup".to_string(),
                        arguments: Vec::new(),
                    },
                })));
            }
            current_group = entry.group_name.0;
            if current_group >= 0 {
                let grp_str = ctx
                    .off_to_str
                    .get(&current_group)
                    .map(|s| s.as_str())
                    .unwrap_or("");
                stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                    object: PropertyPath::simple("Animation"),
                    call: Call {
                        name: "StartGroup".to_string(),
                        arguments: vec![syn(Expr::String(grp_str.to_string()))],
                    },
                })));
            }
        }

        let anim_name_str = if entry.anim_name.0 >= 0 {
            ctx.off_to_str.get(&entry.anim_name.0).cloned()
        } else {
            None
        }
        .or_else(|| ctx.crc_to_str.get(&entry_wrap.key).cloned())
        .unwrap_or_else(|| format!("anim_{:08x}", entry_wrap.key));

        let bank_expr = ctx
            .headers
            .resolve_domain(SymDomain::Anim, entry.bank_index);

        let mut ctor_args = vec![syn(Expr::String(anim_name_str)), syn(bank_expr)];

        for comp in &entry.components {
            match comp {
                defs::def::values::AnimationEntryComponentsEntry::Tag0 { flags } => {
                    let flags_expr = ctx.headers.resolve_domain(SymDomain::AnimFlags, *flags);
                    ctor_args.push(syn(Expr::Constructor(Call {
                        name: "Flags".to_string(),
                        arguments: vec![syn(flags_expr)],
                    })));
                }
                defs::def::values::AnimationEntryComponentsEntry::Tag1 { transition_in_time } => {
                    ctor_args.push(syn(Expr::Constructor(Call {
                        name: "TransTime".to_string(),
                        arguments: vec![syn(Expr::Number(transition_in_time.to_string()))],
                    })));
                }
                defs::def::values::AnimationEntryComponentsEntry::Tag2 { delay } => {
                    ctor_args.push(syn(Expr::Constructor(Call {
                        name: "Delay".to_string(),
                        arguments: vec![syn(Expr::Number(delay.to_string()))],
                    })));
                }
                defs::def::values::AnimationEntryComponentsEntry::Tag3 {
                    start_handedness,
                    end_handedness,
                } => {
                    let h0 = ctx
                        .headers
                        .resolve_domain(SymDomain::Handedness, *start_handedness);
                    let h1 = ctx
                        .headers
                        .resolve_domain(SymDomain::Handedness, *end_handedness);
                    ctor_args.push(syn(Expr::Constructor(Call {
                        name: "Handedness".to_string(),
                        arguments: vec![syn(h0), syn(h1)],
                    })));
                }
                defs::def::values::AnimationEntryComponentsEntry::Tag4 {
                    combo_stage,
                    combo_id,
                } => {
                    let args = if *combo_stage == -777 {
                        vec![syn(Expr::Number(combo_id.to_string()))]
                    } else {
                        vec![
                            syn(Expr::Number(combo_stage.to_string())),
                            syn(Expr::Number(combo_id.to_string())),
                        ]
                    };
                    ctor_args.push(syn(Expr::Constructor(Call {
                        name: "Combo".to_string(),
                        arguments: args,
                    })));
                }
                defs::def::values::AnimationEntryComponentsEntry::Tag5 { recoil_anim_index } => {
                    let rec_expr = ctx
                        .headers
                        .resolve_domain(SymDomain::Anim, *recoil_anim_index);
                    ctor_args.push(syn(Expr::Constructor(Call {
                        name: "Recoil".to_string(),
                        arguments: vec![syn(rec_expr)],
                    })));
                }
                defs::def::values::AnimationEntryComponentsEntry::Tag6 {
                    melee_flourish,
                    melee_knockdown,
                } => {
                    ctor_args.push(syn(Expr::Constructor(Call {
                        name: "CombatMisc".to_string(),
                        arguments: vec![
                            syn(Expr::Bool(*melee_flourish)),
                            syn(Expr::Bool(*melee_knockdown)),
                        ],
                    })));
                }
                defs::def::values::AnimationEntryComponentsEntry::Tag7 {
                    target_offset: _,
                    target_offset_vector_x,
                    target_offset_vector_y,
                } => {
                    ctor_args.push(syn(Expr::Constructor(Call {
                        name: "TargetOffset".to_string(),
                        arguments: vec![
                            syn(Expr::Number(target_offset_vector_x.to_string())),
                            syn(Expr::Number(target_offset_vector_y.to_string())),
                        ],
                    })));
                }
                defs::def::values::AnimationEntryComponentsEntry::Tag9 {
                    animation_speed_multiplier,
                } => {
                    ctor_args.push(syn(Expr::Constructor(Call {
                        name: "SpeedMultiplier".to_string(),
                        arguments: vec![syn(Expr::Number(animation_speed_multiplier.to_string()))],
                    })));
                }
                defs::def::values::AnimationEntryComponentsEntry::Tag11 { next_filter } => {
                    let filter_str = if next_filter.0 >= 0 {
                        ctx.off_to_str
                            .get(&next_filter.0)
                            .map(|s| s.as_str())
                            .unwrap_or("")
                    } else {
                        ""
                    };
                    ctor_args.push(syn(Expr::Constructor(Call {
                        name: "NextAnimFilter".to_string(),
                        arguments: vec![syn(Expr::String(filter_str.to_string()))],
                    })));
                }
                defs::def::values::AnimationEntryComponentsEntry::Tag12 { response_anim_name } => {
                    let resp_str = String::from_utf8_lossy(&response_anim_name.0);
                    ctor_args.push(syn(Expr::Constructor(Call {
                        name: "StrikeResponseAnim".to_string(),
                        arguments: vec![syn(Expr::String(resp_str.into_owned()))],
                    })));
                }
                _ => {}
            }
        }

        stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
            object: PropertyPath::simple("Animation"),
            call: Call {
                name: "Add".to_string(),
                arguments: ctor_args,
            },
        })));
    }
    if current_group >= 0 {
        stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
            object: PropertyPath::simple("Animation"),
            call: Call {
                name: "EndGroup".to_string(),
                arguments: Vec::new(),
            },
        })));
    }
}

fn unlower_particle_morph(
    target: &'static str,
    data: &[u8; 36],
    ctx: &UnlowerCtx,
    stmts: &mut Vec<Spanned<Statement>>,
) {
    let morph_type = i32::from_le_bytes(data[0x00..0x04].try_into().unwrap());
    let blending = i32::from_le_bytes(data[0x04..0x08].try_into().unwrap());
    let location = i32::from_le_bytes(data[0x08..0x0c].try_into().unwrap());
    let loc_name_off = i32::from_le_bytes(data[0x0c..0x10].try_into().unwrap());
    let loc_name = if loc_name_off >= 0 {
        ctx.off_to_str
            .get(&loc_name_off)
            .map(|s| s.as_str())
            .unwrap_or("")
    } else {
        ""
    };
    let loc_idx = i32::from_le_bytes(data[0x10..0x14].try_into().unwrap());
    let part_idx = i32::from_le_bytes(data[0x14..0x18].try_into().unwrap());
    let second_part_idx = i32::from_le_bytes(data[0x18..0x1c].try_into().unwrap());
    let start_val = f32::from_le_bytes(data[0x1c..0x20].try_into().unwrap());
    let end_val = f32::from_le_bytes(data[0x20..0x24].try_into().unwrap());

    let morph_expr = ctx.headers.resolve_domain(SymDomain::HeroMorph, morph_type);
    let part_expr = ctx.headers.resolve_domain(SymDomain::Particle, part_idx);
    let loc_expr = ctx.headers.resolve_domain(SymDomain::MeshLocation, location);

    if blending == 0 {
        stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
            object: PropertyPath::simple(target),
            call: Call {
                name: "Add".to_string(),
                arguments: vec![
                    syn(morph_expr),
                    syn(part_expr),
                    syn(Expr::Number(start_val.to_string())),
                    syn(Expr::Number(end_val.to_string())),
                    syn(loc_expr),
                    syn(Expr::String(loc_name.to_string())),
                    syn(Expr::Number(loc_idx.to_string())),
                ],
            },
        })));
    } else {
        let part2_expr = ctx
            .headers
            .resolve_domain(SymDomain::Particle, second_part_idx);
        stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
            object: PropertyPath::simple(target),
            call: Call {
                name: "AddBlend".to_string(),
                arguments: vec![
                    syn(morph_expr),
                    syn(part_expr),
                    syn(part2_expr),
                    syn(Expr::Number(start_val.to_string())),
                    syn(Expr::Number(end_val.to_string())),
                    syn(loc_expr),
                    syn(Expr::String(loc_name.to_string())),
                    syn(Expr::Number(loc_idx.to_string())),
                ],
            },
        })));
    }
}

pub fn unlower_body(
    def_type: &str,
    body: &mut DefBody,
    base: &mut DefBody,
    ctx: &UnlowerCtx,
) -> Vec<Spanned<Statement>> {
    match def_type {
        "THING" | "BUILDING" | "CREATURE" | "HOLY_SITE" | "MARKER" | "NOISE" | "OBJECT"
        | "PHYSICAL_SWITCH" | "SWITCH" | "VILLAGE" => {
            let mut stmts = unlower_generic_skipping(def_type, body, base, ctx, &["Components"]);
            let comps = match body {
                DefBody::ThingBaseDef(d) => &d.components.entries,
                DefBody::ThingBuildingDef(d) => &d.components.entries,
                DefBody::ThingCreatureDef(d) => &d.components.entries,
                DefBody::ThingHolySiteDef(d) => &d.components.entries,
                DefBody::ThingMarkerDef(d) => &d.components.entries,
                DefBody::ThingNoiseDef(d) => &d.components.entries,
                DefBody::ThingObjectDef(d) => &d.components.entries,
                DefBody::ThingPhysicalSwitchDef(d) => &d.components.entries,
                DefBody::ThingSwitchDef(d) => &d.components.entries,
                DefBody::ThingVillageDef(d) => &d.components.entries,
                _ => &[][..],
            };
            for entry in comps {
                let name_str = if entry.name.0 >= 0 {
                    ctx.off_to_str
                        .get(&entry.name.0)
                        .map(|s| s.as_str())
                        .unwrap_or("")
                } else {
                    ""
                };
                let call = if entry.over {
                    Call {
                        name: "Add".to_string(),
                        arguments: vec![
                            syn(Expr::String(name_str.to_string())),
                            syn(Expr::Symbol("TRUE".to_string())),
                        ],
                    }
                } else {
                    Call {
                        name: "Add".to_string(),
                        arguments: vec![syn(Expr::String(name_str.to_string()))],
                    }
                };
                stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                    object: PropertyPath::simple("Components"),
                    call,
                })));
            }
            stmts
        }
        "CAnimatingObjectDef" => {
            let mut stmts = unlower_generic_skipping(def_type, body, base, ctx, &["Animation"]);
            if let (DefBody::AnimatingObjectDef(b), DefBody::AnimatingObjectDef(base_obj)) =
                (&body, &base)
            {
                unlower_animation_set(&b.animation, &base_obj.animation, ctx, &mut stmts);
            }
            stmts
        }
        "CAppearanceDef" => {
            let mut stmts = unlower_generic_skipping(def_type, body, base, ctx, &["Animation"]);
            if let (DefBody::AppearanceDef(b), DefBody::AppearanceDef(base_app)) = (&body, &base) {
                unlower_animation_set(&b.animation, &base_app.animation, ctx, &mut stmts);
            }
            stmts
        }
        "CCreatureDef" => {
            let mut stmts = unlower_generic_skipping(
                def_type,
                body,
                base,
                ctx,
                &["RandomAppearanceMorph", "Expressions", "WoundMorphs"],
            );
            if let DefBody::CreatureDef(b) = body {
                // Expressions
                for expr in &b.expressions.expressions {
                    let expr_type = ctx
                        .headers
                        .resolve_domain(SymDomain::ExpressionType, expr.type_);
                    let anim_id = ctx
                        .headers
                        .resolve_domain(SymDomain::Anim, expr.data2 as i32);
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("Expressions"),
                        call: Call {
                            name: "Add".to_string(),
                            arguments: vec![syn(expr_type), syn(anim_id)],
                        },
                    })));
                }
                // WoundMorphs
                for w in &b.wound_morphs.morphs {
                    let a0 = i32::from_le_bytes(w.data[0x00..0x04].try_into().unwrap());
                    let a1 = i32::from_le_bytes(w.data[0x04..0x08].try_into().unwrap());
                    let a2 = i32::from_le_bytes(w.data[0x08..0x0c].try_into().unwrap());
                    let a3 = i32::from_le_bytes(w.data[0x0c..0x10].try_into().unwrap());
                    let a4 = i32::from_le_bytes(w.data[0x10..0x14].try_into().unwrap());
                    let a5 = i32::from_le_bytes(w.data[0x14..0x18].try_into().unwrap());
                    let a6 = i32::from_le_bytes(w.data[0x18..0x1c].try_into().unwrap());
                    let fade = i32::from_le_bytes(w.data[0x1c..0x20].try_into().unwrap());
                    let total = i32::from_le_bytes(w.data[0x20..0x24].try_into().unwrap());
                    let perm = f32::from_le_bytes(w.data[0x24..0x28].try_into().unwrap());
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("WoundMorphs"),
                        call: Call {
                            name: "AddWoundAndScar".to_string(),
                            arguments: vec![
                                syn(ctx.headers.resolve_domain(SymDomain::BodyLocation, a0)),
                                syn(ctx.headers.resolve_domain(SymDomain::Texture, a1)),
                                syn(ctx.headers.resolve_domain(SymDomain::Texture, a2)),
                                syn(ctx.headers.resolve_domain(SymDomain::Texture, a3)),
                                syn(ctx.headers.resolve_domain(SymDomain::Texture, a4)),
                                syn(ctx.headers.resolve_domain(SymDomain::Texture, a5)),
                                syn(ctx.headers.resolve_domain(SymDomain::Texture, a6)),
                                syn(Expr::Number(fade.to_string())),
                                syn(Expr::Number(total.to_string())),
                                syn(Expr::Number(perm.to_string())),
                            ],
                        },
                    })));
                }
                // RandomAppearanceMorph
                let body_parts = [
                    (0, &b.random_appearance_morph.body_parts0.meshes),
                    // Note: body_parts1 and body_parts2 have distinct element types in defs::values
                ];
                let _ = body_parts;
                for m in &b.random_appearance_morph.body_parts0.meshes {
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("RandomAppearanceMorph"),
                        call: Call {
                            name: "AddBodyPart".to_string(),
                            arguments: vec![
                                syn(ctx.headers.resolve_domain(SymDomain::BodyPart, 0)),
                                syn(Expr::Number(m.texture_id.to_string())),
                                syn(ctx.headers.resolve_domain(SymDomain::Mesh, m.mesh_id)),
                            ],
                        },
                    })));
                }
                for m in &b.random_appearance_morph.body_parts1.meshes {
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("RandomAppearanceMorph"),
                        call: Call {
                            name: "AddBodyPart".to_string(),
                            arguments: vec![
                                syn(ctx.headers.resolve_domain(SymDomain::BodyPart, 1)),
                                syn(Expr::Number(m.texture_id.to_string())),
                                syn(ctx.headers.resolve_domain(SymDomain::Mesh, m.mesh_id)),
                            ],
                        },
                    })));
                }
                for m in &b.random_appearance_morph.body_parts2.meshes {
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("RandomAppearanceMorph"),
                        call: Call {
                            name: "AddBodyPart".to_string(),
                            arguments: vec![
                                syn(ctx.headers.resolve_domain(SymDomain::BodyPart, 2)),
                                syn(Expr::Number(m.texture_id.to_string())),
                                syn(ctx.headers.resolve_domain(SymDomain::Mesh, m.mesh_id)),
                            ],
                        },
                    })));
                }
                let mut all_meshes: Vec<(i32, Vec<(u32, [u8; 8])>)> = Vec::new();
                for m in &b.random_appearance_morph.body_parts0.meshes {
                    let entries = m
                        .texture_morphs
                        .entries
                        .iter()
                        .map(|e| (e.key, e.morph))
                        .collect();
                    all_meshes.push((m.mesh_id, entries));
                }
                for m in &b.random_appearance_morph.body_parts1.meshes {
                    let entries = m
                        .texture_morphs
                        .entries
                        .iter()
                        .map(|e| (e.key, e.morph))
                        .collect();
                    all_meshes.push((m.mesh_id, entries));
                }
                for m in &b.random_appearance_morph.body_parts2.meshes {
                    let entries = m
                        .texture_morphs
                        .entries
                        .iter()
                        .map(|e| (e.key, e.morph))
                        .collect();
                    all_meshes.push((m.mesh_id, entries));
                }

                let mut seen_tex_mesh_ids = std::collections::HashSet::new();
                for (mesh_id, entries) in all_meshes {
                    if !seen_tex_mesh_ids.insert(mesh_id) || entries.is_empty() {
                        continue;
                    }
                    let mut i = 0;
                    while i < entries.len() {
                        let original = i32::from_le_bytes(entries[i].1[0..4].try_into().unwrap());
                        let mut j = i;
                        while j < entries.len() {
                            let orig_j = i32::from_le_bytes(entries[j].1[0..4].try_into().unwrap());
                            if orig_j != original {
                                break;
                            }
                            j += 1;
                        }
                        let grp = &entries[i..j];
                        let n = if j < entries.len() {
                            if entries[j].0 == 0 {
                                0
                            } else if grp.len() > 1 {
                                (grp[1].0 + 1) as i32
                            } else {
                                1
                            }
                        } else if grp.len() > 1 && grp[1].0 > 0 {
                            (grp[1].0 + 1) as i32
                        } else if b.random_appearance_morph.final_trailing == 0 {
                            0
                        } else {
                            b.random_appearance_morph.final_trailing as i32
                        };
                        let mut args = vec![
                            syn(ctx.headers.resolve_domain(SymDomain::Mesh, mesh_id)),
                            syn(Expr::Number(n.to_string())),
                            syn(ctx.headers.resolve_domain(SymDomain::Texture, original)),
                        ];
                        for e in grp {
                            let repl = i32::from_le_bytes(e.1[4..8].try_into().unwrap());
                            args.push(syn(ctx.headers.resolve_domain(SymDomain::Texture, repl)));
                        }
                        stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                            object: PropertyPath::simple("RandomAppearanceMorph"),
                            call: Call {
                                name: "AddTextureToMesh".to_string(),
                                arguments: args,
                            },
                        })));
                        i = j;
                    }
                }
            }
            stmts
        }
        "CHeroMorphDef" => {
            let mut stmts = unlower_generic_skipping(
                def_type,
                body,
                base,
                ctx,
                &["TextureMorphs", "ParticleMorphs", "IdleParticleMorphs"],
            );
            if let DefBody::HeroMorphDef(b) = body {
                for tm in &b.texture_morphs.morphs {
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("TextureMorphs"),
                        call: Call {
                            name: "Add".to_string(),
                            arguments: vec![
                                syn(Expr::Number(tm.texture_layer.to_string())),
                                syn(ctx
                                    .headers
                                    .resolve_domain(SymDomain::HeroMorph, tm.morph_type)),
                                syn(ctx.headers.resolve_domain(
                                    SymDomain::Texture,
                                    tm.replacing_texture_index,
                                )),
                                syn(ctx
                                    .headers
                                    .resolve_domain(SymDomain::Texture, tm.bank_index)),
                                syn(ctx.headers.resolve_domain(SymDomain::Blend, tm.blend)),
                            ],
                        },
                    })));
                }
                for pm in &b.particle_morphs.morphs {
                    unlower_particle_morph("ParticleMorphs", &pm.data, ctx, &mut stmts);
                }
                for pm in &b.idle_particle_morphs.morphs {
                    unlower_particle_morph("IdleParticleMorphs", &pm.data, ctx, &mut stmts);
                }
            }
            stmts
        }
        "CAppearanceModifierDef" => {
            let mut stmts = unlower_generic_skipping(def_type, body, base, ctx, &["Graphics"]);
            if let DefBody::AppearanceModifierDef(b) = body {
                for g in &b.graphics.graphics {
                    let a0 = i32::from_le_bytes(g.data[0x00..0x04].try_into().unwrap());
                    let a3 = i32::from_le_bytes(g.data[0x04..0x08].try_into().unwrap());
                    let a1 = i32::from_le_bytes(g.data[0x08..0x0c].try_into().unwrap());
                    let a2 = f32::from_le_bytes(g.data[0x0c..0x10].try_into().unwrap());
                    let a4 = f32::from_le_bytes(g.data[0x10..0x14].try_into().unwrap());
                    let a5 = f32::from_le_bytes(g.data[0x14..0x18].try_into().unwrap());
                    let bank_dom = match a0 {
                        1 | 2 => SymDomain::Texture,
                        3 => SymDomain::Particle,
                        _ => SymDomain::Mesh,
                    };
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("Graphics"),
                        call: Call {
                            name: "Add".to_string(),
                            arguments: vec![
                                syn(ctx.headers.resolve_domain(SymDomain::EngineGraphicType, a0)),
                                syn(ctx.headers.resolve_domain(bank_dom, a1)),
                                syn(Expr::Number(a2.to_string())),
                                syn(ctx.headers.resolve_domain(SymDomain::HeroMorph, a3)),
                                syn(Expr::Number(a4.to_string())),
                                syn(Expr::Number(a5.to_string())),
                            ],
                        },
                    })));
                }
            }
            stmts
        }
        "CWeaponDef" => {
            let mut stmts = unlower_generic_skipping(def_type, body, base, ctx, &["WeaponTrails"]);
            if let DefBody::WeaponDef(b) = body {
                for (key, val) in &b.weapon_trails.0 {
                    let aug = key.attack;
                    let g0 = key.knockdown;
                    let g1 = val.to_i32();
                    let aug_expr = ctx
                        .headers
                        .resolve_domain(SymDomain::ObjectAugmentation, aug);
                    let path = PropertyPath {
                        segments: vec![
                            PathSegment::Field("WeaponTrails".to_string()),
                            PathSegment::Index(syn(aug_expr)),
                        ],
                    };
                    stmts.push(syn(Statement::Field(defs::text::Field {
                        path,
                        expr: syn(Expr::Constructor(Call {
                            name: "CWeaponTrailGraphicSet".to_string(),
                            arguments: vec![
                                syn(ctx.headers.resolve_domain(SymDomain::Texture, g0)),
                                syn(ctx.headers.resolve_domain(SymDomain::Texture, g1)),
                            ],
                        })),
                    })));
                }
            }
            stmts
        }
        "CSpecialEffectsDef" => {
            let mut stmts =
                unlower_generic_skipping(def_type, body, base, ctx, &["SpecialEffects"]);
            if let DefBody::SpecialEffectsDef(b) = body {
                for (k, v) in &b.special_effects.f0.0 {
                    let key_str = ctx
                        .crc_to_str
                        .get(k)
                        .cloned()
                        .unwrap_or_else(|| format!("eff_{:08x}", k));
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("SpecialEffects"),
                        call: Call {
                            name: "Add".to_string(),
                            arguments: vec![
                                syn(Expr::String(key_str)),
                                syn(ctx.headers.resolve_domain(SymDomain::Particle, *v)),
                            ],
                        },
                    })));
                }
            }
            stmts
        }
        "CEntitySoundDef" => {
            let mut stmts = unlower_generic_skipping(def_type, body, base, ctx, &["SoundMap"]);
            if let DefBody::EntitySoundDef(b) = body {
                for e in &b.sound_map.f0 {
                    let key_str = ctx
                        .crc_to_str
                        .get(&e.key)
                        .cloned()
                        .unwrap_or_else(|| format!("snd_{:08x}", e.key));
                    let s0 = ctx
                        .headers
                        .resolve_domain(SymDomain::Sound, e.value.first_sound as i32);
                    if e.value.first_sound == e.value.last_sound {
                        stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                            object: PropertyPath::simple("SoundMap"),
                            call: Call {
                                name: "AddSingle".to_string(),
                                arguments: vec![syn(Expr::String(key_str)), syn(s0)],
                            },
                        })));
                    } else {
                        let s1 = ctx
                            .headers
                            .resolve_domain(SymDomain::Sound, e.value.last_sound as i32);
                        stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                            object: PropertyPath::simple("SoundMap"),
                            call: Call {
                                name: "AddPair".to_string(),
                                arguments: vec![syn(Expr::String(key_str)), syn(s0), syn(s1)],
                            },
                        })));
                    }
                }
                for e in &b.sound_map.f1 {
                    let key_str = ctx
                        .crc_to_str
                        .get(&e.key)
                        .cloned()
                        .unwrap_or_else(|| format!("snd_{:08x}", e.key));
                    let crit_str = if e.value.0 >= 0 {
                        ctx.off_to_str
                            .get(&e.value.0)
                            .map(|s| s.as_str())
                            .unwrap_or("")
                    } else {
                        ""
                    };
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("SoundMap"),
                        call: Call {
                            name: "AddCriteria".to_string(),
                            arguments: vec![
                                syn(Expr::String(key_str)),
                                syn(Expr::String(crit_str.to_string())),
                            ],
                        },
                    })));
                }
            }
            stmts
        }
        "CFlammableDef" => {
            let mut stmts =
                unlower_generic_skipping(def_type, body, base, ctx, &["EffectCreationSet"]);
            if let DefBody::FlammableDef(b) = body {
                for (k, v) in &b.effect_creation_set.containment_map.0 {
                    let key_str = if k.0 >= 0 {
                        ctx.off_to_str
                            .get(&k.0)
                            .map(|s| s.as_str())
                            .unwrap_or("")
                    } else {
                        ""
                    };
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("EffectCreationSet"),
                        call: Call {
                            name: "Add".to_string(),
                            arguments: vec![
                                syn(Expr::String(key_str.to_string())),
                                syn(ctx.headers.resolve_domain(SymDomain::Particle, *v)),
                            ],
                        },
                    })));
                }
            }
            stmts
        }
        "OPINION_PERSONALITY" => {
            let mut stmts =
                unlower_generic_skipping(def_type, body, base, ctx, &["PersonalityTraits"]);
            if let DefBody::OpinionPersonalityDef(b) = body {
                let mut base_blob = [0u8; 180];
                if let DefBody::OpinionPersonalityDef(base_p) = base {
                    base_blob = base_p.personality_traits.f0;
                }
                for index in 0..5 {
                    let slice_start = index * 36;
                    let slice_end = slice_start + 36;
                    if b.personality_traits.f0[slice_start..slice_end]
                        != base_blob[slice_start..slice_end]
                    {
                        let mut s = [0.0f32; 9];
                        for slot in 0..9 {
                            let offset = slice_start + slot * 4;
                            s[slot] = f32::from_le_bytes(
                                b.personality_traits.f0[offset..offset + 4]
                                    .try_into()
                                    .unwrap(),
                            );
                        }
                        let args = vec![
                            syn(Expr::Number(index.to_string())),
                            syn(Expr::Number(s[0].to_string())),
                            syn(Expr::Number(s[1].to_string())),
                            syn(Expr::Number(s[2].to_string())),
                            syn(Expr::Number(s[3].to_string())),
                            syn(Expr::Number(s[4].to_string())),
                            syn(Expr::Number(s[5].to_string())),
                            syn(Expr::Number(s[7].to_string())),
                            syn(Expr::Number(s[6].to_string())),
                            syn(Expr::Number(s[8].to_string())),
                        ];
                        stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                            object: PropertyPath::simple("PersonalityTraits"),
                            call: Call {
                                name: "Set".to_string(),
                                arguments: args,
                            },
                        })));
                    }
                }
            }
            stmts
        }
        "OPINION_DEED_EFFECTS" => {
            let mut stmts = unlower_generic_skipping(def_type, body, base, ctx, &["Effects"]);
            if let DefBody::OpinionDeedEffectsDef(b) = body {
                for eff in &b.effects.f0 {
                    let peak = ((eff.offset_per_frame_run_in as f64)
                        * (eff.frames_to_run_in as f64)) as f32;
                    let run_in_secs =
                        if eff.frames_to_run_in == 1 && eff.offset_per_frame_run_in == peak {
                            0.0
                        } else {
                            (eff.frames_to_run_in as f32) / 15.0
                        };
                    let persist = (peak as f64
                        + (eff.offset_per_frame_run_out as f64)
                            * (eff.frames_to_run_out as f64))
                        as f32;
                    let run_out_secs = if eff.frames_to_run_out == 1
                        && eff.offset_per_frame_run_out == (persist - peak)
                    {
                        0.0
                    } else {
                        (eff.frames_to_run_out as f32) / 15.0
                    };
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("Effects"),
                        call: Call {
                            name: "Add".to_string(),
                            arguments: vec![
                                syn(ctx.headers.resolve_domain(SymDomain::Opinion, eff.opinion)),
                                syn(Expr::Number(peak.to_string())),
                                syn(Expr::Number(run_in_secs.to_string())),
                                syn(Expr::Number(run_out_secs.to_string())),
                                syn(Expr::Number(persist.to_string())),
                            ],
                        },
                    })));
                }
            }
            stmts
        }
        "CDegradableDef" => {
            let mut stmts = unlower_generic_skipping(def_type, body, base, ctx, &["Degradations"]);
            if let DefBody::DegradableDef(b) = body {
                for d in &b.degradations {
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("Degradations"),
                        call: Call {
                            name: "Add".to_string(),
                            arguments: vec![
                                syn(Expr::Number(d.health_percentage.to_string())),
                                syn(ctx.headers.resolve_domain(SymDomain::Mesh, d.bank_index)),
                                syn(ctx
                                    .headers
                                    .resolve_domain(SymDomain::Particle, d.smash_particle_emitter)),
                                syn(Expr::Bool(d.blocks_navigation)),
                            ],
                        },
                    })));
                }
            }
            stmts
        }
        "CReplaceableMeshDef" => {
            let mut stmts = unlower_generic_skipping(def_type, body, base, ctx, &["Meshes"]);
            if let DefBody::ReplaceableMeshDef(b) = body {
                for m in &b.meshes.vector {
                    stmts.push(syn(Statement::MethodCall(defs::text::MethodCall {
                        object: PropertyPath::simple("Meshes"),
                        call: Call {
                            name: "Add".to_string(),
                            arguments: vec![
                                syn(ctx
                                    .headers
                                    .resolve_domain(SymDomain::EngineGraphicType, m.graphic_type as i32)),
                                syn(ctx.headers.resolve_domain(SymDomain::Mesh, m.bank_index)),
                            ],
                        },
                    })));
                }
            }
            stmts
        }
        "OPINION_SOURCE" => {
            unlower_generic_skipping(def_type, body, base, ctx, &["BinaryReaction", "BinaryOpinion"])
        }
        _ => unlower_generic_skipping(def_type, body, base, ctx, &[]),
    }
}

/// Un-lower a compiled entry record into an AST [`Definition`].
pub fn unlower_def(
    def_type: &str,
    name: &str,
    specializes: Option<String>,
    record: &EntryRecord,
    base: Option<&mut DefBody>,
    ctx: &UnlowerCtx,
) -> Definition {
    let mut body = record.body.clone();
    let body_stmts = match base {
        Some(b) => unlower_body(def_type, &mut body, b, ctx),
        None => {
            if let Some(nulldef) = ctx.nulldefs.get(def_type) {
                let mut base_clone = nulldef.clone();
                unlower_body(def_type, &mut body, &mut base_clone, ctx)
            } else {
                let mut dummy_base = body.clone();
                unlower_body(def_type, &mut body, &mut dummy_base, ctx)
            }
        }
    };

    let mut stmts = body_stmts;
    if let Some(sub_defs) = &record.sub_defs {
        for rec in sub_defs {
            let tag = ctx
                .crc_to_str
                .get(&rec.name_crc)
                .cloned()
                .or_else(|| {
                    ctx.all_entries
                        .get(&rec.def_index)
                        .map(|(cls, _)| cls.clone())
                })
                .unwrap_or_else(|| format!("Tag_{:08X}", rec.name_crc));

            if let Some((_, sub_body_orig)) = ctx.all_entries.get(&rec.def_index) {
                let mut sub_body = sub_body_orig.clone();
                if let Some(null_sub) = ctx.nulldefs.get(&tag) {
                    let mut sub_base = null_sub.clone();
                    let sub_stmts = unlower_body(&tag, &mut sub_body, &mut sub_base, ctx);
                    stmts.push(syn(Statement::TaggedBlock(defs::text::TaggedBlock {
                        tag,
                        body: sub_stmts,
                    })));
                } else {
                    let mut sub_base = sub_body.clone();
                    let sub_stmts = unlower_body(&tag, &mut sub_body, &mut sub_base, ctx);
                    stmts.push(syn(Statement::TaggedBlock(defs::text::TaggedBlock {
                        tag,
                        body: sub_stmts,
                    })));
                }
            }
        }
    }

    Definition {
        is_template: false,
        def_type: def_type.to_string(),
        name: name.to_string(),
        specializes,
        specializes_span: None,
        body: stmts,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use defs::def::armour::ArmourDef;
    use defs::def::bed::BedDef;
    use defs::def::tavern::TavernDef;
    use defs::def::wire::{DefIndex, DefString};

    #[test]
    fn test_unlower_scalar_def_index() {
        let mut body = DefBody::TavernDef(TavernDef {
            bed_cost: DefIndex(150),
        });
        let mut base = DefBody::TavernDef(TavernDef {
            bed_cost: DefIndex(0),
        });

        let idx_to_name = HashMap::new();
        let off_to_str = HashMap::new();
        let crc_to_str = HashMap::new();
        let nulldefs = HashMap::new();
        let all_entries = HashMap::new();
        let headers = HeaderResolver::empty();

        let ctx = UnlowerCtx {
            idx_to_name: &idx_to_name,
            off_to_str: &off_to_str,
            crc_to_str: &crc_to_str,
            nulldefs: &nulldefs,
            all_entries: &all_entries,
            headers: &headers,
        };

        let stmts = unlower_generic(&mut body, &mut base, &ctx);
        assert_eq!(stmts.len(), 1);
        match &stmts[0].value {
            Statement::Field(f) => {
                assert_eq!(crate::emit::format_path(&f.path), "BedCost");
                assert_eq!(f.expr.value, Expr::Number("150".into()));
            }
            _ => panic!("Expected Field statement"),
        }
    }

    #[test]
    fn test_unlower_normal_def_index_and_strings() {
        let mut idx_to_name = HashMap::new();
        idx_to_name.insert(42, "MATERIAL_FLESH_HUMAN".to_string());

        let mut off_to_str = HashMap::new();
        off_to_str.insert(100, "BED_ANIM_SLEEP".to_string());

        let crc_to_str = HashMap::new();
        let nulldefs = HashMap::new();
        let all_entries = HashMap::new();
        let headers = HeaderResolver::empty();

        let ctx = UnlowerCtx {
            idx_to_name: &idx_to_name,
            off_to_str: &off_to_str,
            crc_to_str: &crc_to_str,
            nulldefs: &nulldefs,
            all_entries: &all_entries,
            headers: &headers,
        };

        // ArmourDef DefIndex resolving
        let mut body = DefBody::ArmourDef(ArmourDef {
            damage_material: DefIndex(42),
            all_hits_knockdown: true,
            ..Default::default()
        });
        let mut base = DefBody::ArmourDef(ArmourDef::default());

        let stmts = unlower_generic(&mut body, &mut base, &ctx);
        assert_eq!(stmts.len(), 2);

        // BedDef DefString resolving
        let mut bed_body = DefBody::BedDef(BedDef {
            sleep_in_bed_anim_name: DefString(100),
            ..Default::default()
        });
        let mut bed_base = DefBody::BedDef(BedDef::default());

        let bed_stmts = unlower_generic(&mut bed_body, &mut bed_base, &ctx);
        assert_eq!(bed_stmts.len(), 1);
        match &bed_stmts[0].value {
            Statement::Field(f) => {
                assert_eq!(crate::emit::format_path(&f.path), "SleepInBedAnimName");
                assert_eq!(f.expr.value, Expr::String("BED_ANIM_SLEEP".into()));
            }
            _ => panic!("Expected Field statement"),
        }
    }
}
