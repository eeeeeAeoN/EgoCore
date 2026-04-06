#pragma once
#include "imgui.h"
#include "FSEBackend.h"
#include "FSEAutosuggest.h"
#include <algorithm> 

static void DrawFSETab() {
    static float leftPaneWidth = 250.0f;
    if (leftPaneWidth < 50.0f) leftPaneWidth = 50.0f;

    static bool showLeftPanel = true;

    if (!g_FSEWorkspace.IsLoaded) {
        LoadQuestsLua();

        FSEAutosuggest::LoadDictionaries("LookupTable/FSEAPI.ini", "LookupTable/APIEnums.ini");
    }

    if (!ImGui::GetIO().WantTextInput && g_Keybinds.ToggleLeftPanel.IsPressed()) {
        showLeftPanel = !showLeftPanel;
    }

    static bool showCreateQuest = false;
    static bool showCreateEntity = false;
    static bool showCreateScript = false;
    static bool showDeleteConfirm = false;
    static int targetQuestIdx = -1;
    static char inputName[128] = "";

    auto DrawMinusButton = []() -> bool {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        bool pressed = ImGui::SmallButton("-");
        ImGui::PopStyleColor(3);
        return pressed;
        };

    if (showLeftPanel) {
        ImGui::BeginChild("FSELeftPane", ImVec2(leftPaneWidth, 0), true);

        if (ImGui::Button("+ Add Quest", ImVec2(-1, 0))) {
            memset(inputName, 0, sizeof(inputName));
            showCreateQuest = true;
        }

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 5));

        for (int i = 0; i < g_FSEWorkspace.Quests.size(); i++) {
            auto& quest = g_FSEWorkspace.Quests[i];

            ImGui::PushID(i);

            if (DrawMinusButton()) {
                g_FSEWorkspace.ActiveItemType = EFSEItemType::QuestMain;
                g_FSEWorkspace.ActiveQuestIdx = i;
                showDeleteConfirm = true;
            }
            ImGui::SameLine();

            if (ImGui::TreeNodeEx(quest.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::Selectable(("[Main] " + quest.Name + ".lua").c_str(), g_FSEWorkspace.ActiveFilePath.find(quest.File) != std::string::npos)) {
                    LoadFSEScriptContent(quest.File, EFSEItemType::QuestMain, i);
                }

                for (const auto& script : quest.ExtraScripts) {
                    ImGui::PushID(script.c_str());
                    if (DrawMinusButton()) {
                        g_FSEWorkspace.ActiveItemType = EFSEItemType::ExtraScript;
                        g_FSEWorkspace.ActiveQuestIdx = i;
                        g_FSEWorkspace.ActiveExtraScriptName = script;
                        showDeleteConfirm = true;
                    }
                    ImGui::SameLine();

                    std::string relPath = quest.Name + "/" + script;
                    if (ImGui::Selectable((script + ".lua").c_str(), g_FSEWorkspace.ActiveFilePath.find(relPath) != std::string::npos)) {
                        LoadFSEScriptContent(relPath, EFSEItemType::ExtraScript, i, -1, script);
                    }
                    ImGui::PopID();
                }

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
                if (ImGui::Selectable("+ Create Script")) {
                    targetQuestIdx = i;
                    memset(inputName, 0, sizeof(inputName));
                    showCreateScript = true;
                }
                ImGui::PopStyleColor();

                if (ImGui::TreeNodeEx("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
                    for (int j = 0; j < quest.Entities.size(); j++) {
                        auto& ent = quest.Entities[j];

                        ImGui::PushID(j);
                        if (DrawMinusButton()) {
                            g_FSEWorkspace.ActiveItemType = EFSEItemType::Entity;
                            g_FSEWorkspace.ActiveQuestIdx = i;
                            g_FSEWorkspace.ActiveEntityIdx = j;
                            showDeleteConfirm = true;
                        }
                        ImGui::SameLine();

                        if (ImGui::Selectable((ent.Name + ".lua").c_str(), g_FSEWorkspace.ActiveFilePath.find(ent.File) != std::string::npos)) {
                            LoadFSEScriptContent(ent.File, EFSEItemType::Entity, i, j);
                        }
                        ImGui::PopID();
                    }

                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
                    if (ImGui::Selectable("+ Create Entity")) {
                        targetQuestIdx = i;
                        memset(inputName, 0, sizeof(inputName));
                        showCreateEntity = true;
                    }
                    ImGui::PopStyleColor();
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::InvisibleButton("vsplitterFSE", ImVec2(4.0f, -1.0f));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsItemActive()) leftPaneWidth += ImGui::GetIO().MouseDelta.x;
        ImGui::SameLine();
    }

    ImGui::BeginChild("FSERightPane", ImVec2(0, 0), true);

    if (!g_FSEWorkspace.ActiveFilePath.empty()) {
        ImGui::TextDisabled("Editing: %s", g_FSEWorkspace.ActiveFilePath.c_str());

        if (g_FSEWorkspace.IsDirty()) {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
            if (ImGui::Button("SAVE")) {
                SaveActiveFSEScript();
            }
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        std::string deleteLabel = (g_FSEWorkspace.ActiveItemType == EFSEItemType::QuestMain) ? "DELETE QUEST" : "DELETE";
        if (ImGui::Button(deleteLabel.c_str())) {
            showDeleteConfirm = true;
        }
        ImGui::PopStyleColor();

        ImGui::Separator();

        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || g_FSEWorkspace.IsSearchOpen) {
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F)) {
                g_FSEWorkspace.IsSearchOpen = !g_FSEWorkspace.IsSearchOpen;
                if (g_FSEWorkspace.IsSearchOpen) ImGui::SetKeyboardFocusHere(0);
            }
        }
        if (g_FSEWorkspace.IsSearchOpen) {
            ImGui::SetNextItemWidth(200);
            static bool needsRefocus = false;
            if (needsRefocus) { ImGui::SetKeyboardFocusHere(0); needsRefocus = false; }
            if (ImGui::InputText("Find", g_FSEWorkspace.SearchBuffer, 128, ImGuiInputTextFlags_EnterReturnsTrue)) {
                FindNextInFSEEditor(); needsRefocus = true;
            }
            ImGui::SameLine(); if (ImGui::Button("Next")) { FindNextInFSEEditor(); needsRefocus = true; }
            ImGui::SameLine(); ImGui::Checkbox("Aa", &g_FSEWorkspace.SearchCaseSensitive);
            ImGui::SameLine(); if (ImGui::Button("X")) g_FSEWorkspace.IsSearchOpen = false;
            ImGui::Separator();
        }

        if (io.KeyCtrl && io.MouseWheel != 0.0f && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
            float step = (io.MouseWheel > 0) ? 0.2f : -0.2f;
            g_FSEWorkspace.EditorFontScale = std::clamp(g_FSEWorkspace.EditorFontScale + step, 0.5f, 3.0f);
        }

        extern ImFont* g_EditorFont;
        ImFont* fontToUse = g_EditorFont ? g_EditorFont : ImGui::GetFont();
        float oldScale = fontToUse->Scale;
        fontToUse->Scale = g_FSEWorkspace.EditorFontScale;
        ImGui::PushFont(fontToUse);

        g_FSEWorkspace.Editor.Render("FSEEditor");

        if (g_AppConfig.EnableAutosuggest) {
            FSEAutosuggest::ProcessEditorInput(g_FSEWorkspace.Editor);

            if (FSEAutosuggest::IsSuggestPopupOpen) {

                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    FSEAutosuggest::IsSuggestPopupOpen = false;
                    FSEAutosuggest::DismissedLine = g_FSEWorkspace.Editor.GetCursorPosition().mLine;
                }

                if (FSEAutosuggest::IsSuggestPopupOpen) {
                    ImVec2 cPos = g_FSEWorkspace.Editor.GetCursorScreenPos();

                    float lineHeight = ImGui::GetTextLineHeight();
                    float wordWidth = ImGui::CalcTextSize(FSEAutosuggest::CurrentFilter.c_str()).x;

                    ImGui::SetNextWindowPos(ImVec2(cPos.x - wordWidth, cPos.y + lineHeight + 2.0f));

                    ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoTitleBar |
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;

                    if (ImGui::Begin("AutosuggestPopup", nullptr, flags)) {
                        for (int i = 0; i < FSEAutosuggest::FilteredSuggestions.size(); i++) {
                            std::string label = FSEAutosuggest::FilteredSuggestions[i];
                            std::string helper = FSEAutosuggest::FilteredSignatures[i];

                            if (ImGui::Selectable(helper.c_str(), false)) {
                                std::string toInsert = label.substr(FSEAutosuggest::CurrentFilter.length());
                                if (helper.find('(') != std::string::npos) toInsert += "()";

                                g_FSEWorkspace.Editor.InsertText(toInsert);

                                if (helper.find("()") == std::string::npos && helper.find('(') != std::string::npos) {
                                    auto newCursor = g_FSEWorkspace.Editor.GetCursorPosition();
                                    newCursor.mColumn -= 1;
                                    g_FSEWorkspace.Editor.SetCursorPosition(newCursor);
                                }
                                FSEAutosuggest::IsSuggestPopupOpen = false;
                            }
                        }
                    }
                    ImGui::End();
                }
            }
        }

        ImGui::PopFont();
        fontToUse->Scale = oldScale;

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
            SaveActiveFSEScript();
        }
    }
    else {
        ImGui::TextDisabled("Select a script from the tree to edit.");
    }
    ImGui::EndChild();

    if (showCreateQuest) ImGui::OpenPopup("Create New Quest");
    if (ImGui::BeginPopupModal("Create New Quest", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Quest Name", inputName, 128);
        ImGui::Separator();
        if (ImGui::Button("Create", ImVec2(100, 0))) {
            if (strlen(inputName) > 0) CreateFSEQuest(inputName);
            showCreateQuest = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) { showCreateQuest = false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    if (showCreateEntity) ImGui::OpenPopup("Create New Entity");
    if (ImGui::BeginPopupModal("Create New Entity", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Quest: %s", g_FSEWorkspace.Quests[targetQuestIdx].Name.c_str());
        ImGui::InputText("Entity Name", inputName, 128);
        ImGui::Separator();
        if (ImGui::Button("Create", ImVec2(100, 0))) {
            if (strlen(inputName) > 0) CreateFSEEntity(targetQuestIdx, inputName);
            showCreateEntity = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) { showCreateEntity = false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    if (showCreateScript) ImGui::OpenPopup("Create New Script");
    if (ImGui::BeginPopupModal("Create New Script", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Quest: %s", g_FSEWorkspace.Quests[targetQuestIdx].Name.c_str());
        ImGui::InputText("Script Name", inputName, 128);
        ImGui::Separator();
        if (ImGui::Button("Create", ImVec2(100, 0))) {
            if (strlen(inputName) > 0) CreateFSEScript(targetQuestIdx, inputName);
            showCreateScript = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) { showCreateScript = false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    if (showDeleteConfirm) ImGui::OpenPopup("Confirm Delete");
    if (ImGui::BeginPopupModal("Confirm Delete", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (g_FSEWorkspace.ActiveItemType == EFSEItemType::QuestMain) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "WARNING: This will delete the ENTIRE quest folder");
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "and all of its child scripts. This cannot be undone.");
        }
        else {
            ImGui::Text("Are you sure you want to delete this script?");
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "This file will be permanently removed.");
        }
        ImGui::Separator();

        if (ImGui::Button("Yes, Delete", ImVec2(120, 0))) {
            DeleteActiveFSEItem();
            showDeleteConfirm = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) { showDeleteConfirm = false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}