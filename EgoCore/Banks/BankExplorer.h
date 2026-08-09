#pragma once
#include "imgui.h"
#include "FileDialogs.h"
#include "DefExplorer.h"
#include "BankTabUI.h" 
#include "FSEBackend.h"
#include "FSETabUI.h"
#include "InputManager.h"
#include "WADBackend.h"
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>

namespace fs = std::filesystem;
static bool g_HasInitialized = false;
static bool g_TriggerKeybindPopup = false;
inline bool g_TriggerScalingPopup = false;
inline bool g_TriggerGeneralSettingsPopup = false;
inline bool g_ShowAboutPopup = false;
inline bool g_TriggerModEnvModal = false;
inline bool g_TriggerFSEModal = false;
inline fs::path g_AppBaseDir;
inline float g_UIScale = 1.0f;
inline bool g_TriggerAssetChangesExitPopup = false;
inline bool g_PendingRunFableLaunch = false;

inline std::atomic<bool> g_IsEditorLoading = false;
inline std::atomic<bool> g_EditorLoadRequested = false;
inline std::mutex g_EditorLoadStatusMutex;
inline std::string g_EditorLoadStatus = "Loading Editor...";

inline void SetEditorLoadStatus(const std::string& status) {
    std::lock_guard<std::mutex> lock(g_EditorLoadStatusMutex);
    g_EditorLoadStatus = status;
}

inline std::string GetEditorLoadStatus() {
    std::lock_guard<std::mutex> lock(g_EditorLoadStatusMutex);
    return g_EditorLoadStatus;
}

extern ID3D11ShaderResourceView* g_BackgroundTexture;
extern int g_BgWidth;
extern int g_BgHeight;
extern ImFont* g_TitleFont;

extern ID3D11ShaderResourceView* g_CloudTexture;
extern int g_CloudWidth;
extern int g_CloudHeight;

struct MainMenuAudio {
    ma_engine engine;
    ma_sound sound;
    bool isInitialized = false;
    bool isMuted = false;

    void ApplyConfig() {
        isMuted = !g_AppConfig.EnableMusic;
        if (isInitialized) {
            ma_sound_set_volume(&sound, isMuted ? 0.0f : 1.0f);
        }
    }

    void Init() {
        if (isInitialized) return;

        if (ma_engine_init(NULL, &engine) == MA_SUCCESS) {
            if (ma_sound_init_from_file(&engine, "Assets/MainMenuTheme.mp3", MA_SOUND_FLAG_STREAM, NULL, NULL, &sound) == MA_SUCCESS) {
                ma_sound_set_looping(&sound, MA_TRUE);
                isInitialized = true;
                ApplyConfig();
                ma_sound_start(&sound);
            }
            else {
                isInitialized = true;
            }
        }
    }

    void Update(bool isInAllowedMode) {
        if (!isInitialized) return;

        bool shouldPlay = g_AppConfig.EnableMusic && isInAllowedMode;

        if (shouldPlay) {
            if (!ma_sound_is_playing(&sound)) {
                ma_sound_start(&sound);
            }
        }
        else {
            if (ma_sound_is_playing(&sound)) {
                ma_sound_stop(&sound);
            }
        }
    }

    void Toggle() {
        if (!isInitialized) return;

        g_AppConfig.EnableMusic = !g_AppConfig.EnableMusic;
        ApplyConfig();
        SaveConfig();
    }

    void Shutdown() {
        if (!isInitialized) return;
        ma_sound_uninit(&sound);
        ma_engine_uninit(&engine);
        isInitialized = false;
    }
};

extern MainMenuAudio g_MenuAudio;
extern ImTextureID g_MusicOnTexture;
extern ImTextureID g_MusicOffTexture;

enum class EAppState {
    Setup,
    Frontend,
    ModCreator,
    ModsManager
};

// --- This must be included here to avoid compiler errors. This is what happens when you make a project out of all headers. 
#include "ModManagerUI.h"

inline EAppState g_CurrentAppState = EAppState::Setup;

static void UpdateUIScale(float scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;

    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    style.ScaleAllSizes(scale);
}

static void DrawGoldModalHeader(const char* title, float headerHeight = 58.0f) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

    // Outer Gold Glow Aura
    for (int i = 3; i >= 1; i--) {
        float expand = (float)i * 1.8f;
        float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
        ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
        drawList->AddRect(
            ImVec2(winPos.x - expand, winPos.y - expand),
            ImVec2(winMax.x + expand, winMax.y + expand),
            glowCol, 10.0f + expand, 0, 1.2f
        );
    }

    ImVec2 headerMin = winPos;
    ImVec2 headerMax = ImVec2(winMax.x, winPos.y + headerHeight);
    drawList->AddRectFilledMultiColor(
        headerMin, headerMax,
        IM_COL32(242, 193, 78, 30),
        IM_COL32(110, 140, 175, 15),
        IM_COL32(0, 0, 0, 0),
        IM_COL32(0, 0, 0, 0)
    );

    if (g_TitleFont) ImGui::PushFont(g_TitleFont);
    ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "%s", title);
    if (g_TitleFont) ImGui::PopFont();

    ImGui::Dummy(ImVec2(0, 4));
    drawList->AddLine(
        ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
        ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
        IM_COL32(242, 193, 78, 110), 1.2f
    );
    ImGui::Dummy(ImVec2(0, 12));
}

inline void ApplyModEnvironmentFix(const std::string& gameRoot) {
    fs::path rootPath(gameRoot);
    fs::path resourcesDir = (g_AppBaseDir.empty() ? fs::current_path() : g_AppBaseDir) / "Resources";

    if (fs::exists(resourcesDir)) {
        std::error_code ec;

        fs::path resourcesData = resourcesDir / "Data";
        if (fs::exists(resourcesData)) {
            fs::copy(resourcesData, rootPath / "Data",
                fs::copy_options::overwrite_existing | fs::copy_options::recursive, ec);
        }

        fs::path srcFseFolder = resourcesDir / "Mods";
        if (fs::exists(srcFseFolder)) {
            fs::copy(srcFseFolder, rootPath / "Mods",
                fs::copy_options::overwrite_existing | fs::copy_options::recursive, ec);
        }

        fs::path srcDinput = resourcesDir / "dinput8.dll";
        fs::path srcAsi = resourcesDir / "FableModLoader.asi";

        if (fs::exists(srcDinput)) {
            fs::copy_file(srcDinput, rootPath / "dinput8.dll",
                fs::copy_options::overwrite_existing, ec);
        }
        if (fs::exists(srcAsi)) {
            fs::copy_file(srcAsi, rootPath / "FableModLoader.asi",
                fs::copy_options::overwrite_existing, ec);
        }
    }

    g_AppConfig.ModEnvironmentSetup = true;
    SaveConfig();
}

inline void ApplyFSEInstallation(const std::string& gameRoot) {
    fs::path rootPath(gameRoot);
    fs::path resourcesDir = (g_AppBaseDir.empty() ? fs::current_path() : g_AppBaseDir) / "Resources";

    if (fs::exists(resourcesDir)) {
        std::error_code ec;

        fs::path srcFseDll = resourcesDir / "FableScriptExtender.dll";
        if (fs::exists(srcFseDll)) {
            fs::copy_file(srcFseDll, rootPath / "FableScriptExtender.dll",
                fs::copy_options::overwrite_existing, ec);
        }

        fs::path srcModsIni = resourcesDir / "Mods.ini";
        if (fs::exists(srcModsIni)) {
            fs::copy_file(srcModsIni, rootPath / "Mods.ini",
                fs::copy_options::overwrite_existing, ec);
        }

        fs::path srcFseFolder = resourcesDir / "FSE";
        if (fs::exists(srcFseFolder)) {
            fs::copy(srcFseFolder, rootPath / "FSE",
                fs::copy_options::overwrite_existing | fs::copy_options::recursive, ec);
        }
    }
    CheckFSEInstalled(gameRoot);
    g_FSEWorkspace.IsLoaded = false;
    LoadQuestsLua();

    g_AppConfig.FseSetup = true;
    SaveConfig();
}

inline void CheckModEnvironmentAndFSE(const std::string& gameRoot) {
    if (gameRoot.empty() || !fs::exists(gameRoot)) return;

    fs::path rootPath(gameRoot);
    fs::path miscDir = rootPath / "Data" / "Misc";
    fs::path defsDir = rootPath / "Data" / "Defs";

    if (!g_AppConfig.ModEnvironmentSetup) {
        bool loaderValid = fs::exists(rootPath / "dinput8.dll") &&
            fs::exists(rootPath / "FableModLoader.asi");

        bool miscValid = fs::exists(miscDir / "sound_animation_events.txt") &&
            fs::exists(miscDir / "game_animation_events.txt");

        bool defsValid = fs::exists(defsDir / "building.tpl") &&
            fs::exists(defsDir / "creature.tpl") &&
            fs::exists(defsDir / "objects.tpl");

        if (!loaderValid || !miscValid || !defsValid) {
            g_TriggerModEnvModal = true;
            return;
        }
        else {
            g_AppConfig.ModEnvironmentSetup = true;
            SaveConfig();
        }
    }

    if (!g_AppConfig.FseSetup) {
        bool fseValid = fs::exists(rootPath / "FableScriptExtender.dll");

        if (!fseValid) {
            g_TriggerFSEModal = true;
        }
        else {
            g_AppConfig.FseSetup = true;
            SaveConfig();
        }
    }
}

