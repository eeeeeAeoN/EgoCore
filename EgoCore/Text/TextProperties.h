#pragma once
#include "imgui.h"
#include "BankBackend.h" 
#include "TextParser.h"
#include "BinaryParser.h" 
#include "DefBackend.h" 
#include "AudioBackend.h" 
#include "FileDialogs.h"
#include "LipSyncProperties.h" // Backend functions are here
#include <string>
#include <vector>
#include <cstring>
#include <algorithm> 
#include <functional>
#include <regex>
#include <map>
#include <memory>
#include <filesystem>

static CTextParser g_TextParser;

// --- PERSISTENT STATE ---
static bool g_IsTextDirty = false;
static int g_LastEntryID = -1;
static void* g_LastBankPtr = nullptr;

// --- STATE FOR ADD ENTRY POPUP ---
static bool g_ShowAddGroupItemPopup = false;
static char g_GroupSearchBuf[128] = "";

// --- BACKGROUND AUDIO STATE ---
static std::map<std::string, std::shared_ptr<AudioBankParser>> g_BackgroundAudioBanks;

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

inline std::string PeekEntryName(LoadedBank* bank, uint32_t id) {
    if (!bank) return "Unknown";
    for (const auto& e : bank->Entries) {
        if (e.ID == id) return e.Name;
    }
    return "Unknown ID";
}

// --- AUDIO LOGIC HELPERS ---

static std::string FormatAudioTime(float seconds) {
    int m = (int)seconds / 60;
    int s = (int)seconds % 60;
    int ms = (int)((seconds - (int)seconds) * 100);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d.%02d", m, s, ms);
    return std::string(buf);
}

inline std::string GetHeaderName(const std::string& speechBank) {
    std::string stem = speechBank;
    size_t lastDot = stem.find_last_of('.');
    if (lastDot != std::string::npos) stem = stem.substr(0, lastDot);
    std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);

    if (stem == "dialogue")             return "dialoguesnds.h";
    if (stem == "dialogue2")            return "dialoguesnds2.h";
    if (stem == "scriptdialogue")       return "scriptdialoguesnds.h";
    if (stem == "scriptdialogue2")      return "scriptdialoguesnds2.h";
    return stem + "snds.h";
}

inline int FindHeaderIndex(const std::string& headerName) {
    for (int i = 0; i < (int)g_DefWorkspace.AllEnums.size(); i++) {
        std::string path = g_DefWorkspace.AllEnums[i].FilePath;
        std::string fname = std::filesystem::path(path).filename().string();
        std::transform(fname.begin(), fname.end(), fname.begin(), ::tolower);
        if (fname == headerName) return i;
    }
    return -1;
}

inline int32_t ResolveAudioID(const std::string& speechBank, const std::string& identifier) {
    if (speechBank.empty() || identifier.empty()) return -1;
    std::string headerName = GetHeaderName(speechBank);
    int enumIdx = FindHeaderIndex(headerName);
    if (enumIdx == -1) return -1;

    const std::string& content = g_DefWorkspace.AllEnums[enumIdx].FullContent;
    std::string idSafe = identifier;
    std::string patternStr = "(SND_|TEXT_SND_)" + idSafe + "\\s*=\\s*(\\d+)";
    std::regex re(patternStr, std::regex::icase);
    std::smatch match;
    if (std::regex_search(content, match, re)) {
        if (match.size() >= 3) {
            try { return std::stoi(match[2].str()); }
            catch (...) { return -1; }
        }
    }
    return -1;
}

inline std::shared_ptr<AudioBankParser> GetOrLoadAudioBank(const std::string& bankName) {
    std::string key = bankName;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    if (g_BackgroundAudioBanks.count(key)) return g_BackgroundAudioBanks[key];

    std::string stem = bankName;
    size_t lastDot = stem.find_last_of('.');
    if (lastDot != std::string::npos) stem = stem.substr(0, lastDot);
    std::string filename = stem + ".lut";

    std::string root = g_AppConfig.GameRootPath;
    std::vector<std::string> candidates = {
        root + "\\Data\\Lang\\English\\" + filename,
        root + "\\Data\\" + filename,
        root + "\\Data\\Audio\\" + filename
    };

    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            auto parser = std::make_shared<AudioBankParser>();
            if (parser->Parse(path)) {
                g_BackgroundAudioBanks[key] = parser;
                return parser;
            }
        }
    }
    return nullptr;
}

inline uint32_t GetMaxIDInAudioBank(std::shared_ptr<AudioBankParser> bank) {
    uint32_t maxID = 0;
    for (const auto& e : bank->Entries) {
        if (e.SoundID > maxID) maxID = e.SoundID;
    }
    return maxID;
}

inline void InjectHeaderDefinition(int enumIdx, const std::string& entryName, uint32_t id) {
    if (enumIdx < 0 || enumIdx >= g_DefWorkspace.AllEnums.size()) return;
    auto& entry = g_DefWorkspace.AllEnums[enumIdx];
    size_t closing = entry.FullContent.rfind("};");
    if (closing != std::string::npos) {
        std::string insertion = "\t" + entryName + " = " + std::to_string(id) + ",\n";
        entry.FullContent.insert(closing, insertion);
    }
}

// --- MAIN DRAWER ---

