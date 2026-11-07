//! Diff engine for comparing retail/original binaries against modded binaries.

use std::collections::{HashMap, HashSet};
use std::path::Path;

use defs::binary::DefBody;
use defs::def::binary::DefBinary;
use defs::def::visit::{FieldRef, FieldVisitor};
use defs::names::Names;
use defs::text::{Definition, Expr, PathSegment, Statement};

use crate::unlower::{
    HeaderResolver, UnlowerCtx, body_to_semval, is_scalar_def_index, unlower_def,
};

/// Result of diffing a single binary (`game.bin`, `frontend.bin`, or `script.bin`).
pub struct BinaryDiffResult {
    pub binary_name: String,
    pub modified_count: usize,
    pub descendant_count: usize,
    pub newly_added_count: usize,
    pub modified_definitions: HashMap<String, Definition>,
    pub added_definitions: Vec<Definition>,
}

impl BinaryDiffResult {
    pub fn all_definitions(&self) -> impl Iterator<Item = &Definition> {
        self.modified_definitions
            .values()
            .chain(self.added_definitions.iter())
    }
}

/// Aggregate output of a decompile diff run across all binaries.
pub struct DecompileDiffOutput {
    pub results: Vec<BinaryDiffResult>,
}

impl DecompileDiffOutput {
    /// Format all definitions into `.def` syntax.
    pub fn emit_to_string(&self) -> String {
        let mut out = String::new();
        for res in &self.results {
            for def in res.all_definitions() {
                out.push_str(&crate::emit::format_definition(def));
            }
        }
        out
    }

    /// Write all formatted definitions to a file on disk.
    pub fn write_to_file(&self, path: &Path) -> std::io::Result<()> {
        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent)?;
        }
        let content = self.emit_to_string();
        std::fs::write(path, content)
    }

    /// Patch modified definitions in-place into existing `.def` files and write newly added definitions to `z_mod_decompiled.def`.
    pub fn write_to_target_dir(&self, target_defs_dir: &Path) -> std::io::Result<()> {
        let all_files = def_compiler::walk_def_files(target_defs_dir);

        for res in &self.results {
            let is_script = res.binary_name == "script";
            let is_frontend = res.binary_name == "frontend";
            let is_game = res.binary_name == "game";

            let mut patched_names = HashSet::new();

            // 1. In-place patch modified definitions in existing files
            if !res.modified_definitions.is_empty() {
                for file_path in &all_files {
                    if let Some(fname) = file_path.file_name().and_then(|f| f.to_str()) {
                        if fname.starts_with("z_mod_decompiled") || fname.contains("_deprecated.") {
                            continue;
                        }
                    }
                    let p_str = file_path.to_string_lossy().replace('\\', "/");
                    let file_is_script =
                        p_str.contains("/ScriptDefs/") || p_str.contains("ScriptDefs/");
                    let file_is_frontend =
                        p_str.contains("/FrontEndDefs/") || p_str.contains("FrontEndDefs/");
                    let file_is_game = !file_is_script && !file_is_frontend;

                    let matches_scope = (is_script && file_is_script)
                        || (is_frontend && file_is_frontend)
                        || (is_game && file_is_game);
                    if !matches_scope {
                        continue;
                    }

                    let raw = std::fs::read(file_path)?;
                    let mut content = String::from_utf8_lossy(&raw).into_owned();
                    let Ok(ast) =
                        defs::text::parse_source(&content, defs::text::FileId::ANONYMOUS)
                    else {
                        continue;
                    };

                    let mut replacements = Vec::new();
                    for item in &ast.items {
                        if let defs::text::Item::Definition(spanned_def) = item {
                            let name = &spanned_def.value.name;
                            if let Some(mod_def) = res.modified_definitions.get(name) {
                                patched_names.insert(name.clone());
                                let mut def = mod_def.clone();
                                def.is_template = spanned_def.value.is_template;
                                let formatted = crate::emit::format_definition(&def);
                                let formatted_trimmed =
                                    formatted.trim_end_matches(&['\r', '\n'][..]);
                                replacements.push((
                                    spanned_def.span.start,
                                    spanned_def.span.end,
                                    formatted_trimmed.to_string(),
                                ));
                            }
                        }
                    }

                    if !replacements.is_empty() {
                        replacements.sort_by(|a, b| b.0.cmp(&a.0));
                        for (start, end, rep) in replacements {
                            content.replace_range(start..end, &rep);
                        }
                        std::fs::write(file_path, content)?;
                    }
                }
            }

            // Fallback: If any modified definition was not found in any existing file, add it to added_definitions
            let mut final_added = res.added_definitions.clone();
            for (name, def) in &res.modified_definitions {
                if !patched_names.contains(name) {
                    eprintln!(
                        "Warning: [{}] modified definition '{}' not found in existing files, writing to z_mod_decompiled.def instead",
                        res.binary_name, name
                    );
                    final_added.push(def.clone());
                }
            }

            // 2. Write newly added definitions to z_mod_decompiled.def
            let z_path = match res.binary_name.as_str() {
                "script" => target_defs_dir.join("ScriptDefs").join("z_mod_decompiled.def"),
                "frontend" => target_defs_dir.join("FrontEndDefs").join("z_mod_decompiled.def"),
                _ => target_defs_dir.join("z_mod_decompiled.def"),
            };

            if final_added.is_empty() {
                if z_path.exists() {
                    let _ = std::fs::remove_file(&z_path);
                }
            } else {
                if let Some(parent) = z_path.parent() {
                    std::fs::create_dir_all(parent)?;
                }
                let mut content = String::new();
                for def in &final_added {
                    content.push_str(&crate::emit::format_definition(def));
                }
                std::fs::write(&z_path, content)?;
            }
        }

        Ok(())
    }
}

