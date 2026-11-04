{
  description = "fable-defs — a from-scratch def compiler for Fable: The Lost Chapters";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    rust-overlay.url = "github:oxalica/rust-overlay";
  };

  outputs =
    { nixpkgs, rust-overlay, ... }:
    let
      system = "x86_64-linux";
      overlays = [ (import rust-overlay) ];
      pkgs = import nixpkgs {
        inherit system overlays;
      };

      # Built from `minimal` (rustc + cargo + host std) plus exactly the
      # components asked for, rather than `default` — `default` also carries
      # rustdoc, rust-gdb and rust-lldb, which nothing here uses.
      #
      # The host std (x86_64-unknown-linux-gnu) always comes along: build scripts
      # and the defs-derive proc macro compile for the host, whatever the final
      # target is.
      rustWith =
        extensions:
        pkgs.rust-bin.stable.latest.minimal.override {
          inherit extensions;
          targets = [
            # MSVC ABI only. def-compiler-sys ships a static library and the
            # consumer (EgoCore) is an MSVC v143 x64 project; mingw archives do
            # not link cleanly into one, so there is no reason to carry a
            # windows-gnu target or a mingw toolchain here.
            "x86_64-pc-windows-msvc"
            # Release builds of `defc` for Linux. The default gnu target bakes
            # the absolute path of *this shell's* glibc loader into the ELF
            # interpreter (a /nix/store/... path), so such a binary cannot start
            # on any machine that lacks that exact store path — it fails with a
            # misleading "no such file or directory". musl links libc statically
            # and needs no interpreter at all, so the artifact is portable.
            # The workspace is pure Rust (miniz_oxide, no libz-sys), so this
            # needs no C cross-toolchain.
            "x86_64-unknown-linux-musl"
          ];
        };

      # Cross-links the MSVC target from Linux: fetches the Windows SDK and CRT
      # import libraries and drives lld-link. See packages/def-compiler-sys.
      crossTools = with pkgs; [
        cargo-xwin
        llvmPackages.bintools
      ];

      shellHook = ''
        # Keep the downloaded Windows SDK out of $HOME and inside the repo.
        export XWIN_CACHE_DIR="$PWD/target/xwin"
      '';
    in
    {
      devShells.${system} = {
        # Interactive development.
        default = pkgs.mkShell {
          inherit shellHook;
          buildInputs = [
            (rustWith [
              "clippy"
              "rustfmt"
              "rust-src" # rust-analyzer resolves std through this
              "rust-analyzer"
              "rust-docs"
            ])
          ] ++ crossTools;
        };

        # What CI runs: `nix develop .#ci`. Only what fmt, clippy, test and the
        # two release builds need — no analyzer, no offline docs, no debuggers.
        # Kept in the same file as `default` so the two cannot drift apart in
        # rustc version or target list.
        ci = pkgs.mkShell {
          inherit shellHook;
          buildInputs = [
            (rustWith [
              "clippy"
              "rustfmt"
            ])
          ] ++ crossTools;
        };
      };
    };
}
