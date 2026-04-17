#pragma once
#include "imgui.h"
#include "FileDialogs.h"
#include "DefExplorer.h"
#include "BankTabUI.h" 
#include "FSEBackend.h"
#include "FSETabUI.h"
#include "InputManager.h"
#include "ModManagerUI.h"
#include "WADBackend.h"
#include <windows.h>
#include <shellapi.h>

static bool g_HasInitialized = false;
static bool g_TriggerKeybindPopup = false;
inline bool g_TriggerScalingPopup = false;
inline bool g_TriggerGeneralSettingsPopup = false;
inline bool g_ShowAboutPopup = false;
inline float g_UIScale = 1.0f;

enum class EAppState {
    Setup,
    Frontend,
    ModCreator,
    ModsManager
};

inline EAppState g_CurrentAppState = EAppState::Setup;

static void UpdateUIScale(float scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;

    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    style.ScaleAllSizes(scale);
}

static void DrawLaunchPopup() {
    if (g_LaunchState == 1) {
        ImGui::OpenPopup("Launching Fable");
        g_LaunchState = 2;
    }

    if (ImGui::BeginPopupModal("Launching Fable", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("Preparing to launch Fable...");
        ImGui::Dummy(ImVec2(0, 10));

        if (g_AppConfig.ModSystemDirty || g_AppConfig.DefSystemDirty) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Compiling modified banks and applying load order...");
            ImGui::Text("Please wait. The application will exit automatically when finished.");
        }
        else {
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Load order clean. Launching directly...");
        }

        if (g_LaunchState >= 2 && g_LaunchState < 6) {
            g_LaunchState++;
        }
        else if (g_LaunchState == 6) {
            g_LaunchState = 7; // Advance state so it doesn't loop
            ModManagerBackend::ProcessModsAndLaunch();
        }

        // BUGFIX: If the compiler successfully kicked off, close this popup 
        // so the "Compiling..." modal can take over!
        if (g_IsCompiling) {
            g_LaunchState = 0;
            ImGui::CloseCurrentPopup();
        }

        if (g_ShowFallbackLaunchPopup) {
            g_LaunchState = 0;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

static void DrawFrontendHub() {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 450));

    if (ImGui::Begin("EgoCore Hub", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {

        ImGui::SetWindowFontScale(1.5f);
        ImGui::Text("EgoCore");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 10));

        if (ImGui::Button("Launch Fable", ImVec2(-1, 50))) {
            g_LaunchState = 1;
        }
        ImGui::Dummy(ImVec2(0, 5));

        if (ImGui::Button("Mod Manager", ImVec2(-1, 50))) {
            ModManagerBackend::InitializeAndLoad();
            g_CurrentAppState = EAppState::ModsManager;
        }
        ImGui::Dummy(ImVec2(0, 5));

        if (ImGui::Button("Editor", ImVec2(-1, 50))) {
            g_CurrentAppState = EAppState::ModCreator;
            PerformAutoLoad();
        }
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 10));

        if (ImGui::Button("About", ImVec2(185, 40))) {
            g_ShowAboutPopup = true;
        }
        ImGui::SameLine(ImGui::GetWindowWidth() - 185.0f - ImGui::GetStyle().WindowPadding.x);
        if (ImGui::Button("Exit", ImVec2(185, 40))) {
            exit(0);
        }
    }
    ImGui::End();
}