/// Build `parent_map: child -> parent` from existing `.def`/`.tpl` files in `target_defs_dir`.
pub fn build_parent_map(target_defs_dir: &Path) -> HashMap<String, String> {
    let mut parent_map = HashMap::new();
    let files = def_compiler::walk_def_files(target_defs_dir);
    for file_path in files {
        if let Some(file_name) = file_path.file_name().and_then(|f| f.to_str()) {
            if file_name.starts_with("z_mod_decompiled") || file_name.contains("_deprecated.") {
                continue;
            }
        }
        if let Ok(raw) = std::fs::read(&file_path) {
            let content = String::from_utf8_lossy(&raw);
            if let Ok(ast) = defs::text::parse_source(&content, defs::text::FileId::ANONYMOUS) {
                for def in ast.definitions() {
                    if let Some(parent) = &def.value.specializes {
                        parent_map.insert(def.value.name.clone(), parent.clone());
                    }
                }
            }
        }
    }
    parent_map
}

fn collect_strings_from_expr(expr: &Expr, crc_map: &mut HashMap<u32, String>) {
    match expr {
        Expr::String(s) => {
            crc_map.insert(defs::crc32::crc(s.as_bytes()), s.clone());
        }
        Expr::Symbol(s) => {
            crc_map.insert(defs::crc32::crc(s.as_bytes()), s.clone());
        }
        Expr::Constructor(call) => {
            crc_map.insert(defs::crc32::crc(call.name.as_bytes()), call.name.clone());
            for arg in &call.arguments {
                collect_strings_from_expr(&arg.value, crc_map);
            }
        }
        Expr::BitOr(terms) | Expr::Add(terms) => {
            for t in terms {
                collect_strings_from_expr(&t.value, crc_map);
            }
        }
        _ => {}
    }
}

pub(crate) fn collect_strings_from_statement(stmt: &Statement, crc_map: &mut HashMap<u32, String>) {
    match stmt {
        Statement::Field(f) => {
            collect_strings_from_expr(&f.expr.value, crc_map);
            for seg in &f.path.segments {
                match seg {
                    PathSegment::Field(name) => {
                        crc_map.insert(defs::crc32::crc(name.as_bytes()), name.clone());
                    }
                    PathSegment::Index(idx) => {
                        collect_strings_from_expr(&idx.value, crc_map);
                    }
                }
            }
        }
        Statement::MethodCall(m) => {
            crc_map.insert(defs::crc32::crc(m.call.name.as_bytes()), m.call.name.clone());
            for arg in &m.call.arguments {
                collect_strings_from_expr(&arg.value, crc_map);
            }
        }
        Statement::TaggedBlock(tb) => {
            crc_map.insert(defs::crc32::crc(tb.tag.as_bytes()), tb.tag.clone());
            for s in &tb.body {
                collect_strings_from_statement(&s.value, crc_map);
            }
        }
    }
}

