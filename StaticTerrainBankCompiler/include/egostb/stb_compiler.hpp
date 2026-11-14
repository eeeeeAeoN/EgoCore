#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>

namespace egostb {

struct CompilerConfig {
    std::string fableRoot;       // Base game root path (if empty, read from egocore_config.ini)
    std::string wldName;         // World file name (e.g. "dean_2003.wld" or "FinalAlbion.wld")
    std::string outputPath;      // Optional output STB path. Defaults to Data\Levels\<wld_stem>_RT.stb
    std::string iniConfigPath;   // Optional explicit path to egocore_config.ini
    bool retailMode = true;      // Hardcoded true for retail _RT.stb format
    bool verbose = true;
    bool outputIsm = true;       // Write compiled <wldStem>_RT.ism alongside output STB
    std::function<void(const std::string&)> logCallback = nullptr;
    std::function<void(float progress, const std::string& status)> progressCallback = nullptr;
    std::atomic<bool>* cancelToken = nullptr;

    // Quality & Tuning Parameters (defaults preserve exact retail STB behavior)
    float meshDetail = 2.0f;            // Factor for LOD mesh simplification detail (default 2.0). Higher = denser meshes.
    float fgDistanceThreshold = 74.0f;  // Maximum distance (meters) to include full foreground patches (default 74.0).
    float lodDistanceScale = 1.0f;      // Multiplier for background LOD band distance thresholds (default 1.0).
    int textureScale = 1;               // Texture resolution multiplier for baked background patches (default 1 -> 64/128).
    bool generateMipmaps = false;       // Disabled by default (complete mipmaps crash the game).
    bool highQualityDxt = true;         // If true, use high quality cluster fit for DXT1 encoding (default true).
    bool shareLodBands = false;         // If true, bands 4, 5, 6 share lastSavedBand. If false (default), generate unique patches for all bands.
    int maxLodBand = 7;                 // Max LOD band to generate (1..7, default 7).
    bool enableRangeCompression = true; // If false, write uncompressed raw streams (0x00) for VB/IB.
    bool respectModsIni = true;         // Inspect <fableRoot>\Mods.ini and raise fgDistanceThreshold/lodDistanceScale for active fade-extender DLL mods.
};

struct CompileResult {
    bool success = false;
    std::string errorMessage;
    std::string outputPath;
    size_t fileSize = 0;
    int mapsCompiled = 0;
    int totalNodes = 0;
    int totalPatches = 0;
    double durationSeconds = 0.0;
};

class StbCompiler {
public:
    // Core compiler entry point for EgoCore and CLI tools
    static CompileResult compileWorld(const CompilerConfig& config);

    // Helper to find and read Root= from egocore_config.ini
    static std::string findGameRootFromIni(const std::string& explicitIni = "");

    // Load compiler parameters from an STBconfig.ini file into config
    static bool loadConfigFile(const std::string& iniPath, CompilerConfig& config);

    // Inspect <fableRoot>/Mods.ini for active DLL fade-extender mods (reading sibling <stem>.ini)
    // and raise fgDistanceThreshold / lodDistanceScale floors if ForegroundFadeEnd > 64.0m.
    static bool applyActiveModsFromIni(
        const std::string& fableRoot,
        CompilerConfig& config,
        const std::function<void(const std::string&)>& log = nullptr);
};

} // namespace egostb
