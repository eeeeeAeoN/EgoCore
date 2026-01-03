#pragma once
#include "imgui.h"
#include "TextBackend.h" 
#include "FileDialogs.h"
#include "LipSyncCompiler.h"
#include <functional> // Added for std::function

static bool g_ShowAddGroupItemPopup = false;
static char g_GroupSearchBuf[128] = "";

static std::string g_OriginalIdentifier = "";

inline bool InputString(const char* label, std::string& str, float width = 0.0f) {
    static char buffer[1024];
    strncpy_s(buffer, sizeof(buffer), str.c_str(), _TRUNCATE);
    if (width != 0.0f) ImGui::SetNextItemWidth(width);
    bool changed = ImGui::InputText(label, buffer, sizeof(buffer));
    if (changed) str = buffer;
    return changed;
}

inline std::string EnforceLugExtension(const std::string& bankName) {
    std::string fixed = bankName;
    size_t lastDot = fixed.find_last_of('.');
    if (lastDot != std::string::npos) {
        std::string ext = fixed.substr(lastDot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".lut" || ext == ".bin") {
            fixed = fixed.substr(0, lastDot) + ".lug";
        }
    }
    else if (!fixed.empty()) {
        fixed += ".lug";
    }
    return fixed;
}

// Updated Signature to include onJump callback
inline void DrawTextProperties(LoadedBank* bank, std::function<void()> onSave, std::function<void(std::string, uint32_t, std::string)> onJump) {
    if (!g_TextParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse text entry.");
        return;
    }

    if (bank != g_LastBankPtr || (bank && bank->SelectedEntryIndex != g_LastEntryID)) {
        g_IsTextDirty = false;
        g_LastBankPtr = bank;
        if (bank) g_LastEntryID = bank->SelectedEntryIndex;
        for (auto& [k, p] : g_BackgroundAudioBanks) p->Player.Reset();

        if (g_TextParser.IsParsed) g_OriginalIdentifier = g_TextParser.TextData.Identifier;
    }

    bool isAudioModified = false;
    if (g_TextParser.IsParsed && !g_TextParser.IsGroup && !g_TextParser.IsNarratorList) {
        if (!g_TextParser.TextData.SpeechBank.empty()) {
            std::string key = g_TextParser.TextData.SpeechBank;
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            if (g_BackgroundAudioBanks.count(key)) {
                if (!g_BackgroundAudioBanks[key]->ModifiedCache.empty()) isAudioModified = true;
            }
        }
    }

    bool isLipSyncModified = !g_LipSyncState.PendingAdds.empty() || !g_LipSyncState.PendingDeletes.empty();

    if (g_IsTextDirty || isAudioModified || isLipSyncModified) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));

        bool triggered = ImGui::Button("SAVE ENTRY CHANGES (Ctrl+S)", ImVec2(-FLT_MIN, 40));
        if ((ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) || triggered) {
            if (onSave) {
                g_TextParser.TextData.SpeechBank = EnforceLugExtension(g_TextParser.TextData.SpeechBank);

                if (!g_OriginalIdentifier.empty() && g_TextParser.TextData.Identifier != g_OriginalIdentifier) {
                    if (!g_TextParser.TextData.SpeechBank.empty()) {
                        UpdateHeaderDefinition(g_TextParser.TextData.SpeechBank, g_OriginalIdentifier, g_TextParser.TextData.Identifier);
                    }
                    if (bank && bank->SelectedEntryIndex >= 0) {
                        bank->Entries[bank->SelectedEntryIndex].Name = g_TextParser.TextData.Identifier;
                    }
                    g_OriginalIdentifier = g_TextParser.TextData.Identifier;
                }

                if (bank && bank->SelectedEntryIndex >= 0) {
                    auto& entry = bank->Entries[bank->SelectedEntryIndex];
                    std::string currentBank = g_TextParser.TextData.SpeechBank;

                    if (!currentBank.empty()) {
                        bool exists = false;
                        for (const auto& d : entry.Dependencies) {
                            if (d == currentBank) exists = true;
                        }

                        if (!exists) {
                            entry.Dependencies.clear();
                            entry.Dependencies.push_back(currentBank);
                        }
                    }
                }

                onSave();

                if (isAudioModified) {
                    std::string key = g_TextParser.TextData.SpeechBank;
                    size_t dot = key.find_last_of('.');
                    if (dot != std::string::npos) key = key.substr(0, dot) + ".lut";

                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                    if (g_BackgroundAudioBanks.count(key)) {
                        g_BackgroundAudioBanks[key]->SaveBank(g_BackgroundAudioBanks[key]->FileName);
                        g_BackgroundAudioBanks[key]->ModifiedCache.clear();
                    }
                }

                if (isLipSyncModified) {
                    if (EnsureLipSyncLoaded()) {
                        if (LipSyncCompiler::CompileLipSyncFromState(g_LipSyncState)) {
                            g_LipSyncState.PendingAdds.clear();
                            g_LipSyncState.PendingDeletes.clear();
                            g_LipSyncState.CachedSubBankIndex = -1;
                        }
                    }
                }

                if (!g_TextParser.TextData.SpeechBank.empty()) {
                    SaveAssociatedHeader(g_TextParser.TextData.SpeechBank);
                }

                g_IsTextDirty = false;
            }
        }
        ImGui::PopStyleColor(2);
    }

    if (g_TextParser.IsGroup) {
        // ... (Group logic same as before) ...
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Group Entry (%zu Items)", g_TextParser.GroupData.Items.size());
        ImGui::Separator();

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

                ImGui::TableSetColumnIndex(0); ImGui::Text("%d", item.ID);
                ImGui::TableSetColumnIndex(1);
                if (item.CachedName.empty() && bank) item.CachedName = PeekEntryName(bank, item.ID);
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

        if (ImGui::Button("+ Add Entry")) {
            g_ShowAddGroupItemPopup = true;
            g_GroupSearchBuf[0] = '\0';
            ImGui::OpenPopup("Add Group Item");
        }

        if (ImGui::BeginPopupModal("Add Group Item", &g_ShowAddGroupItemPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            // ... (Popup logic same as before) ...
            ImGui::Text("Search for an entry to add:");
            ImGui::InputText("##search", g_GroupSearchBuf, 128);
            ImGui::Separator();
            std::string search = g_GroupSearchBuf;
            std::transform(search.begin(), search.end(), search.begin(), ::tolower);

            int suggestionsFound = 0;
            if (bank && !search.empty()) {
                if (ImGui::BeginListBox("##suggestions", ImVec2(400, 150))) {
                    for (const auto& entry : bank->Entries) {
                        if (entry.Type != 0) continue;
                        if (entry.ID == bank->Entries[bank->SelectedEntryIndex].ID) continue;

                        std::string nameLower = entry.Name;
                        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                        std::string idStr = std::to_string(entry.ID);

                        if (nameLower.find(search) != std::string::npos || idStr.find(search) != std::string::npos) {
                            std::string label = entry.Name + " (" + std::to_string(entry.ID) + ")";
                            if (ImGui::Selectable(label.c_str())) {
                                CTextGroupItem newItem;
                                newItem.ID = entry.ID;
                                newItem.CachedName = entry.Name;
                                g_TextParser.GroupData.Items.push_back(newItem);
                                g_IsTextDirty = true;
                                g_ShowAddGroupItemPopup = false;
                                ImGui::CloseCurrentPopup();
                            }
                            suggestionsFound++;
                            if (suggestionsFound >= 5) break;
                        }
                    }
                    ImGui::EndListBox();
                }
            }
            ImGui::Separator();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { g_ShowAddGroupItemPopup = false; ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        ImGui::Spacing(); ImGui::Separator();
    }
    else if (g_TextParser.IsNarratorList) {
        // ... (Narrator logic same as before) ...
        ImGui::Text("Narrator List");
        ImGui::Separator();
        if (ImGui::BeginTable("NarratorTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn("String ID", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            for (int i = 0; i < g_TextParser.NarratorStrings.size(); i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", g_TextParser.NarratorStrings[i].c_str());
            }
            ImGui::EndTable();
        }
    }
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

            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Speaker");
            ImGui::TableSetColumnIndex(1); if (InputString("##speaker", e.Speaker, -FLT_MIN)) g_IsTextDirty = true;

            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Sound Bank");
            ImGui::TableSetColumnIndex(1);
            if (g_AvailableSoundBanks.empty()) {
                if (InputString("##soundbank", e.SpeechBank, -FLT_MIN)) g_IsTextDirty = true;
            }
            else {
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::BeginCombo("##soundbank", e.SpeechBank.c_str())) {
                    for (const auto& sb : g_AvailableSoundBanks) {
                        std::string cleanName = EnforceLugExtension(sb);
                        bool isSelected = (e.SpeechBank == cleanName);
                        if (ImGui::Selectable(cleanName.c_str(), isSelected)) {
                            e.SpeechBank = cleanName;
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
                ImGui::TableSetColumnIndex(0); if (InputString("##name", e.Tags[i].Name, -FLT_MIN)) g_IsTextDirty = true;
                ImGui::TableSetColumnIndex(1); if (ImGui::Button("X")) tagToDelete = i;
                ImGui::PopID();
            }
            ImGui::EndTable();
            if (tagToDelete != -1) { e.Tags.erase(e.Tags.begin() + tagToDelete); g_IsTextDirty = true; }
        }

        if (ImGui::Button("+ Add New Tag")) {
            CTextTag newTag; newTag.Position = 0; newTag.Name = "NEW_TAG";
            e.Tags.push_back(newTag);
            g_IsTextDirty = true;
        }

        ImGui::Spacing(); ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Linked Media");
        ImGui::Separator();

        bool audioFound = false;
        std::shared_ptr<AudioBankParser> audioBank = nullptr;
        int audioIndex = -1;

        if (!e.SpeechBank.empty() && !e.Identifier.empty()) {
            int32_t soundID = ResolveAudioID(e.SpeechBank, e.Identifier);

            if (soundID != -1) {
                std::string loadPath = e.SpeechBank;
                if (loadPath.find(".lug") != std::string::npos) loadPath = loadPath.substr(0, loadPath.find(".lug")) + ".lut";

                audioBank = GetOrLoadAudioBank(loadPath);
                if (audioBank) {
                    for (int i = 0; i < (int)audioBank->Entries.size(); i++) {
                        if (audioBank->Entries[i].SoundID == (uint32_t)soundID) {
                            audioIndex = i;
                            audioFound = true;
                            break;
                        }
                    }

                    if (audioFound) {
                        ImGui::Text("Linked ID: %d", soundID);
                        if (audioBank->ModifiedCache.count(audioIndex)) {
                            ImGui::SameLine(); ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "[PENDING SAVE]");
                        }

                        // --- NEW BUTTONS START ---
                        if (ImGui::Button("Go to Phonemes (Dialogue)", ImVec2(200, 0))) {
                            if (onJump) onJump("dialogue.big", (uint32_t)soundID, e.SpeechBank);
                        }

                        if (ImGui::Button("Go to Sample (Audio)", ImVec2(200, 0))) {
                            std::string lut = e.SpeechBank;
                            if (lut.find(".lug") != std::string::npos) lut = lut.substr(0, lut.find(".lug")) + ".lut";
                            else if (lut.find(".") == std::string::npos) lut += ".lut";

                            if (onJump) onJump(lut, (uint32_t)soundID, ""); // No sub-bank hint needed for .lut
                        }
                        // --- NEW BUTTONS END ---

                        ImGui::Spacing();

                        auto& player = audioBank->Player;
                        float currentT = player.GetCurrentTime();
                        float totalT = player.GetTotalDuration();
                        float progress = player.GetProgress();

                        ImGui::PushItemWidth(300);
                        if (ImGui::SliderFloat("##seek", &progress, 0.0f, 1.0f, "")) {
                            player.Seek(progress);
                        }
                        ImGui::PopItemWidth();
                        ImGui::SameLine();
                        ImGui::Text("%s / %s", FormatAudioTime(currentT).c_str(), FormatAudioTime(totalT).c_str());

                        if (ImGui::Button(player.IsPlaying() ? "Pause" : "Play", ImVec2(80, 0))) {
                            if (totalT == 0.0f) {
                                auto pcm = audioBank->GetDecodedAudio(audioIndex);
                                if (!pcm.empty()) player.PlayPCM(pcm, 22050);
                            }
                            else {
                                if (player.IsPlaying()) player.Pause(); else player.Play();
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Export Wav", ImVec2(80, 0))) {
                            auto pcm = audioBank->GetDecodedAudio(audioIndex);
                            if (!pcm.empty()) {
                                std::string savePath = SaveFileDialog("WAV File\0*.wav\0");
                                if (!savePath.empty()) {
                                    if (savePath.find(".wav") == std::string::npos) savePath += ".wav";
                                    WriteWavFile(savePath, pcm, 22050, 1);
                                }
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Import Wav", ImVec2(80, 0))) {
                            std::string openPath = OpenFileDialog("WAV File\0*.wav\0All Files\0*.*\0");
                            if (!openPath.empty()) {
                                if (audioBank->ImportWav(audioIndex, openPath)) {
                                    player.Reset();
                                    g_IsTextDirty = true;
                                }
                            }
                        }
                    }
                    else {
                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "ID %d found in Defs, but not in Audio Bank.", soundID);
                    }
                }
                else {
                    ImGui::TextDisabled("Audio bank not found on disk (.lut missing).");
                }
            }
            else {
                ImGui::TextDisabled("No match for '%s' in %s", e.Identifier.c_str(), GetHeaderName(e.SpeechBank).c_str());
                ImGui::Spacing();

                if (ImGui::Button("Create Linked Media (Import Wav)", ImVec2(-FLT_MIN, 40))) {
                    std::string loadPath = e.SpeechBank;
                    if (loadPath.find(".lug") != std::string::npos) loadPath = loadPath.substr(0, loadPath.find(".lug")) + ".lut";

                    audioBank = GetOrLoadAudioBank(loadPath);
                    if (audioBank) {
                        uint32_t nextID = GetNextIDFromHeader(e.SpeechBank);

                        std::string wavPath = OpenFileDialog("WAV File\0*.wav\0");
                        if (!wavPath.empty()) {
                            std::string hName = GetHeaderName(e.SpeechBank);
                            int hIdx = FindHeaderIndex(hName);
                            if (hIdx != -1) {
                                std::string defName = "SND_" + e.Identifier;
                                InjectHeaderDefinition(hIdx, defName, nextID);
                                if (audioBank->AddEntry(nextID, wavPath)) {
                                    float dur = AudioBankParser::GetWavDuration(wavPath);
                                    AddLipSyncEntry(e.SpeechBank, nextID, dur);
                                    g_IsTextDirty = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        else {
            ImGui::TextDisabled("Assign a SpeechBank and Identifier to link media.");
        }
    }
}