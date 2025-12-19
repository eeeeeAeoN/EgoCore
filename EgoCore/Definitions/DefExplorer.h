#pragma once
#include "imgui.h"
#include "DefBackend.h"

static void DrawDefTab() {
    static float leftPaneWidth = 350.0f;
    if (leftPaneWidth < 50.0f) leftPaneWidth = 50.0f;
    if (leftPaneWidth > ImGui::GetWindowWidth() - 100.0f) leftPaneWidth = ImGui::GetWindowWidth() - 100.0f;

    // State for deletion
    static DefEntry entryToDeleteCopy;
    static bool triggerDeletePopup = false;

    // State for adding
    static std::string typeToAddPending = "";
    static bool triggerAddPopup = false;

    if (!g_DefWorkspace.IsLoaded) {
        ImGui::TextDisabled("No Definitions loaded.");
        if (ImGui::Button("Open Defs Folder")) {
            std::string folder = OpenFolderDialog();
            if (!folder.empty()) LoadDefsFromFolder(folder);
        }
    }
    else {
        if (ImGui::BeginTabBar("DefSubTabs", ImGuiTabBarFlags_None)) {

            // --- TAB 1: DEFINITIONS ---
            if (ImGui::BeginTabItem("Definitions")) {
                ImGui::BeginChild("DefLeftPane", ImVec2(leftPaneWidth, 0), true);

                // --- FOLDER / CONTEXT SELECTOR ---
                // This mimics the Bank Explorer's sub-bank selection
                if (!g_DefWorkspace.Contexts.empty()) {
                    DefContext& current = g_DefWorkspace.Contexts[g_DefWorkspace.ActiveContextIndex];
                    ImGui::SetNextItemWidth(-FLT_MIN); // Full width
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
                // ---------------------------------

                // Keyboard Nav (Unchanged)
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
                    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                        g_DefWorkspace.SelectedEntryIndex--;
                        if (g_DefWorkspace.SelectedEntryIndex < 0) g_DefWorkspace.SelectedEntryIndex = 0;
                        if (!g_DefWorkspace.SelectedType.empty())
                            LoadDefContent(g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType][g_DefWorkspace.SelectedEntryIndex]);
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                        if (!g_DefWorkspace.SelectedType.empty()) {
                            int max = (int)g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType].size() - 1;
                            g_DefWorkspace.SelectedEntryIndex++;
                            if (g_DefWorkspace.SelectedEntryIndex > max) g_DefWorkspace.SelectedEntryIndex = max;
                            LoadDefContent(g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType][g_DefWorkspace.SelectedEntryIndex]);
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

                    // --- ADD BUTTON ---
                    ImGui::PushID(type.c_str());
                    if (ImGui::SmallButton("+")) {
                        if (g_DefWorkspace.ShowAddConfirm) {
                            typeToAddPending = type;
                            triggerAddPopup = true;
                        }
                        else {
                            typeToAdd = type;
                        }
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

                            // --- RED DELETE BUTTON ---
                            ImGui::PushID((type + std::to_string(k)).c_str());

                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

                            if (ImGui::SmallButton("-")) {
                                if (g_DefWorkspace.ShowDeleteConfirm) {
                                    entryToDeleteCopy = entries[k];
                                    triggerDeletePopup = true;
                                }
                                else {
                                    DeleteDefEntry(entries[k]);
                                    ImGui::PopStyleColor(3);
                                    ImGui::PopID();
                                    ImGui::TreePop();
                                    ImGui::EndChild();
                                    ImGui::EndTabItem();
                                    ImGui::EndTabBar();
                                    return;
                                }
                            }
                            ImGui::PopStyleColor(3);
                            ImGui::PopID();
                            // -------------------------

                            ImGui::SameLine();

                            std::string label = entries[k].Name + "##" + type + std::to_string(k);
                            bool isSelected = (g_DefWorkspace.SelectedType == type && g_DefWorkspace.SelectedEntryIndex == k);
                            if (ImGui::Selectable(label.c_str(), isSelected)) {
                                g_DefWorkspace.SelectedType = type;
                                g_DefWorkspace.SelectedEntryIndex = k;
                                LoadDefContent(entries[k]);
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                if (!typeToAdd.empty()) CreateNewDef(typeToAdd);

                // --- POPUP HANDLERS ---

                // 1. Delete Modal
                if (triggerDeletePopup) {
                    ImGui::OpenPopup("DeleteConfirmation");
                    triggerDeletePopup = false;
                }
                if (ImGui::BeginPopupModal("DeleteConfirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Do you want to delete '%s'?", entryToDeleteCopy.Name.c_str());
                    ImGui::TextDisabled("This action permanently removes the text block.");
                    ImGui::Separator();

                    static bool dontShowDeleteAgain = false;
                    ImGui::Checkbox("Don't show this message again", &dontShowDeleteAgain);

                    ImGui::Dummy(ImVec2(0, 10));

                    if (ImGui::Button("Yes, Delete", ImVec2(120, 0))) {
                        if (dontShowDeleteAgain) g_DefWorkspace.ShowDeleteConfirm = false;
                        DeleteDefEntry(entryToDeleteCopy);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SetItemDefaultFocus();
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                // 2. Add Modal
                if (triggerAddPopup) {
                    ImGui::OpenPopup("AddConfirmation");
                    triggerAddPopup = false;
                }
                if (ImGui::BeginPopupModal("AddConfirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Create a new definition of type '%s'?", typeToAddPending.c_str());
                    ImGui::Separator();

                    static bool dontShowAddAgain = false;
                    ImGui::Checkbox("Don't show this message again", &dontShowAddAgain);

                    ImGui::Dummy(ImVec2(0, 10));

                    if (ImGui::Button("Yes, Create", ImVec2(120, 0))) {
                        if (dontShowAddAgain) g_DefWorkspace.ShowAddConfirm = false;
                        CreateNewDef(typeToAddPending);
                        typeToAddPending = "";
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SetItemDefaultFocus();
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        typeToAddPending = "";
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                // ---------------------------------

                ImGui::EndChild();

                ImGui::SameLine();
                ImGui::InvisibleButton("vsplitterDef", ImVec2(4.0f, -1.0f));
                if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                if (ImGui::IsItemActive()) leftPaneWidth += ImGui::GetIO().MouseDelta.x;
                ImGui::SameLine();

                ImGui::BeginChild("DefRightPane", ImVec2(0, 0), true);
                if (!g_DefWorkspace.SelectedType.empty() && g_DefWorkspace.SelectedEntryIndex != -1) {
                    // Check bounds again as reload might have happened
                    if (g_DefWorkspace.CategorizedDefs.count(g_DefWorkspace.SelectedType) &&
                        g_DefWorkspace.SelectedEntryIndex < g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType].size()) {

                        auto& entries = g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType];
                        DefEntry& entry = entries[g_DefWorkspace.SelectedEntryIndex];

                        ImGui::Text("%s", entry.Name.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled("| %s", entry.SourceFile.c_str());

                        bool isDirty = (g_DefWorkspace.Editor.GetText() != g_DefWorkspace.OriginalContent);
                        if (isDirty) {
                            ImGui::SameLine();
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
                            if (ImGui::Button("SAVE CHANGES")) {
                                SaveDefEntry(entry);
                            }
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

                        extern ImFont* g_EditorFont;
                        ImFont* fontToUse = g_EditorFont ? g_EditorFont : ImGui::GetFont();
                        float oldScale = fontToUse->Scale;
                        fontToUse->Scale = g_DefWorkspace.EditorFontScale;
                        ImGui::PushFont(fontToUse);
                        g_DefWorkspace.Editor.Render("CodeEditor");
                        ImGui::PopFont();
                        fontToUse->Scale = oldScale;

                        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
                            SaveDefEntry(entry);
                        }
                    }
                }
                else {
                    ImGui::Text("Select a Definition to edit.");
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // --- TAB 2: HEADERS (Unchanged) ---
            if (ImGui::BeginTabItem("Headers")) {
                ImGui::BeginChild("HeadLeftPane", ImVec2(leftPaneWidth, 0), true);

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

                        bool isSelected = (g_DefWorkspace.SelectedEnumIndex == i);
                        if (ImGui::Selectable(g_DefWorkspace.AllEnums[i].Name.c_str(), isSelected)) {
                            g_DefWorkspace.SelectedEnumIndex = i;
                            LoadHeaderContent(g_DefWorkspace.AllEnums[i]);
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
                    ImGui::SameLine();
                    ImGui::TextDisabled("| %s", entry.FilePath.c_str());

                    bool isDirty = (g_DefWorkspace.Editor.GetText() != g_DefWorkspace.OriginalContent);
                    if (isDirty) {
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
                        if (ImGui::Button("SAVE CHANGES")) {
                            SaveHeaderEntry(entry);
                        }
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

                    extern ImFont* g_EditorFont;
                    ImFont* fontToUse = g_EditorFont ? g_EditorFont : ImGui::GetFont();
                    float oldScale = fontToUse->Scale;
                    fontToUse->Scale = g_DefWorkspace.EditorFontScale;

                    ImGui::PushFont(fontToUse);
                    g_DefWorkspace.Editor.Render("HeaderEditor");
                    ImGui::PopFont();
                    fontToUse->Scale = oldScale;

                    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
                        SaveHeaderEntry(entry);
                    }

                }
                else {
                    ImGui::Text("Select a Header to view.");
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}