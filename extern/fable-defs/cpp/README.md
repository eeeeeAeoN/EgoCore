# C++ port

Two C++ projects:

| Project | Role |
|---|---|
| [`fable-defs`](fable-defs) | The def data model: original class/enum declarations, header-only |
| [`fable-defs-compiler`](fable-defs-compiler) | Text parser, symbol table, and the compiler pipeline |

The Rust workspace in `packages/` remains the reference compiler. The C++
`fable-defc build` command emits all four retail binaries. Current parity
against the Rust build:

| Binary | Status |
|---|---|
| `script.bin` | byte-identical after decompression |
| `names.bin` | byte-identical |
| `frontend.bin` | matches the Rust build (one retail-only difference in `UI PARTICLE_TEST`) |
| `game.bin` | same 13,239 entries; named semantic ledger matches the Rust build except one inventory field the port currently reproduces correctly |

The C++ writer currently uses valid stored DEFLATE blocks rather than zlib
level-1, so the compressed bytes are larger while the decompressed entry bodies
are the parity target above.


## Build

The root project wires both packages together. There is no `build.sh`; the
presets in `CMakePresets.json` are the entry point.

```bash
cmake --preset debug          # or release
cmake --build --preset debug
ctest --preset debug

build/cpp-debug/bin/fable-defc build ~/fable-ref/Data/Defs /tmp/fable-out
```

On Windows, `cmake --preset vs2022` generates a Visual Studio solution; the
build/test presets drive MSBuild through CMake:

```powershell
cmake --preset vs2022
cmake --build --preset vs2022-release
ctest --preset vs2022-release
```

To invoke MSBuild directly, use the generated solution:

```powershell
msbuild build\vs2022\fable-defs-workspace.sln /p:Configuration=Release /m
```

CMake is the cross-platform build; the VS generator exists so the same sources
can be built by Visual Studio/MSBuild when that is the preferred environment.

## Embedding for EgoCore

EgoCore includes `fable/defs/compiler/build.hpp` and links the
`fable-defs-compiler` static library (or compiles its sources directly):

```cpp
#include "fable/defs/compiler/build.hpp"

auto report = fable::defs::compiler::build_directory(inputDir, outputDir);
if (!report.ok) {
    // report.log is the rendered diagnostic text
}
```

`build_directory` performs discovery, parsing, symbol evaluation and all four
binary writes; it never prints to stdout or stderr.