/// Build a comprehensive CRC32 -> String lookup table from names and text files.
pub fn build_crc_to_str(
    orig_names: &Names,
    mod_names: &Names,
    target_defs_dir: &Path,
) -> HashMap<u32, String> {
    let mut crc_to_str = HashMap::new();
    for entry in orig_names.map.values() {
        crc_to_str.insert(defs::crc32::crc(entry.string.as_bytes()), entry.string.clone());
    }
    for entry in mod_names.map.values() {
        crc_to_str.insert(defs::crc32::crc(entry.string.as_bytes()), entry.string.clone());
    }

    let files = def_compiler::walk_def_files(target_defs_dir);
    for file_path in files {
        if let Some(file_name) = file_path.file_name().and_then(|f| f.to_str()) {
            if file_name.starts_with("z_mod_decompiled") || file_name.contains("_deprecated.") {
                continue;
            }
        }
        if let Ok(raw) = std::fs::read(&file_path) {
            let content = String::from_utf8_lossy(&raw);
            if let Ok(ast) = defs::text::parse_source(&content, defs::text::FileId::ANONYMOUS) {
                for item in &ast.items {
                    if let defs::text::Item::Definition(def) = item {
                        crc_to_str.insert(
                            defs::crc32::crc(def.value.name.as_bytes()),
                            def.value.name.clone(),
                        );
                        crc_to_str.insert(
                            defs::crc32::crc(def.value.def_type.as_bytes()),
                            def.value.def_type.clone(),
                        );
                        for s in &def.value.body {
                            collect_strings_from_statement(&s.value, &mut crc_to_str);
                        }
                    }
                }
            }
        }
    }

    crc_to_str
}

fn collect_def_indices(field: &mut FieldRef<'_>, name: &str, indices: &mut HashSet<i32>) {
    match field {
        FieldRef::DefIndex(di) => {
            if !is_scalar_def_index(name) && di.0 > 0 {
                indices.insert(di.0);
            }
        }
        FieldRef::Vec(slot) => {
            for i in 0..slot.len() {
                let mut el = slot.element(i);
                collect_def_indices(&mut el, name, indices);
            }
        }
        FieldRef::Map(slot) => {
            slot.for_each_pair(&mut |mut k, mut v| {
                collect_def_indices(&mut k, name, indices);
                collect_def_indices(&mut v, name, indices);
            });
        }
        FieldRef::Struct(slot) => {
            for i in 0..slot.member_count() {
                let mname = slot.member_name(i).unwrap_or("?");
                if let Some(mut m) = slot.member(i) {
                    collect_def_indices(&mut m, mname, indices);
                }
            }
        }
        FieldRef::Variant(slot) => {
            for i in 0..slot.member_count() {
                let mname = slot.member_name(i).unwrap_or("?");
                if let Some(mut m) = slot.member(i) {
                    collect_def_indices(&mut m, mname, indices);
                }
            }
        }
        FieldRef::Array(slot) => {
            for i in 0..slot.len() {
                let mut el = slot.element(i);
                collect_def_indices(&mut el, name, indices);
            }
        }
        _ => {}
    }
}

struct DefIndexCollector<'a> {
    indices: &'a mut HashSet<i32>,
}

impl FieldVisitor for DefIndexCollector<'_> {
    fn field(&mut self, name: &'static str, mut field: FieldRef<'_>) {
        collect_def_indices(&mut field, name, self.indices);
    }
}

fn is_unreferenced_subdef_orphan(
    def_type: &str,
    global_index: usize,
    referenced_by_def_index: &HashSet<i32>,
    referenced_in_subdefs: &HashSet<u32>,
) -> bool {
    if !def_type.starts_with('C') {
        return false;
    }
    const NON_ORPHAN_C_TYPES: &[&str] = &[
        "CAMERA_MANAGER",
        "CAMERA_MANAGER_SET",
        "CAMERA_MODE",
        "COMBAT_DIALOGUE_DEF",
        "COMBAT_SEQUENCE",
        "COMBAT_TYPE",
        "CONFIG_OPTIONS_DEFAULTS_DEF",
        "CONTROL_SCHEME",
        "CREATURE",
        "CREATURE_ABILITY",
        "CREATURE_GENERATION_FAMILY",
        "CREATURE_GENERATOR",
        "CScriptDef",
        "CCutsceneDef",
        "CRegionScriptDef",
    ];
    if NON_ORPHAN_C_TYPES.contains(&def_type) {
        return false;
    }
    let idx = global_index as i32;
    let idx_u32 = global_index as u32;
    !referenced_by_def_index.contains(&idx) && !referenced_in_subdefs.contains(&idx_u32)
}

