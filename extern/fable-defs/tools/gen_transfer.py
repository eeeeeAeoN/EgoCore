#!/usr/bin/env python3
"""Generate transfer reflection from `decomp-cli lift` Transfer bodies.

Input:  /tmp/fable-transfer-lifts/<Class>.txt (or --lifts DIR)
Output: cpp/fable-defs/include/fable/defs/transfer.hpp

The lifted Transfer body is the authoritative wire field list: wire name,
member path, transfer kind, and default. This tool turns it into:

  * visit_transfer_fields(Class&, visitor) — exact wire names/members
  * apply_transfer_defaults(Class&)        — explicit Transfer defaults
"""
import argparse
import glob
import os
import re
import sys

MODEL = 'cpp/fable-defs/include/fable/defs/classes.hpp'
OUT = 'cpp/fable-defs/include/fable/defs/transfer.hpp'


def angle_close(s, start):
    depth = 0
    for i in range(start, len(s)):
        if s[i] == '<':
            depth += 1
        elif s[i] == '>':
            depth -= 1
            if depth == 0:
                return i
    return -1


def split_args(s):
    out, cur, depth = [], '', 0
    prev = ''
    for ch in s:
        if ch in '<(':
            depth += 1
        elif ch == '>' and prev != '-':
            depth -= 1
        elif ch == ')':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur.strip())
            cur = ''
        else:
            cur += ch
        prev = ch
    if cur.strip():
        out.append(cur.strip())
    return out


def parse_call(line):
    """Return (function, template_args, args) for a `<context>.<fn>` call."""
    m = re.search(r'\b[A-Za-z_]\w*\.(Transfer[A-Za-z_]*)', line)
    if not m:
        return None
    fn = m.group(1)
    pos = m.end()
    while pos < len(line) and line[pos].isspace():
        pos += 1
    template_args = ''
    if pos < len(line) and line[pos] == '<':
        close = angle_close(line, pos)
        if close < 0:
            return None
        template_args = line[pos + 1:close].strip()
        pos = close + 1
    while pos < len(line) and line[pos].isspace():
        pos += 1
    if pos >= len(line) or line[pos] != '(':
        return None
    depth = 0
    close = -1
    for i in range(pos, len(line)):
        if line[i] == '(':
            depth += 1
        elif line[i] == ')':
            depth -= 1
            if depth == 0:
                close = i
                break
    if close < 0:
        return None
    args = split_args(line[pos + 1:close])
    return fn, template_args, args


def strip_member(expr):
    expr = expr.strip()
    pointer = False
    if expr.startswith('*'):
        pointer = True
        expr = expr[1:].strip()
    if expr.startswith('this->'):
        expr = expr[len('this->'):]
    expr = expr.replace('->', '.')
    return pointer, expr


