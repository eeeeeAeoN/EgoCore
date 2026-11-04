#pragma once
#include "imgui.h"
#include "FileDialogs.h"
#include "DefBackend.h"
#include "ConfigBackend.h" 
#include "CompilerBackend.h"
#include "EventBackend.h"
#include "ModPackageTracker.h"
#include <algorithm>
#include "InputManager.h"

struct DefDragPayload { int ContextIndex; char Category[64]; int EntryIndex; };
struct HeaderDragPayload { int EnumIndex; };
struct EventDragPayload { int FileType; int EventIndex; };

extern ImTextureID g_SaveTexture;
extern ImTextureID g_DeleteTexture;
extern ImTextureID g_CompileTexture;

static void DrawDefTab() {
    if (g_SidebarWidth < 50.0f) g_SidebarWidth = 50.0f;
    if (g_SidebarWidth > ImGui::GetWindowWidth() - 100.0f) g_SidebarWidth = ImGui::GetWindowWidth() - 100.0f;

    static bool showLeftPanel = true;

    static DefEntry entryToDeleteCopy;
    static bool triggerDeletePopup = false;
    static std::string typeToAddPending = "";
    static DefEntry s_DefToDuplicate;
    static bool s_TriggerDuplicateDef = false;
    static DefEntry s_DefToDelete;
    static bool s_TriggerDirectDeleteDef = false;
    static EventEntry s_EventToDuplicate;
    static bool s_TriggerDuplicateEvent = false;
    static bool triggerAddPopup = false;
    static int eventToDeleteIndex = -1;
    static bool triggerEventDeletePopup = false;

    if (!ImGui::GetIO().WantTextInput) {
        if (g_CurrentDefView == EDefViewType::Defs) {
            if (!g_DefWorkspace.SelectedType.empty() && g_DefWorkspace.SelectedEntryIndex != -1) {
                if (g_DefWorkspace.CategorizedDefs.count(g_DefWorkspace.SelectedType) &&
                    g_DefWorkspace.SelectedEntryIndex < (int)g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType].size()) {
                    auto& curEntry = g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType][g_DefWorkspace.SelectedEntryIndex];
                    if (g_Keybinds.MarkForPackaging.IsPressed()) {
                        StagedModPackageEntry staged;
                        staged.Category = EModAssetCategory::Definition;
                        staged.EntryName = curEntry.Name;
                        staged.TypeName = "Definition";
                        staged.BankName = std::filesystem::path(curEntry.SourceFile).filename().string();
                        staged.SubBankName = g_DefWorkspace.SelectedType;
                        staged.SourceFullPath = curEntry.SourceFile;
                        ModPackageTracker::ToggleMark(staged);
                    }
                    if (g_Keybinds.DuplicateEntry.IsPressed()) {
                        s_DefToDuplicate = curEntry;
                        s_TriggerDuplicateDef = true;
                    }
                    if (g_Keybinds.DeleteEntry.IsPressed()) {
                        if (g_AppConfig.ShowDeleteConfirm) {
                            entryToDeleteCopy = curEntry;
                            triggerDeletePopup = true;
                        }
                        else {
                            s_DefToDelete = curEntry;
                            s_TriggerDirectDeleteDef = true;
                        }
                    }
                }
            }
        }
        else if (g_CurrentDefView == EDefViewType::Headers) {
            if (g_DefWorkspace.SelectedEnumIndex != -1 && g_DefWorkspace.SelectedEnumIndex < (int)g_DefWorkspace.AllEnums.size()) {
                auto& curEnum = g_DefWorkspace.AllEnums[g_DefWorkspace.SelectedEnumIndex];
                if (g_Keybinds.MarkForPackaging.IsPressed()) {
                    StagedModPackageEntry staged;
                    staged.Category = EModAssetCategory::Header;
                    staged.EntryName = curEnum.Name;
                    staged.TypeName = "C++ Header";
                    staged.BankName = std::filesystem::path(curEnum.FilePath).filename().string();
                    staged.SubBankName = "Defs";
                    staged.SourceFullPath = curEnum.FilePath;
                    ModPackageTracker::ToggleMark(staged);
                }
            }
        }
        else if (g_CurrentDefView == EDefViewType::Events) {
            EventFile* curEvtFile = g_EventWorkspace.GetActiveFile();
            if (curEvtFile && g_EventWorkspace.SelectedEventIndex != -1 && g_EventWorkspace.SelectedEventIndex < (int)curEvtFile->Events.size()) {
                auto& curEvt = curEvtFile->Events[g_EventWorkspace.SelectedEventIndex];
                if (g_Keybinds.MarkForPackaging.IsPressed()) {
                    StagedModPackageEntry staged;
                    staged.Category = EModAssetCategory::AnimationEvent;
                    staged.EntryName = curEvt.AnimName;
                    staged.TypeName = (g_EventWorkspace.SelectedFileType == 0) ? "Sound Event" : "Game Event";
                    staged.BankName = curEvtFile->FileName;
                    staged.SubBankName = "Misc";
                    staged.SourceFullPath = curEvtFile->FilePath;
                    ModPackageTracker::ToggleMark(staged);
                }
                if (g_Keybinds.DuplicateEntry.IsPressed()) {
                    s_EventToDuplicate = curEvt;
                    s_TriggerDuplicateEvent = true;
                }
                if (g_Keybinds.DeleteEntry.IsPressed()) {
                    if (g_AppConfig.ShowDeleteConfirm) {
                        eventToDeleteIndex = g_EventWorkspace.SelectedEventIndex;
                        triggerEventDeletePopup = true;
                    }
                    else {
                        int delIdx = g_EventWorkspace.SelectedEventIndex;
                        ModPackageTracker::RemoveMark(curEvtFile->FileName, curEvtFile->Events[delIdx].AnimName);
                        curEvtFile->Events.erase(curEvtFile->Events.begin() + delIdx);
                        g_EventWorkspace.SelectedEventIndex = -1;
                        g_EventWorkspace.Editor.SetText("");
                        curEvtFile->Save();
                    }
                }
            }
        }
    }

    if (!ImGui::GetIO().WantTextInput && g_Keybinds.ToggleLeftPanel.IsPressed()) {
        showLeftPanel = !showLeftPanel;
    }

    auto PushCurrentDefState = [&]() {
        if (g_CurrentDefView == EDefViewType::Defs) PushDefHistory(EDefViewType::Defs, g_DefWorkspace.ActiveContextIndex, g_DefWorkspace.SelectedType, g_DefWorkspace.SelectedEntryIndex);
        else if (g_CurrentDefView == EDefViewType::Headers) PushDefHistory(EDefViewType::Headers, 0, "", g_DefWorkspace.SelectedEnumIndex);
        else if (g_CurrentDefView == EDefViewType::Events) PushDefHistory(EDefViewType::Events, g_EventWorkspace.SelectedFileType, "", g_EventWorkspace.SelectedEventIndex);
        };

    auto RequestLoadDef = [&](const std::string& type, int index) {
        PushCurrentDefState();
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
        PushCurrentDefState();
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
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
    ImGui::BeginChild("DefHeaderBar", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Category:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    const char* viewTypes[] = { "Definitions", "Headers", "Events" };
    if (ImGui::BeginCombo("##DefViewCombo", viewTypes[(int)g_CurrentDefView])) {
        for (int i = 0; i < 3; i++) {
            if (ImGui::Selectable(viewTypes[i], (int)g_CurrentDefView == i)) {
                PushCurrentDefState();
                g_CurrentDefView = (EDefViewType)i;
                if (g_CurrentDefView == EDefViewType::Defs) g_DefWorkspace.ShowDefsMode = true;
                if (g_CurrentDefView == EDefViewType::Headers) g_DefWorkspace.ShowDefsMode = false;
            }
        }
        ImGui::EndCombo();
    }

    if (g_CurrentDefView == EDefViewType::Defs) {
        if (!g_DefWorkspace.Contexts.empty()) {
            ImGui::SameLine(0, 20);
            ImGui::Text("Context:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            DefContext& current = g_DefWorkspace.Contexts[g_DefWorkspace.ActiveContextIndex];
            if (ImGui::BeginCombo("##defcontext", current.Name.c_str())) {
                for (int i = 0; i < g_DefWorkspace.Contexts.size(); i++) {
                    if (ImGui::Selectable(g_DefWorkspace.Contexts[i].Name.c_str(), g_DefWorkspace.ActiveContextIndex == i)) {
                        PushCurrentDefState();
                        g_DefWorkspace.ActiveContextIndex = i;
                        LoadDefsFromFolder(g_DefWorkspace.RootPath);
                    }
                }
                ImGui::EndCombo();
            }
        }
    }
    else if (g_CurrentDefView == EDefViewType::Events) {
        ImGui::SameLine(0, 20);
        ImGui::Text("Event Type:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(250);
        const char* fileTypes[] = { "Sound Animation Events", "Game Animation Events" };
        if (ImGui::BeginCombo("##evtcontext", fileTypes[g_EventWorkspace.SelectedFileType])) {
            for (int i = 0; i < 2; i++) {
                if (ImGui::Selectable(fileTypes[i], g_EventWorkspace.SelectedFileType == i)) {
                    PushCurrentDefState();
                    g_EventWorkspace.SelectedFileType = i;
                    g_EventWorkspace.SelectedEventIndex = -1;
                    g_EventWorkspace.Editor.SetText("");
                }
            }
            ImGui::EndCombo();
        }
    }

    bool hasUnsavedChanges = g_DefWorkspace.IsDirty();

    const float compileBtnSize = 20.0f;
    ImGui::SameLine(ImGui::GetWindowWidth() - compileBtnSize - 16.0f);

    // --- RECOMPILE BUTTON ---
    float pulse = 0.5f + 0.5f * sinf((float)ImGui::GetTime() * 4.0f);
    ImVec4 orangeCol = hasUnsavedChanges 
        ? ImVec4(1.0f, 0.60f + 0.25f * pulse, 0.15f, 1.0f) 
        : ImVec4(1.0f, 0.55f, 0.10f, 0.85f);

    if (g_CompileTexture) {
        if (ImGui::ImageButton("##CompileDefsBtn", g_CompileTexture, ImVec2(compileBtnSize, compileBtnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), orangeCol)) {
            if (!g_IsCompiling) {
                CompileAllDefs_Native();
            }
        }
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, orangeCol);
        if (ImGui::Button("Compile", ImVec2(60, compileBtnSize))) {
            if (!g_IsCompiling) {
                CompileAllDefs_Native();
            }
        }
        ImGui::PopStyleColor();
    }

    if (hasUnsavedChanges) {
        ImVec2 btnMin = ImGui::GetItemRectMin();
        ImVec2 btnMax = ImGui::GetItemRectMax();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float btnRounding = 4.0f;
        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.5f;
            float alpha = pulse * (1.0f - (float)i / 4.0f) * 0.70f;
            ImU32 glowCol = IM_COL32(255, 140, 25, (uint32_t)(alpha * 255.0f));
            dl->AddRect(ImVec2(btnMin.x - expand, btnMin.y - expand), ImVec2(btnMax.x + expand, btnMax.y + expand), glowCol, btnRounding + expand, 0, 1.5f);
        }
    }

    if (ImGui::IsItemHovered()) {
        if (hasUnsavedChanges) ImGui::SetTooltip("Recompile (Unsaved changes pending)");
        else ImGui::SetTooltip("Recompile");
    }

    if (g_CurrentMode == EAppMode::Defs && !g_IsCompiling && g_Keybinds.Compile.IsPressed() && !ImGui::GetIO().WantTextInput) {
        CompileAllDefs_Native();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (g_CurrentDefView == EDefViewType::Defs) {
        if (showLeftPanel) {
            ImGui::BeginChild("DefLeftPane", ImVec2(g_SidebarWidth, 0), true);

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

            float availW = ImGui::GetContentRegionAvail().x;
            float checkW = ImGui::GetFrameHeight();
            ImGui::SetNextItemWidth(availW - checkW - ImGui::GetStyle().ItemSpacing.x);
            ImGui::InputTextWithHint("##DefFilter", "Filter...", g_DefWorkspace.FilterText, 128);
            ImGui::SameLine();
            ImGui::Checkbox("##DefMarked", &g_DefWorkspace.ShowMarkedOnly);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Marked Entries Only");

            std::string filterLower = g_DefWorkspace.FilterText;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
            bool hasFilter = !filterLower.empty() || g_DefWorkspace.ShowMarkedOnly;
            ImGui::Separator();

            std::string typeToAdd = "";
            for (auto& [type, entries] : g_DefWorkspace.CategorizedDefs) {
                bool showFolder = !hasFilter;
                if (hasFilter) {
                    std::string typeLower = type;
                    std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(), ::tolower);
                    if (!filterLower.empty() && typeLower.find(filterLower) != std::string::npos && !g_DefWorkspace.ShowMarkedOnly) showFolder = true;
                    if (!showFolder) {
                        for (auto& e : entries) {
                            if (g_DefWorkspace.ShowMarkedOnly) {
                                std::string bankFileName = std::filesystem::path(e.SourceFile).filename().string();
                                if (!ModPackageTracker::IsMarked(bankFileName, e.Name)) continue;
                            }
                            if (!filterLower.empty()) {
                                std::string nameLo = e.Name;
                                std::transform(nameLo.begin(), nameLo.end(), nameLo.begin(), ::tolower);
                                if (nameLo.find(filterLower) == std::string::npos) continue;
                            }
                            showFolder = true;
                            break;
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
                        std::string bankFileName = std::filesystem::path(entries[k].SourceFile).filename().string();
                        bool isMarked = ModPackageTracker::IsMarked(bankFileName, entries[k].Name);
                        if (g_DefWorkspace.ShowMarkedOnly && !isMarked) continue;

                        if (!filterLower.empty()) {
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
                            else { s_DefToDelete = entries[k]; s_TriggerDirectDeleteDef = true; }
                        }
                        ImGui::PopStyleColor(3);
                        ImGui::PopID();

                        ImGui::SameLine();

                        std::string label = entries[k].Name;
                        if (isMarked) label += " [M]";
                        label += "##" + type + std::to_string(k);

                        if (isMarked) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));

                        bool isSelected = (g_DefWorkspace.SelectedType == type && g_DefWorkspace.SelectedEntryIndex == k);
                        if (ImGui::Selectable(label.c_str(), isSelected)) {
                            RequestLoadDef(type, k);
                        }

                        // --- DRAG AND DROP SOURCE FOR DEFS ---
                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                            DefDragPayload payload;
                            payload.ContextIndex = g_DefWorkspace.ActiveContextIndex;
                            snprintf(payload.Category, sizeof(payload.Category), "%s", type.c_str());
                            payload.EntryIndex = k;

                            ImGui::SetDragDropPayload("DEF_ENTRY_PAYLOAD", &payload, sizeof(DefDragPayload));
                            ImGui::Text("Stage Def: %s", entries[k].Name.c_str());
                            ImGui::EndDragDropSource();
                        }

                        if (isMarked) ImGui::PopStyleColor();

                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::MenuItem(isMarked ? "Unmark for Packaging" : "Mark for Packaging (Tracker)")) {
                                StagedModPackageEntry staged;
                                staged.Category = EModAssetCategory::Definition;
                                staged.EntryName = entries[k].Name;
                                staged.TypeName = "Definition";
                                staged.BankName = bankFileName;
                                staged.SubBankName = type;
                                staged.SourceFullPath = entries[k].SourceFile;
                                ModPackageTracker::ToggleMark(staged);
                            }
                            ImGui::Separator();
                            if (ImGui::MenuItem("Duplicate Entry")) {
                                s_DefToDuplicate = entries[k];
                                s_TriggerDuplicateDef = true;
                            }
                            if (ImGui::MenuItem("Delete Entry")) {
                                if (g_AppConfig.ShowDeleteConfirm) {
                                    entryToDeleteCopy = entries[k];
                                    triggerDeletePopup = true;
                                }
                                else {
                                    s_DefToDelete = entries[k];
                                    s_TriggerDirectDeleteDef = true;
                                }
                            }
                            ImGui::EndPopup();
                        }
                    }
                    ImGui::TreePop();
                }
            }
            if (!typeToAdd.empty()) CreateNewDef(typeToAdd);

            if (s_TriggerDirectDeleteDef) {
                DeleteDefEntry(s_DefToDelete);
                s_TriggerDirectDeleteDef = false;
            }
            if (s_TriggerDuplicateDef) {
                DuplicateDefEntry(s_DefToDuplicate);
                s_TriggerDuplicateDef = false;
            }

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
            if (ImGui::IsItemActive()) g_SidebarWidth += ImGui::GetIO().MouseDelta.x;
            ImGui::SameLine();
        }

        ImGui::BeginChild("DefRightPane", ImVec2(0, 0), true);
        if (!g_DefWorkspace.SelectedType.empty() && g_DefWorkspace.SelectedEntryIndex != -1) {
            if (g_DefWorkspace.CategorizedDefs.count(g_DefWorkspace.SelectedType) &&
                g_DefWorkspace.SelectedEntryIndex < g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType].size()) {

                auto& entries = g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType];
                DefEntry& entry = entries[g_DefWorkspace.SelectedEntryIndex];
                ImGui::Text("%s", entry.Name.c_str());
                ImGui::SameLine(); ImGui::TextDisabled("| %s", entry.SourceFile.c_str());

                if (g_DefWorkspace.IsDirty()) {
                    float btnSize = 20.0f;
                    float marginRight = 10.0f;
                    float contentRight = ImGui::GetContentRegionMax().x;
                    float rightPos = contentRight - marginRight - btnSize;
                    float cursorX = ImGui::GetCursorPosX();
                    if (rightPos > cursorX) {
                        ImGui::SameLine(rightPos - cursorX);
                    }
                    else {
                        ImGui::SameLine();
                    }

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.82f, 0.45f, 0.3f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.82f, 0.45f, 0.5f));
                    ImVec4 saveTint = ImVec4(0.95f, 0.82f, 0.45f, 1.0f);
                    if (ImGui::ImageButton("##SaveDefBtn", g_SaveTexture, ImVec2(btnSize, btnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), saveTint)) {
                        SaveDefEntry(entry);
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save changes to this definition");
                    ImGui::PopStyleColor(3);
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
                extern ImFont* g_CodeFont;
                ImFont* fontToUse = g_CodeFont ? g_CodeFont : ImGui::GetFont();
                float oldScale = fontToUse->Scale;
                fontToUse->Scale = g_DefWorkspace.EditorFontScale;
                ImGui::PushFont(fontToUse);
                g_DefWorkspace.Editor.Render("CodeEditor");
                ImGui::PopFont();
                fontToUse->Scale = oldScale;

                // --- GO TO DEFINITION LOGIC ---
                std::string hoveredWordOriginal = g_DefWorkspace.Editor.GetHoveredWord();

                if (g_AppConfig.EnableLookupGeneration && g_Keybinds.LookupDefinition.IsDown() && g_DefWorkspace.Editor.IsHovered() && !hoveredWordOriginal.empty()) {

                    std::string hoveredWord = hoveredWordOriginal;
                    std::transform(hoveredWord.begin(), hoveredWord.end(), hoveredWord.begin(), ::toupper);

                    bool isEnum = g_DefWorkspace.EnumMemberLookup.count(hoveredWord) > 0;
                    bool isDef = g_DefWorkspace.DefEntryLookup.count(hoveredWord) > 0;

                    if (isEnum || isDef) {
                        ImGui::BeginTooltip();
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Ctrl+Click to go to definition:");
                        ImGui::Separator();

                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));

                        if (isDef) {
                            auto& targets = g_DefWorkspace.DefEntryLookup[hoveredWord];
                            for (const auto& target : targets) {
                                ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.6f, 1.0f), "Def:");
                                ImGui::SameLine();
                                ImGui::Text("[%s] %s", target.first.c_str(), hoveredWordOriginal.c_str());
                            }
                        }

                        if (isEnum) {
                            auto& targets = g_DefWorkspace.EnumMemberLookup[hoveredWord];
                            for (int targetIdx : targets) {
                                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.6f, 1.0f), "Header:");
                                ImGui::SameLine();
                                ImGui::Text("%s", g_DefWorkspace.AllEnums[targetIdx].Name.c_str());
                            }
                        }

                        ImGui::PopStyleColor();
                        ImGui::EndTooltip();

                        if (ImGui::IsMouseClicked(0)) {
                            if (isDef) {
                                auto& targets = g_DefWorkspace.DefEntryLookup[hoveredWord];
                                RequestLoadDef(targets[0].first, targets[0].second);
                            }
                            else if (isEnum) {
                                auto& targets = g_DefWorkspace.EnumMemberLookup[hoveredWord];
                                RequestLoadHeader(targets[0]);
                            }
                        }
                    }
                }

                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::GetIO().KeyCtrl && g_Keybinds.SaveEntry.IsPressed()) SaveDefEntry(entry);
            }
        }
        ImGui::EndChild();
    }
    else if (g_CurrentDefView == EDefViewType::Headers) {
        if (showLeftPanel) {
            ImGui::BeginChild("HeadLeftPane", ImVec2(g_SidebarWidth, 0), true);

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

            float availHW = ImGui::GetContentRegionAvail().x;
            float checkHW = ImGui::GetFrameHeight();
            ImGui::SetNextItemWidth(availHW - checkHW - ImGui::GetStyle().ItemSpacing.x);
            ImGui::InputTextWithHint("##HeaderFilter", "Filter...", g_DefWorkspace.HeaderFilter, 128);
            ImGui::SameLine();
            ImGui::Checkbox("##HeadMarked", &g_DefWorkspace.ShowMarkedHeadersOnly);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Marked Entries Only");

            std::string hFilter = g_DefWorkspace.HeaderFilter;
            std::transform(hFilter.begin(), hFilter.end(), hFilter.begin(), ::tolower);
            ImGui::Separator();

            if (ImGui::BeginListBox("##headerList", ImVec2(-FLT_MIN, -FLT_MIN))) {
                for (int i = 0; i < g_DefWorkspace.AllEnums.size(); i++) {
                    auto& enumEntry = g_DefWorkspace.AllEnums[i];
                    std::string bankFileName = std::filesystem::path(enumEntry.FilePath).filename().string();
                    bool isMarked = ModPackageTracker::IsMarked(bankFileName, enumEntry.Name);
                    if (g_DefWorkspace.ShowMarkedHeadersOnly && !isMarked) continue;

                    if (!hFilter.empty()) {
                        std::string nameLo = g_DefWorkspace.AllEnums[i].Name;
                        std::transform(nameLo.begin(), nameLo.end(), nameLo.begin(), ::tolower);
                        if (nameLo.find(hFilter) == std::string::npos) continue;
                    }

                    std::string label = enumEntry.Name;
                    if (isMarked) label += " [M]";
                    label += "##" + std::to_string(i);

                    if (isMarked) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));

                    bool isSelected = (g_DefWorkspace.SelectedEnumIndex == i);

                    if (ImGui::Selectable(label.c_str(), isSelected)) {
                        RequestLoadHeader(i);
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();

                    // --- DRAG AND DROP SOURCE FOR HEADERS ---
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        HeaderDragPayload payload = { i };
                        ImGui::SetDragDropPayload("HEADER_ENTRY_PAYLOAD", &payload, sizeof(HeaderDragPayload));
                        ImGui::Text("Stage Header: %s", enumEntry.Name.c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (isMarked) ImGui::PopStyleColor();

                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem(isMarked ? "Unmark for Packaging" : "Mark for Packaging (Tracker)")) {
                            StagedModPackageEntry staged;
                            staged.Category = EModAssetCategory::Header;
                            staged.EntryName = enumEntry.Name;
                            staged.TypeName = "C++ Header";
                            staged.BankName = bankFileName;
                            staged.SubBankName = "Defs";
                            staged.SourceFullPath = enumEntry.FilePath;
                            ModPackageTracker::ToggleMark(staged);
                        }
                        ImGui::Separator();
                        ImGui::BeginDisabled(true);
                        ImGui::MenuItem("Duplicate Entry");
                        ImGui::MenuItem("Delete Entry");
                        ImGui::EndDisabled();
                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                            ImGui::SetTooltip("Headers cannot be duplicated or deleted.");
                        }
                        ImGui::EndPopup();
                    }
                }
                ImGui::EndListBox();
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::InvisibleButton("vsplitterHead", ImVec2(4.0f, -1.0f));
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            if (ImGui::IsItemActive()) g_SidebarWidth += ImGui::GetIO().MouseDelta.x;
            ImGui::SameLine();
        }

        ImGui::BeginChild("HeadRightPane", ImVec2(0, 0), true);
        if (g_DefWorkspace.SelectedEnumIndex != -1 && g_DefWorkspace.SelectedEnumIndex < g_DefWorkspace.AllEnums.size()) {
            EnumEntry& entry = g_DefWorkspace.AllEnums[g_DefWorkspace.SelectedEnumIndex];
            ImGui::Text("%s", entry.Name.c_str());
            ImGui::SameLine(); ImGui::TextDisabled("| %s", entry.FilePath.c_str());

            if (g_DefWorkspace.IsDirty()) {
                float btnSize = 20.0f;
                float marginRight = 10.0f;
                float contentRight = ImGui::GetContentRegionMax().x;
                float rightPos = contentRight - marginRight - btnSize;
                float cursorX = ImGui::GetCursorPosX();
                if (rightPos > cursorX) {
                    ImGui::SameLine(rightPos - cursorX);
                }
                else {
                    ImGui::SameLine();
                }

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.82f, 0.45f, 0.3f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.82f, 0.45f, 0.5f));
                ImVec4 saveTint = ImVec4(0.95f, 0.82f, 0.45f, 1.0f);
                if (ImGui::ImageButton("##SaveHeaderBtn", g_SaveTexture, ImVec2(btnSize, btnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), saveTint)) {
                    SaveHeaderEntry(entry);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save changes to this header");
                ImGui::PopStyleColor(3);
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
            extern ImFont* g_CodeFont;
            ImFont* fontToUse = g_CodeFont ? g_CodeFont : ImGui::GetFont();
            float oldScale = fontToUse->Scale;
            fontToUse->Scale = g_DefWorkspace.EditorFontScale;
            ImGui::PushFont(fontToUse);
            g_DefWorkspace.Editor.Render("HeaderEditor");
            ImGui::PopFont();
            fontToUse->Scale = oldScale;

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::GetIO().KeyCtrl && g_Keybinds.SaveEntry.IsPressed()) SaveHeaderEntry(entry);
        }
        ImGui::EndChild();
    }
    else if (g_CurrentDefView == EDefViewType::Events) {
        if (!g_EventWorkspace.SoundEvents.IsLoaded && g_DefWorkspace.IsLoaded) {
            g_EventWorkspace.LoadAll(g_DefWorkspace.RootPath);
        }

        EventFile* activeFile = g_EventWorkspace.GetActiveFile();

        if (showLeftPanel) {
            ImGui::BeginChild("EvtLeftPane", ImVec2(g_SidebarWidth, 0), true);

            if (ImGui::Button("+", ImVec2(24, 0))) {
                EventEntry newEvent;
                newEvent.AnimName = "NEW_ANIMATION_EVENT";
                newEvent.Content = "";
                activeFile->Events.push_back(newEvent);
                g_EventWorkspace.SelectedEventIndex = (int)activeFile->Events.size() - 1;
                g_EventWorkspace.Editor.SetText("");
                g_EventWorkspace.OriginalContent = "";
                g_EventWorkspace.SavedUndoIndex = g_EventWorkspace.Editor.GetUndoIndex();
                activeFile->Save();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Event");

            ImGui::SameLine();
            float availEW = ImGui::GetContentRegionAvail().x;
            float checkEW = ImGui::GetFrameHeight();
            ImGui::SetNextItemWidth(availEW - checkEW - ImGui::GetStyle().ItemSpacing.x);
            ImGui::InputTextWithHint("##EvtFilter", "Filter...", g_EventWorkspace.FilterText, 128);
            ImGui::SameLine();
            ImGui::Checkbox("##EvtMarked", &g_EventWorkspace.ShowMarkedOnly);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Marked Entries Only");

            std::string filterLower = g_EventWorkspace.FilterText;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
            ImGui::Separator();

            
            static bool s_ScrollToSelectedEvent = false;

            std::vector<int> visibleEvents;
            if (activeFile && activeFile->IsLoaded) {
                visibleEvents.reserve(activeFile->Events.size());
                for (int i = 0; i < (int)activeFile->Events.size(); i++) {
                    if (g_EventWorkspace.ShowMarkedOnly) {
                        bool isMarked = ModPackageTracker::IsMarked(activeFile->FileName, activeFile->Events[i].AnimName);
                        if (!isMarked) continue;
                    }

                    if (!filterLower.empty()) {
                        std::string n = activeFile->Events[i].AnimName;
                        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                        if (n.find(filterLower) == std::string::npos) continue;
                    }
                    visibleEvents.push_back(i);
                }
            }

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && !visibleEvents.empty() && !ImGui::GetIO().WantTextInput) {
                if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                    int direction = ImGui::IsKeyPressed(ImGuiKey_DownArrow) ? 1 : -1;
                    int currentPos = -1;
                    for (int k = 0; k < (int)visibleEvents.size(); k++) {
                        if (visibleEvents[k] == g_EventWorkspace.SelectedEventIndex) {
                            currentPos = k;
                            break;
                        }
                    }

                    if (currentPos == -1) {
                        currentPos = (direction == 1) ? -1 : (int)visibleEvents.size();
                    }

                    int newPos = std::clamp(currentPos + direction, 0, (int)visibleEvents.size() - 1);
                    int targetIdx = visibleEvents[newPos];

                    if (targetIdx != g_EventWorkspace.SelectedEventIndex) {
                        PushDefHistory(EDefViewType::Events, g_EventWorkspace.SelectedFileType, "", g_EventWorkspace.SelectedEventIndex);

                        if (g_EventWorkspace.SelectedEventIndex != -1 && g_EventWorkspace.IsDirty()) {
                            activeFile->Events[g_EventWorkspace.SelectedEventIndex].Content = g_EventWorkspace.Editor.GetText();
                        }
                        g_EventWorkspace.SelectedEventIndex = targetIdx;
                        g_EventWorkspace.Editor.SetText(activeFile->Events[targetIdx].Content);
                        g_EventWorkspace.OriginalContent = g_EventWorkspace.Editor.GetText();
                        g_EventWorkspace.SavedUndoIndex = g_EventWorkspace.Editor.GetUndoIndex();
                        s_ScrollToSelectedEvent = true;
                    }
                }
            }

            ImGui::BeginChild("EvtList");
            if (activeFile && activeFile->IsLoaded) {
                for (int i : visibleEvents) {
                    auto& ev = activeFile->Events[i];

                    ImGui::PushID(i);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

                    if (ImGui::SmallButton("-")) {
                        if (g_AppConfig.ShowDeleteConfirm) {
                            eventToDeleteIndex = i;
                            triggerEventDeletePopup = true;
                        }
                        else {
                            activeFile->Events.erase(activeFile->Events.begin() + i);
                            if (g_EventWorkspace.SelectedEventIndex == i) {
                                g_EventWorkspace.SelectedEventIndex = -1;
                                g_EventWorkspace.Editor.SetText("");
                            }
                            else if (g_EventWorkspace.SelectedEventIndex > i) {
                                g_EventWorkspace.SelectedEventIndex--;
                            }
                            activeFile->Save();
                            ImGui::PopStyleColor(3);
                            ImGui::PopID();
                            continue;
                        }
                    }
                    ImGui::PopStyleColor(3);
                    ImGui::PopID();

                    ImGui::SameLine();

                    std::string prefix = g_EventWorkspace.SelectedFileType == 0 ? "SoundEvent" : "GameEvent";
                    bool isMarked = ModPackageTracker::IsMarked(activeFile->FileName, ev.AnimName);

                    std::string label = prefix + std::to_string(i + 1) + ": " + ev.AnimName;
                    if (isMarked) label += " [M]";
                    label += "##" + std::to_string(i);

                    if (isMarked) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));

                    bool isSelected = (g_EventWorkspace.SelectedEventIndex == i);
                    if (ImGui::Selectable(label.c_str(), isSelected)) {
                        PushDefHistory(EDefViewType::Events, g_EventWorkspace.SelectedFileType, "", g_EventWorkspace.SelectedEventIndex);

                        if (g_EventWorkspace.SelectedEventIndex != -1 && g_EventWorkspace.IsDirty()) {
                            activeFile->Events[g_EventWorkspace.SelectedEventIndex].Content = g_EventWorkspace.Editor.GetText();
                        }
                        g_EventWorkspace.SelectedEventIndex = i;
                        g_EventWorkspace.Editor.SetText(ev.Content);
                        g_EventWorkspace.OriginalContent = g_EventWorkspace.Editor.GetText();
                        g_EventWorkspace.SavedUndoIndex = g_EventWorkspace.Editor.GetUndoIndex();
                    }

                    if (isSelected && s_ScrollToSelectedEvent) {
                        ImGui::SetScrollHereY(0.5f);
                        s_ScrollToSelectedEvent = false;
                    }

                    // --- DRAG AND DROP SOURCE FOR EVENTS ---
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        EventDragPayload payload = { g_EventWorkspace.SelectedFileType, i };
                        ImGui::SetDragDropPayload("EVENT_ENTRY_PAYLOAD", &payload, sizeof(EventDragPayload));
                        ImGui::Text("Stage Event: %s", ev.AnimName.c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (isMarked) ImGui::PopStyleColor();

                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem(isMarked ? "Unmark for Packaging" : "Mark for Packaging (Tracker)")) {
                            StagedModPackageEntry staged;
                            staged.Category = EModAssetCategory::AnimationEvent;
                            staged.EntryName = ev.AnimName;
                            staged.TypeName = (g_EventWorkspace.SelectedFileType == 0) ? "Sound Event" : "Game Event";
                            staged.BankName = activeFile->FileName;
                            staged.SubBankName = "Misc";
                            staged.SourceFullPath = activeFile->FilePath;
                            ModPackageTracker::ToggleMark(staged);
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Duplicate Entry")) {
                            s_EventToDuplicate = ev;
                            s_TriggerDuplicateEvent = true;
                        }
                        if (ImGui::MenuItem("Delete Entry")) {
                            if (g_AppConfig.ShowDeleteConfirm) {
                                eventToDeleteIndex = i;
                                triggerEventDeletePopup = true;
                            }
                            else {
                                ModPackageTracker::RemoveMark(activeFile->FileName, activeFile->Events[i].AnimName);
                                activeFile->Events.erase(activeFile->Events.begin() + i);
                                if (g_EventWorkspace.SelectedEventIndex == i) {
                                    g_EventWorkspace.SelectedEventIndex = -1;
                                    g_EventWorkspace.Editor.SetText("");
                                }
                                else if (g_EventWorkspace.SelectedEventIndex > i) {
                                    g_EventWorkspace.SelectedEventIndex--;
                                }
                                activeFile->Save();
                            }
                        }
                        ImGui::EndPopup();
                    }
                }
            }
            else {
                ImGui::TextDisabled("File not found or loaded.");
            }
            ImGui::EndChild();

            if (s_TriggerDuplicateEvent && activeFile) {
                std::string newAnimName = GenerateDuplicateName(s_EventToDuplicate.AnimName, [&](const std::string& candidate) {
                    for (const auto& existingEv : activeFile->Events) {
                        if (existingEv.AnimName == candidate) return true;
                    }
                    return false;
                });
                EventEntry newEv = s_EventToDuplicate;
                newEv.AnimName = newAnimName;
                activeFile->Events.push_back(newEv);
                activeFile->Save();
                g_EventWorkspace.SelectedEventIndex = (int)activeFile->Events.size() - 1;
                g_EventWorkspace.Editor.SetText(newEv.Content);
                g_EventWorkspace.OriginalContent = g_EventWorkspace.Editor.GetText();
                g_EventWorkspace.SavedUndoIndex = g_EventWorkspace.Editor.GetUndoIndex();
                s_ScrollToSelectedEvent = true;
                s_TriggerDuplicateEvent = false;
            }

            if (triggerEventDeletePopup) { ImGui::OpenPopup("DeleteEventConfirmation"); triggerEventDeletePopup = false; }
            if (ImGui::BeginPopupModal("DeleteEventConfirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Do you want to delete '%s'?", activeFile->Events[eventToDeleteIndex].AnimName.c_str());
                ImGui::Separator();
                static bool dontShowDelete = false; ImGui::Checkbox("Don't show again", &dontShowDelete);
                if (ImGui::Button("Yes", ImVec2(100, 0))) {
                    if (dontShowDelete) { g_AppConfig.ShowDeleteConfirm = false; SaveConfig(); }

                    ModPackageTracker::RemoveMark(activeFile->FileName, activeFile->Events[eventToDeleteIndex].AnimName);
                    activeFile->Events.erase(activeFile->Events.begin() + eventToDeleteIndex);
                    if (g_EventWorkspace.SelectedEventIndex == eventToDeleteIndex) {
                        g_EventWorkspace.SelectedEventIndex = -1;
                        g_EventWorkspace.Editor.SetText("");
                    }
                    else if (g_EventWorkspace.SelectedEventIndex > eventToDeleteIndex) {
                        g_EventWorkspace.SelectedEventIndex--;
                    }
                    activeFile->Save();

                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine(); if (ImGui::Button("Cancel", ImVec2(100, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::InvisibleButton("vsplitterEvt", ImVec2(4.0f, -1.0f));
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            if (ImGui::IsItemActive()) g_SidebarWidth += ImGui::GetIO().MouseDelta.x;
            ImGui::SameLine();
        }

        ImGui::BeginChild("EvtRightPane", ImVec2(0, 0), true);
        if (g_EventWorkspace.SelectedEventIndex != -1 && g_EventWorkspace.SelectedEventIndex < activeFile->Events.size()) {
            auto& ev = activeFile->Events[g_EventWorkspace.SelectedEventIndex];

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Animation:");
            ImGui::SameLine();

            static char animNameBuf[256];
            static int lastEvtIdx = -1;
            static int lastEvtType = -1;
            if (lastEvtIdx != g_EventWorkspace.SelectedEventIndex || lastEvtType != g_EventWorkspace.SelectedFileType) {
                strncpy_s(animNameBuf, sizeof(animNameBuf), ev.AnimName.c_str(), _TRUNCATE);
                lastEvtIdx = g_EventWorkspace.SelectedEventIndex;
                lastEvtType = g_EventWorkspace.SelectedFileType;
            }

            ImGui::SetNextItemWidth(300);
            if (ImGui::InputText("##animNameEdit", animNameBuf, 256)) {
                ev.AnimName = animNameBuf;
            }

            float btnSize = 20.0f;
            float marginRight = 10.0f;
            float contentRight = ImGui::GetContentRegionMax().x;
            float rightPos = contentRight - marginRight - btnSize;
            float cursorX = ImGui::GetCursorPosX();
            if (rightPos > cursorX) {
                ImGui::SameLine(rightPos - cursorX);
            }
            else {
                ImGui::SameLine();
            }

            bool isDirty = g_EventWorkspace.IsDirty() || std::string(animNameBuf) != ev.AnimName;

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.82f, 0.45f, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.82f, 0.45f, 0.5f));
            ImVec4 saveTint = ImVec4(0.95f, 0.82f, 0.45f, 1.0f);
            if (ImGui::ImageButton("##SaveEventBtn", g_SaveTexture, ImVec2(btnSize, btnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), saveTint)) {
                ev.Content = g_EventWorkspace.Editor.GetText();
                g_EventWorkspace.OriginalContent = ev.Content;
                g_EventWorkspace.SavedUndoIndex = g_EventWorkspace.Editor.GetUndoIndex();
                activeFile->Save();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(isDirty ? "Save changes to this event" : "Save file");
            }
            ImGui::PopStyleColor(3);

            ImGui::Separator();

            extern ImFont* g_CodeFont;
            ImFont* eventFontToUse = g_CodeFont ? g_CodeFont : ImGui::GetFont();
            ImGui::PushFont(eventFontToUse);
            g_EventWorkspace.Editor.Render("EventEditor");
            ImGui::PopFont();

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::GetIO().KeyCtrl && g_Keybinds.SaveEntry.IsPressed()) {
                ev.Content = g_EventWorkspace.Editor.GetText();
                g_EventWorkspace.OriginalContent = ev.Content;
                g_EventWorkspace.SavedUndoIndex = g_EventWorkspace.Editor.GetUndoIndex();
                activeFile->Save();
            }
        }
        ImGui::EndChild();
    }
}