/// Diff a single compiled binary (`game`, `frontend`, or `script`) between `orig` and `mod`.
pub fn diff_binary(
    binary_name: &str,
    orig_bin: &DefBinary,
    orig_names: &Names,
    mod_bin: &DefBinary,
    mod_names: &Names,
    parent_map: &HashMap<String, String>,
    crc_to_str: &HashMap<u32, String>,
    headers: &HeaderResolver,
) -> BinaryDiffResult {
    let orig_idx_to_name: HashMap<i32, String> = orig_bin
        .entries(orig_names)
        .filter_map(|e| match e.file_name {
            Some(f) if !f.starts_with("NULLDEF_") => Some((e.global_index as i32, f.to_string())),
            _ => None,
        })
        .collect();
    let orig_off_to_str: HashMap<i32, String> = orig_names
        .map
        .iter()
        .map(|(off, entry)| (*off as i32, entry.string.clone()))
        .collect();
    let orig_nulldefs: HashMap<String, DefBody> = orig_bin
        .entries(orig_names)
        .filter_map(|e| match (e.file_name, e.def_name) {
            (Some(f), Some(d)) if f.starts_with("NULLDEF_") => {
                Some((d.to_string(), e.record.body.clone()))
            }
            _ => None,
        })
        .collect();
    let orig_all_entries: HashMap<u32, (String, DefBody)> = orig_bin
        .entries(orig_names)
        .map(|e| {
            (
                e.global_index as u32,
                (
                    e.def_name.unwrap_or("?").to_string(),
                    e.record.body.clone(),
                ),
            )
        })
        .collect();

    let mod_idx_to_name: HashMap<i32, String> = mod_bin
        .entries(mod_names)
        .filter_map(|e| match e.file_name {
            Some(f) if !f.starts_with("NULLDEF_") => Some((e.global_index as i32, f.to_string())),
            _ => None,
        })
        .collect();
    let mod_off_to_str: HashMap<i32, String> = mod_names
        .map
        .iter()
        .map(|(off, entry)| (*off as i32, entry.string.clone()))
        .collect();
    let mod_nulldefs: HashMap<String, DefBody> = mod_bin
        .entries(mod_names)
        .filter_map(|e| match (e.file_name, e.def_name) {
            (Some(f), Some(d)) if f.starts_with("NULLDEF_") => {
                Some((d.to_string(), e.record.body.clone()))
            }
            _ => None,
        })
        .collect();
    let mod_all_entries: HashMap<u32, (String, DefBody)> = mod_bin
        .entries(mod_names)
        .map(|e| {
            (
                e.global_index as u32,
                (
                    e.def_name.unwrap_or("?").to_string(),
                    e.record.body.clone(),
                ),
            )
        })
        .collect();

    let orig_ctx = UnlowerCtx {
        idx_to_name: &orig_idx_to_name,
        off_to_str: &orig_off_to_str,
        crc_to_str,
        nulldefs: &orig_nulldefs,
        all_entries: &orig_all_entries,
        headers,
    };

    let mod_ctx = UnlowerCtx {
        idx_to_name: &mod_idx_to_name,
        off_to_str: &mod_off_to_str,
        crc_to_str,
        nulldefs: &mod_nulldefs,
        all_entries: &mod_all_entries,
        headers,
    };

    let orig_by_name: HashMap<&str, defs::binary::DefBinaryEntry> = orig_bin
        .entries(orig_names)
        .filter_map(|e| match e.file_name {
            Some(f) if !f.starts_with("NULLDEF_") => Some((f, e)),
            _ => None,
        })
        .collect();

    let mut referenced_in_subdefs = HashSet::new();
    let mut referenced_by_def_index = HashSet::new();
    for e in mod_bin.entries(mod_names) {
        if let Some(sd) = &e.record.sub_defs {
            for rec in sd {
                referenced_in_subdefs.insert(rec.def_index);
            }
        }
        let mut body = e.record.body.clone();
        let mut collector = DefIndexCollector {
            indices: &mut referenced_by_def_index,
        };
        body.visit_active(&mut collector);
    }

    let mut modified_names = HashSet::new();
    let mut newly_added_names = HashSet::new();

    for e in mod_bin.entries(mod_names) {
        let Some(fname) = e.file_name else { continue };
        if fname.starts_with("NULLDEF_") {
            continue;
        }
        let def_type = e.def_name.unwrap_or("");

        if let Some(orig_e) = orig_by_name.get(fname) {
            let mut o_body = orig_e.record.body.clone();
            let mut m_body = e.record.body.clone();
            let o_sem = body_to_semval(&mut o_body, &orig_ctx);
            let m_sem = body_to_semval(&mut m_body, &mod_ctx);
            let mut is_mod = o_sem != m_sem;

            if !is_mod {
                let osd = orig_e.record.sub_defs.as_deref().unwrap_or(&[]);
                let msd = e.record.sub_defs.as_deref().unwrap_or(&[]);
                if osd.len() != msd.len() {
                    is_mod = true;
                } else {
                    for (o, m) in osd.iter().zip(msd.iter()) {
                        if o.name_crc != m.name_crc {
                            is_mod = true;
                            break;
                        }
                        let ob = orig_all_entries.get(&o.def_index).map(|(_, b)| b.clone());
                        let mb = mod_all_entries.get(&m.def_index).map(|(_, b)| b.clone());
                        match (ob, mb) {
                            (Some(mut o_body), Some(mut m_body)) => {
                                let os = body_to_semval(&mut o_body, &orig_ctx);
                                let ms = body_to_semval(&mut m_body, &mod_ctx);
                                if os != ms {
                                    is_mod = true;
                                    break;
                                }
                            }
                            (None, None) => {}
                            _ => {
                                is_mod = true;
                                break;
                            }
                        }
                    }
                }
            }

            if is_mod {
                modified_names.insert(fname.to_string());
            }
        } else {
            // Newly added
            if referenced_in_subdefs.contains(&(e.global_index as u32)) {
                continue;
            }
            if is_unreferenced_subdef_orphan(
                def_type,
                e.global_index,
                &referenced_by_def_index,
                &referenced_in_subdefs,
            ) {
                continue;
            }
            newly_added_names.insert(fname.to_string());
        }
    }

    // Step 2: Descendants of modified definitions
    let mut descendant_names = HashSet::new();
    for e in mod_bin.entries(mod_names) {
        let Some(fname) = e.file_name else { continue };
        if fname.starts_with("NULLDEF_") || modified_names.contains(fname) {
            continue;
        }
        let mut curr = fname.to_string();
        let mut visited = HashSet::new();
        while let Some(parent) = parent_map.get(&curr) {
            if !visited.insert(parent.clone()) {
                break;
            }
            if modified_names.contains(parent) {
                descendant_names.insert(fname.to_string());
                break;
            }
            curr = parent.clone();
        }
    }

    // Step 4: Separate modified definitions and newly added definitions
    let mut modified_definitions = HashMap::new();
    let mut added_definitions = Vec::new();
    for e in mod_bin.entries(mod_names) {
        let Some(fname) = e.file_name else { continue };
        if fname.starts_with("NULLDEF_") {
            continue;
        }
        let def_type = e.def_name.unwrap_or("");
        if modified_names.contains(fname) {
            let def = unlower_def(def_type, fname, None, e.record, None, &mod_ctx);
            modified_definitions.insert(fname.to_string(), def);
        } else if newly_added_names.contains(fname) {
            let def = unlower_def(def_type, fname, None, e.record, None, &mod_ctx);
            added_definitions.push(def);
        }
    }

    BinaryDiffResult {
        binary_name: binary_name.to_string(),
        modified_count: modified_names.len(),
        descendant_count: descendant_names.len(),
        newly_added_count: newly_added_names.len(),
        modified_definitions,
        added_definitions,
    }
}