inline void DrawEnvironmentModals() {
    if (g_TriggerModEnvModal) {
        ImGui::OpenPopup("Fix Modding Environment");
        g_TriggerModEnvModal = false;
    }
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(520, 220), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Fix Modding Environment", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.8f;
            float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
            ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
            drawList->AddRect(
                ImVec2(winPos.x - expand, winPos.y - expand),
                ImVec2(winMax.x + expand, winMax.y + expand),
                glowCol, 10.0f + expand, 0, 1.2f
            );
        }
        ImVec2 headerMin = winPos;
        ImVec2 headerMax = ImVec2(winMax.x, winPos.y + 58.0f);
        drawList->AddRectFilledMultiColor(
            headerMin, headerMax,
            IM_COL32(242, 193, 78, 30),
            IM_COL32(110, 140, 175, 15),
            IM_COL32(0, 0, 0, 0),
            IM_COL32(0, 0, 0, 0)
        );
        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Fix Modding Environment");
        if (g_TitleFont) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 4));
        drawList->AddLine(
            ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
            ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
            IM_COL32(242, 193, 78, 110), 1.2f
        );
        ImGui::Dummy(ImVec2(0, 12));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 0.95f));
        ImGui::TextWrapped("Required modding files or definition templates are missing from your game directory.");
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::TextWrapped("Would you like EgoCore to fix your modding environment now?");
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 16));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        if (ImGui::Button("Yes, Fix Environment", ImVec2(180, 28))) {
            ApplyModEnvironmentFix(g_AppConfig.GameRootPath);
            ImGui::CloseCurrentPopup();

            CheckModEnvironmentAndFSE(g_AppConfig.GameRootPath);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.18f, 0.24f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.32f, 0.42f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.14f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        if (ImGui::Button("Not Now", ImVec2(110, 28))) {
            ImGui::CloseCurrentPopup();

            static bool s_SkippedEnvThisSession = false;
            if (!s_SkippedEnvThisSession) {
                s_SkippedEnvThisSession = true;
                if (!g_AppConfig.FseSetup && !fs::exists(fs::path(g_AppConfig.GameRootPath) / "FableScriptExtender.dll")) {
                    g_TriggerFSEModal = true;
                }
            }
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    if (g_TriggerFSEModal) {
        ImGui::OpenPopup("Install Fable Script Extender");
        g_TriggerFSEModal = false;
    }
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(520, 220), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Install Fable Script Extender", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.8f;
            float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
            ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
            drawList->AddRect(
                ImVec2(winPos.x - expand, winPos.y - expand),
                ImVec2(winMax.x + expand, winMax.y + expand),
                glowCol, 10.0f + expand, 0, 1.2f
            );
        }
        ImVec2 headerMin = winPos;
        ImVec2 headerMax = ImVec2(winMax.x, winPos.y + 58.0f);
        drawList->AddRectFilledMultiColor(
            headerMin, headerMax,
            IM_COL32(242, 193, 78, 30),
            IM_COL32(110, 140, 175, 15),
            IM_COL32(0, 0, 0, 0),
            IM_COL32(0, 0, 0, 0)
        );

        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Install Fable Script Extender");
        if (g_TitleFont) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 4));

        drawList->AddLine(
            ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
            ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
            IM_COL32(242, 193, 78, 110), 1.2f
        );
        ImGui::Dummy(ImVec2(0, 12));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 0.95f));
        ImGui::TextWrapped("Fable Script Extender (FSE) is not installed in your game directory.");
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::TextWrapped("Would you like EgoCore to install FSE now?");
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 16));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        if (ImGui::Button("Yes, Install FSE", ImVec2(160, 28))) {
            ApplyFSEInstallation(g_AppConfig.GameRootPath);
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0, 12.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.18f, 0.24f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.32f, 0.42f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.14f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        if (ImGui::Button("Not Now", ImVec2(110, 28))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}

static void DrawLaunchPopup() {
    if (g_LaunchState == 1) {
        ImGui::OpenPopup("Launching Fable");
        g_LaunchState = 2;
    }

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Launching Fable", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.8f;
            float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
            ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
            drawList->AddRect(
                ImVec2(winPos.x - expand, winPos.y - expand),
                ImVec2(winMax.x + expand, winMax.y + expand),
                glowCol, 10.0f + expand, 0, 1.2f
            );
        }

        ImVec2 headerMin = winPos;
        ImVec2 headerMax = ImVec2(winMax.x, winPos.y + 58.0f);
        drawList->AddRectFilledMultiColor(
            headerMin, headerMax,
            IM_COL32(242, 193, 78, 30),
            IM_COL32(110, 140, 175, 15),
            IM_COL32(0, 0, 0, 0),
            IM_COL32(0, 0, 0, 0)
        );

        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Launching Fable");
        if (g_TitleFont) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 4));

        drawList->AddLine(
            ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
            ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
            IM_COL32(242, 193, 78, 110), 1.2f
        );
        ImGui::Dummy(ImVec2(0, 12));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 0.95f));
        ImGui::TextWrapped("Preparing to launch Fable...");
        ImGui::Dummy(ImVec2(0, 4));

        if (g_IsProcessingLaunch) {
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "%s", GetLaunchStatus().c_str());

            static int dots = 0; if (ImGui::GetFrameCount() % 20 == 0) dots = (dots + 1) % 4;
            std::string spinner = "Please wait"; for (int i = 0; i < dots; i++) spinner += ".";
            ImGui::TextDisabled("%s", spinner.c_str());
        }
        else if (g_AppConfig.ModSystemDirty || g_AppConfig.DefSystemDirty) {
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Compiling modified banks and applying load order...");
            ImGui::TextDisabled("Please wait. EgoCore will process mods and launch automatically.");
        }
        else {
            ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.50f, 1.0f), "Load order clean. Launching directly...");
        }
        ImGui::PopStyleColor();

        if (g_LaunchState >= 2 && g_LaunchState < 6) {
            g_LaunchState++;
        }
        else if (g_LaunchState == 6) {
            g_LaunchState = 7;
            SetLaunchStatus("Patching banks and applying load order...");
            g_IsProcessingLaunch = true;
            std::thread([]() {
                ModManagerBackend::ProcessModsAndLaunch();
                g_IsProcessingLaunch = false;
                }).detach();
        }

        if (!g_IsProcessingLaunch && (g_IsCompiling || g_ShowFallbackLaunchPopup)) {
            g_LaunchState = 0;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}

static bool DrawCardButton(const char* id, const char* title, const char* subtitle, ImVec2 size, ImU32 accentColor) {
    ImGuiID id_hash = ImGui::GetID(id);
    ImVec2 pos = ImGui::GetCursorScreenPos();

    bool pressed = ImGui::InvisibleButton(id, size);
    bool hovered = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();

    static std::unordered_map<ImGuiID, float> s_HoverTimers;
    float& hoverTime = s_HoverTimers[id_hash];
    if (hovered) {
        hoverTime += ImGui::GetIO().DeltaTime;
    }
    else {
        hoverTime = 0.0f;
    }

    float subtextAlpha = 0.0f;
    if (hoverTime > 1.0f) {
        subtextAlpha = (std::min)(1.0f, (hoverTime - 1.0f) / 0.25f);
    }

    float offsetY = (hovered && !held) ? -2.0f : (held ? 1.0f : 0.0f);
    ImVec2 pMin = ImVec2(pos.x, pos.y + offsetY);
    ImVec2 pMax = ImVec2(pos.x + size.x, pos.y + size.y + offsetY);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float rounding = 8.0f;

    ImU32 bgColor = hovered ? IM_COL32(24, 28, 38, 240) : IM_COL32(14, 16, 22, 215);
    ImU32 borderColor = hovered ? accentColor : IM_COL32(48, 54, 68, 160);

    if (hovered) {
        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.5f;
            float glowAlpha = (1.0f - (float)i / 4.0f) * 0.35f;
            ImU32 glowCol = (accentColor & 0x00FFFFFF) | ((uint32_t)(glowAlpha * 255.0f) << 24);
            drawList->AddRect(
                ImVec2(pMin.x - expand, pMin.y - expand),
                ImVec2(pMax.x + expand, pMax.y + expand),
                glowCol, rounding + expand, 0, 1.2f
            );
        }
    }

    drawList->AddRectFilled(pMin, pMax, bgColor, rounding);
    uint32_t alphaLeft = hovered ? 0x88000000 : 0x44000000;
    ImU32 gradLeft = (accentColor & 0x00FFFFFF) | alphaLeft;
    ImU32 gradRight = IM_COL32(0, 0, 0, 0);

    ImVec2 gradMin = ImVec2(pMin.x + 1.0f, pMin.y + 1.0f);
    ImVec2 gradMax = ImVec2(pMax.x - 1.0f, pMax.y - 1.0f);
    drawList->AddRectFilledMultiColor(gradMin, gradMax, gradLeft, gradRight, gradRight, gradLeft);

    drawList->AddRect(pMin, pMax, borderColor, rounding, 0, hovered ? 1.8f : 1.0f);
    ImFont* font = ImGui::GetFont();
    float titleFontSize = ImGui::GetFontSize() * 1.15f;
    float subFontSize = ImGui::GetFontSize() * 0.90f;

    ImVec2 titleSize = font->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, title);
    float centerTitleX = pMin.x + (size.x - titleSize.x) * 0.5f;

    float centerTitleY = pMin.y + (size.y - titleSize.y) * 0.5f;
    float topTitleY = (subtitle && subtitle[0] != '\0') ? (pMin.y + 7.0f) : centerTitleY;
    float titleY = centerTitleY + (topTitleY - centerTitleY) * subtextAlpha;

    ImU32 titleCol = hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(220, 225, 235, 240);
    drawList->AddText(font, titleFontSize, ImVec2(centerTitleX, titleY), titleCol, title);

    if (subtitle && subtitle[0] != '\0' && subtextAlpha > 0.0f) {
        ImVec2 subSize = font->CalcTextSizeA(subFontSize, FLT_MAX, 0.0f, subtitle);
        float centerSubX = pMin.x + (size.x - subSize.x) * 0.5f;
        float subY = pMin.y + 28.0f;

        ImU32 subCol = IM_COL32(150, 162, 180, (uint32_t)(subtextAlpha * 220.0f));
        drawList->AddText(font, subFontSize, ImVec2(centerSubX, subY), subCol, subtitle);
    }

    return pressed;
}

