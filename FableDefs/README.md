# FableDefs

The native Fable def compiler and C++ def data model.

It compiles the text `.def` / `.tpl` / `.h` corpus into the retail binaries
`game.bin`, `frontend.bin`, `script.bin` and `names.bin`. The same package
contains the compiler library, the `fable-defc` command-line front end, an
in-progress def decompiler, and the generated def model.

## Layout

| Path | Role |
|---|---|
| `include/fable/defs.hpp` | def model entry point |
| `include/fable/defs/` | generated class, enum, transfer and reflection headers |
| `include/fable/defs/compiler/` | lexer, parser, lowering and build pipeline headers |
| `src/` | compiler library sources |
| `tools/defc_main.cpp` | `fable-defc` command-line front end |
| `tools/def_decompiler_main.cpp` | `fable-def-decompiler` front end |
| `tests/` | compiler and model tests |
| `FableDefs.vcxproj` | Windows static-library project used by EgoCore |
| `CMakeLists.txt` | cross-platform library, CLI and test build |

## Build with CMake

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release

build/release/bin/fable-defc build <Defs-dir> <out-dir>
build/release/bin/fable-def-decompiler dump <bin-dir> <defs-dir> <out.def>
build/release/bin/fable-def-decompiler diff <orig-dir> <mod-dir> <defs-dir> <out.def>
```

Debug and Visual Studio presets are also available in `CMakePresets.json`.

## Build with MSBuild

EgoCore references `FableDefs.vcxproj` from `EgoCore.sln`; the static library is
linked into `EgoCore.exe`. The project is x64-only, matching the solution's
existing configuration mapping.

## Embedding API

```cpp
#include "fable/defs/compiler/build.hpp"

auto report = fable::defs::compiler::build_directory(inputDir, outputDir);
if (!report.ok) {
    // report.log contains the rendered diagnostics
}
```

`build_directory` performs discovery, parsing, symbol evaluation and all four
binary writes. It never prints to stdout or stderr.

## Decompiler

The decompiler reads `names.bin` and the compiled binary container directly,
including zlib-compressed chunks. It currently provides:

- zlib/DEFLATE inflate
- binary container and `names.bin` loading
- typed `DefBody` parsing through the generated C++ model
- generic `.def` text emission against each type's NULLDEF defaults
- header/enum symbol resolution (`enum` tables and field-domain training)
- special reconstruction for animations, creature morphs, hero morphs,
  components, weapon trails, appearance graphics, sound maps and special effects
- named-definition diffing between two compiled builds
- in-place patching of existing `.def` files with `specialises` preserved

Still missing relative to the upstream Rust decompiler: a few remaining
bespoke statement forms, and EgoCore UI integration.

## License

[Zlib license](./LICENSE)
