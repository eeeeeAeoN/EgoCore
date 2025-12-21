#pragma once
#include "imgui.h"
#include "BankBackend.h" 
#include "TextParser.h"
#include "BinaryParser.h" 
#include "DefBackend.h" // Needed for g_AvailableSoundBanks
#include <string>
#include <vector>
#include <cstring>
#include <algorithm> // for transform, tolower

static CTextParser g_TextParser;

// --- PERSISTENT STATE ---
static bool g_IsTextDirty = false;
static int g_LastEntryID = -1;
static void* g_LastBankPtr = nullptr;

// --- STATE FOR ADD ENTRY POPUP ---
static bool g_ShowAddGroupItemPopup = false;
static char g_GroupSearchBuf[128] = "";

// --- HELPERS ---

inline std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

inline std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

inline bool InputString(const char* label, std::string& str, float width = 0.0f) {
    static char buffer[1024];
    strncpy_s(buffer, sizeof(buffer), str.c_str(), _TRUNCATE);
    if (width != 0.0f) ImGui::SetNextItemWidth(width);
    bool changed = ImGui::InputText(label, buffer, sizeof(buffer));
    if (changed) str = buffer;
    return changed;
}

// NOTE: We need this helper to show previews in the Group list. 
// It was previously in BankExplorer.h, but we duplicate the logic here or rely on the cached data if available.
inline std::string PeekEntryName(LoadedBank* bank, uint32_t id) {
    if (!bank) return "Unknown";
    for (const auto& e : bank->Entries) {
        if (e.ID == id) return e.Name;
    }
    return "Unknown ID";
}

// --- MAIN DRAWER ---