// Compact "pill" tab button for the top bar mode switcher (Banks / Defs / FSE).
// Same visual language as DrawCardButton (glow, gradient fill, accent border) but
// sized to sit inside a normal menu-bar row instead of a big hub tile.
//
// IMPORTANT: `rowHeight` is the FULL row height (same value passed to every other
// item on that row) and it is what gets reserved for the item itself. The pill
// is drawn a few px smaller than that reserved space, but that shrink is *only*
// in the drawing -- never in the size given to InvisibleButton. That's what
// keeps this pixel-aligned with everything else on the row via plain SameLine(),
// with no manual SetCursorPosY offset required anywhere.
static bool DrawModeTabButton(const char* id, const char* label, bool active, ImU32 accentColor, float rowHeight) {
    ImVec2 textSize = ImGui::CalcTextSize(label);
    const float paddingX = 14.0f;
    ImVec2 size = ImVec2(textSize.x + paddingX * 2.0f, rowHeight);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton(id, size);
    bool hovered = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Cosmetic-only inset: shrinks the drawn pill within the reserved bounds so
    // it reads as a tab, without touching layout/alignment at all.
    const float inset = 3.0f;
    ImVec2 pMin = ImVec2(pos.x, pos.y + inset);
    ImVec2 pMax = ImVec2(pos.x + size.x, pos.y + size.y - inset);
    float rounding = (pMax.y - pMin.y) * 0.30f;

    ImU32 bgColor, borderColor, textColor;
    if (active) {
        bgColor = (accentColor & 0x00FFFFFF) | 0xB4000000;
        borderColor = accentColor;
        textColor = IM_COL32(255, 255, 255, 255);
    }
    else if (hovered) {
        bgColor = (accentColor & 0x00FFFFFF) | (held ? 0x33000000 : 0x55000000);
        borderColor = (accentColor & 0x00FFFFFF) | 0x99000000;
        textColor = IM_COL32(230, 232, 238, 255);
    }
    else {
        bgColor = IM_COL32(255, 255, 255, 12);
        borderColor = IM_COL32(255, 255, 255, 24);
        textColor = IM_COL32(160, 165, 175, 255);
    }

    if (active) {
        for (int i = 2; i >= 1; i--) {
            float expand = (float)i * 1.4f;
            float glowAlpha = (1.0f - (float)i / 3.0f) * 0.30f;
            ImU32 glowCol = (accentColor & 0x00FFFFFF) | ((uint32_t)(glowAlpha * 255.0f) << 24);
            drawList->AddRect(ImVec2(pMin.x - expand, pMin.y - expand), ImVec2(pMax.x + expand, pMax.y + expand), glowCol, rounding + expand, 0, 1.2f);
        }
    }

    drawList->AddRectFilled(pMin, pMax, bgColor, rounding);
    drawList->AddRect(pMin, pMax, borderColor, rounding, 0, active ? 1.4f : 1.0f);

    ImVec2 textPos = ImVec2(pMin.x + (size.x - textSize.x) * 0.5f, pMin.y + ((pMax.y - pMin.y) - textSize.y) * 0.5f);
    drawList->AddText(textPos, textColor, label);

    return pressed;
}