/// Run diff across `{game,frontend,script}.bin` and `names.bin`.
pub fn run_diff(
    orig_dir: &Path,
    mod_dir: &Path,
    target_defs_dir: &Path,
) -> Result<DecompileDiffOutput, Box<dyn std::error::Error>> {
    let orig_names = Names::load(&orig_dir.join("names.bin")).map_err(|e| format!("{e:?}"))?;
    let mod_names = Names::load(&mod_dir.join("names.bin")).map_err(|e| format!("{e:?}"))?;

    let parent_map = build_parent_map(target_defs_dir);
    let crc_to_str = build_crc_to_str(&orig_names, &mod_names, target_defs_dir);
    let headers = HeaderResolver::build(target_defs_dir);

    let mut results = Vec::new();

    for bin_stem in ["game", "frontend", "script"] {
        let bin_file = format!("{bin_stem}.bin");
        let orig_path = orig_dir.join(&bin_file);
        let mod_path = mod_dir.join(&bin_file);

        if !orig_path.exists() || !mod_path.exists() {
            continue;
        }

        let orig_bin = defs::wire::with_lenient_control_ids(|| {
            DefBinary::load_with_names(&orig_path, &orig_names)
        })
        .map_err(|e| format!("{e:?}"))?;
        let mod_bin = defs::wire::with_lenient_control_ids(|| {
            DefBinary::load_with_names(&mod_path, &mod_names)
        })
        .map_err(|e| format!("{e:?}"))?;

        let res = diff_binary(
            bin_stem,
            &orig_bin,
            &orig_names,
            &mod_bin,
            &mod_names,
            &parent_map,
            &crc_to_str,
            &headers,
        );

        results.push(res);
    }

    Ok(DecompileDiffOutput { results })
}
