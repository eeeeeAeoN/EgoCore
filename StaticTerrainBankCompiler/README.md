**EgoSTB: Fable Static Terrain Bank Compiler**  
A standalone, high-performance compiler and modern C++ library for creating retail-compatible Fable Static Terrain Bank (.stb) files and companion spatial index maps (.ism).  
EgoSTB reads Fable world definitions (.wld), map levels (.lev), terrain heightmaps, themes, clutter distributions, and materials to build the complete terrain runtime pipeline—including foreground multi-textured patches, procedural water/sea geometry, and multi-tier background LOD quadtrees.  
![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAnEAAAACCAYAAAA3pIp+AAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAANUlEQVR4nO3OMQ2AABAAsSPBCj5fFyM6mJHAjAU2QtIq6DIzW7UHAMBfnGt1V8fXEwAAXrsexOEF35f1aEgAAAAASUVORK5CYII=)  
**Table of Contents**  
- [Quick Start](#anchor-1 "#anchor-1")  
- [How to Run the Compiler](#anchor-2 "#anchor-2")  
  - [Positional Arguments](#anchor-3 "#anchor-3")  
  - [Auto-Detection with egocore_config.ini](#anchor-4 "#anchor-4")  
- [Command-Line Parameters & Defaults](#anchor-5 "#anchor-5")  
  - [Summary of Default Values](#anchor-6 "#anchor-6")  
  - [Detailed Parameter Reference](#anchor-7 "#anchor-7")  
- [Recommended Quality Presets](#anchor-8 "#anchor-8")  
  - [1. Vanilla / Retail Match (Fastest & Smallest)](#anchor-9 "#anchor-9")  
  - [2. Modern Enhanced / Remaster (Recommended for Modding)](#anchor-10 "#anchor-10")  
  - [3. Extreme Distance & Anti-Pop-In](#anchor-11 "#anchor-11")  
- [Understanding the Output Files](#anchor-12 "#anchor-12")  
- [Building from Source](#anchor-13 "#anchor-13")  
![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAnEAAAACCAYAAAA3pIp+AAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAANUlEQVR4nO3OYQ1AABSAwY8JoIGqr4Z6Eoiggn9mu0twy8wc1RkAAH9xbdVa7V9PAAB47X4A9C4EIsmYmgsAAAAASUVORK5CYII=)  
**Quick Start (Step-by-Step for Windows)**  
**How to Open the Terminal**  
1. **Open File Explorer** and navigate to your StaticTerrainBankCompiler folder:  
2. **Click inside the address bar** at the top of the File Explorer window (where the folder path is shown).  
3. **Type ** **powershell** (or cmd) directly into the address bar and press  **Enter**:  
  - A terminal window will pop up immediately, already set to that folder.  
![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAnEAAAACCAYAAAA3pIp+AAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAANUlEQVR4nO3OQQmAABRAsSd49m4tA8nPaQJjWMGbCFuCLTOzV2cAAPzFvVZbdXw9AQDgtesBorcEPwOKyvQAAAAASUVORK5CYII=)  
   
**Running the Compiler**  
Inside that terminal window:  
1. **Check that it works and see available options:**  
2. .\stb_compiler.exe --help  
   
![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAnEAAAACCAYAAAA3pIp+AAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAANUlEQVR4nO3OMQ2AABAAsSPBCj7fFwtCmJHAjAU2QtIq6DIzW7UHAMBfnGt1V8fHEQAA3rsexOkF3va0dq8AAAAASUVORK5CYII=)  
**How to Run the Compiler**  
**Syntax**  
stb_compiler.exe [fable_path] [wld_name] [output_stb] [options...]  
   
**Positional Arguments**  
The first three arguments are positional. If you wish to accept the default value for an earlier positional argument while providing a later one or passing options, you can pass . or -:  
| | | | |  
|-|-|-|-|  
| **Argument** | **Position** | **Default Value** | **Description** |   
| fable_path | 1 | Auto-detected from egocore_config.ini | Root directory of Fable: The Lost Chapters (contains Fable.exe and Data\). |   
| wld_name | 2 | dean_2003.wld | Name of the world definition file inside Data\Levels\. |   
| output_stb | 3 | <fable_path>\Data\Levels\<wld_stem>_RT.stb | Destination path for the compiled .stb. |   
   
***Examples***  
# Explicit game root, explicit world, explicit output  
 .\stb_compiler.exe "C:\Games\Fable TLC" "FinalAlbion.wld" "C:\Games\Fable\Data\Levels\FinalAlbion_RT.stb"  
   
 # Use auto-detected game path, but compile FinalAlbion.wld  
 .\stb_compiler.exe . FinalAlbion.wld  
   
 # Use auto-detected game path and default world, but save to a custom output path  
 .\stb_compiler.exe . . "C:\Output\test.stb"  
   
   
   
**Auto-Detection with **egocore_config.ini  
If fable_path is omitted or passed as . or -, the compiler looks for egocore_config.ini in the current working directory or the parent folder, reading the Root= entry under [Fable].  
![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAnEAAAACCAYAAAA3pIp+AAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAANklEQVR4nO3OQQmAABRAsSfYxZo/khWsYQLPJrCCNxG2BFtmZquOAAD4i3Ot7mr/egIAwGvXA4qjBdKlX6OKAAAAAElFTkSuQmCC)  
**Command-Line Parameters & Defaults**  
Every quality and tuning parameter has a default designed to **strictly reproduce retail Fable behavior and file layout**. You only need to pass options when you want to customize or enhance terrain quality.  
**Summary of Default Values**  
| | | | |  
|-|-|-|-|  
| **Option** | **Type / Range** | **Default Value** | **Purpose** |   
| --mesh-detail | float (≥ 0.1) | **2.0** | Geometric error threshold for background LOD meshes. |   
| --fg-distance | float (meters) | **74.0** | Distance radius where high-detail multi-textured foreground patches transition to baked background LOD patches. |   
| --lod-dist-scale | float (≥ 0.1) | **1.0** | Distance multiplier for the 7 background LOD transition rings. |   
| --tex-scale | int (1, 2, 4) | **1** | Resolution multiplier for baked background terrain textures. |   
| --gen-mipmaps | Flag | **Disabled** (false) | Generates complete mipmap chains in patch CTexture chunks. |   
| --dxt-quality | fast | high | **fast** | DXT1 block encoder algorithm (fast vs. optimal cluster fit). |   
| --no-lod-sharing | Flag | **Disabled** (Bands 4–6 shared) | Bakes unique meshes and textures for all 7 LOD bands. |   
| --max-lod-band | int (1–7) | **7** | Maximum simplification band generated in the quadtree. |   
| --no-range-compression | Flag | **Disabled** (Compressed) | Disables range compression on vertex/index streams. |   
| --no-ism | Flag | **Disabled** (ISM generated) | Skips generating the companion .ism spatial index file. |   
| -q, --quiet | Flag | **Disabled** (Verbose) | Suppresses detailed step-by-step console logging. |   
| -h, --help | Flag | — | Shows the usage summary. |   
   
![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAnEAAAACCAYAAAA3pIp+AAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAANUlEQVR4nO3OMQ2AABAAsSNhwgJmkPYLLpnRgQU2QtIq6DIze3UGAMBf3Gu1VcfHEQAA3rseaHkEMn1wK7sAAAAASUVORK5CYII=)  
   
   
   
**Detailed Parameter Reference**  
***1. Terrain Geometry & LOD Detail***  
- **--mesh-detail <float>** *(Default: * *2.0* *)*  
  - Controls the geometric error tolerance when simplifying background terrain patches.  
  - **Default (** **2.0** **):** Matches original Fable polygon budgets.  
  - **Higher values (** **3.0** **–** **6.0** **):** Enforces tighter error tolerances, keeping ridges, hilltops, and cliff silhouettes sharp and dense even at medium/far distances.  
- **--fg-distance <float>** *(Default: * *74.0* * meters)*  
  - Defines the distance from the player where the game renders full-detail multi-textured foreground terrain with dynamic texture splatting and clutter.  
  - **Default (** **74.0** **):** Vanilla culling distance.  
  - **Higher values (** **100.0** **–** **150.0** **):** Keeps full multi-layered detail visible further away, pushing back the ring where baked background LOD textures begin.  
- **--lod-dist-scale <float>** *(Default: * *1.0* *)*  
  - Multiplies the distance thresholds for the 7 background quadtree LOD bands (vanilla distances: 64m, 128m, 256m, 512m, 1024m, 2048m, 4096m).  
  - **1.5** **:** Transitions occur 50% further away (96m, 192m, 384m, etc.).  
  - **2.0** **:** Transitions occur twice as far away (128m, 256m, 512m, etc.), dramatically mitigating distant pop-in.  
- **--max-lod-band <1-7>** *(Default: * *7* *)*  
  - Caps the maximum simplification band generated in the quadtree.  
  - By default (7), distant horizon terrain is simplified down to broad, low-poly patches.  
  - Setting this to 4 or 5 prevents terrain from degrading past band 4/5, maintaining high geometric fidelity all the way to the horizon.  
- **--no-lod-sharing** *(Default: Bands 4–6 shared)*  
  - In the vanilla Fable engine, background bands 4, 5, and 6 share the same simplified mesh and baked texture to conserve RAM.  
  - Passing --no-lod-sharing forces the compiler to build and bake unique, independently tailored geometry and textures for each individual band.  
   
***2. Texture & Compression Controls***  
- **--tex-scale <1|2|4>** *(Default: * *1* *)*  
  - Multiplies the resolution of baked background terrain patches:  
    - **1** ** (Default):**64×64 for distant patches, 128×128 for near patches.  
    - **2** **:**128×128 distant, 256×256 near.  
    - **4** **:**256×256 distant, 512×512 near.  
  - Higher resolutions eliminate blurry textures on surrounding mountains and distant hills.  
- **--gen-mipmaps** *(Default: off)*  
  - Generates complete DXT1 mipmap chains inside the background patch CTexture blocks instead of only storing the top-level mip.  
  - Recommended when increasing --tex-scale or camera draw distance to prevent pixel shimmering and moiré patterns at sharp grazing angles.  
- **--dxt-quality <fast|high>** *(Default: * *fast* *)*  
  - **fast** **:** Standard DXT1 block compressor matching retail compilation speeds.  
  - **high** ** (or ** **best** **, ** **hq** **):** Performs optimal cluster fit with perceptual color weighting across blocks, eliminating macroblocking artifacts and color banding across smooth grass and rock gradients.  
- **--no-range-compression** *(Default: Range compression enabled)*  
  - The retail Fable STB format encodes raw vertex and index streams using range compression.  
  - Omitting this flag preserves retail compression (0x01 chunk flag). Passing --no-range-compression writes raw uncompressed streams (0x00 chunk flag) for debugging or custom toolchains.  
***3. General & File Options***  
- **--no-ism** *(Default: ISM enabled)*  
  - By default, the compiler automatically generates a companion <wld_stem>_RT.ism spatial index file alongside the .stb. Pass --no-ism if you only want the .stb binary.  
- **-q** **, ** **--quiet** *(Default: Verbose enabled)*  
  - Silences informational progress messages, only printing warnings, errors, and the final timing summary.  
![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAnEAAAACCAYAAAA3pIp+AAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAANklEQVR4nO3OQQmAABRAsScYxpg/h5VMYARvRrCCNxG2BFtmZquOAAD4i3Ot7mr/egIAwGvXA224BcUMk6pDAAAAAElFTkSuQmCC)  
**Recommended Quality Presets**  
**1. Vanilla / Retail Match (Fastest & Smallest)**  
Matches the exact visual fidelity, file size, and LOD behavior of the original Lionhead release:  
.\stb_compiler.exe  
   
**2. Modern Enhanced / Remaster (Recommended for Modding)**  
Significantly sharpens background terrain, pushes back LOD transitions, and enables high-resolution textures with clean mipmapping:  
.\stb_compiler.exe . "dean_2003.wld" "Data\Levels\dean_2003_RT.stb" `  
   --mesh-detail 4.0 `  
   --fg-distance 110.0 `  
   --lod-dist-scale 1.5 `  
   --tex-scale 2 `  
   --gen-mipmaps `  
   --dxt-quality high `  
   --no-lod-sharing  
   
**3. Extreme Distance & Anti-Pop-In**  
Maximizes mesh density, extends the foreground transition radius, and prevents distant horizon geometry from collapsing into low-poly shapes:  
.\stb_compiler.exe . "dean_2003.wld" "Data\Levels\dean_2003_RT.stb" `  
   --mesh-detail 6.0 `  
   --fg-distance 140.0 `  
   --lod-dist-scale 2.0 `  
   --max-lod-band 5 `  
   --tex-scale 2 `  
   --gen-mipmaps `  
   --dxt-quality high `  
   --no-lod-sharing  
   
   
   
   
   
   
   
   
![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAnEAAAACCAYAAAA3pIp+AAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAANklEQVR4nO3OQQmAABRAsScYxpg/iX2MYARvRrCCNxG2BFtmZquOAAD4i3Ot7mr/egIAwGvXA22oBcdqfJWKAAAAAElFTkSuQmCC)  
**Understanding the Output Files**  
When compilation completes successfully, two files are written to the output destination (by default in <Fable>\Data\Levels\):  
1. **<wld_stem>_RT.stb** (Static Terrain Bank)  
  - The binary archive containing all terrain nodes, vertex buffers, index buffers, material tables, sea mesh strips, and quadtree LOD hierarchies.  
2. **<wld_stem>_RT.ism** (Index Spatial Map)  
  - Spatial lookup table generated alongside the STB for fast bounding-volume spatial queries at engine runtime.  
   
