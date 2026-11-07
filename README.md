# fable-defs

This project is for [Fable (2004 video game)](https://en.wikipedia.org/wiki/Fable_(2004_video_game))'s definitions, also called defs.

The defs are game data that specify things like item values, game behavior, and so on. They're edited as text files (.def) and compiled to binary files (.bin).

This project has a def compiler, a Rust library for reading and writing defs, and a CLI tool (see the [Releases](https://github.com/jamen/fable-defs/releases) page for a download). It's also integrated into the great [EgoCore](https://github.com/eeeeeAeoN/EgoCore) modding tool.

A [`fable-defs-cpp`](cpp/fable-defs) package provides the original def classes as a header-only C++ library, recovered once with `fable-decomp-engine` and then curated. A complete C++ compiler port, including the EgoCore embedding API, lives in [`cpp/`](cpp/).

Come visit the [Fable Modders' Guild](https://discord.gg/xUYzkCw) Discord server to discuss the project. 

**AI notice**: coding agents have been used.

## C++ build

The C++ model and compiler port use CMake presets from the repository root:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

On Windows, `cmake --preset vs2022` generates a Visual Studio solution and the
build presets drive MSBuild. See [`cpp/README.md`](cpp/README.md).
