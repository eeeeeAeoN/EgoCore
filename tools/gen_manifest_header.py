#!/usr/bin/env python3
"""Generate the C++ manifest header from the Rust manifest lists."""
import re

SRC = 'packages/def-compiler/src/manifest.rs'
OUT = 'cpp/fable-defs-compiler/include/fable/defs/compiler/manifest.hpp'

text = open(SRC, encoding='utf-8').read()
binary_text = open('packages/defs/src/binary.rs', encoding='utf-8').read()


def array(name):
    m = re.search(rf'pub const {name}: &\[&str\] = &\[(.*?)\];', text, re.S)
    if not m:
        return []
    body = re.sub(r'//.*', '', m.group(1))
    return re.findall(r'"([^"]+)"', body)


frontend_files = array('FRONTEND_DEF_FILES')
game = array('GAME_NULLDEF_ENTRIES')
frontend = array('FRONTEND_NULLDEF_ENTRIES')
script = array('SCRIPT_NULLDEF_ENTRIES')

out = [
    '// Generated from packages/def-compiler/src/manifest.rs.',
    '#ifndef FABLE_DEFS_COMPILER_MANIFEST_HPP',
    '#define FABLE_DEFS_COMPILER_MANIFEST_HPP',
    '',
    '#include <array>',
    '#include <cstddef>',
    '#include <cstdint>',
    '#include <string_view>',
    '',
    'namespace fable::defs::compiler {',
    '',
    'enum class BinaryKind { Game, Frontend, Script };',
    '',
    'struct BinaryHeaderInfo {',
    '    bool use_names_bin;',
    '    std::uint32_t file_indicator;',
    '    std::uint32_t platform_indicator;',
    '};',
    '',
]
out.append('inline constexpr const char *FRONTEND_DEF_FILES[] = {')
for item in frontend_files:
    out.append(f'    "{item}",')
out.append('};')
out.append(f'inline constexpr std::size_t FRONTEND_DEF_FILE_COUNT = {len(frontend_files)};')
out.append('')

for name, items in [('GAME', game), ('FRONTEND', frontend), ('SCRIPT', script)]:
    out.append(f'inline constexpr const char *{name}_NULLDEF_ENTRIES[] = {{')
    for item in items:
        out.append(f'    "{item}",')
    out.append('};')
    out.append(f'inline constexpr std::size_t {name}_NULLDEF_COUNT = {len(items)};')
    out.append('')

def header_info(name):
    m = re.search(rf'pub const {name}: DefBinaryHeader = DefBinaryHeader \{{(.*?)\}};', text, re.S)
    body = m.group(1) if m else ''
    use = re.search(r'use_names_bin:\s*(true|false)', body)
    file_ind = re.search(r'file_indicator:\s*(0x[0-9A-Fa-f]+|\d+)', body)
    plat = re.search(r'platform_indicator:\s*(0x[0-9A-Fa-f]+|\d+)', body)
    return (use.group(1) if use else 'false',
            file_ind.group(1) if file_ind else '0',
            plat.group(1) if plat else '0')

for name, cname in [('GAME_HEADER', 'GAME'), ('FRONTEND_HEADER', 'FRONTEND'), ('SCRIPT_HEADER', 'SCRIPT')]:
    use, file_ind, plat = header_info(name)
    out.append(f'inline constexpr BinaryHeaderInfo {cname}_BINARY_HEADER = {{')
    out.append(f'    {use}, {file_ind}, {plat},')
    out.append('};')
    out.append('')

m = re.search(r'pub const NAMES_HEADER_BYTES: \[u8; 20\] = \[(.*?)\];', text, re.S)
name_bytes = re.findall(r'0x([0-9A-Fa-f]{2})', m.group(1)) if m else []
out.append('inline constexpr std::array<std::uint8_t, 20> NAMES_HEADER_BYTES = {')
for i in range(0, len(name_bytes), 8):
    out.append('    ' + ', '.join('0x' + b for b in name_bytes[i:i+8]) + ',')
out.append('};')
out.append('')

m = re.search(r'sub_def_names! \{(.*?)\}', binary_text, re.S)
sub_names = re.findall(r'"([^"]+)"', m.group(1)) if m else []
out.append('inline bool has_subdef_table(std::string_view name) {')
for n in sub_names:
    out.append(f'    if (name == "{n}") return true;')
out += [
    '    return false;',
    '}',
    '',
]

out += [
    'inline const char *const *null_entries(BinaryKind kind) {',
    '    switch (kind) {',
    '    case BinaryKind::Game: return GAME_NULLDEF_ENTRIES;',
    '    case BinaryKind::Frontend: return FRONTEND_NULLDEF_ENTRIES;',
    '    case BinaryKind::Script: return SCRIPT_NULLDEF_ENTRIES;',
    '    }',
    '    return GAME_NULLDEF_ENTRIES;',
    '}',
    '',
    'inline std::size_t null_entry_count(BinaryKind kind) {',
    '    switch (kind) {',
    '    case BinaryKind::Game: return GAME_NULLDEF_COUNT;',
    '    case BinaryKind::Frontend: return FRONTEND_NULLDEF_COUNT;',
    '    case BinaryKind::Script: return SCRIPT_NULLDEF_COUNT;',
    '    }',
    '    return GAME_NULLDEF_COUNT;',
    '}',
    '',
    'inline bool is_allowed_type(BinaryKind kind, std::string_view type) {',
    '    const char *const *entries = null_entries(kind);',
    '    for (std::size_t i = 0; i < null_entry_count(kind); ++i) {',
    '        if (type == entries[i]) {',
    '            return true;',
    '        }',
    '    }',
    '    return false;',
    '}',
    '',
    '} // namespace fable::defs::compiler',
    '',
    '#endif // FABLE_DEFS_COMPILER_MANIFEST_HPP',
    '',
]
open(OUT, 'w').write('\n'.join(out))
print(f'wrote {OUT}: game {len(game)}, frontend {len(frontend)}, script {len(script)}')