inline void DrawTextProperties(LoadedBank* bank, std::function<void()> onSave) {
    if (!g_TextParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse text entry.");
        return;
    }

    // Reset dirty flag if we switched entries
    if (bank != g_LastBankPtr || (bank && bank->SelectedEntryIndex != g_LastEntryID)) {
        g_IsTextDirty = false;
        g_LastBankPtr = bank;
        if (bank) g_LastEntryID = bank->SelectedEntryIndex;
        // Reset player if active?
        for (auto& [k, p] : g_BackgroundAudioBanks) p->Player.Reset();
    }

    // --- SHORTCUT: CTRL + S ---
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        if (onSave) {
            onSave();
            g_IsTextDirty = false;
        }
    }

    // =========================================================
    // TYPE 1: GROUP ENTRY
    // =========================================================
    if (g_TextParser.IsGroup) {
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
    // =========================================================
    // TYPE 2: NARRATOR LIST
    // =========================================================
    else if (g_TextParser.IsNarratorList) {
        ImGui::Text("Narrator List");
        ImGui::Separator();

        // Simple List View
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
                        bool isSelected = (e.SpeechBank == sb);
                        if (ImGui::Selectable(sb.c_str(), isSelected)) { e.SpeechBank = sb; g_IsTextDirty = true; }
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

        // =========================================================
        // AUDIO & LIPSYNC INTEGRATION
        // =========================================================
        ImGui::Spacing(); ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Linked Media");
        ImGui::Separator();

        bool audioFound = false;
        std::shared_ptr<AudioBankParser> audioBank = nullptr;
        int audioIndex = -1;

        if (!e.SpeechBank.empty() && !e.Identifier.empty()) {
            int32_t soundID = ResolveAudioID(e.SpeechBank, e.Identifier);

            if (soundID != -1) {
                // --- AUDIO ---
                audioBank = GetOrLoadAudioBank(e.SpeechBank);
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
                    ImGui::TextDisabled("Audio bank not found.");
                }

                // --- LIPSYNC ---
                ImGui::Separator();
                ImGui::Text("LipSync Data:");
                static std::unique_ptr<CLipSyncData> cachedLipSync = nullptr;
                static int32_t lastCachedID = -1;

                if (lastCachedID != soundID) {
                    cachedLipSync = FetchLipSyncData(soundID, e.SpeechBank);
                    lastCachedID = soundID;
                }

                if (cachedLipSync && cachedLipSync->IsParsed) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0, 1, 1, 1), "Found (%.2fs)", cachedLipSync->Duration);
                    RenderLipSyncFrames(*cachedLipSync);
                }
                else {
                    ImGui::SameLine();
                    ImGui::TextDisabled("Not found or failed to load (dialogue.big)");
                }

            }
            else {
                // --- CREATE LOGIC ---
                ImGui::TextDisabled("No match for '%s' in %s", e.Identifier.c_str(), GetHeaderName(e.SpeechBank).c_str());
                ImGui::Spacing();

                if (ImGui::Button("Create Linked Media (Import Wav)", ImVec2(-FLT_MIN, 40))) {
                    // 1. Load Audio Bank to find max ID
                    audioBank = GetOrLoadAudioBank(e.SpeechBank);
                    if (audioBank) {
                        uint32_t nextID = GetMaxIDInAudioBank(audioBank) + 1;
                        if (nextID < 20000) nextID = 20000;

                        // 2. Open Wav
                        std::string wavPath = OpenFileDialog("WAV File\0*.wav\0");
                        if (!wavPath.empty()) {
                            // 3. Inject into Header
                            std::string hName = GetHeaderName(e.SpeechBank);
                            int hIdx = FindHeaderIndex(hName);
                            if (hIdx != -1) {
                                std::string defName = "SND_" + e.Identifier; // Default convention
                                InjectHeaderDefinition(hIdx, defName, nextID);

                                // 4. Add to Audio Bank
                                if (audioBank->AddEntry(nextID, wavPath)) {
                                    // 5. Add to LipSync (Placeholder)
                                    AddLipSyncEntry(e.SpeechBank, nextID);

                                    g_IsTextDirty = true; // Trigger save button availability
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

        ImGui::Spacing(); ImGui::Separator();
    }

    // =========================================================
    // GLOBAL SAVE BUTTON
    // =========================================================
    bool isAudioModified = false;
    // Check if the related audio bank is modified
    if (g_TextParser.IsParsed && !g_TextParser.IsGroup && !g_TextParser.IsNarratorList) {
        if (!g_TextParser.TextData.SpeechBank.empty()) {
            std::string key = g_TextParser.TextData.SpeechBank;
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            if (g_BackgroundAudioBanks.count(key)) {
                if (!g_BackgroundAudioBanks[key]->ModifiedCache.empty()) isAudioModified = true;
            }
        }
    }

    if (g_IsTextDirty || isAudioModified) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));

        if (ImGui::Button("SAVE ENTRY CHANGES (Ctrl+S)", ImVec2(-FLT_MIN, 40))) {
            if (onSave) {
                // 1. Save Text Entry (Memory)
                onSave();

                // 2. Save Audio Bank (Disk)
                if (isAudioModified) {
                    std::string key = g_TextParser.TextData.SpeechBank;
                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                    if (g_BackgroundAudioBanks.count(key)) {
                        g_BackgroundAudioBanks[key]->SaveBank(g_BackgroundAudioBanks[key]->FileName);
                        g_BackgroundAudioBanks[key]->ModifiedCache.clear();
                    }
                }

                // 3. Save Header if Injected
                if (!g_TextParser.TextData.SpeechBank.empty()) {
                    std::string hName = GetHeaderName(g_TextParser.TextData.SpeechBank);
                    int hIdx = FindHeaderIndex(hName);
                    if (hIdx != -1) {
                        auto& entry = g_DefWorkspace.AllEnums[hIdx];
                        std::ofstream outFile(entry.FilePath, std::ios::binary);
                        outFile << entry.FullContent;
                        outFile.close();
                    }
                }

                g_IsTextDirty = false;
            }
        }
        ImGui::PopStyleColor(2);
    }
}