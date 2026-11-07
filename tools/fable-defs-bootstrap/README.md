# fable-defs-bootstrap

A one-time bootstrap tool. It reads a `decomp-cli tree` output from
[`fable-decomp-engine`](https://github.com/jamen/fable-decomp-engine)
(`~/git/fable-decomp-engine`) and writes a single cleaned C++ header with the
original Fable def classes.

The maintained C++ package lives in [`cpp/fable-defs`](../../cpp/fable-defs).
This tool is not part of it; it exists so the initial extraction can be
reproduced if the decompiler or the PDB changes.

## Run

```bash
# Let the tool run the decompiler.
cargo run --manifest-path tools/fable-defs-bootstrap/Cargo.toml -- \
  --decomp-cli ~/git/fable-decomp-engine/target/release/decomp-cli

# Or clean an existing tree.
cargo run --manifest-path tools/fable-defs-bootstrap/Cargo.toml -- \
  --tree /tmp/fable-tree

# Write elsewhere / inspect.
cargo run --manifest-path tools/fable-defs-bootstrap/Cargo.toml -- \
  --tree /tmp/fable-tree --output /tmp/fable_defs.hpp
cargo run --manifest-path tools/fable-defs-bootstrap/Cargo.toml -- \
  --tree /tmp/fable-tree --stdout | head
```

The default output is `tools/fable-defs-bootstrap/out/fable_defs.hpp`.

## What it does

- Parses every `.hpp` in the tree (classes, structs, enums, nested types).
- Selects classes deriving from `CDefClassBase` plus their bases and field
  dependencies.
- Drops methods, constructors, statics and runtime bases.
- Removes MSVC calling conventions and default template arguments.
- Normalizes `long`/`int` to `<cstdint>` types.
- Writes one header.

The output is a starting point for the curated package, not the package itself.
The curated package rewrites the aliases to standard C++ types and adds
CMake packaging, tests and documentation.

## Requirements

The decompiler needs `FableWin.exe` and `FableWin.pdb`. They are not part of
this repository.