static void DrawBankExplorer() {
    if (!g_HasInitialized) {
        LoadConfig();

        ModPackageTracker::LoadMarkedState();

        if (g_AppConfig.IsConfigured) {
            WADBackend::TriggerPrompt(g_AppConfig.GameRootPath);
            LoadSystemBinaries(g_AppConfig.GameRootPath);
            CheckFSEInstalled(g_AppConfig.GameRootPath);

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

    DrawLaunchPopup();
    WADBackend::DrawWADModal();

    if (g_IsCompiling) { ImGui::OpenPopup("Compiling..."); }
    if (ImGui::BeginPopupModal("Compiling...", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("%s", g_CompileStatus.c_str());
        ImGui::Separator();
        static int dots = 0; if (ImGui::GetFrameCount() % 20 == 0) dots = (dots + 1) % 4;
        std::string spinner = "Please wait"; for (int i = 0; i < dots; i++) spinner += ".";
        ImGui::Text("%s", spinner.c_str());

        if (!g_IsCompiling) {
            ImGui::CloseCurrentPopup();
            if (g_PendingGameLaunch) {
                g_PendingGameLaunch = false;
                ModManagerBackend::LaunchGame();
            }
            else {
                g_TriggerCompileSuccess = true;
                PerformAutoLoad();
            }
        }
        ImGui::EndPopup();
    }

    if (g_ShowFallbackLaunchPopup) {
        ImGui::OpenPopup("Launcher Not Found");
    }

    if (ImGui::BeginPopupModal("Launcher Not Found", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("FableLauncher.exe was not found in your Fable directory.");
        ImGui::Text("Make sure you have downloaded it or properly set it up.");
        ImGui::Dummy(ImVec2(0, 10));

        ImGui::Text("Do you want to run Fable.exe instead?");
        ImGui::TextDisabled("(Note any .dll hook mods won't run in this mode)");
        ImGui::Separator();

        if (ImGui::Button("Launch", ImVec2(120, 0))) {
            std::string fallbackPath = g_AppConfig.GameRootPath + "\\Fable.exe";
            ShellExecuteA(NULL, "open", fallbackPath.c_str(), NULL, g_AppConfig.GameRootPath.c_str(), SW_SHOWDEFAULT);
            exit(0);
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            g_ShowFallbackLaunchPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (g_TriggerCompileSuccess) {
        if (g_DefCompileSuccess) {
            ImGui::OpenPopup("Compile Complete");
        }
        else {
            ImGui::OpenPopup("Compile Error");
        }
        g_TriggerCompileSuccess = false;
    }

    if (ImGui::BeginPopupModal("Compile Complete", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Successfully compiled definitions!");
        ImGui::Text("Generated: frontend.bin, game.bin, names.bin, script.bin");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Compile Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error during def compilation.");
        ImGui::Text("Please check your defs for syntax or linking errors.");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (g_TriggerGeneralSettingsPopup) {
        ImGui::OpenPopup("General Settings");
        g_TriggerGeneralSettingsPopup = false;
    }

    if (ImGui::BeginPopupModal("General Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("General Application Settings");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 5));

        if (ImGui::Checkbox("Skip Frontend Menu", &g_AppConfig.SkipFrontend)) {
            SaveConfig();
        }

        if (ImGui::Checkbox("Generate Lookup Dictionary", &g_AppConfig.EnableLookupGeneration)) {
            SaveConfig();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enables the 'Ctrl+Click' Go to Definition feature.\nChanges will take effect the next time you load.");

        if (ImGui::Checkbox("Enable Autosuggest", &g_AppConfig.EnableAutosuggest)) {
            SaveConfig();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shows the smart dropdown list when typing functions in FSE.");

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (g_ShowAboutPopup) {
        ImGui::OpenPopup("About EgoCore");
        g_ShowAboutPopup = false;
    }

    ImGui::SetNextWindowSize(ImVec2(520, 480), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("About EgoCore", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {
        float buttonAreaHeight = 45.0f;
        ImGui::BeginChild("##content", ImVec2(0, -buttonAreaHeight));
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "EgoCore - Asset Bank Editor for Fable");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Text("Version: 17.4.26 [BETA]");
        ImGui::Text("Author: AeoN (AlbionSecrets)");
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::TextWrapped("EgoCore is the culmination of over twenty years of obsession with the inner workings of Fable. What began as programming related curiosity has evolved into a six-month intensive development journey to provide the community with a modern, robust, and versatile modding framework.");
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::TextWrapped("I believe the tools to keep this game alive should be accessible to everyone. That is why EgoCore is, and will always be, Free and Open Source.");
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::TextWrapped("Developing a tool of this scale involves hundreds of hours of reverse engineering, debugging, and refinement. I don't believe in paywalling progress, so I rely entirely on the generosity of the community to keep this project sustainable. If EgoCore has saved you time or helped you bring a new vision to life, please consider supporting the project.");
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 10));

        auto DrawLink = [](const char* label, const char* url) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
            ImGui::Text("%s", label);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                if (ImGui::IsItemClicked()) {
                    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
                }
            }
        };

        DrawLink("Support EgoCore on Ko-fi", "https://ko-fi.com/aeon5798");
        DrawLink("Source Code", "https://github.com/eeeeeAeoN/EgoCore");
        DrawLink("EgoCore Discord", "https://discord.gg/Rw4as5ar3S");
        ImGui::Text("[Documentation] - Coming Soon!");
        ImGui::Dummy(ImVec2(0, 10));

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Text("Special thanks to:");
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Text("Odarenkoas - helped by parsing most of the audio banks format.");
        ImGui::Text("MahknoBlazed - main tester of EgoCore.");
        ImGui::Text("Jamen - helped in various ways during development.");
        ImGui::Dummy(ImVec2(0, 10));

        ImGui::EndChild();

        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (g_CurrentAppState == EAppState::Setup) {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 200));
        ImGui::SetNextWindowFocus();

        if (ImGui::Begin("Welcome to EgoCore", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
            ImGui::Text("Welcome to EgoCore!");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::TextWrapped("To get started, please select your main Fable game folder (The folder containing Fable.exe and Data).");
            ImGui::Dummy(ImVec2(0, 20));

            if (ImGui::Button("Select Game Folder", ImVec2(-1, 40))) {
                std::string root = OpenFolderDialog();
                if (!root.empty()) {
                    InitializeSetup(root);
                    LoadSystemBinaries(root);
                    WADBackend::TriggerPrompt(root);
                    g_CurrentAppState = EAppState::Frontend;
                }
            }
        }
        ImGui::End();
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
                g_LaunchState = 1;
            }
            if (ImGui::MenuItem("Change Game Folder")) {
                std::string root = OpenFolderDialog();
                if (!root.empty()) { InitializeSetup(root); LoadSystemBinaries(root); }
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

        float rightAlign = ImGui::GetWindowWidth() - 210.0f;
        if (rightAlign > 0) ImGui::SameLine(rightAlign);

        bool isBanks = (g_CurrentMode == EAppMode::Banks);
        if (isBanks) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.43f, 0.34f, 0.17f, 1.0f));
        if (ImGui::Button("Banks", ImVec2(60, 0))) g_CurrentMode = EAppMode::Banks;
        if (isBanks) ImGui::PopStyleColor();

        ImGui::SameLine();

        bool isDefs = (g_CurrentMode == EAppMode::Defs);
        if (isDefs) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.52f, 0.14f, 0.24f, 1.0f));
        if (ImGui::Button("Defs", ImVec2(60, 0))) g_CurrentMode = EAppMode::Defs;
        if (isDefs) ImGui::PopStyleColor();

        ImGui::SameLine();

        if (g_FSEWorkspace.IsInstalled) {
            ImGui::SameLine();
            bool isFSE = (g_CurrentMode == EAppMode::FSE);
            if (isFSE) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.52f, 0.24f, 1.0f));
            if (ImGui::Button("FSE", ImVec2(60, 0))) g_CurrentMode = EAppMode::FSE;
            if (isFSE) ImGui::PopStyleColor();
        }

        ImGui::EndMenuBar();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (g_TriggerKeybindPopup) {
        ImGui::OpenPopup("KeybindingsConfig");
        g_TriggerKeybindPopup = false;
    }

    static ShortcutKey* g_ListeningForRebind = nullptr;
    if (ImGui::BeginPopupModal("KeybindingsConfig", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Configure Shortcuts");
        ImGui::Separator();

        auto DrawBindRow = [](const char* label, ShortcutKey& shortcut) {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", label);
            ImGui::SameLine(150);
            std::string btnLabel = shortcut.ToString() + "##" + label;
            if (g_ListeningForRebind == &shortcut) btnLabel = "[ Press any key... ]";

            if (ImGui::Button(btnLabel.c_str(), ImVec2(180, 0))) {
                g_ListeningForRebind = &shortcut;
            }
            };

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
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Press desired key combination. Press ESC to cancel.");
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

        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            g_ListeningForRebind = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (g_TriggerScalingPopup) {
        ImGui::OpenPopup("ScalingConfig");
        g_TriggerScalingPopup = false;
    }

    if (ImGui::BeginPopupModal("ScalingConfig", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Interface Scaling");
        ImGui::Separator();

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

        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

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

    if (ImGui::BeginPopupModal("UnsavedChangesGlobal", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("You have unsaved changes in your Definitions or Banks.");
        ImGui::Text("What would you like to do?");
        ImGui::Separator();

        static bool dontShowUnsaved = false;
        ImGui::Checkbox("Don't show this message again", &dontShowUnsaved);
        ImGui::Dummy(ImVec2(0, 10));

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

            if (g_DefWorkspace.PendingNav.Action == DefAction::ExitProgram) exit(0);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Discard & Continue", ImVec2(140, 0))) {
            if (dontShowUnsaved) {
                g_AppConfig.ShowUnsavedChangesWarning = false;
                SaveConfig();
            }
            if (g_DefWorkspace.PendingNav.Action == DefAction::ExitProgram) exit(0);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (g_CurrentAppState == EAppState::ModCreator) {
        DrawModPackageWindow();
    }
}