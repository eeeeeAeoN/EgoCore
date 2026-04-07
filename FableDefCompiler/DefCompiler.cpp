#include "DefCompiler.h"
#include "DefinitionManager.h"
#include "DefObjects.h"
#include <fstream>

std::ofstream g_LogFile;
void LogToFile(const std::string& msg) {
    if (g_LogFile.is_open()) {
        g_LogFile << msg << "\n";
        g_LogFile.flush();
    }
}

namespace FableCompiler {

    bool CompileFrontendNative(const std::string& fableRootPath) {

        std::string logPath = fableRootPath + "\\compiler.log";
        g_LogFile.open(logPath, std::ios::out | std::ios::trunc);
        LogToFile("--- Fable Frontend Compiler Started ---");

        std::wstring root(fableRootPath.begin(), fableRootPath.end());
        std::wstring outPath = root + L"\\Data\\CompiledDefs\\frontend.bin";
        std::wstring defsRoot = root + L"\\Data\\Defs\\";

        CDefinitionManager defManager(outPath);

        defManager.AddDefClass("FRONT_END", Alloc_CFrontEndDef);
        defManager.AddDefClass("UI_MISC_THINGS_DEF", Alloc_CUIMiscThingsDef);
        defManager.AddDefClass("UI_ICONS_DEF", Alloc_CUIIconsDef);
        defManager.AddDefClass("CONTROL_SCHEME", Alloc_CControlsDef);
        defManager.AddDefClass("ENGINE_VIDEO_OPTIONS", Alloc_CEngineVideoOptionsDef);
        defManager.AddDefClass("ENGINE", Alloc_CEngineDef);
        defManager.AddDefClass("UI", Alloc_CUIDef);
        defManager.AddDefClass("CONFIG_OPTIONS_DEFAULTS_DEF", Alloc_CConfigOptionsDefaultsDef);

        LogToFile("Queueing symbol headers...");
        defManager.SetSymbolPaths({
            defsRoot + L"RetailHeaders\\pc\\textures.h",
            defsRoot + L"RetailHeaders\\fonts.h",
            defsRoot + L"RetailHeaders\\pc\\front_end_bank.h",
            defsRoot + L"RetailHeaders\\pc\\particles_frontend.h",
            defsRoot + L"FrontEndDefs\\engine.def",
            defsRoot + L"FrontEndDefs\\engine_video_options.def",
            defsRoot + L"FrontEndDefs\\front_end.def",
            defsRoot + L"ui_types.h",
            defsRoot + L"ui_dialogs.def",
            defsRoot + L"FrontEndDefs\\pc_frontend.def",
            defsRoot + L"config_options_defaults.def",
            defsRoot + L"controller_def.h",
            defsRoot + L"keyboard_keys.h",
            defsRoot + L"pc_controls.def"
            });

        LogToFile("Queueing definition files...");
        defManager.SetCompilePaths({
            defsRoot + L"FrontEndDefs\\engine.def",
            defsRoot + L"FrontEndDefs\\engine_video_options.def",
            defsRoot + L"FrontEndDefs\\front_end.def",
            defsRoot + L"ui_dialogs.def",
            defsRoot + L"FrontEndDefs\\pc_frontend.def",
            defsRoot + L"pc_controls.def",
            defsRoot + L"config_options_defaults.def"
            });

        defManager.Compile();

        LogToFile("--- Compilation Finished ---");
        if (g_LogFile.is_open()) g_LogFile.close();

        return true;
    }
}