def parse_lift(path):
    text = open(path, encoding='utf-8', errors='replace').read()
    m = re.search(r'void\s+__thiscall\s+([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)::Transfer\s*\(', text)
    if not m:
        return None
    cls = m.group(1)
    fields = []
    base = None
    temp_member = None
    aliases = {}
    temps = {}
    for line in text.splitlines():
        line = line.strip()
        m = re.match(r'(?:bool\s+)?temp\s*=\s*this->([A-Za-z_]\w*(?:\.\w+)*)\s*;', line)
        if m:
            temp_member = m.group(1)
            continue
        # `CCombatAbilityData &base = this->CombatAbility;`
        m = re.match(r'[A-Za-z_][\w:<> ,]*?\s*&\s*([A-Za-z_]\w*)\s*=\s*this->([A-Za-z_]\w*(?:\.\w+)*)\s*;', line)
        if m:
            aliases[m.group(1)] = m.group(2)
            continue
        # `long flags = this->Flags;` / `bool temp = this->Short;`
        m = re.match(r'([A-Za-z_][\w:<> ]*)\s+([A-Za-z_]\w*)\s*=\s*this->([A-Za-z_]\w*(?:\.\w+)*)\s*;', line)
        if m and not line.startswith('this->'):
            aliases[m.group(2)] = m.group(3)
            if m.group(1).strip() == 'bool' and m.group(2) == 'temp':
                temp_member = m.group(3)
            continue
        # `this->Member = std::set<...>(temp.begin(), temp.end());`
        m = re.search(r'this->([A-Za-z_]\w*(?:\.\w+)*)\s*=\s*std::set<.*?>\(\s*([A-Za-z_]\w*)\.begin\(\)', line)
        if m:
            temps[m.group(2)] = m.group(1)
            continue
        # `temp.vector<...>(this->Member.begin(), ...);`
        m = re.search(r'([A-Za-z_]\w*)\.vector<.*?>\(\s*this->([A-Za-z_]\w*(?:\.\w+)*)\.begin\(\)', line)
        if m:
            temps[m.group(1)] = m.group(2)
            continue
        m = re.search(r'(?:this->)?([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)::Transfer\s*\(\s*persist\s*\)\s*;', line)
        if m:
            base = m.group(1)
            continue
        call = parse_call(line)
        if not call:
            continue
        fn, template_args, args = call
        if len(args) < 2:
            continue
        name = args[0].strip()
        if name.startswith('"') and name.endswith('"'):
            name = name[1:-1]
        else:
            continue
        member_expr = args[1]
        default = args[2].strip() if len(args) >= 3 else None
        pointer, member = strip_member(member_expr)
        bitfield = False
        if member == 'temp' and temp_member:
            member = temp_member
            bitfield = True
        elif member in aliases:
            member = aliases[member]
        elif member in temps:
            member = temps[member]
        if not member:
            continue
        unresolved = False
        for index_text in re.findall(r'\[([^\]]+)\]', member):
            if not index_text.strip().isdigit():
                unresolved = True
                break
        if unresolved:
            continue
        kind = {
            'Transfer': 'Value',
            'TransferVector': 'Vector',
            'TransferMap': 'Map',
            'TransferSubComponent': 'SubComponent',
            'TransferObject': 'Object',
            'TransferVectorOfSubComponents': 'VectorOfSubComponents',
            'TransferVectorOfObjects': 'VectorOfObjects',
            'TransferMultiMap': 'Map',
        }.get(fn, fn)
        fields.append({
            'wire': name,
            'member': member,
            'kind': kind,
            'type': template_args,
            'default': default,
            'pointer': pointer,
            'bitfield': bitfield,
        })
    # Transfer bodies sometimes contain a real duplicate wire control
    # (`CScriptDef::Transfer` emits TemplePrayerFactorHighest twice). Branchy
    # save/load conversions produce accidental duplicates instead, so only the
    # explicitly known duplicate is preserved.
    special_duplicates = {
        'CScriptDef': {'TemplePrayerFactorHighest': 'TemplePrayerFactorHighest2'},
        'CHeroExperienceDef': {'StatIncreasePerHitType': 'StatIncreasePerHitType2'},
        'CVillagePeopleDef': {'MaleTeacher': 'FemaleTeacher'},
    }
    deduped = {}
    out_fields = []
    for field in fields:
        wire = field['wire']
        if wire in deduped:
            if cls in special_duplicates and wire in special_duplicates[cls]:
                field = dict(field)
                field['member'] = special_duplicates[cls][wire]
                out_fields.append(field)
            continue
        deduped[wire] = field
        out_fields.append(field)
    return {'class': cls, 'base': base, 'fields': out_fields}


