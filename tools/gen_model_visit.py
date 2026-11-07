#!/usr/bin/env python3
"""Generate fable/defs/visit.hpp (field reflection) from classes.hpp.

This is a development tool: the generated header is committed. It intentionally
parses only the declaration style emitted by the model generator.
"""
import re
import sys

SRC = 'cpp/fable-defs/include/fable/defs/classes.hpp'
OUT = 'cpp/fable-defs/include/fable/defs/visit.hpp'

lines = open(SRC, encoding='utf-8').read().splitlines()

classes = []
class_stack = []
ns_stack = []

class CClass:
    def __init__(self, name, qname, bases, parent):
        self.name = name
        self.qname = qname
        self.bases = bases
        self.parent = parent
        self.fields = []  # (type, name, suffix, bitfield)


def last_identifier(s):
    depth = 0
    spans = []
    i = 0
    while i < len(s):
        ch = s[i]
        if ch == '<':
            depth += 1
            i += 1
            continue
        if ch == '>':
            depth -= 1
            i += 1
            continue
        if ch.isalpha() or ch == '_':
            start = i
            i += 1
            while i < len(s) and (s[i].isalnum() or s[i] == '_'):
                i += 1
            if depth == 0:
                before = s[:start].rstrip()
                after = s[i:].lstrip()
                if not before.endswith('::') and not after.startswith('::'):
                    spans.append((start, i))
            continue
        i += 1
    return spans[-1] if spans else None


def split_member(line):
    s = line.strip()
    if not s.endswith(';'):
        return None
    s = s[:-1].strip()
    # Default member initializers are not part of reflection.
    s = s.split('=', 1)[0].rstrip()
    bit = False
    m = re.search(r'\s*:\s*\d+\s*$', s)
    if m:
        s = s[:m.start()].rstrip()
        bit = True
    while True:
        m = re.search(r'\s*\[[^\]]*\]\s*$', s)
        if not m:
            break
        s = s[:m.start()].rstrip()
    span = last_identifier(s)
    if not span:
        return None
    start, end = span
    decl = s[:start].strip()
    name = s[start:end]
    if not decl:
        return None
    return decl, name, bit


i = 0
while i < len(lines):
    line = lines[i].strip()
    i += 1
    if not line or line.startswith('//') or line.startswith('#'):
        continue
    m = re.match(r'^namespace\s+([A-Za-z_]\w*)\s*\{$', line)
    if m:
        ns_stack.append(m.group(1))
        continue
    if line.startswith('}'):
        if class_stack:
            class_stack.pop()
        elif ns_stack:
            ns_stack.pop()
        continue
    m = re.match(r'^class\s+([A-Za-z_]\w*)\s*(?::\s*(.*))?$', line)
    if m:
        # A class definition is followed by a line containing only `{`.
        if i < len(lines) and lines[i].strip() == '{':
            i += 1
            name = m.group(1)
            bases_text = m.group(2) or ''
            bases = []
            for base in bases_text.split(','):
                base = base.strip()
                for kw in ('public ', 'protected ', 'private ', 'virtual '):
                    base = base.replace(kw, '')
                base = base.strip()
                if base and not base.startswith('std::'):
                    bases.append(base)
            parent = class_stack[-1] if class_stack else None
            if parent:
                qname = parent.qname + '::' + name
            else:
                ns = '::'.join(ns_stack)
                qname = (ns + '::' + name) if ns else name
            # Strip the fable::defs prefix: everything is inside that namespace.
            qname = qname.replace('fable::defs::', '')
            cls = CClass(name, qname, bases, parent)
            classes.append(cls)
            class_stack.append(cls)
            continue
        continue
    # Nested enum: skip its body.
    if re.match(r'^enum\s+', line):
        if i < len(lines) and lines[i].strip() == '{':
            i += 1
            depth = 1
            while i < len(lines) and depth > 0:
                depth += lines[i].count('{') - lines[i].count('}')
                i += 1
        continue
    # Member line.
    if class_stack and line.endswith(';') and not line.startswith('using ') \
            and not line.startswith('typedef ') and not line.startswith('friend '):
        member = split_member(line)
        if member:
            decl, name, bit = member
            class_stack[-1].fields.append((decl, name, bit))

# Any classes left on the stack at EOF are a parse failure.
if class_stack or ns_stack:
    print('unbalanced parse:', [c.qname for c in class_stack], ns_stack, file=sys.stderr)
    sys.exit(1)

