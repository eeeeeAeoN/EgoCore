#pragma once
#include "imgui.h"
#include "FileDialogs.h"
#include "DefBackend.h"
#include "BankExplorer.h"
#include "ConfigBackend.h" 
#include "CompilerBackend.h"

static void DrawDefTab() {
    static float leftPaneWidth = 350.0f;
    if (leftPaneWidth < 50.0f) leftPaneWidth = 50.0f;
    if (leftPaneWidth > ImGui::GetWindowWidth() - 100.0f) leftPaneWidth = ImGui::GetWindowWidth() - 100.0f;

    static DefEntry entryToDeleteCopy;
    static bool triggerDeletePopup = false;
    static std::string typeToAddPending = "";
    static bool triggerAddPopup = false;
    static bool triggerCompileSuccess = false;

    auto RequestLoadDef = [&](const std::string& type, int index) {
        if (g_DefWorkspace.IsDirty() && g_AppConfig.ShowUnsavedChangesWarning) {
            g_DefWorkspace.PendingNav = { DefAction::SwitchToDef, type, index };
            g_DefWorkspace.TriggerUnsavedPopup = true;
        }
        else {
            g_DefWorkspace.ShowDefsMode = true;
            g_DefWorkspace.SelectedType = type;
            g_DefWorkspace.SelectedEntryIndex = index;
            LoadDefContent(g_DefWorkspace.CategorizedDefs[type][index]);
        }
        };

    auto RequestLoadHeader = [&](int index) {
        if (g_DefWorkspace.IsDirty() && g_AppConfig.ShowUnsavedChangesWarning) {
            g_DefWorkspace.PendingNav = { DefAction::SwitchToHeader, "", index };
            g_DefWorkspace.TriggerUnsavedPopup = true;
        }
        else {
            g_DefWorkspace.ShowDefsMode = false;
            g_DefWorkspace.SelectedEnumIndex = index;
            LoadHeaderContent(g_DefWorkspace.AllEnums[index]);
        }
        };

    if (!g_DefWorkspace.IsLoaded) {
        ImGui::TextDisabled("No Definitions loaded.");
        if (ImGui::Button("Open Defs Folder")) {
            std::string folder = OpenFolderDialog();
            if (!folder.empty()) LoadDefsFromFolder(folder);
        }
    }
    else {

        if (ImGui::Button("Compile All Defs")) {
            CompileAllDefs_Stealth();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Compiles definitions for BOTH Frontend and Game.\n(Generates frontend.bin and game.bin)");

        if (g_IsCompiling) { ImGui::OpenPopup("Compiling..."); }

        if (ImGui::BeginPopupModal("Compiling...", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::Text("%s", g_CompileStatus.c_str());
            ImGui::Separator();
            static int dots = 0; if (ImGui::GetFrameCount() % 20 == 0) dots = (dots + 1) % 4;
            std::string spinner = "Please wait"; for (int i = 0; i < dots; i++) spinner += ".";
            ImGui::Text("%s", spinner.c_str());
            if (!g_IsCompiling) { ImGui::CloseCurrentPopup(); triggerCompileSuccess = true; }
            ImGui::EndPopup();
        }

        if (triggerCompileSuccess) { ImGui::OpenPopup("Compile Complete"); triggerCompileSuccess = false; }
        if (ImGui::BeginPopupModal("Compile Complete", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Successfully compiled definitions!");
            ImGui::Text("Generated: frontend.bin & game.bin");
            ImGui::Separator(); if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        ImGui::Separator();

        if (ImGui::BeginTabBar("DefSubTabs", ImGuiTabBarFlags_None)) {

            if (ImGui::BeginTabItem("Definitions")) {
                g_DefWorkspace.ShowDefsMode = true;
                ImGui::BeginChild("DefLeftPane", ImVec2(leftPaneWidth, 0), true);

                if (!g_DefWorkspace.Contexts.empty()) {
                    DefContext& current = g_DefWorkspace.Contexts[g_DefWorkspace.ActiveContextIndex];
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::BeginCombo("##defcontext", current.Name.c_str())) {
                        for (int i = 0; i < g_DefWorkspace.Contexts.size(); i++) {
                            bool isSelected = (g_DefWorkspace.ActiveContextIndex == i);
                            if (ImGui::Selectable(g_DefWorkspace.Contexts[i].Name.c_str(), isSelected)) {
                                g_DefWorkspace.ActiveContextIndex = i;
                                LoadDefsFromFolder(g_DefWorkspace.RootPath);
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::Separator();
                }

                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
                    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                        int newIdx = g_DefWorkspace.SelectedEntryIndex - 1;
                        if (newIdx >= 0 && !g_DefWorkspace.SelectedType.empty())
                            RequestLoadDef(g_DefWorkspace.SelectedType, newIdx);
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                        if (!g_DefWorkspace.SelectedType.empty()) {
                            int max = (int)g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType].size() - 1;
                            if (g_DefWorkspace.SelectedEntryIndex < max)
                                RequestLoadDef(g_DefWorkspace.SelectedType, g_DefWorkspace.SelectedEntryIndex + 1);
                        }
                    }
                }

                ImGui::InputText("Filter", g_DefWorkspace.FilterText, 128);
                std::string filterLower = g_DefWorkspace.FilterText;
                std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
                bool hasFilter = !filterLower.empty();
                ImGui::Separator();

                std::string typeToAdd = "";
                for (auto& [type, entries] : g_DefWorkspace.CategorizedDefs) {
                    bool showFolder = !hasFilter;
                    if (hasFilter) {
                        std::string typeLower = type;
                        std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(), ::tolower);
                        if (typeLower.find(filterLower) != std::string::npos) showFolder = true;
                        if (!showFolder) {
                            for (auto& e : entries) {
                                std::string nameLo = e.Name;
                                std::transform(nameLo.begin(), nameLo.end(), nameLo.begin(), ::tolower);
                                if (nameLo.find(filterLower) != std::string::npos) { showFolder = true; break; }
                            }
                        }
                    }
                    if (!showFolder) continue;
                    if (hasFilter) ImGui::SetNextItemOpen(true, ImGuiCond_Always);

                    ImGui::PushID(type.c_str());
                    if (ImGui::SmallButton("+")) {
                        if (g_AppConfig.ShowAddConfirm) { typeToAddPending = type; triggerAddPopup = true; }
                        else { typeToAdd = type; }
                    }
                    ImGui::PopID();
                    ImGui::SameLine();

                    if (ImGui::TreeNode(type.c_str())) {
                        for (int k = 0; k < (int)entries.size(); k++) {
                            if (hasFilter) {
                                std::string n = entries[k].Name;
                                std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                                if (n.find(filterLower) == std::string::npos) continue;
                            }

                            ImGui::PushID((type + std::to_string(k)).c_str());
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
                            if (ImGui::SmallButton("-")) {
                                if (g_AppConfig.ShowDeleteConfirm) { entryToDeleteCopy = entries[k]; triggerDeletePopup = true; }
                                else { DeleteDefEntry(entries[k]); ImGui::PopStyleColor(3); ImGui::PopID(); ImGui::TreePop(); ImGui::EndChild(); ImGui::EndTabItem(); ImGui::EndTabBar(); return; }
                            }
                            ImGui::PopStyleColor(3);
                            ImGui::PopID();

                            ImGui::SameLine();

                            std::string label = entries[k].Name + "##" + type + std::to_string(k);
                            bool isSelected = (g_DefWorkspace.SelectedType == type && g_DefWorkspace.SelectedEntryIndex == k);
                            if (ImGui::Selectable(label.c_str(), isSelected)) {
                                RequestLoadDef(type, k);
                            }
                        }
                        ImGui::TreePop();
                    }
                }
                if (!typeToAdd.empty()) CreateNewDef(typeToAdd);

                if (triggerDeletePopup) { ImGui::OpenPopup("DeleteConfirmation"); triggerDeletePopup = false; }
                if (ImGui::BeginPopupModal("DeleteConfirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Do you want to delete '%s'?", entryToDeleteCopy.Name.c_str());
                    ImGui::Separator();
                    static bool dontShowDelete = false; ImGui::Checkbox("Don't show again", &dontShowDelete);
                    if (ImGui::Button("Yes", ImVec2(100, 0))) {
                        if (dontShowDelete) { g_AppConfig.ShowDeleteConfirm = false; SaveConfig(); }
                        DeleteDefEntry(entryToDeleteCopy); ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine(); if (ImGui::Button("Cancel", ImVec2(100, 0))) ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                }

                if (triggerAddPopup) { ImGui::OpenPopup("AddConfirmation"); triggerAddPopup = false; }
                if (ImGui::BeginPopupModal("AddConfirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Create definition of type '%s'?", typeToAddPending.c_str());
                    ImGui::Separator();
                    static bool dontShowAdd = false; ImGui::Checkbox("Don't show again", &dontShowAdd);
                    if (ImGui::Button("Yes", ImVec2(100, 0))) {
                        if (dontShowAdd) { g_AppConfig.ShowAddConfirm = false; SaveConfig(); }
                        CreateNewDef(typeToAddPending); typeToAddPending = ""; ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine(); if (ImGui::Button("Cancel", ImVec2(100, 0))) { typeToAddPending = ""; ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }

                ImGui::EndChild();
                ImGui::SameLine();
                ImGui::InvisibleButton("vsplitterDef", ImVec2(4.0f, -1.0f));
                if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                if (ImGui::IsItemActive()) leftPaneWidth += ImGui::GetIO().MouseDelta.x;
                ImGui::SameLine();

                ImGui::BeginChild("DefRightPane", ImVec2(0, 0), true);
                if (!g_DefWorkspace.SelectedType.empty() && g_DefWorkspace.SelectedEntryIndex != -1) {
                    if (g_DefWorkspace.CategorizedDefs.count(g_DefWorkspace.SelectedType) &&
                        g_DefWorkspace.SelectedEntryIndex < g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType].size()) {

                        auto& entries = g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType];
                        DefEntry& entry = entries[g_DefWorkspace.SelectedEntryIndex];
                        ImGui::Text("%s", entry.Name.c_str());
                        ImGui::SameLine(); ImGui::TextDisabled("| %s", entry.SourceFile.c_str());

                        if (g_DefWorkspace.IsDirty()) {
                            ImGui::SameLine();
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
                            if (ImGui::Button("SAVE CHANGES")) SaveDefEntry(entry);
                            ImGui::PopStyleColor();
                        }
                        ImGui::Separator();

                        ImGuiIO& io = ImGui::GetIO();
                        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || g_DefWorkspace.IsSearchOpen) {
                            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F)) {
                                g_DefWorkspace.IsSearchOpen = !g_DefWorkspace.IsSearchOpen;
                                if (g_DefWorkspace.IsSearchOpen) ImGui::SetKeyboardFocusHere(0);
                            }
                        }
                        if (g_DefWorkspace.IsSearchOpen) {
                            ImGui::SetNextItemWidth(200);
                            static bool needsRefocus = false;
                            if (needsRefocus) { ImGui::SetKeyboardFocusHere(0); needsRefocus = false; }
                            if (ImGui::InputText("Find", g_DefWorkspace.SearchBuffer, 128, ImGuiInputTextFlags_EnterReturnsTrue)) {
                                FindNextInEditor(); needsRefocus = true;
                            }
                            ImGui::SameLine(); if (ImGui::Button("Next")) { FindNextInEditor(); needsRefocus = true; }
                            ImGui::SameLine(); ImGui::Checkbox("Aa", &g_DefWorkspace.SearchCaseSensitive);
                            ImGui::SameLine(); if (ImGui::Button("X")) g_DefWorkspace.IsSearchOpen = false;
                            ImGui::Separator();
                        }

                        if (io.KeyCtrl && io.MouseWheel != 0.0f && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
                            float step = (io.MouseWheel > 0) ? 0.2f : -0.2f;
                            g_DefWorkspace.EditorFontScale = std::clamp(g_DefWorkspace.EditorFontScale + step, 0.5f, 3.0f);
                        }
                        extern ImFont* g_EditorFont;
                        ImFont* fontToUse = g_EditorFont ? g_EditorFont : ImGui::GetFont();
                        float oldScale = fontToUse->Scale;
                        fontToUse->Scale = g_DefWorkspace.EditorFontScale;
                        ImGui::PushFont(fontToUse);
                        g_DefWorkspace.Editor.Render("CodeEditor");
                        ImGui::PopFont();
                        fontToUse->Scale = oldScale;

                        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) SaveDefEntry(entry);
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Headers")) {
                g_DefWorkspace.ShowDefsMode = false;
                ImGui::BeginChild("HeadLeftPane", ImVec2(leftPaneWidth, 0), true);

                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
                    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                        if (g_DefWorkspace.SelectedEnumIndex > 0) {
                            RequestLoadHeader(g_DefWorkspace.SelectedEnumIndex - 1);
                        }
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                        if (g_DefWorkspace.SelectedEnumIndex < (int)g_DefWorkspace.AllEnums.size() - 1) {
                            RequestLoadHeader(g_DefWorkspace.SelectedEnumIndex + 1);
                        }
                    }
                }

                ImGui::Text("Header Filter");
                ImGui::InputText("##hFilter", g_DefWorkspace.HeaderFilter, 128);
                ImGui::Separator();
                std::string hFilter = g_DefWorkspace.HeaderFilter;
                std::transform(hFilter.begin(), hFilter.end(), hFilter.begin(), ::tolower);

                if (ImGui::BeginListBox("##headerList", ImVec2(-FLT_MIN, -FLT_MIN))) {
                    for (int i = 0; i < g_DefWorkspace.AllEnums.size(); i++) {
                        if (!hFilter.empty()) {
                            std::string nameLo = g_DefWorkspace.AllEnums[i].Name;
                            std::transform(nameLo.begin(), nameLo.end(), nameLo.begin(), ::tolower);
                            if (nameLo.find(hFilter) == std::string::npos) continue;
                        }

                        std::string label = g_DefWorkspace.AllEnums[i].Name + "##" + std::to_string(i);
                        bool isSelected = (g_DefWorkspace.SelectedEnumIndex == i);

                        if (ImGui::Selectable(label.c_str(), isSelected)) {
                            RequestLoadHeader(i);
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndListBox();
                }
                ImGui::EndChild();
                ImGui::SameLine();
                ImGui::InvisibleButton("vsplitterHead", ImVec2(4.0f, -1.0f));
                if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                if (ImGui::IsItemActive()) leftPaneWidth += ImGui::GetIO().MouseDelta.x;
                ImGui::SameLine();

                ImGui::BeginChild("HeadRightPane", ImVec2(0, 0), true);
                if (g_DefWorkspace.SelectedEnumIndex != -1 && g_DefWorkspace.SelectedEnumIndex < g_DefWorkspace.AllEnums.size()) {
                    EnumEntry& entry = g_DefWorkspace.AllEnums[g_DefWorkspace.SelectedEnumIndex];
                    ImGui::Text("%s", entry.Name.c_str());
                    ImGui::SameLine(); ImGui::TextDisabled("| %s", entry.FilePath.c_str());

                    if (g_DefWorkspace.IsDirty()) {
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
                        if (ImGui::Button("SAVE CHANGES")) SaveHeaderEntry(entry);
                        ImGui::PopStyleColor();
                    }
                    ImGui::Separator();

                    ImGuiIO& io = ImGui::GetIO();
                    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || g_DefWorkspace.IsSearchOpen) {
                        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F)) {
                            g_DefWorkspace.IsSearchOpen = !g_DefWorkspace.IsSearchOpen;
                            if (g_DefWorkspace.IsSearchOpen) ImGui::SetKeyboardFocusHere(0);
                        }
                    }
                    if (g_DefWorkspace.IsSearchOpen) {
                        ImGui::SetNextItemWidth(200);
                        static bool needsRefocus = false;
                        if (needsRefocus) { ImGui::SetKeyboardFocusHere(0); needsRefocus = false; }
                        if (ImGui::InputText("Find", g_DefWorkspace.SearchBuffer, 128, ImGuiInputTextFlags_EnterReturnsTrue)) {
                            FindNextInEditor(); needsRefocus = true;
                        }
                        ImGui::SameLine(); if (ImGui::Button("Next")) { FindNextInEditor(); needsRefocus = true; }
                        ImGui::SameLine(); ImGui::Checkbox("Aa", &g_DefWorkspace.SearchCaseSensitive);
                        ImGui::SameLine(); if (ImGui::Button("X")) g_DefWorkspace.IsSearchOpen = false;
                        ImGui::Separator();
                    }

                    if (io.KeyCtrl && io.MouseWheel != 0.0f && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
                        float step = (io.MouseWheel > 0) ? 0.2f : -0.2f;
                        g_DefWorkspace.EditorFontScale = std::clamp(g_DefWorkspace.EditorFontScale + step, 0.5f, 3.0f);
                    }
                    extern ImFont* g_EditorFont;
                    ImFont* fontToUse = g_EditorFont ? g_EditorFont : ImGui::GetFont();
                    float oldScale = fontToUse->Scale;
                    fontToUse->Scale = g_DefWorkspace.EditorFontScale;
                    ImGui::PushFont(fontToUse);
                    g_DefWorkspace.Editor.Render("HeaderEditor");
                    ImGui::PopFont();
                    fontToUse->Scale = oldScale;

                    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) SaveHeaderEntry(entry);
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
}