static void DrawFrontendHub() {
    static float s_ScrollY = 0.0f;
    static EAppState s_LastAppState = EAppState::Setup;
    static bool s_StatusCollapsed = true;
    static bool s_AudioInitialized = false;

    if (s_LastAppState != EAppState::Frontend && g_CurrentAppState == EAppState::Frontend) {
        s_ScrollY = 0.0f;
    }
    s_LastAppState = g_CurrentAppState;

    if (g_BackgroundTexture && g_BgWidth > 0 && g_BgHeight > 0) {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;

        float scaledWidth = displaySize.x;
        float aspect = (float)g_BgHeight / (float)g_BgWidth;
        float scaledHeight = scaledWidth * aspect;
        float maxScroll = scaledHeight - displaySize.y;

        if (maxScroll > 0.0f) {
            s_ScrollY += 2.0f * ImGui::GetIO().DeltaTime;
            if (s_ScrollY > maxScroll) s_ScrollY = maxScroll;
        }
        else {
            s_ScrollY = 0.0f;
        }

        ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();
        ImVec2 pMin = ImVec2(0.0f, -s_ScrollY);
        ImVec2 pMax = ImVec2(scaledWidth, scaledHeight - s_ScrollY);

        bgDrawList->AddImage((ImTextureID)g_BackgroundTexture, pMin, pMax);

        if (g_CloudTexture && g_CloudWidth > 0 && g_CloudHeight > 0) {
            static float s_CloudTime = 0.0f;
            s_CloudTime += ImGui::GetIO().DeltaTime;
            struct CloudPuff { float xFrac, yFrac, radius, speed, alpha; bool flip; };
            static const CloudPuff puffs[] = {
                { 0.05f, 0.15f, 260.0f, 6.0f, 235.0f, false },
                { 0.50f, 0.08f, 340.0f, 4.0f, 190.0f, true  },
                { 0.80f, 0.30f, 220.0f, 8.0f, 210.0f, false },
                { 0.25f, 0.50f, 300.0f, 5.0f, 170.0f, true  },
                { 0.65f, 0.62f, 240.0f, 7.0f, 200.0f, false },
            };

            for (const auto& p : puffs) {
                float period = scaledWidth + p.radius * 2.0f;
                float baseX = p.xFrac * scaledWidth;
                float x = fmodf(baseX + s_CloudTime * p.speed, period) - p.radius;
                float y = p.yFrac * scaledHeight - s_ScrollY;

                float halfW = p.radius;
                float halfH = p.radius * 0.55f;
                ImVec2 spriteMin(x - halfW, y - halfH);
                ImVec2 spriteMax(x + halfW, y + halfH);

                ImVec2 uv0 = p.flip ? ImVec2(1.0f, 0.0f) : ImVec2(0.0f, 0.0f);
                ImVec2 uv1 = p.flip ? ImVec2(0.0f, 1.0f) : ImVec2(1.0f, 1.0f);

                ImU32 tint = IM_COL32(255, 255, 255, (int)p.alpha);
                bgDrawList->AddImage((ImTextureID)g_CloudTexture, spriteMin, spriteMax, uv0, uv1, tint);
            }
        }

        float vignetteSize = 120.0f;
        ImU32 colDark = IM_COL32(0, 0, 0, 200);
        ImU32 colClear = IM_COL32(0, 0, 0, 0);

        bgDrawList->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(displaySize.x, vignetteSize), colDark, colDark, colClear, colClear);
        bgDrawList->AddRectFilledMultiColor(ImVec2(0, displaySize.y - vignetteSize), ImVec2(displaySize.x, displaySize.y), colClear, colClear, colDark, colDark);
        bgDrawList->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(vignetteSize, displaySize.y), colDark, colClear, colClear, colDark);
        bgDrawList->AddRectFilledMultiColor(ImVec2(displaySize.x - vignetteSize, 0), ImVec2(displaySize.x, displaySize.y), colClear, colDark, colDark, colClear);
    }

    bool wadDecompiled = false;
    if (!g_AppConfig.GameRootPath.empty()) {
        fs::path finalAlbionPath = fs::path(g_AppConfig.GameRootPath) / "Data" / "Levels" / "FinalAlbion";
        wadDecompiled = fs::exists(finalAlbionPath);
    }

    bool fseDetected = g_AppConfig.FseSetup || fs::exists("fse.dll") || fs::exists("dinput8.dll") ||
        (!g_AppConfig.GameRootPath.empty() && (fs::exists(fs::path(g_AppConfig.GameRootPath) / "fse.dll") || fs::exists(fs::path(g_AppConfig.GameRootPath) / "dinput8.dll")));

    bool modEnvOk = g_AppConfig.ModEnvironmentSetup ||
        (g_AppConfig.IsConfigured && !g_AppConfig.GameRootPath.empty() && fs::exists(g_AppConfig.GameRootPath));

    int activeModsCount = 0;
    if (!ModManagerBackend::g_LoadedMods.empty()) {
        for (const auto& m : ModManagerBackend::g_LoadedMods) {
            if (m.IsEnabled) activeModsCount++;
        }
    }
    else {
        for (const auto& item : g_SavedModOrder) {
            if (item.second) activeModsCount++;
        }
    }

    if (!s_AudioInitialized) {
        g_MenuAudio.Init();
        s_AudioInitialized = true;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 16.0f, viewport->Pos.y + viewport->Size.y - 16.0f), ImGuiCond_Always, ImVec2(0.0f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.07f, 0.82f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));

    if (ImGui::Begin("##MusicToggleOverlay", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing))
    {
        float buttonSize = 24.0f;
        ImTextureID currentTexture = g_MenuAudio.isMuted ? g_MusicOffTexture : g_MusicOnTexture;

        if (currentTexture != 0) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.30f));

            if (ImGui::ImageButton("##MusicToggleBtn", currentTexture, ImVec2(buttonSize, buttonSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0))) {
                g_MenuAudio.Toggle();
            }

            ImGui::PopStyleColor(3);
        }
        else {
            if (ImGui::Button(g_MenuAudio.isMuted ? "Unmute" : "Mute")) {
                g_MenuAudio.Toggle();
            }
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(g_MenuAudio.isMuted ? "Unmute Music" : "Mute Music");
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 60.0f, viewport->Pos.y + viewport->Size.y - 16.0f), ImGuiCond_Always, ImVec2(0.0f, 1.0f));

    float labelOffset = 165.0f;
    float pathWidth = g_AppConfig.GameRootPath.empty() ? 70.0f : ImGui::CalcTextSize(g_AppConfig.GameRootPath.c_str()).x;
    float minStatusTrayWidth = (std::max)(240.0f, labelOffset + pathWidth + 24.0f);

    ImGui::SetNextWindowSizeConstraints(ImVec2(minStatusTrayWidth, -1.0f), ImVec2(FLT_MAX, -1.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.07f, 0.82f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));

    if (ImGui::Begin("##SystemStatusOverlay", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing))
    {
        ImGui::TextDisabled("SYSTEM STATUS");

        float btnWidth = 20.0f;
        float btnHeight = 20.0f;
        ImGui::SameLine(ImGui::GetWindowWidth() - btnWidth - ImGui::GetStyle().WindowPadding.x);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.22f, 0.27f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.38f, 0.46f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.17f, 0.20f, 1.00f));

        if (ImGui::Button(s_StatusCollapsed ? "+" : "-", ImVec2(btnWidth, btnHeight))) {
            s_StatusCollapsed = !s_StatusCollapsed;
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        if (!s_StatusCollapsed) {
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 2));

            ImGui::Text("Game Root:");
            ImGui::SameLine(labelOffset);
            if (!g_AppConfig.GameRootPath.empty()) {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", g_AppConfig.GameRootPath.c_str());
            }
            else {
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Not Set");
            }

            ImGui::Text("WAD decompiled:");
            ImGui::SameLine(labelOffset);
            if (wadDecompiled) ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Yes");
            else               ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "No");

            ImGui::Text("FSE detected:");
            ImGui::SameLine(labelOffset);
            if (fseDetected)   ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Yes");
            else               ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No");

            ImGui::Text("Modding environment:");
            ImGui::SameLine(labelOffset);
            if (modEnvOk)      ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "OK");
            else               ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Not OK");

            ImGui::Text("Mods loaded:");
            ImGui::SameLine(labelOffset);
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%d Active", activeModsCount);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 16.0f, viewport->Pos.y + viewport->Size.y - 16.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    if (ImGui::Begin("##WatermarkOverlay", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBackground))
    {
        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 0.90f), "AlbionSecrets");
        if (g_TitleFont) ImGui::PopFont();

        const char* verStr = "7.8.26";
        float verWidth = ImGui::CalcTextSize(verStr).x;
        float winWidth = ImGui::GetWindowWidth();

        ImGui::SetCursorPosX(winWidth - verWidth);
        ImGui::TextDisabled("%s", verStr);
    }
    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.09f, 0.12f, 0.80f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(390, 445));

    if (ImGui::Begin("EgoCore Hub", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {

        ImGui::Dummy(ImVec2(0, 4));

        if (g_TitleFont) ImGui::PushFont(g_TitleFont);

        ImGui::SetWindowFontScale(2.0f);

        float titleWidth = ImGui::CalcTextSize("EgoCore").x;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "EgoCore");

        ImGui::SetWindowFontScale(1.0f);

        if (g_TitleFont) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 12));

        float cardWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 fullCardSize(cardWidth, 50.0f);

        if (DrawCardButton("##BtnLaunch", "Launch Fable", "Hook mods and start game", fullCardSize, IM_COL32(242, 193, 78, 255))) {
            g_LaunchState = 1;
        }
        ImGui::Dummy(ImVec2(0, 6));

        char modSubtext[64];
        snprintf(modSubtext, sizeof(modSubtext), "Manage DLLs & mod packages (%d active)", activeModsCount);
        if (DrawCardButton("##BtnModMgr", "Mod Manager", modSubtext, fullCardSize, IM_COL32(138, 79, 255, 255))) {
            ModManagerBackend::InitializeAndLoad();
            g_CurrentAppState = EAppState::ModsManager;
        }
        ImGui::Dummy(ImVec2(0, 6));

        if (!g_IsEditorLoading) {
            if (DrawCardButton("##BtnEditor", "Editor", "Decompile banks, edit defs & assets", fullCardSize, IM_COL32(32, 178, 170, 255))) {
                if (!g_MenuAudio.isMuted) {
                    g_MenuAudio.Toggle();
                }
                SetEditorLoadStatus("Loading definitions and banks...");
                g_EditorLoadRequested = true;
                g_IsEditorLoading = true;

                std::thread([]() {
                    PerformAutoLoad();
                    g_IsEditorLoading = false;
                    }).detach();
            }
        }
        else {
            ImGui::BeginDisabled();
            DrawCardButton("##BtnEditor", "Editor", "Loading...", fullCardSize, IM_COL32(32, 178, 170, 255));
            ImGui::EndDisabled();
        }

        ImGui::Dummy(ImVec2(0, 14));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 10));

        float halfWidth = (cardWidth - 8.0f) * 0.5f;
        ImVec2 halfCardSize(halfWidth, 50.0f);

        if (DrawCardButton("##BtnAbout", "About", "Hub & app details", halfCardSize, IM_COL32(110, 140, 175, 255))) {
            g_ShowAboutPopup = true;
        }

        ImGui::SameLine();

        if (DrawCardButton("##BtnExit", "Exit", "Close EgoCore", halfCardSize, IM_COL32(220, 65, 65, 255))) {
            if (g_AppConfig.ModSystemDirty || g_AppConfig.DefSystemDirty || g_AppConfig.TngSystemDirty) {
                g_TriggerAssetChangesExitPopup = true;
            }
            else {
                exit(0);
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    if (g_IsEditorLoading) { ImGui::OpenPopup("Loading Editor..."); }

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.32f, 0.70f, 0.67f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(480, 190), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Loading Editor...", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.8f;
            float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
            ImU32 glowCol = IM_COL32(32, 178, 170, (uint32_t)(glowAlpha * 255.0f));
            drawList->AddRect(
                ImVec2(winPos.x - expand, winPos.y - expand),
                ImVec2(winMax.x + expand, winMax.y + expand),
                glowCol, 10.0f + expand, 0, 1.2f
            );
        }

        ImVec2 headerMin = winPos;
        ImVec2 headerMax = ImVec2(winMax.x, winPos.y + 58.0f);
        drawList->AddRectFilledMultiColor(
            headerMin, headerMax,
            IM_COL32(32, 178, 170, 30), IM_COL32(110, 140, 175, 15),
            IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0)
        );

        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.35f, 0.78f, 0.75f, 1.0f), "Loading Editor");
        if (g_TitleFont) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 4));
        drawList->AddLine(
            ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
            ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
            IM_COL32(32, 178, 170, 110), 1.2f
        );
        ImGui::Dummy(ImVec2(0, 12));

        ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "%s", GetEditorLoadStatus().c_str());
        ImGui::Dummy(ImVec2(0, 6));

        static int dots = 0; if (ImGui::GetFrameCount() % 20 == 0) dots = (dots + 1) % 4;
        std::string spinner = "Please wait"; for (int i = 0; i < dots; i++) spinner += ".";
        ImGui::TextDisabled("%s", spinner.c_str());

        if (!g_IsEditorLoading) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    if (g_EditorLoadRequested && !g_IsEditorLoading) {
        g_EditorLoadRequested = false;
        g_CurrentAppState = EAppState::ModCreator;
    }
}

