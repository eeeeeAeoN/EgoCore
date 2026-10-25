#include "egostb/stb_compiler.hpp"
#include "gui.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>

static void printHelp() {
    std::cout << "Usage:\n";
    std::cout << "  stb_compiler.exe [fable_path] [wld_name] [output_stb] [options...]\n\n";
    std::cout << "Interface Modes:\n";
    std::cout << "  (no arguments)              Launch modern Graphical User Interface (ImGui)\n";
    std::cout << "  --gui, -g                   Launch modern Graphical User Interface\n\n";
    std::cout << "Positional arguments (use \".\" or \"-\" for default):\n";
    std::cout << "  fable_path                  Base game root path (default: read from egocore_config.ini)\n";
    std::cout << "  wld_name                    World file name (default: dean_2003.wld)\n";
    std::cout << "  output_stb                  Output STB path (default: Data\\Levels\\<wld_stem>_RT.stb)\n\n";
    std::cout << "Quality & Tuning Options:\n";
    std::cout << "  --mesh-detail <float>       LOD mesh geometric error factor (default: 2.0). Higher = denser meshes.\n";
    std::cout << "  --fg-distance <float>       Foreground patch culling distance in meters (default: 74.0).\n";
    std::cout << "                              Higher values keep full-detail multi-textured terrain further away.\n";
    std::cout << "  --lod-dist-scale <float>    Multiplier for background LOD band distance thresholds (default: 1.0).\n";
    std::cout << "                              e.g. 1.5 or 2.0 pushes LOD transitions further into distance.\n";
    std::cout << "  --tex-scale <1|2|4>         Baked background texture resolution multiplier (default: 1 -> 64/128).\n";
    std::cout << "  --gen-mipmaps               Generate complete mipmap chains in patch CTextures (default: off).\n";
    std::cout << "  --dxt-quality <fast|high>   DXT1 compression mode (default: fast).\n";
    std::cout << "                              'high' uses optimal cluster fit with perceptual color weighting.\n";
    std::cout << "  --no-lod-sharing            Bake unique meshes/textures for all LOD bands 1-7 (default: share 4-6).\n";
    std::cout << "  --max-lod-band <1-7>        Maximum LOD simplification band to generate (default: 7).\n";
    std::cout << "  --no-range-compression      Disable range compression for uncompressed raw streams (default: on).\n";
    std::cout << "  --no-ism                    Do not generate companion <wldStem>_RT.ism file (default: on).\n";
    std::cout << "  -q, --quiet                 Quiet mode (reduced console output).\n";
    std::cout << "  -h, --help                  Show this help message.\n";
}

int main(int argc, char* argv[]) {
    // If launched with no arguments or with --gui, open GUI mode
    if (argc == 1 || (argc == 2 && (std::string(argv[1]) == "--gui" || std::string(argv[1]) == "-g"))) {
        return egostb::runCompilerGui();
    }

    std::cout << "===========================================\n";
    std::cout << "   EgoSTB: Multi-Map Fable STB Compiler    \n";
    std::cout << "   (Standalone 100% Background Quadtree)   \n";
    std::cout << "===========================================\n\n";

    egostb::CompilerConfig config;
    config.retailMode = true;
    config.verbose = true;
    config.outputIsm = true;

    std::vector<std::string> positionals;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help" || arg == "/?") {
            printHelp();
            return 0;
        } else if (arg == "-q" || arg == "--quiet") {
            config.verbose = false;
        } else if (arg == "--gen-mipmaps" || arg == "--generate-mipmaps") {
            config.generateMipmaps = true;
        } else if (arg == "--no-lod-sharing" || arg == "--full-lods") {
            config.shareLodBands = false;
        } else if (arg == "--no-range-compression") {
            config.enableRangeCompression = false;
        } else if (arg == "--no-ism") {
            config.outputIsm = false;
        } else if (arg == "--mesh-detail" && i + 1 < argc) {
            config.meshDetail = std::max(0.1f, static_cast<float>(std::atof(argv[++i])));
        } else if (arg == "--fg-distance" && i + 1 < argc) {
            config.fgDistanceThreshold = std::max(0.0f, static_cast<float>(std::atof(argv[++i])));
        } else if (arg == "--lod-dist-scale" && i + 1 < argc) {
            config.lodDistanceScale = std::max(0.1f, static_cast<float>(std::atof(argv[++i])));
        } else if (arg == "--tex-scale" && i + 1 < argc) {
            config.textureScale = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--max-lod-band" && i + 1 < argc) {
            config.maxLodBand = std::clamp(std::atoi(argv[++i]), 1, 7);
        } else if (arg == "--dxt-quality" && i + 1 < argc) {
            std::string q = argv[++i];
            if (q == "high" || q == "best" || q == "hq") {
                config.highQualityDxt = true;
            } else {
                config.highQualityDxt = false;
            }
        } else if (!arg.empty() && arg[0] != '-') {
            positionals.push_back(arg);
        } else {
            std::cerr << "Warning: Unknown option ignored: " << arg << "\n";
        }
    }

    if (positionals.size() >= 1 && positionals[0] != "." && positionals[0] != "-") {
        config.fableRoot = positionals[0];
    }
    if (positionals.size() >= 2 && positionals[1] != "." && positionals[1] != "-") {
        config.wldName = positionals[1];
    } else {
        config.wldName = "dean_2003.wld";
    }
    if (positionals.size() >= 3 && positionals[2] != "." && positionals[2] != "-") {
        config.outputPath = positionals[2];
    }

    auto result = egostb::StbCompiler::compileWorld(config);

    if (!result.success) {
        std::cerr << "\nCompilation FAILED: " << result.errorMessage << "\n";
        return 1;
    }

    std::cout << "\nCompilation completed successfully in " << result.durationSeconds << "s!\n";
    return 0;
}