def model_classes(path):
    classes = set()
    ns = []
    stack = []
    for line in open(path, encoding='utf-8'):
        line = line.strip()
        m = re.match(r'namespace\s+([A-Za-z_]\w*)\s*\{$', line)
        if m:
            ns.append(m.group(1))
            stack.append('ns')
            continue
        if line.startswith('}'):
            if stack:
                kind = stack.pop()
                if kind == 'ns':
                    ns.pop()
            continue
        m = re.match(r'^(?:class|struct)\s+([A-Za-z_]\w*)\s*(?::\s*[^{]*)?$', line)
        if m and ';' not in line:
            # Next non-empty line is `{` for a definition.
            name = m.group(1)
            parent = next((s for s in reversed(stack) if s != 'ns'), None)
            # stack entries are strings; class nesting is not tracked separately here
            if ns:
                q = '::'.join(ns) + '::' + name
            else:
                q = name
            for prefix in ('fable::defs::', 'fable::', 'defs::'):
                if q.startswith(prefix):
                    q = q[len(prefix):]
                    break
            classes.add(q)
            stack.append('class')
            continue
    return classes


def load_symbols():
    symbols = {}
    for path in ('cpp/fable-defs/include/fable/defs/enums.hpp',
                 'cpp/fable-defs/include/fable/defs/classes.hpp'):
        try:
            text = open(path, encoding='utf-8').read()
        except OSError:
            continue
        for m in re.finditer(r'\b([A-Z][A-Z0-9_]+)\s*=\s*(-?\d+)\b', text):
            symbols[m.group(1)] = int(m.group(2))
    return symbols


SYMBOLS = load_symbols()


