#pragma once
#include "imgui.h"
#include "FileDialogs.h"
#include "DefExplorer.h"
#include "BankTabUI.h" 

static bool g_HasInitialized = false;

static void DrawBankExplorer() {
    if (!g_HasInitialized) {
        LoadConfig();
        if (g_AppConfig.IsConfigured) {
            PerformAutoLoad();
            LoadSystemBinaries(g_AppConfig.GameRootPath);
        }
        g_HasInitialized = true;
    }

    if (!g_AppConfig.IsConfigured) {
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
                }
            }
        }
        ImGui::End();
        return;
    }

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Bank File (.BIG / .LUT)")) {
                std::string path = OpenFileDialog("Fable Banks\0*.big;*.lut\0All Files\0*.*\0");
                if (!path.empty()) LoadBank(path);
            }

            if (ImGui::MenuItem("Open Binary Definition (.bin)")) {
                std::string path = OpenFileDialog("Binary Files\0*.bin;*.h\0All Files\0*.*\0");
                if (!path.empty()) {
                    BinaryParser parser;
                    parser.Parse(path);
                    if (parser.Data.IsParsed) g_LoadedBinaries.push_back(std::move(parser));
                }
            }

            if (ImGui::MenuItem("Change Game Folder")) {
                std::string root = OpenFolderDialog();
                if (!root.empty()) {
                    InitializeSetup(root);
                    LoadSystemBinaries(root);
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                if (g_DefWorkspace.IsDirty() && g_AppConfig.ShowUnsavedChangesWarning) {
                    g_DefWorkspace.PendingNav = { DefAction::ExitProgram, "", -1 };
                    g_DefWorkspace.TriggerUnsavedPopup = true;
                }
                else {
                    exit(0);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::SameLine();

        if (!g_LoadedBinaries.empty()) {
            ImGui::TextDisabled("| Loaded %zu Binaries", g_LoadedBinaries.size());
        }
        else {
            ImGui::TextDisabled("| %s", g_BankStatus.c_str());
        }

        ImGui::EndMainMenuBar();
    }

    if (ImGui::BeginTabBar("ModeTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Bank Archives")) {
            DrawBankTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Game Definitions")) {
            DrawDefTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Binary Definitions")) {
            DrawBinaryTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if (g_DefWorkspace.TriggerUnsavedPopup) {
        ImGui::OpenPopup("UnsavedChangesGlobal");
        g_DefWorkspace.TriggerUnsavedPopup = false;
    }

    if (ImGui::BeginPopupModal("UnsavedChangesGlobal", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("You have unsaved changes.");
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

            if (dontShowUnsaved) {
                g_AppConfig.ShowUnsavedChangesWarning = false;
                SaveConfig();
            }

            if (g_DefWorkspace.PendingNav.Action == DefAction::ExitProgram) {
                exit(0);
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Discard & Continue", ImVec2(140, 0))) {
            if (dontShowUnsaved) {
                g_AppConfig.ShowUnsavedChangesWarning = false;
                SaveConfig();
            }

            if (g_DefWorkspace.PendingNav.Action == DefAction::ExitProgram) {
                exit(0);
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}