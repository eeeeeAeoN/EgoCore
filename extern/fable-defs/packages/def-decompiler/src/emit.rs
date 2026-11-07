//! Clean `.def` text formatting.

use defs::text::{Definition, Expr, PathSegment, PropertyPath, Statement};

/// Format an [`Expr`] into its text representation in `.def` source.
pub fn format_expr(expr: &Expr) -> String {
    match expr {
        Expr::Number(n) => n.clone(),
        Expr::Bool(b) => if *b { "TRUE".into() } else { "FALSE".into() },
        Expr::String(s) => {
            let mut out = String::from("\"");
            for ch in s.chars() {
                match ch {
                    '"' => out.push_str("\\\""),
                    '\\' => out.push_str("\\\\"),
                    _ => out.push(ch),
                }
            }
            out.push('"');
            out
        }
        Expr::Symbol(s) => s.clone(),
        Expr::Constructor(c) => {
            let args: Vec<String> = c.arguments.iter().map(|a| format_expr(&a.value)).collect();
            format!("{}({})", c.name, args.join(", "))
        }
        Expr::BitOr(terms) => {
            let parts: Vec<String> = terms.iter().map(|t| format_expr(&t.value)).collect();
            parts.join(" | ")
        }
        Expr::Add(terms) => {
            let parts: Vec<String> = terms.iter().map(|t| format_expr(&t.value)).collect();
            parts.join(" + ")
        }
    }
}

/// Format a [`PropertyPath`] into its text representation (e.g. `Graphic.BankIndex` or `Stock[0].Type`).
pub fn format_path(path: &PropertyPath) -> String {
    let mut out = String::new();
    for (i, seg) in path.segments.iter().enumerate() {
        match seg {
            PathSegment::Field(name) => {
                if i > 0 {
                    out.push('.');
                }
                out.push_str(name.as_str());
            }
            PathSegment::Index(expr) => {
                out.push('[');
                out.push_str(&format_expr(&expr.value));
                out.push(']');
            }
        }
    }
    out
}

/// Format a single [`Statement`] with indentation.
pub fn format_statement(stmt: &Statement, indent_level: usize, out: &mut String) {
    let indent = "    ".repeat(indent_level);
    match stmt {
        Statement::Field(f) => {
            let path_str = format_path(&f.path);
            let expr_str = format_expr(&f.expr.value);
            out.push_str(&format!("{indent}{path_str} {expr_str};\n"));
        }
        Statement::MethodCall(m) => {
            let obj_str = format_path(&m.object);
            let args: Vec<String> = m
                .call
                .arguments
                .iter()
                .map(|a| format_expr(&a.value))
                .collect();
            let args_joined = args.join(", ");
            if obj_str.is_empty() {
                out.push_str(&format!("{indent}{}({args_joined});\n", m.call.name));
            } else {
                out.push_str(&format!("{indent}{obj_str}.{}({args_joined});\n", m.call.name));
            }
        }
        Statement::TaggedBlock(tb) => {
            out.push_str(&format!("{indent}<{}>\n", tb.tag));
            for s in &tb.body {
                format_statement(&s.value, indent_level + 1, out);
            }
            out.push_str(&format!("{indent}<\\{}>\n", tb.tag));
        }
    }
}

/// Format a full [`Definition`] into standard `.def` syntax.
pub fn format_definition(def: &Definition) -> String {
    let mut out = String::new();
    let kw = if def.is_template {
        "#definition_template"
    } else {
        "#definition"
    };
    out.push_str(&format!("{kw} {} {}", def.def_type, def.name));
    if let Some(parent) = &def.specializes {
        out.push_str(&format!(" specialises {parent}"));
    }
    out.push('\n');
    for stmt in &def.body {
        format_statement(&stmt.value, 1, &mut out);
    }
    out.push_str("#end_definition\n\n");
    out
}

#[cfg(test)]
mod tests {
    use super::*;
    use defs::text::{Call, Field, MethodCall, Span, Spanned, TaggedBlock};

    fn syn<T>(value: T) -> Spanned<T> {
        Spanned {
            span: Span::SYNTHETIC,
            value,
        }
    }

    #[test]
    fn test_format_definition() {
        let def = Definition {
            is_template: false,
            def_type: "ARMOUR".into(),
            name: "ARMOUR_BANDIT".into(),
            specializes: Some("ARMOUR_NONE".into()),
            specializes_span: None,
            body: vec![
                syn(Statement::Field(Field {
                    path: PropertyPath::simple("DamageMaterial"),
                    expr: syn(Expr::Symbol("MATERIAL_FLESH_HUMAN".into())),
                })),
                syn(Statement::MethodCall(MethodCall {
                    object: PropertyPath::simple("Animation"),
                    call: Call {
                        name: "Add".into(),
                        arguments: vec![syn(Expr::String("IDLE".into())), syn(Expr::Number("1.0".into()))],
                    },
                })),
                syn(Statement::TaggedBlock(TaggedBlock {
                    tag: "CShopDef".into(),
                    body: vec![syn(Statement::Field(Field {
                        path: PropertyPath::simple("AlwaysOpen"),
                        expr: syn(Expr::Bool(true)),
                    }))],
                })),
            ],
        };

        let formatted = format_definition(&def);
        let expected = "\
#definition ARMOUR ARMOUR_BANDIT specialises ARMOUR_NONE
    DamageMaterial MATERIAL_FLESH_HUMAN;
    Animation.Add(\"IDLE\", 1.0);
    <CShopDef>
        AlwaysOpen TRUE;
    <\\CShopDef>
#end_definition\n\n";

        assert_eq!(formatted, expected);

        // Verify it parses cleanly with defs parser
        let ast = defs::text::parse_source(&formatted, defs::text::FileId::ANONYMOUS).unwrap();
        assert_eq!(ast.definitions().count(), 1);
        let parsed = ast.definitions().next().unwrap();
        assert_eq!(parsed.value.name, "ARMOUR_BANDIT");
        assert_eq!(parsed.value.specializes.as_deref(), Some("ARMOUR_NONE"));
        assert_eq!(parsed.value.body.len(), 3);
    }
}
