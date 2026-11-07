#!/usr/bin/env python3
"""Generate the text def-type -> C++ class registry from the Rust DefBody table."""
import re

BINARY = 'packages/defs/src/binary.rs'
CLASSES = 'cpp/fable-defs/include/fable/defs/classes.hpp'
OUT = 'cpp/fable-defs-compiler/include/fable/defs/compiler/type_registry.hpp'

classes = set()
for line in open(CLASSES, encoding='utf-8'):
    m = re.match(r'^class ([A-Za-z_]\w*)\b', line)
    if m:
        classes.add(m.group(1))

ui = {
    'UiDef': 'NUISystem::CUIDef',
    'UiIconsDef': 'NUISystem::CUIIconsDef',
    'UiLocaleGraphicsDef': 'NUISystem::CUILocaleGraphicsDef',
    'UILocaleGraphicsDef': 'NUISystem::CUILocaleGraphicsDef',
    'UiMiscThingsDef': 'NUISystem::CUIMiscThingsDef',
    'UiStateDef': 'NUISystem::CUIStateDef',
}

entries = []
for line in open(BINARY, encoding='utf-8'):
    m = re.match(r'^\s+\w+\((\w+)\) => \[(.*)\],\s*$', line)
    if not m:
        continue
    rust_type = m.group(1)
    aliases = [a.strip().strip('"') for a in m.group(2).split(',') if a.strip()]
    cpp = ui.get(rust_type, 'C' + rust_type)
    simple = cpp.split('::')[-1]
    if simple not in classes:
        continue
    for alias in aliases:
        entries.append((alias, cpp))

entries = sorted(set(entries))
lines = [
    '// Text def type -> C++ model class. Generated from the Rust DefBody table.',
    '#ifndef FABLE_DEFS_COMPILER_TYPE_REGISTRY_HPP',
    '#define FABLE_DEFS_COMPILER_TYPE_REGISTRY_HPP',
    '',
    '#include <string>',
    '',
    '#include <fable/defs.hpp>',
    '',
    'namespace fable::defs::compiler {',
    '',
    'template <class F>',
    'bool with_class_for_type(const std::string &type, F &&f) {',
]
for alias, cpp in entries:
    lines.append(f'    if (type == "{alias}") {{')
    lines.append(f'        f(::fable::defs::{cpp}{{}});')
    lines.append('        return true;')
    lines.append('    }')
lines += [
    '    return false;',
    '}',
    '',
    'inline const char *class_name_for_type(const std::string &type) {',
]
for alias, cpp in entries:
    lines.append(f'    if (type == "{alias}") return "{cpp}";')
lines += [
    '    return nullptr;',
    '}',
    '',
    '} // namespace fable::defs::compiler',
    '',
    '#endif // FABLE_DEFS_COMPILER_TYPE_REGISTRY_HPP',
    '',
]
open(OUT, 'w').write('\n'.join(lines))
print(f'wrote {OUT}: {len(entries)} aliases')
