#pragma once
#include "imgui.h"
#include "FileDialogs.h"
#include "DefExplorer.h"
#include "BankTabUI.h" 
#include "InputManager.h"

static bool g_HasInitialized = false;
static bool g_TriggerKeybindPopup = false;
inline bool g_TriggerScalingPopup = false;
inline float g_UIScale = 1.0f;

static void UpdateUIScale(float scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;

    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    style.ScaleAllSizes(scale);
}

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

    // --- GLOBAL SHORTCUT LISTENER ---
    if (!ImGui::GetIO().WantTextInput && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {

        if (g_Keybinds.SwitchBankMode.IsPressed()) g_CurrentMode = EAppMode::Banks;
        if (g_Keybinds.SwitchDefMode.IsPressed())  g_CurrentMode = EAppMode::Defs;

        // BACK NAVIGATION (Esc)
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

                    // Crucial: Load the SubBank before selecting the entry!
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

        // FORWARD NAVIGATION (F1)
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

    // --- LAYER 1: LOCAL APP MENU BAR ---
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::BeginChild("LocalMenuBarChild", ImVec2(0, ImGui::GetFrameHeight()), false, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load Bank (.BIG / .LUT / .LUG)")) {
                std::string path = OpenFileDialog("Fable Banks\0*.big;*.lut;*.lug\0All Files\0*.*\0");
                if (!path.empty()) LoadBank(path);
            }
            if (ImGui::MenuItem("Run Fable")) g_BankStatus = "Run Fable clicked (Placeholder)";
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

        if (ImGui::BeginMenu("About")) { ImGui::TextDisabled("EgoCore"); ImGui::EndMenu(); }

        float rightAlign = ImGui::GetWindowWidth() - 140.0f;
        if (rightAlign > 0) ImGui::SameLine(rightAlign);

        // --- FIXED BANKS BUTTON ---
        bool isBanks = (g_CurrentMode == EAppMode::Banks);
        if (isBanks) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.43f, 0.34f, 0.17f, 1.0f));
        if (ImGui::Button("Banks", ImVec2(60, 0))) g_CurrentMode = EAppMode::Banks;
        if (isBanks) ImGui::PopStyleColor();

        ImGui::SameLine();

        // --- FIXED DEFS BUTTON ---
        bool isDefs = (g_CurrentMode == EAppMode::Defs);
        if (isDefs) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.52f, 0.14f, 0.24f, 1.0f));
        if (ImGui::Button("Defs", ImVec2(60, 0))) g_CurrentMode = EAppMode::Defs;
        if (isDefs) ImGui::PopStyleColor();

        ImGui::EndMenuBar();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    // --- END LAYER 1 ---


    // --- KEYBINDINGS MODAL (Safely Rendered Outside) ---
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
        DrawBindRow("Save Entry", g_Keybinds.SaveEntry);
        DrawBindRow("Compile Active", g_Keybinds.Compile);
        DrawBindRow("Navigate Back", g_Keybinds.NavigateBack);
        DrawBindRow("Navigate Forward", g_Keybinds.NavigateForward);

        // Rebind Listener Logic
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
                        g_ListeningForRebind = nullptr; // Cancel
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

    // ROUTING
    if (g_CurrentMode == EAppMode::Banks) {
        DrawBankTab();
    }
    else {
        DrawDefTab();
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
}