# Emit.
out = []
out.append('// Field reflection for the def data model. Generated from classes.hpp.')
out.append('#ifndef FABLE_DEFS_VISIT_HPP')
out.append('#define FABLE_DEFS_VISIT_HPP')
out.append('')
out.append('#include <cstdint>')
out.append('#include <map>')
out.append('#include <memory>')
out.append('#include <string>')
out.append('#include <type_traits>')
out.append('#include <utility>')
out.append('#include <vector>')
out.append('')
out.append('#include <fable/defs.hpp>')
out.append('')
out.append('namespace fable {')
out.append('namespace defs {')
out.append('namespace detail {')
out.append('')
out.append('/// A mutable handle to a bit-field, which cannot bind to a reference.')
out.append('template <class T>')
out.append('class BitfieldRef {')
out.append('public:')
out.append('    using Getter = bool (*)(const T &);')
out.append('    using Setter = void (*)(T &, bool);')
out.append('    BitfieldRef(T &value, Getter get, Setter set)')
out.append('        : value_(&value), get_(get), set_(set) {}')
out.append('    bool get() const { return get_(*value_); }')
out.append('    void set(bool value) { set_(*value_, value); }')
out.append('')
out.append('private:')
out.append('    T *value_;')
out.append('    Getter get_;')
out.append('    Setter set_;')
out.append('};')
out.append('')
out.append('template <class T, class Get, class Set>')
out.append('BitfieldRef<T> bitfield(T &value, Get get, Set set) {')
out.append('    return BitfieldRef<T>(value, get, set);')
out.append('}')
out.append('')
out.append('} // namespace detail')
out.append('')
out.append('/// Whether a type is one of the observed model classes.')
out.append('template <class T> struct is_model_class : std::false_type {};')
for c in classes:
    out.append(f'template <> struct is_model_class<{c.qname}> : std::true_type {{}};')
out.append('')

declared = set()
for c in classes:
    if c.qname in declared:
        continue
    declared.add(c.qname)
    out.append(f'template <class V> void visit_fields({c.qname} &value, V &&visitor);')
out.append('')

for c in classes:
    out.append('template <class V>')
    out.append(f'void visit_fields({c.qname} &value, V &&visitor) {{')
    out.append('    (void)value;')
    out.append('    (void)visitor;')
    for base in c.bases:
        if any(x.qname == base for x in classes):
            out.append(f'    visit_fields(static_cast<{base} &>(value), visitor);')
    for decl, name, bit in c.fields:
        if bit:
            out.append(f'    visitor("{name}", detail::bitfield(value,')
            out.append(f'        [](const {c.qname} &o) {{ return bool(o.{name}); }},')
            out.append(f'        []({c.qname} &o, bool v) {{ o.{name} = v; }}));')
        else:
            out.append(f'    visitor("{name}", value.{name});')
    out.append('}')
    out.append('')

out.append('} // namespace defs')
out.append('} // namespace fable')
out.append('')
out.append('#endif // FABLE_DEFS_VISIT_HPP')
out.append('')

open(OUT, 'w').write('\n'.join(out))
print(f'wrote {OUT}: {len(classes)} classes, {sum(len(c.fields) for c in classes)} fields')


# ── class registry for the CLI ──────────────────────────────────────────────
REG = 'cpp/fable-defs-compiler/include/fable/defs/compiler/model_registry.hpp'
reg = []
reg.append('// Runtime class lookup for the CLI. Generated from classes.hpp.')
reg.append('#ifndef FABLE_DEFS_COMPILER_MODEL_REGISTRY_HPP')
reg.append('#define FABLE_DEFS_COMPILER_MODEL_REGISTRY_HPP')
reg.append('')
reg.append('#include <string>')
reg.append('#include <utility>')
reg.append('')
reg.append('#include <fable/defs.hpp>')
reg.append('')
reg.append('namespace fable::defs::compiler {')
reg.append('')
reg.append('/// Call `f` with a default-constructed instance of the named model class.')
reg.append('template <class F>')
reg.append('bool with_class(const std::string &name, F &&f) {')
seen = set()
for c in classes:
    if c.qname in seen:
        continue
    seen.add(c.qname)
    reg.append(f'    if (name == "{c.qname}") {{')
    reg.append(f'        f(::fable::defs::{c.qname}{{}});')
    reg.append('        return true;')
    reg.append('    }')
reg.append('    return false;')
reg.append('}')
reg.append('')
reg.append('} // namespace fable::defs::compiler')
reg.append('')
reg.append('#endif // FABLE_DEFS_COMPILER_MODEL_REGISTRY_HPP')
reg.append('')
open(REG, 'w').write('\n'.join(reg))
print(f'wrote {REG}: {len(seen)} classes')
