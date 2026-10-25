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
    bool generateMipmaps = false;       // If true, generate complete mipmap chain for patch CTextures (default false).
    bool highQualityDxt = false;        // If true, use high quality cluster fit for DXT1 encoding (default false).
    bool shareLodBands = true;          // If true, bands 4, 5, 6 share lastSavedBand (default true). If false, generate unique patches for all bands.
    int maxLodBand = 7;                 // Max LOD band to generate (1..7, default 7).
    bool enableRangeCompression = true; // If false, write uncompressed raw streams (0x00) for VB/IB.
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
};

} // namespace egostb