def cpp_default(default, member):
    if default is None:
        return None
    d = default.strip()
    if d == 'true' or d == 'false':
        return d
    target = 'value.' + member
    if d.startswith('this->'):
        source = 'value.' + d[len('this->'):].replace('->', '.')
        if source == target:
            return None
        return source
    if re.fullmatch(r'-?\d+(?:\.\d+)?[fFuUlL]*', d):
        return f'static_cast<decltype({target})>({d})'
    if d == 'NULL':
        return f'static_cast<decltype({target})>(0)'
    simple = d.split('::')[-1]
    if simple in SYMBOLS:
        return f'static_cast<decltype({target})>({SYMBOLS[simple]})'
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--lifts', default='/tmp/fable-transfer-lifts')
    ap.add_argument('--model', default=MODEL)
    ap.add_argument('--out', default=OUT)
    args = ap.parse_args()

    valid = model_classes(args.model)
    records = {}
    for path in sorted(glob.glob(os.path.join(args.lifts, '*.txt'))):
        record = parse_lift(path)
        if not record or record['class'] not in valid:
            continue
        records[record['class']] = record

    out = []
    out.append('// Wire transfer reflection, generated from lifted Transfer bodies.')
    out.append('// Do not edit by hand; see tools/gen_transfer.py.')
    out.append('#ifndef FABLE_DEFS_TRANSFER_HPP')
    out.append('#define FABLE_DEFS_TRANSFER_HPP')
    out.append('')
    out.append('#include <memory>')
    out.append('#include <type_traits>')
    out.append('#include <utility>')
    out.append('')
    out.append('#include <fable/defs.hpp>')
    out.append('#include <fable/defs/visit.hpp>')
    out.append('')
    out.append('namespace fable {')
    out.append('namespace defs {')
    out.append('')
    out.append('enum class TransferKind {')
    out.append('    Value,')
    out.append('    Vector,')
    out.append('    Map,')
    out.append('    SubComponent,')
    out.append('    Object,')
    out.append('    VectorOfSubComponents,')
    out.append('    VectorOfObjects,')
    out.append('};')
    out.append('')
    out.append('template <class T>')
    out.append('T &transfer_ref(std::shared_ptr<T> &ptr) {')
    out.append('    if (!ptr) {')
    out.append('        ptr = std::make_shared<T>();')
    out.append('    }')
    out.append('    return *ptr;')
    out.append('}')
    out.append('')
    out.append('template <class T, class V>')
    out.append('void visit_transfer_fields(T &value, V &&visitor);')
    out.append('')
    out.append('template <class T>')
    out.append('void apply_transfer_defaults(T &value) {')
    out.append('    (void)value;')
    out.append('}')
    out.append('')
    out.append('template <class T> struct has_transfer : std::false_type {};')
    for name in sorted(records):
        out.append(f'template <> struct has_transfer<{name}> : std::true_type {{}};')
    out.append('')
    for name in sorted(records):
        out.append(f'template <class V> void visit_transfer_fields({name} &value, V &&visitor);')
    out.append('')

    for name, record in sorted(records.items()):
        out.append('template <class V>')
        out.append(f'void visit_transfer_fields({name} &value, V &&visitor) {{')
        out.append('    (void)value;')
        out.append('    (void)visitor;')
        if record['base']:
            out.append(f'    if constexpr (has_transfer<{record["base"]}>::value) {{')
            out.append(f'        visit_transfer_fields(static_cast<{record["base"]} &>(value), visitor);')
            out.append('    } else {')
            out.append(f'        visit_fields(static_cast<{record["base"]} &>(value), visitor);')
            out.append('    }')
        for f in record['fields']:
            member = 'value.' + f['member']
            if f['wire'] == 'CombatAbility' and name.startswith('CCombatAbility'):
                member = ('static_cast<::fable::defs::CCombatAbilityData &>('
                          + member + ')')
            if f['pointer']:
                member = f'transfer_ref({member})'
            if f['bitfield']:
                out.append(f'    visitor("{f["wire"]}", ::fable::defs::detail::bitfield(value,')
                out.append(f'        [](const {name} &o) {{ return bool(o.{f["member"]}); }},')
                out.append(f'        []({name} &o, bool v) {{ o.{f["member"]} = v; }}));')
            else:
                out.append(f'    visitor("{f["wire"]}", {member});')
        if name == 'COpinionSourceDef':
            out.append('    for (std::size_t i = 0; i < 79; ++i) {')
            out.append('        visitor("BinaryReaction", value.ReactionFlags[i]);')
            out.append('    }')
            out.append('    for (std::size_t i = 0; i < 5 && i < value.OpinionProduced.OpinionValues.size(); ++i) {')
            out.append('        visitor("BinaryOpinion", value.OpinionProduced.OpinionValues[i]);')
            out.append('    }')
        out.append('}')
        out.append('')

    for name, record in sorted(records.items()):
        defaults = []
        for f in record['fields']:
            expr = cpp_default(f['default'], f['member'])
            if expr is None:
                continue
            if f['bitfield']:
                defaults.append(f'    value.{f["member"]} = {expr};')
            else:
                defaults.append(f'    value.{f["member"]} = {expr};')
        out.append(f'template <>')
        out.append(f'inline void apply_transfer_defaults<{name}>({name} &value) {{')
        out.append('    (void)value;')
        if record['base']:
            out.append(f'    apply_transfer_defaults(static_cast<{record["base"]} &>(value));')
        out.extend(defaults)
        out.append('}')
        out.append('')

    out.append('template <class T, class V>')
    out.append('void visit_model_fields(T &value, V &&visitor) {')
    out.append('    if constexpr (has_transfer<T>::value) {')
    out.append('        visit_transfer_fields(value, visitor);')
    out.append('    } else {')
    out.append('        visit_fields(value, visitor);')
    out.append('    }')
    out.append('}')
    out.append('')
    out.append('template <class T>')
    out.append('void apply_model_defaults(T &value) {')
    out.append('    apply_transfer_defaults(value);')
    out.append('}')
    out.append('')
    out.append('} // namespace defs')
    out.append('} // namespace fable')
    out.append('')
    out.append('#endif // FABLE_DEFS_TRANSFER_HPP')
    out.append('')

    open(args.out, 'w').write('\n'.join(out))
    print(f'wrote {args.out}: {len(records)} transfer classes')


if __name__ == '__main__':
    main()