static void DrawBankExplorer() {
    if (!g_HasInitialized) {
        g_AppBaseDir = fs::current_path();
        LoadConfig();

        ModPackageTracker::LoadMarkedState();

        if (g_AppConfig.IsConfigured) {
            CheckModEnvironmentAndFSE(g_AppConfig.GameRootPath);
            WADBackend::TriggerPrompt(g_AppConfig.GameRootPath);
            LoadSystemBinaries(g_AppConfig.GameRootPath);
            CheckFSEInstalled(g_AppConfig.GameRootPath);

            PerformAutoLoad();

            if (g_AppConfig.SkipFrontend) {
                g_CurrentAppState = EAppState::ModCreator;
                PerformAutoLoad();
            }
            else {
                g_CurrentAppState = EAppState::Frontend;
            }
        }
        else {
            g_CurrentAppState = EAppState::Setup;
        }
        g_HasInitialized = true;
    }

    DrawEnvironmentModals();
    DrawLaunchPopup();
    WADBackend::DrawWADModal();

    if (g_IsCompiling) { ImGui::OpenPopup("Compiling..."); }

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(480, 190), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Compiling...", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.8f;
            float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
            ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
            drawList->AddRect(
                ImVec2(winPos.x - expand, winPos.y - expand),
                ImVec2(winMax.x + expand, winMax.y + expand),
                glowCol, 10.0f + expand, 0, 1.2f
            );
        }

        ImVec2 headerMin = winPos;
        ImVec2 headerMax = ImVec2(winMax.x, winPos.y + 58.0f);
        drawList->AddRectFilledMultiColor(
            headerMin, headerMax,
            IM_COL32(242, 193, 78, 30), IM_COL32(110, 140, 175, 15),
            IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0)
        );

        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Compiling Assets & Definitions");
        if (g_TitleFont) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 4));
        drawList->AddLine(
            ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
            ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
            IM_COL32(242, 193, 78, 110), 1.2f
        );
        ImGui::Dummy(ImVec2(0, 12));

        ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "%s", g_CompileStatus.c_str());
        ImGui::Dummy(ImVec2(0, 6));

        static int dots = 0; if (ImGui::GetFrameCount() % 20 == 0) dots = (dots + 1) % 4;
        std::string spinner = "Please wait"; for (int i = 0; i < dots; i++) spinner += ".";
        ImGui::TextDisabled("%s", spinner.c_str());

        if (!g_IsCompiling) {
            ImGui::CloseCurrentPopup();
            const bool launch = g_PendingGameLaunch && g_DefCompileSuccess;
            g_PendingGameLaunch = false;
            if (launch) {
                ModManagerBackend::LaunchGame();
            }
            else {
                g_TriggerCompileSuccess = true;
                PerformAutoLoad();
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);


    if (g_TriggerCompileSuccess) {
        if (g_DefCompileSuccess) {
            ImGui::OpenPopup("Compile Complete");
        }
        else {
            ImGui::OpenPopup("Compile Error");
        }
        g_TriggerCompileSuccess = false;
    }

    // --- 1. COMPILE COMPLETE ---
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(480, 210), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Compile Complete", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.8f;
            float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
            ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
            drawList->AddRect(ImVec2(winPos.x - expand, winPos.y - expand), ImVec2(winMax.x + expand, winMax.y + expand), glowCol, 10.0f + expand, 0, 1.2f);
        }

        ImVec2 headerMin = winPos;
        ImVec2 headerMax = ImVec2(winMax.x, winPos.y + 58.0f);
        drawList->AddRectFilledMultiColor(headerMin, headerMax, IM_COL32(242, 193, 78, 30), IM_COL32(110, 140, 175, 15), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));

        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Compile Complete");
        if (g_TitleFont) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 4));
        drawList->AddLine(
            ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
            ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
            IM_COL32(242, 193, 78, 110), 1.2f
        );
        ImGui::Dummy(ImVec2(0, 12));

        ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.50f, 1.0f), "Successfully compiled definitions!");
        ImGui::TextDisabled("Generated: frontend.bin, game.bin, names.bin, script.bin");
        ImGui::Dummy(ImVec2(0, 14));

        float closeBtnWidth = 120.0f;
        ImGui::SetCursorPosX((winSize.x - closeBtnWidth) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        if (ImGui::Button("OK", ImVec2(closeBtnWidth, 26.0f))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);


    // --- 2. COMPILE ERROR ---
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(880, 480), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Compile Error", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.8f;
            float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
            ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
            drawList->AddRect(ImVec2(winPos.x - expand, winPos.y - expand), ImVec2(winMax.x + expand, winMax.y + expand), glowCol, 10.0f + expand, 0, 1.2f);
        }

        ImVec2 headerMin = winPos;
        ImVec2 headerMax = ImVec2(winMax.x, winPos.y + 58.0f);
        drawList->AddRectFilledMultiColor(headerMin, headerMax, IM_COL32(220, 65, 65, 30), IM_COL32(110, 140, 175, 15), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));

        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "Compilation Error");
        if (g_TitleFont) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 4));
        drawList->AddLine(
            ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
            ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
            IM_COL32(242, 193, 78, 110), 1.2f
        );
        ImGui::Dummy(ImVec2(0, 12));

        if (!g_DefCompileLog.empty()) {
            ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "The definition compiler reported the following diagnostic output:");
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.05f, 0.07f, 0.80f));
            extern ImFont* g_CodeFont;
            if (g_CodeFont) ImGui::PushFont(g_CodeFont);
            ImGui::InputTextMultiline("##defcompilelog", g_DefCompileLog.data(),
                g_DefCompileLog.size() + 1, ImVec2(ImGui::GetContentRegionAvail().x, 300),
                ImGuiInputTextFlags_ReadOnly);
            if (g_CodeFont) ImGui::PopFont();
            ImGui::PopStyleColor();

            ImGui::Dummy(ImVec2(0, 10));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.18f, 0.24f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.32f, 0.42f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.14f, 0.18f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

            if (ImGui::Button("Copy Log to Clipboard", ImVec2(180, 28))) {
                ImGui::SetClipboardText(g_DefCompileLog.c_str());
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);

            ImGui::SameLine(0, 12.0f);
        }
        else {
            ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "Please check your defs for syntax or linking errors.");
            ImGui::Dummy(ImVec2(0, 10));
        }
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        if (ImGui::Button("OK", ImVec2(120, 28))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    if (g_ShowAboutPopup) {
        ImGui::OpenPopup("About EgoCore");
        g_ShowAboutPopup = false;
    }

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(560, 540), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("About EgoCore", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.8f;
            float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
            ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
            drawList->AddRect(
                ImVec2(winPos.x - expand, winPos.y - expand),
                ImVec2(winMax.x + expand, winMax.y + expand),
                glowCol, 10.0f + expand, 0, 1.2f
            );
        }

        ImVec2 headerMin = winPos;
        ImVec2 headerMax = ImVec2(winMax.x, winPos.y + 70.0f);
        drawList->AddRectFilledMultiColor(
            headerMin, headerMax,
            IM_COL32(242, 193, 78, 30),
            IM_COL32(110, 140, 175, 15),
            IM_COL32(0, 0, 0, 0),
            IM_COL32(0, 0, 0, 0)
        );
        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "EgoCore");
        if (g_TitleFont) ImGui::PopFont();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 0.9f), "Asset Bank Editor and Mod Manager for Fable");

        ImGui::TextDisabled("Version: 7.8.26");
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 0.8f), "Creator: AlbionSecrets");
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 0.8f), "Co-developer: jamen");

        ImGui::Dummy(ImVec2(0, 6));

        drawList->AddLine(
            ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
            ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
            IM_COL32(242, 193, 78, 110), 1.2f
        );
        ImGui::Dummy(ImVec2(0, 10));

        float footerHeight = 42.0f;
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.04f, 0.05f, 0.07f, 0.50f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.30f, 0.35f, 0.45f, 0.60f));

        ImGui::BeginChild("##about_scroll_content", ImVec2(0, -footerHeight), false);

        ImGui::TextWrapped("EgoCore is the culmination of over twenty years of obsession with the inner workings of Fable. What began as programming related curiosity has evolved into a six-month intensive development journey to provide the community with a modern, robust, and versatile modding framework.");
        ImGui::Dummy(ImVec2(0, 8));

        ImGui::TextWrapped("I believe the tools to keep this game alive should be accessible to everyone. That is why EgoCore is, and will always be, Free and Open Source.");
        ImGui::Dummy(ImVec2(0, 8));

        ImGui::TextWrapped("Developing a tool of this scale involves hundreds of hours of reverse engineering, debugging, and refinement. If EgoCore has saved you time or helped you bring a new vision to life, please consider supporting the project.");

        ImGui::Dummy(ImVec2(0, 12));

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.05f, 0.07f, 0.70f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));

        if (ImGui::BeginChild("##links_card", ImVec2(0, 110), true, ImGuiWindowFlags_NoScrollbar)) {
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 0.9f), "RESOURCE LINKS");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 4));

            auto DrawLink = [](const char* label, const char* url) {
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.75f, 1.0f, 1.0f));
                ImGui::Text("  > %s", label);
                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered()) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    ImVec2 textSize = ImGui::CalcTextSize(label);
                    ImGui::GetWindowDrawList()->AddLine(
                        ImVec2(pos.x + 20.0f, pos.y + textSize.y + 2.0f),
                        ImVec2(pos.x + 20.0f + textSize.x, pos.y + textSize.y + 2.0f),
                        IM_COL32(100, 190, 255, 255), 1.0f
                    );
                    if (ImGui::IsItemClicked()) {
                        ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
                    }
                }
                };

            DrawLink("EgoCore Source Code", "https://github.com/eeeeeAeoN/EgoCore");
            DrawLink("Fable Definition Compiler Source Code", "https://github.com/jamen/fable-defs");
            DrawLink("FableLauncher Source Code", "https://github.com/eeeeeAeoN/FableLauncher");
            DrawLink("FableScriptExtender Source Code", "https://github.com/eeeeeAeoN/FableScriptExtender");
            DrawLink("EgoCore Discord", "https://discord.gg/Rw4as5ar3S");
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 8));

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.05f, 0.07f, 0.70f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));

        if (ImGui::BeginChild("##credits_card", ImVec2(0, 110), true, ImGuiWindowFlags_NoScrollbar)) {
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 0.9f), "SPECIAL THANKS TO");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::TextColored(ImVec4(0.70f, 0.85f, 1.0f, 1.0f), "jamen");
            ImGui::SameLine(); ImGui::TextDisabled("- created the Fable Definition Compiler, used by EgoCore.");

            ImGui::TextColored(ImVec4(0.70f, 0.85f, 1.0f, 1.0f), "Odarenkoas");
            ImGui::SameLine(); ImGui::TextDisabled("- parsed and worked out most of the audio banks format.");

            ImGui::TextColored(ImVec4(0.70f, 0.85f, 1.0f, 1.0f), "MahknoBlazed");
            ImGui::SameLine(); ImGui::TextDisabled("- main tester of EgoCore.");
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleColor(2);
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));

        float closeBtnWidth = 140.0f;
        ImGui::SetCursorPosX((winSize.x - closeBtnWidth) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.18f, 0.24f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.70f, 0.30f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.50f, 0.20f, 1.00f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        if (ImGui::Button("Close", ImVec2(closeBtnWidth, 26.0f))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);


    if (g_TriggerAssetChangesExitPopup) {
        ImGui::OpenPopup("AssetChangesPending");
        g_TriggerAssetChangesExitPopup = false;
    }

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(550, 220), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("AssetChangesPending", NULL,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.8f;
            float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
            ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
            drawList->AddRect(
                ImVec2(winPos.x - expand, winPos.y - expand),
                ImVec2(winMax.x + expand, winMax.y + expand),
                glowCol, 10.0f + expand, 0, 1.2f
            );
        }

        ImVec2 headerMin = winPos;
        ImVec2 headerMax = ImVec2(winMax.x, winPos.y + 58.0f);
        drawList->AddRectFilledMultiColor(
            headerMin, headerMax,
            IM_COL32(242, 193, 78, 30),
            IM_COL32(110, 140, 175, 15),
            IM_COL32(0, 0, 0, 0),
            IM_COL32(0, 0, 0, 0)
        );

        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Pending Asset Changes");
        if (g_TitleFont) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 4));
        drawList->AddLine(
            ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
            ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
            IM_COL32(242, 193, 78, 110), 1.2f
        );
        ImGui::Dummy(ImVec2(0, 12));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 0.95f));
        ImGui::TextWrapped("You have pending changes to asset mods (reordered, enabled/disabled, or added).");
        ImGui::TextWrapped("These changes will only take effect after you launch the game through EgoCore.");
        ImGui::TextWrapped("If you exit now, the changes will not be applied.");
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 12));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        if (ImGui::Button("Launch Game", ImVec2(160, 28))) {
            ImGui::CloseCurrentPopup();
            g_LaunchState = 1;          // triggers the normal launch sequence
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0, 12.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.18f, 0.24f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.32f, 0.42f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.14f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        if (ImGui::Button("Exit", ImVec2(100, 28))) {
            ImGui::CloseCurrentPopup();
            exit(0);                    // simply exit - flags stay dirty
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);


    if (g_CurrentAppState == EAppState::Setup) {
        if (g_BackgroundTexture && g_BgWidth > 0 && g_BgHeight > 0) {
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            float scaledWidth = displaySize.x;
            float aspect = (float)g_BgHeight / (float)g_BgWidth;
            float scaledHeight = scaledWidth * aspect;

            ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();
            bgDrawList->AddImage((ImTextureID)g_BackgroundTexture, ImVec2(0.0f, 0.0f), ImVec2(scaledWidth, scaledHeight));
            bgDrawList->AddRectFilled(ImVec2(0, 0), displaySize, IM_COL32(0, 0, 0, 140));
            float vignetteSize = 280.0f;
            ImU32 colDark = IM_COL32(0, 0, 0, 255);
            ImU32 colClear = IM_COL32(0, 0, 0, 0);

            bgDrawList->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(displaySize.x, vignetteSize), colDark, colDark, colClear, colClear);
            bgDrawList->AddRectFilledMultiColor(ImVec2(0, displaySize.y - vignetteSize), ImVec2(displaySize.x, displaySize.y), colClear, colClear, colDark, colDark);
            bgDrawList->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(vignetteSize, displaySize.y), colDark, colClear, colClear, colDark);
            bgDrawList->AddRectFilledMultiColor(ImVec2(displaySize.x - vignetteSize, 0), ImVec2(displaySize.x, displaySize.y), colClear, colDark, colDark, colClear);
        }
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.09f, 0.12f, 0.96f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.45f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22.0f, 20.0f));

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(460, 260));
        ImGui::SetNextWindowFocus();

        if (ImGui::Begin("Welcome to EgoCore", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 winPos = ImGui::GetWindowPos();
            ImVec2 winSize = ImGui::GetWindowSize();
            ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

            for (int i = 3; i >= 1; i--) {
                float expand = (float)i * 1.5f;
                float glowAlpha = (1.0f - (float)i / 4.0f) * 0.30f;
                ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
                drawList->AddRect(
                    ImVec2(winPos.x - expand, winPos.y - expand),
                    ImVec2(winMax.x + expand, winMax.y + expand),
                    glowCol, 10.0f + expand, 0, 1.2f
                );
            }

            if (g_TitleFont) ImGui::PushFont(g_TitleFont);
            ImGui::SetWindowFontScale(1.30f);

            float titleWidth = ImGui::CalcTextSize("Welcome to EgoCore").x;
            ImGui::SetCursorPosX((winSize.x - titleWidth) * 0.5f);
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Welcome to EgoCore");

            ImGui::SetWindowFontScale(1.0f);
            if (g_TitleFont) ImGui::PopFont();

            ImGui::Dummy(ImVec2(0, 4));

            drawList->AddLine(
                ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
                ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
                IM_COL32(242, 193, 78, 125), 1.2f
            );
            ImGui::Dummy(ImVec2(0, 12));

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.92f, 0.90f));
            ImGui::TextWrapped("To get started, please select your main Fable game directory (the folder containing Fable.exe and the Data directory).");
            ImGui::PopStyleColor();

            ImGui::Dummy(ImVec2(0, 18));

            float cardWidth = ImGui::GetContentRegionAvail().x;
            if (DrawCardButton("##BtnSelectFolder", "Select Game Folder", "Browse to Fable root folder", ImVec2(cardWidth, 54.0f), IM_COL32(242, 193, 78, 255))) {
                std::string root = OpenFolderDialog();
                if (!root.empty()) {
                    InitializeSetup(root);

                    g_AppConfig.ModEnvironmentSetup = false;
                    g_AppConfig.FseSetup = false;
                    SaveConfig();

                    CheckModEnvironmentAndFSE(root);
                    LoadSystemBinaries(root);
                    WADBackend::TriggerPrompt(root);

                    PerformAutoLoad();

                    g_CurrentAppState = EAppState::Frontend;
                }
            }
        }
        ImGui::End();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        return;
    }

    if (g_CurrentAppState == EAppState::Frontend) {
        DrawFrontendHub();
        return;
    }

    if (g_CurrentAppState == EAppState::ModsManager) {
        DrawModManagerWindow();
        return;
    }
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.16f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.22f, 0.20f, 0.18f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.30f, 0.27f, 0.18f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.22f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.30f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.38f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.90f, 0.75f, 0.35f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.70f, 0.58f, 0.28f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.90f, 0.75f, 0.35f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.20f, 0.26f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.32f, 0.28f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.42f, 0.36f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.85f, 0.70f, 0.30f, 0.30f));
    // ----- Tab colors (new) -----
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.18f, 0.18f, 0.22f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.35f, 0.30f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.60f, 0.48f, 0.22f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.15f, 0.15f, 0.18f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(0.40f, 0.35f, 0.20f, 1.00f));
    // ----- style vars (same) -----
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.2f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.10f, 0.11f, 0.15f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.15f, 0.16f, 0.22f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));

    if (!ImGui::GetIO().WantTextInput && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {

        if (g_Keybinds.SwitchBankMode.IsPressed()) g_CurrentMode = EAppMode::Banks;
        if (g_Keybinds.SwitchDefMode.IsPressed())  g_CurrentMode = EAppMode::Defs;
        if (g_FSEWorkspace.IsInstalled && g_Keybinds.SwitchFSEMode.IsPressed()) g_CurrentMode = EAppMode::FSE;

        if (g_Keybinds.NavigateBack.IsPressed()) {
            g_IsNavigating = true;
            if (g_CurrentMode == EAppMode::Banks && !g_BankHistory.empty()) {
                if (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < g_OpenBanks.size()) {
                    g_BankForwardHistory.push_back({ g_ActiveBankIndex, g_OpenBanks[g_ActiveBankIndex].ActiveSubBankIndex, g_OpenBanks[g_ActiveBankIndex].SelectedEntryIndex });
                }

                auto node = g_BankHistory.back();
                g_BankHistory.pop_back();

                if (node.BankIndex >= 0 && node.BankIndex < g_OpenBanks.size()) {
                    g_ActiveBankIndex = node.BankIndex;
                    auto& bank = g_OpenBanks[g_ActiveBankIndex];

                    if (bank.ActiveSubBankIndex != node.SubBankIndex && node.SubBankIndex >= 0) {
                        LoadSubBankEntries(&bank, node.SubBankIndex);
                    }

                    if (node.EntryIndex >= 0 && node.EntryIndex < bank.Entries.size()) {
                        SelectEntry(&bank, node.EntryIndex);
                        g_ScrollToSelected = true;
                    }
                    else {
                        bank.SelectedEntryIndex = -1;
                    }
                }
            }
            else if (g_CurrentMode == EAppMode::Defs && !g_DefHistory.empty()) {
                if (g_CurrentDefView == EDefViewType::Defs) g_DefForwardHistory.push_back({ g_CurrentDefView, g_DefWorkspace.ActiveContextIndex, g_DefWorkspace.SelectedType, g_DefWorkspace.SelectedEntryIndex });
                else if (g_CurrentDefView == EDefViewType::Headers) g_DefForwardHistory.push_back({ g_CurrentDefView, 0, "", g_DefWorkspace.SelectedEnumIndex });
                else if (g_CurrentDefView == EDefViewType::Events) g_DefForwardHistory.push_back({ g_CurrentDefView, g_EventWorkspace.SelectedFileType, "", g_EventWorkspace.SelectedEventIndex });

                auto node = g_DefHistory.back();
                g_DefHistory.pop_back();
                g_CurrentDefView = node.View;

                if (node.View == EDefViewType::Defs) {
                    if (g_DefWorkspace.ActiveContextIndex != node.ContextIndex) {
                        g_DefWorkspace.ActiveContextIndex = node.ContextIndex;
                        LoadDefsFromFolder(g_DefWorkspace.RootPath);
                    }
                    g_DefWorkspace.SelectedType = node.Category;
                    g_DefWorkspace.SelectedEntryIndex = node.Index;
                    if (g_DefWorkspace.CategorizedDefs.count(node.Category) && node.Index >= 0 && node.Index < (int)g_DefWorkspace.CategorizedDefs[node.Category].size()) {
                        LoadDefContent(g_DefWorkspace.CategorizedDefs[node.Category][node.Index]);
                    }
                    else {
                        g_DefWorkspace.SelectedEntryIndex = -1;
                        g_DefWorkspace.Editor.SetText("");
                    }
                }
                else if (node.View == EDefViewType::Headers) {
                    g_DefWorkspace.SelectedEnumIndex = node.Index;
                    if (node.Index >= 0 && node.Index < (int)g_DefWorkspace.AllEnums.size()) LoadHeaderContent(g_DefWorkspace.AllEnums[node.Index]);
                }
                else if (node.View == EDefViewType::Events) {
                    g_EventWorkspace.SelectedFileType = node.ContextIndex;
                    g_EventWorkspace.SelectedEventIndex = node.Index;
                    EventFile* activeFile = g_EventWorkspace.GetActiveFile();
                    if (!activeFile->IsLoaded) {
                        g_EventWorkspace.LoadAll(g_DefWorkspace.RootPath);
                        activeFile = g_EventWorkspace.GetActiveFile();
                    }
                    if (node.Index >= 0 && node.Index < (int)activeFile->Events.size()) {
                        g_EventWorkspace.Editor.SetText(activeFile->Events[node.Index].Content);
                        g_EventWorkspace.OriginalContent = g_EventWorkspace.Editor.GetText();
                    }
                }
            }
            g_IsNavigating = false;
        }

        if (g_Keybinds.NavigateForward.IsPressed()) {
            g_IsNavigating = true;
            if (g_CurrentMode == EAppMode::Banks && !g_BankForwardHistory.empty()) {
                if (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < g_OpenBanks.size()) {
                    g_BankHistory.push_back({ g_ActiveBankIndex, g_OpenBanks[g_ActiveBankIndex].ActiveSubBankIndex, g_OpenBanks[g_ActiveBankIndex].SelectedEntryIndex });
                }

                auto node = g_BankForwardHistory.back();
                g_BankForwardHistory.pop_back();

                if (node.BankIndex >= 0 && node.BankIndex < g_OpenBanks.size()) {
                    g_ActiveBankIndex = node.BankIndex;
                    auto& bank = g_OpenBanks[g_ActiveBankIndex];

                    if (bank.ActiveSubBankIndex != node.SubBankIndex && node.SubBankIndex >= 0) {
                        LoadSubBankEntries(&bank, node.SubBankIndex);
                    }

                    if (node.EntryIndex >= 0 && node.EntryIndex < bank.Entries.size()) {
                        SelectEntry(&bank, node.EntryIndex);
                        g_ScrollToSelected = true;
                    }
                    else {
                        bank.SelectedEntryIndex = -1;
                    }
                }
            }
            else if (g_CurrentMode == EAppMode::Defs && !g_DefForwardHistory.empty()) {
                if (g_CurrentDefView == EDefViewType::Defs) g_DefHistory.push_back({ g_CurrentDefView, g_DefWorkspace.ActiveContextIndex, g_DefWorkspace.SelectedType, g_DefWorkspace.SelectedEntryIndex });
                else if (g_CurrentDefView == EDefViewType::Headers) g_DefHistory.push_back({ g_CurrentDefView, 0, "", g_DefWorkspace.SelectedEnumIndex });
                else if (g_CurrentDefView == EDefViewType::Events) g_DefHistory.push_back({ g_CurrentDefView, g_EventWorkspace.SelectedFileType, "", g_EventWorkspace.SelectedEventIndex });

                auto node = g_DefForwardHistory.back();
                g_DefForwardHistory.pop_back();
                g_CurrentDefView = node.View;

                if (node.View == EDefViewType::Defs) {
                    if (g_DefWorkspace.ActiveContextIndex != node.ContextIndex) {
                        g_DefWorkspace.ActiveContextIndex = node.ContextIndex;
                        LoadDefsFromFolder(g_DefWorkspace.RootPath);
                    }
                    g_DefWorkspace.SelectedType = node.Category;
                    g_DefWorkspace.SelectedEntryIndex = node.Index;
                    if (g_DefWorkspace.CategorizedDefs.count(node.Category) && node.Index >= 0 && node.Index < (int)g_DefWorkspace.CategorizedDefs[node.Category].size()) {
                        LoadDefContent(g_DefWorkspace.CategorizedDefs[node.Category][node.Index]);
                    }
                    else {
                        g_DefWorkspace.SelectedEntryIndex = -1;
                        g_DefWorkspace.Editor.SetText("");
                    }
                }
                else if (node.View == EDefViewType::Headers) {
                    g_DefWorkspace.SelectedEnumIndex = node.Index;
                    if (node.Index >= 0 && node.Index < (int)g_DefWorkspace.AllEnums.size()) LoadHeaderContent(g_DefWorkspace.AllEnums[node.Index]);
                }
                else if (node.View == EDefViewType::Events) {
                    g_EventWorkspace.SelectedFileType = node.ContextIndex;
                    g_EventWorkspace.SelectedEventIndex = node.Index;
                    EventFile* activeFile = g_EventWorkspace.GetActiveFile();
                    if (!activeFile->IsLoaded) {
                        g_EventWorkspace.LoadAll(g_DefWorkspace.RootPath);
                        activeFile = g_EventWorkspace.GetActiveFile();
                    }
                    if (node.Index >= 0 && node.Index < (int)activeFile->Events.size()) {
                        g_EventWorkspace.Editor.SetText(activeFile->Events[node.Index].Content);
                        g_EventWorkspace.OriginalContent = g_EventWorkspace.Editor.GetText();
                    }
                }
            }
            g_IsNavigating = false;
        }
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    // BeginMenuBar() paints its own separate background (ImGuiCol_MenuBarBg).
    // Left at the theme default it doesn't exactly match ChildBg, which shows
    // up as a thin seam/line along the bottom edge of the bar. This is a pure
    // color fix -- it doesn't touch height, position, or layout at all, so it
    // can't reintroduce any of the alignment/clipping issues from before.
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

    // -----------------------------------------------------------------------
    // Reverted back to the exact geometry from the known-good baseline: plain
    // ImGui::GetFrameHeight() for the child, no FramePadding pushes, no custom
    // offset math, no measurement caching, no absolute screen-space
    // positioning. Every bug in this bar over the last several rounds came
    // from "clever" additions layered on top of this -- the baseline itself
    // was never broken. The only actual change from that baseline is
    // reskinning Banks/Defs/FSE via DrawModeTabButton for a nicer look, using
    // the same ImGui::GetFrameHeight() as their height (exactly what the old
    // ImGui::Button(60, 0) resolved to) so they still fall on the same
    // baseline as the menu titles next to them, the same way the original
    // plain buttons did.
    // -----------------------------------------------------------------------
    ImGui::BeginChild("LocalMenuBarChild", ImVec2(0, ImGui::GetFrameHeight()), false, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Return to Main Menu")) {
                g_CurrentAppState = EAppState::Frontend;
            }
            if (ImGui::MenuItem("Load Bank (.BIG / .LUT / .LUG)")) {
                std::string path = OpenFileDialog("Fable Banks\0*.big;*.lut;*.lug\0All Files\0*.*\0");
                if (!path.empty()) LoadBank(path);
            }
            if (ImGui::MenuItem("Decompile WAD (.WAD)")) {
                std::string path = OpenFileDialog("WAD Files\0*.wad\0All Files\0*.*\0");
                if (!path.empty()) WADBackend::StartManualUnpack(path);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Create Mod Package")) {
                g_ShowModPackageWindow = true;
            }
            if (ImGui::MenuItem("Run Fable")) {
                if ((g_DefWorkspace.IsDirty() || HasUnsavedBankChanges()) && g_AppConfig.ShowUnsavedChangesWarning) {
                    g_PendingRunFableLaunch = true;
                    g_DefWorkspace.TriggerUnsavedPopup = true;
                }
                else {
                    ModManagerBackend::LaunchGame();
                }
            }
            if (ImGui::MenuItem("Change Game Folder")) {
                std::string root = OpenFolderDialog();
                if (!root.empty()) {
                    ResetWorkspaceForCompile();

                    InitializeSetup(root);
                    g_AppConfig.ModEnvironmentSetup = false;
                    g_AppConfig.FseSetup = false;
                    SaveConfig();

                    CheckModEnvironmentAndFSE(root);
                    WADBackend::TriggerPrompt(root);

                    LoadSystemBinaries(root);
                    PerformAutoLoad();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                if ((g_DefWorkspace.IsDirty() || HasUnsavedBankChanges()) && g_AppConfig.ShowUnsavedChangesWarning) {
                    g_DefWorkspace.PendingNav = { DefAction::ExitProgram, "", -1 };
                    g_DefWorkspace.TriggerUnsavedPopup = true;
                }
                else exit(0);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings")) {
            if (ImGui::MenuItem("General")) {
                g_TriggerGeneralSettingsPopup = true;
            }
            if (ImGui::MenuItem("Keybindings")) {
                g_TriggerKeybindPopup = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Scaling")) {
                g_TriggerScalingPopup = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("About")) {
            if (ImGui::MenuItem("About EgoCore")) {
                g_ShowAboutPopup = true;
            }
            ImGui::EndMenu();
        }

        // ---- Right-side mode switcher: Banks / Defs / FSE, reskinned via
        // DrawModeTabButton but positioned exactly like the old plain
        // ImGui::Button(60, 0) version -- SameLine() chaining, right inside
        // the menu bar, height = GetFrameHeight() (what Button(.., 0) resolved
        // to). Width is measured per-label instead of the old hardcoded 210,
        // since the pill shape needs a bit more horizontal room than a plain
        // button did.
        const float btnHeight = ImGui::GetFrameHeight();
        const float tabPad = 30.0f; // must match DrawModeTabButton's internal padding*2
        const float tabSpacing = ImGui::GetStyle().ItemSpacing.x;

        float banksW = ImGui::CalcTextSize("Banks").x + tabPad;
        float defsW = ImGui::CalcTextSize("Defs").x + tabPad;
        float fseW = g_FSEWorkspace.IsInstalled ? (ImGui::CalcTextSize("FSE").x + tabPad) : 0.0f;

        float groupWidth = banksW + tabSpacing + defsW;
        if (g_FSEWorkspace.IsInstalled) groupWidth += tabSpacing + fseW;

        float rightAlign = ImGui::GetWindowWidth() - groupWidth - 12.0f;
        if (rightAlign > 0) ImGui::SameLine(rightAlign);

        if (DrawModeTabButton("##mode_banks", "Banks", g_CurrentMode == EAppMode::Banks, IM_COL32(216, 168, 62, 255), btnHeight)) {
            g_CurrentMode = EAppMode::Banks;
        }

        ImGui::SameLine();
        if (DrawModeTabButton("##mode_defs", "Defs", g_CurrentMode == EAppMode::Defs, IM_COL32(196, 60, 84, 255), btnHeight)) {
            g_CurrentMode = EAppMode::Defs;
        }

        if (g_FSEWorkspace.IsInstalled) {
            ImGui::SameLine();
            if (DrawModeTabButton("##mode_fse", "FSE", g_CurrentMode == EAppMode::FSE, IM_COL32(56, 176, 96, 255), btnHeight)) {
                g_CurrentMode = EAppMode::FSE;
            }
        }

        ImGui::EndMenuBar();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor(2);

    if (g_TriggerGeneralSettingsPopup) {
        ImGui::OpenPopup("General Settings");
        g_TriggerGeneralSettingsPopup = false;
    }

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(480, 320), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("General Settings", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
        ImVec2 winSize = ImGui::GetWindowSize();
        DrawGoldModalHeader("General Settings");

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 0.95f));
        ImGui::TextWrapped("Options that apply across all of EgoCore's Frontend, Editor, and Mod Manager screens.");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 10));

        if (ImGui::Checkbox("Skip Frontend Menu", &g_AppConfig.SkipFrontend)) {
            SaveConfig();
        }
        ImGui::Dummy(ImVec2(0, 4));

        if (ImGui::Checkbox("Generate Lookup Dictionary", &g_AppConfig.EnableLookupGeneration)) {
            SaveConfig();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enables the 'Ctrl+Click' Go to Definition feature.\nChanges will take effect the next time you load.");
        ImGui::Dummy(ImVec2(0, 4));

        if (ImGui::Checkbox("Enable Autosuggest", &g_AppConfig.EnableAutosuggest)) {
            SaveConfig();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shows the smart dropdown list when typing functions in FSE.");

        ImGui::Dummy(ImVec2(0, 14));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 8));

        float closeBtnWidth = 140.0f;
        ImGui::SetCursorPosX((winSize.x - closeBtnWidth) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        if (ImGui::Button("Close", ImVec2(closeBtnWidth, 26.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    if (g_TriggerKeybindPopup) {
        ImGui::OpenPopup("KeybindingsConfig");
        g_TriggerKeybindPopup = false;
    }

    static ShortcutKey* g_ListeningForRebind = nullptr;

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(560, 640), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("KeybindingsConfig", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
        ImVec2 winSize = ImGui::GetWindowSize();
        DrawGoldModalHeader("Configure Shortcuts");

        auto DrawBindRow = [](const char* label, ShortcutKey& shortcut) {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", label);
            ImGui::SameLine(170);
            std::string btnLabel = shortcut.ToString() + "##" + label;
            if (g_ListeningForRebind == &shortcut) btnLabel = "[ Press any key... ]";

            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.70f, 0.30f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.50f, 0.20f, 1.00f));
            if (ImGui::Button(btnLabel.c_str(), ImVec2(200, 0))) {
                g_ListeningForRebind = &shortcut;
            }
            ImGui::PopStyleColor(2);
            };

        float footerHeight = 52.0f;
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.04f, 0.05f, 0.07f, 0.50f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.30f, 0.35f, 0.45f, 0.60f));
        ImGui::BeginChild("##keybind_scroll_content", ImVec2(0, -footerHeight), false);

        DrawBindRow("Switch to Banks", g_Keybinds.SwitchBankMode);
        DrawBindRow("Switch to Defs", g_Keybinds.SwitchDefMode);
        DrawBindRow("Switch to FSE", g_Keybinds.SwitchFSEMode);
        DrawBindRow("Save Entry", g_Keybinds.SaveEntry);
        DrawBindRow("Compile Active", g_Keybinds.Compile);
        DrawBindRow("Navigate Back", g_Keybinds.NavigateBack);
        DrawBindRow("Navigate Forward", g_Keybinds.NavigateForward);
        DrawBindRow("Delete Entry", g_Keybinds.DeleteEntry);
        DrawBindRow("Toggle Left Panel", g_Keybinds.ToggleLeftPanel);
        DrawBindRow("Lookup Definition", g_Keybinds.LookupDefinition);

        if (g_ListeningForRebind) {
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Press desired key combination. Press ESC to cancel.");
            ImGuiIO& io = ImGui::GetIO();
            for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; i++) {
                ImGuiKey key = (ImGuiKey)i;
                bool isModifier = (key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl ||
                    key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift ||
                    key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt ||
                    key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper);

                if (ImGui::IsKeyPressed(key) && !isModifier) {
                    if (key == ImGuiKey_Escape) {
                        g_ListeningForRebind = nullptr;
                    }
                    else {
                        g_ListeningForRebind->Key = key;
                        g_ListeningForRebind->Ctrl = io.KeyCtrl;
                        g_ListeningForRebind->Shift = io.KeyShift;
                        g_ListeningForRebind->Alt = io.KeyAlt;
                        g_ListeningForRebind = nullptr;
                    }
                    break;
                }
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor(2);

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 6));

        float closeBtnWidth = 140.0f;
        ImGui::SetCursorPosX((winSize.x - closeBtnWidth) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        if (ImGui::Button("Close", ImVec2(closeBtnWidth, 26.0f))) {
            g_ListeningForRebind = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    if (g_TriggerScalingPopup) {
        ImGui::OpenPopup("ScalingConfig");
        g_TriggerScalingPopup = false;
    }

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(480, 250), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("ScalingConfig", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
        ImVec2 winSize = ImGui::GetWindowSize();
        DrawGoldModalHeader("Interface Scaling");

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Global Scale");
        ImGui::SameLine(150);

        ImGui::SetNextItemWidth(180);
        if (ImGui::SliderFloat("##UIScaleSlider", &g_UIScale, 0.5f, 1.5f, "%.1f")) {
            UpdateUIScale(g_UIScale);
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjusts font size and UI padding. Default is 1.00.");
        }

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 8));

        float closeBtnWidth = 140.0f;
        ImGui::SetCursorPosX((winSize.x - closeBtnWidth) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        if (ImGui::Button("Close", ImVec2(closeBtnWidth, 26.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    if (g_CurrentMode == EAppMode::Banks) {
        DrawBankTab();
    }
    else if (g_CurrentMode == EAppMode::Defs) {
        DrawDefTab();
    }
    else if (g_CurrentMode == EAppMode::FSE) {
        DrawFSETab();
    }

    if (g_DefWorkspace.TriggerUnsavedPopup) {
        ImGui::OpenPopup("UnsavedChangesGlobal");
        g_DefWorkspace.TriggerUnsavedPopup = false;
    }

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(520, 240), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("UnsavedChangesGlobal", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
        DrawGoldModalHeader("Unsaved Changes");

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 0.95f));
        ImGui::TextWrapped("You have unsaved changes in your Definitions or Banks. What would you like to do?");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 8));

        static bool dontShowUnsaved = false;
        ImGui::Checkbox("Don't show this message again", &dontShowUnsaved);
        ImGui::Dummy(ImVec2(0, 10));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        if (ImGui::Button("Save & Continue", ImVec2(140, 0))) {
            if (g_DefWorkspace.ShowDefsMode) {
                if (!g_DefWorkspace.SelectedType.empty() && g_DefWorkspace.SelectedEntryIndex != -1)
                    SaveDefEntry(g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType][g_DefWorkspace.SelectedEntryIndex]);
            }
            else {
                if (g_DefWorkspace.SelectedEnumIndex != -1)
                    SaveHeaderEntry(g_DefWorkspace.AllEnums[g_DefWorkspace.SelectedEnumIndex]);
            }
            for (auto& b : g_OpenBanks) {
                if (b.Type == EBankType::Audio && b.LugParserPtr && b.LugParserPtr->IsDirty) {
                    SaveAudioBank(&b);
                }
            }

            if (dontShowUnsaved) {
                g_AppConfig.ShowUnsavedChangesWarning = false;
                SaveConfig();
            }

            if (g_PendingRunFableLaunch) {
                g_PendingRunFableLaunch = false;
                ModManagerBackend::LaunchGame();
            }
            else if (g_DefWorkspace.PendingNav.Action == DefAction::ExitProgram) exit(0);
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.20f, 0.18f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.62f, 0.28f, 0.24f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.15f, 0.13f, 1.00f));
        if (ImGui::Button("Discard & Continue", ImVec2(140, 0))) {
            if (dontShowUnsaved) {
                g_AppConfig.ShowUnsavedChangesWarning = false;
                SaveConfig();
            }

            if (g_PendingRunFableLaunch) {
                g_PendingRunFableLaunch = false;
                ModManagerBackend::LaunchGame();
            }
            else if (g_DefWorkspace.PendingNav.Action == DefAction::ExitProgram) exit(0);
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.20f, 0.23f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.34f, 0.31f, 0.22f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.42f, 0.36f, 0.20f, 1.00f));
        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            g_PendingRunFableLaunch = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    if (g_CurrentAppState == EAppState::ModCreator) {
        DrawModPackageWindow();
    }
    ImGui::PopStyleColor(22);
    ImGui::PopStyleVar(7);
}