inline void DrawTextProperties(LoadedBank* bank) {
    if (!g_TextParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse text entry.");
        return;
    }

    // Reset dirty flag if we switched entries
    if (bank != g_LastBankPtr || (bank && bank->SelectedEntryIndex != g_LastEntryID)) {
        g_IsTextDirty = false;
        g_LastBankPtr = bank;
        if (bank) g_LastEntryID = bank->SelectedEntryIndex;
    }

    // =========================================================
    // TYPE 1: GROUP ENTRY (Restored & Editable)
    // =========================================================
    if (g_TextParser.IsGroup) {
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Group Entry (%zu Items)", g_TextParser.GroupData.Items.size());
        ImGui::Separator();

        // 1. ITEMS TABLE
        if (ImGui::BeginTable("GroupTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable, ImVec2(0, 300))) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Name (Preview)", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableHeadersRow();

            int itemToRemove = -1;

            for (int i = 0; i < g_TextParser.GroupData.Items.size(); i++) {
                auto& item = g_TextParser.GroupData.Items[i];
                ImGui::PushID(i);
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", item.ID);

                ImGui::TableSetColumnIndex(1);
                // If CachedName is empty (e.g. newly added), try to resolve it
                if (item.CachedName.empty() && bank) {
                    item.CachedName = PeekEntryName(bank, item.ID);
                }
                ImGui::Text("%s", item.CachedName.c_str());

                ImGui::TableSetColumnIndex(2);
                if (ImGui::Button("X")) itemToRemove = i;

                ImGui::PopID();
            }
            ImGui::EndTable();

            if (itemToRemove != -1) {
                g_TextParser.GroupData.Items.erase(g_TextParser.GroupData.Items.begin() + itemToRemove);
                g_IsTextDirty = true;
            }
        }

        // 2. ADD ENTRY BUTTON
        if (ImGui::Button("+ Add Entry")) {
            g_ShowAddGroupItemPopup = true;
            g_GroupSearchBuf[0] = '\0';
            ImGui::OpenPopup("Add Group Item");
        }

        // 3. ADD ENTRY POPUP (Search/Suggest)
        if (ImGui::BeginPopupModal("Add Group Item", &g_ShowAddGroupItemPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Search for an entry to add:");
            ImGui::InputText("##search", g_GroupSearchBuf, 128);

            ImGui::Separator();
            ImGui::TextDisabled("Suggestions:");

            std::string search = g_GroupSearchBuf;
            std::transform(search.begin(), search.end(), search.begin(), ::tolower);

            int suggestionsFound = 0;
            if (bank && !search.empty()) {
                if (ImGui::BeginListBox("##suggestions", ImVec2(400, 150))) {
                    for (const auto& entry : bank->Entries) {
                        // 1. Skip if not Type 0 (Text Entry)
                        if (entry.Type != 0) continue;

                        // 2. Skip if it's the current entry itself
                        if (entry.ID == bank->Entries[bank->SelectedEntryIndex].ID) continue;

                        std::string nameLower = entry.Name;
                        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

                        // Match ID or Name
                        std::string idStr = std::to_string(entry.ID);
                        if (nameLower.find(search) != std::string::npos || idStr.find(search) != std::string::npos) {

                            std::string label = entry.Name + " (" + std::to_string(entry.ID) + ")";
                            if (ImGui::Selectable(label.c_str())) {
                                // Add to Group
                                CTextGroupItem newItem;
                                newItem.ID = entry.ID;
                                newItem.CachedName = entry.Name;
                                g_TextParser.GroupData.Items.push_back(newItem);
                                g_IsTextDirty = true;
                                g_ShowAddGroupItemPopup = false;
                                ImGui::CloseCurrentPopup();
                            }
                            suggestionsFound++;
                            if (suggestionsFound >= 5) break; // Limit to 5
                        }
                    }
                    ImGui::EndListBox();
                }
            }
            else {
                ImGui::TextDisabled("(Type to see suggestions)");
            }

            ImGui::Separator();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                g_ShowAddGroupItemPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Spacing();
        ImGui::Separator();
    }
    // =========================================================
    // TYPE 2: NARRATOR LIST
    // =========================================================
    else if (g_TextParser.IsNarratorList) {
        ImGui::Text("Narrator List (Editing not implemented)");
        return; // Don't show save button for this yet
    }
    // =========================================================
    // TYPE 0: TEXT ENTRY
    // =========================================================
    else {
        CTextEntry& e = g_TextParser.TextData;

        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Text Entry Editor");
        ImGui::Separator();

        if (InputString("Identifier", e.Identifier)) g_IsTextDirty = true;
        ImGui::SameLine(); ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Internal ID used by scripts.");

        ImGui::Spacing();

        if (ImGui::BeginTable("MetaTable", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Speaker");
            ImGui::TableSetColumnIndex(1);
            if (InputString("##speaker", e.Speaker, -FLT_MIN)) g_IsTextDirty = true;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Sound Bank");
            ImGui::TableSetColumnIndex(1);

            if (g_AvailableSoundBanks.empty()) {
                if (InputString("##soundbank", e.SpeechBank, -FLT_MIN)) g_IsTextDirty = true;
            }
            else {
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::BeginCombo("##soundbank", e.SpeechBank.c_str())) {
                    for (const auto& sb : g_AvailableSoundBanks) {
                        bool isSelected = (e.SpeechBank == sb);
                        if (ImGui::Selectable(sb.c_str(), isSelected)) {
                            e.SpeechBank = sb;
                            g_IsTextDirty = true;
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::EndTable();
        }

        ImGui::Spacing(); ImGui::Separator();

        static char contentBuf[8192];
        std::string utf8Content = WStringToString(e.Content);
        strncpy_s(contentBuf, sizeof(contentBuf), utf8Content.c_str(), _TRUNCATE);

        ImGui::Text("Content:");
        if (ImGui::InputTextMultiline("##content", contentBuf, sizeof(contentBuf), ImVec2(-FLT_MIN, 100))) {
            e.Content = StringToWString(contentBuf);
            g_IsTextDirty = true;
        }

        ImGui::Spacing(); ImGui::Separator();

        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Tags / Modifiers (%zu)", e.Tags.size());
        if (ImGui::BeginTable("TagsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Tag Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableHeadersRow();

            int tagToDelete = -1;
            for (int i = 0; i < e.Tags.size(); i++) {
                ImGui::PushID(i);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (InputString("##name", e.Tags[i].Name, -FLT_MIN)) g_IsTextDirty = true;
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Button("X")) tagToDelete = i;
                ImGui::PopID();
            }
            ImGui::EndTable();

            if (tagToDelete != -1) {
                e.Tags.erase(e.Tags.begin() + tagToDelete);
                g_IsTextDirty = true;
            }
        }

        if (ImGui::Button("+ Add New Tag")) {
            CTextTag newTag; newTag.Position = 0; newTag.Name = "NEW_TAG";
            e.Tags.push_back(newTag);
            g_IsTextDirty = true;
        }
        ImGui::Spacing(); ImGui::Separator();
    }

    // =========================================================
    // GLOBAL SAVE BUTTON (For Group & Text)
    // =========================================================
    if (g_IsTextDirty) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));

        if (ImGui::Button("SAVE ENTRY CHANGES", ImVec2(-FLT_MIN, 40))) {
            if (bank && bank->SelectedEntryIndex != -1) {
                std::vector<uint8_t> newBytes = g_TextParser.Recompile();
                bank->ModifiedEntryData[bank->SelectedEntryIndex] = newBytes;
                bank->CurrentEntryRawData = newBytes;
                bank->Entries[bank->SelectedEntryIndex].Size = (uint32_t)newBytes.size();
                g_IsTextDirty = false;
            }
        }
        ImGui::PopStyleColor(2);
    }
}