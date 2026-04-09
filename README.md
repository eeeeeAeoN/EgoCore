# EgoCore - Asset Bank Editor for Fable

EgoCore is the culmination of over twenty years of obsession with the inner workings of Fable. What began as programming-related curiosity has evolved into an intensive development journey to provide the community with a modern, robust, and versatile modding framework.

I believe the tools to keep this game alive should be accessible to everyone. That is why EgoCore is, and will always be, Free and Open Source.

## ✨ Key Features

* **Full Bank Editing & Recompilation:** Deep dive into Fable's archive formats (`.big`, `.lut`, `.lug`). **EACH parsed bank can be fully edited and recompiled from the ground up.**
* **3D Mesh & GLTF Pipeline:** Integrated mesh rendering and full bidirectional GLTF import/export support. Bring modern 3D models into Fable or extract vanilla assets flawlessly.
* **Definition Editor & Compiler:** A powerful built-in workspace for viewing, editing, and compiling game definitions, featuring automatic lookup dictionary generation.
* **Fable Script Extender (FSE) Support:** A dedicated environment bridging raw `.lua` scripts with the internal Fable API, complete with smart autosuggest.
* **Advanced Audio Tooling:** Built-in LUG and MET parsing for exploring, extracting, and modifying Fable's complex audio banks.
* **Mod Manager & Creator:** Package your custom modifications cleanly, manage your load order, and safely deploy them without permanently destroying vanilla files.
* **WAD Decompiler:** Fully automated extraction of `FinalAlbion.wad` into its raw `.lev` and `.tng` components.

## 🚀 Getting Started

0. **Set up the Unified Build of Fable (Critical):** Weather you're a modder or user it's highly recommended to get the Unified Build of Fable. Without it the .def editing, compilation and mods that use plan text definitions won't work. Check the UnifiedBuildSetup.pdf file in the archive for more details.
1. **Extract the Archive:** Unpack the EgoCore folder anywhere on your PC. 
2. **Launch:** Run `EgoCore.exe`.
3. **Setup:** On your first launch, EgoCore will ask for your Fable installation directory (the folder containing `Fable.exe` and the `Data` folder).
4. **Decompile FinalAlbion (Recommended):** If prompted, allow EgoCore to decompile the master WAD file. This is crucial for seamless map editing.

*Fun fact: You can change the EgoCore font, by replacing the Font.ttf file with a .ttf true type font file of your own.

## ⚙️ Requirements

* A clean installation of Fable: The Lost Chapters.
* Windows 10 / 11 (64-bit).

## 🤝 Community & Support

Developing a tool of this scale involves hundreds of hours of reverse engineering, debugging, and refinement. I don't believe in paywalling progress, so I rely entirely on the generosity of the community to keep this project sustainable. 

If EgoCore has saved you time or helped you bring a new vision to life, please consider supporting the project or joining the community:

* ☕ **Support the Project:** [Ko-fi](https://ko-fi.com/aeon5798)
* 🐛 **Report Bugs & View Source:** [GitHub Repository](https://github.com/eeeeeAeoN/EgoCore)
* 💬 **Join the Conversation:** [Fable Modding Discord](https://discord.gg/Rw4as5ar3S)
* 📚 **Learn to Mod:** [Official EgoCore Wiki](Coming Soon - Hopefully)

## 📄 License

This software is provided as-is, free and open-source. Please see the `LICENSE` file for more details. 

*Fable is a registered trademark of Microsoft Corporation. EgoCore is an unofficial, community-driven project and is not affiliated with or endorsed by Microsoft or Lionhead Studios.*
