#pragma once
#include "imgui.h"
#include "DefBackend.h"

static void DrawDefTab() {
    static float leftPaneWidth = 350.0f;
    if (leftPaneWidth < 50.0f) leftPaneWidth = 50.0f;
    if (leftPaneWidth > ImGui::GetWindowWidth() - 100.0f) leftPaneWidth = ImGui::GetWindowWidth() - 100.0f;

    if (!g_DefWorkspace.IsLoaded) {
        ImGui::TextDisabled("No Definitions loaded.");
        if (ImGui::Button("Open Defs Folder")) {
            std::string folder = OpenFolderDialog();
            if (!folder.empty()) LoadDefsFromFolder(folder);
        }
    }
    else {
        if (ImGui::BeginTabBar("DefSubTabs", ImGuiTabBarFlags_None)) {

            if (ImGui::BeginTabItem("Definitions")) {

                ImGui::BeginChild("DefLeftPane", ImVec2(leftPaneWidth, 0), true);

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

                    if (ImGui::TreeNode(type.c_str())) {
                        for (int k = 0; k < (int)entries.size(); k++) {
                            if (hasFilter) {
                                std::string n = entries[k].Name;
                                std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                                if (n.find(filterLower) == std::string::npos) continue;
                            }
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
                ImGui::EndChild();

                ImGui::SameLine();
                ImGui::InvisibleButton("vsplitterDef", ImVec2(4.0f, -1.0f));
                if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                if (ImGui::IsItemActive()) leftPaneWidth += ImGui::GetIO().MouseDelta.x;
                ImGui::SameLine();

                ImGui::BeginChild("DefRightPane", ImVec2(0, 0), true);

                if (!g_DefWorkspace.SelectedType.empty() && g_DefWorkspace.SelectedEntryIndex != -1) {
                    auto& entries = g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType];
                    if (g_DefWorkspace.SelectedEntryIndex < (int)entries.size()) {
                        DefEntry& entry = entries[g_DefWorkspace.SelectedEntryIndex];
                        ImGui::Text("%s", entry.Name.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled("| %s", entry.SourceFile.c_str());
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
                            g_DefWorkspace.Editor.SetText(g_DefWorkspace.AllEnums[i].FullContent);
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
                    ImGui::Text("%s", g_DefWorkspace.AllEnums[g_DefWorkspace.SelectedEnumIndex].Name.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("| %s", g_DefWorkspace.AllEnums[g_DefWorkspace.SelectedEnumIndex].FilePath.c_str());
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