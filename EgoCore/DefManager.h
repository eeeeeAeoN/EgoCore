#include "DefCompiler.h"
#include <zlib.h>

// Forward declarations for your factories (you will auto-generate these later)
IDefObject* CreateFrontEndDef() { return nullptr; /* new CFrontEndDef(); */ }
IDefObject* CreateUIMiscThingsDef() { return nullptr; }
IDefObject* CreateUIIconsDef() { return nullptr; }
IDefObject* CreateControlsDef() { return nullptr; }

static void NativeCompileFrontend() {
    g_CompileStatus = "Native Compiling Frontend (1/2)...";

    std::wstring outPath = std::wstring(g_AppConfig.GameRootPath.begin(), g_AppConfig.GameRootPath.end()) + L"\\Data\\CompiledDefs\\frontend.bin";
    CDefinitionManager defManager(outPath);

    // Register Classes (Translating AddDefClass from your disassembly)
    defManager.AddDefClass("FRONT_END", CreateFrontEndDef);
    defManager.AddDefClass("UI_MISC_THINGS_DEF", CreateUIMiscThingsDef);
    defManager.AddDefClass("UI_ICONS_DEF", CreateUIIconsDef);
    defManager.AddDefClass("CONTROL_SCHEME", CreateControlsDef);

    // Setup hardcoded paths from InitAndCompilePlatformSpecific
    defManager.SetSymbolPaths({
        L"textures.h", L"fonts.h", L"front_end_bank.h", L"particles_frontend.h",
        L"ui_types.h", L"controller_def.h", L"keyboard_keys.h"
        });

    defManager.SetCompilePaths({
        L"engine.def", L"engine_video_options.def", L"front_end.def",
        L"ui_dialogs.def", L"pc_frontend.def", L"config_options_defaults.def", L"pc_controls.def"
        });

    // Fire the native compiler
    defManager.Compile();
}