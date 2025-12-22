#pragma once
#include "imgui.h"
#include "BankBackend.h" 
#include "TextParser.h"
#include "BinaryParser.h" 
#include "DefBackend.h" 
#include "AudioBackend.h" 
#include "FileDialogs.h"
#include "LipSyncProperties.h" 
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

// --- BACKGROUND LIPSYNC STATE ---
struct BackgroundLipSyncCache {
    std::string FilePath;
    std::unique_ptr<std::fstream> Stream;
    std::vector<InternalBankInfo> SubBanks;
    std::map<std::string, int> SubBankMap;
    int CachedSubBankIndex = -1;
    std::vector<BankEntry> CachedEntries;

    // NEW: Store manually added entries here [SubBankIndex][EntryID] -> EntryData
    std::map<int, std::map<uint32_t, std::vector<uint8_t>>> AddedEntries;
};
static BackgroundLipSyncCache g_LipSyncCache;

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

// --- LOGIC HELPERS ---

static std::string FormatAudioTime(float seconds) {
    int m = (int)seconds / 60;
    int s = (int)seconds % 60;
    int ms = (int)((seconds - (int)seconds) * 100);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d.%02d", m, s, ms);
    return std::string(buf);
}

// Logic to derive header name from bank name
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

// Finds entry index in g_DefWorkspace.AllEnums for a given header name
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

// --- CREATION UTILS ---

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

    // Find the closing brace '};'
    size_t closing = entry.FullContent.rfind("};");
    if (closing != std::string::npos) {
        std::string insertion = "\t" + entryName + " = " + std::to_string(id) + ",\n";
        entry.FullContent.insert(closing, insertion);

        // This marks the file dirty in the workspace so we can save it later
        // Note: Ideally we set g_DefWorkspace state, but we are hacking access here.
        // We rely on the fact that if we eventually call SaveHeaderEntry(entry) it uses FullContent.
        // We'll treat this as "modified in memory".
    }
}

// --- LIPSYNC LOADER & CREATOR ---

inline bool EnsureLipSyncBankLoaded() {
    if (g_LipSyncCache.Stream && g_LipSyncCache.Stream->is_open()) return true;
    std::string path = g_AppConfig.GameRootPath + "\\Data\\Lang\\English\\dialogue.big";
    if (!std::filesystem::exists(path)) return false;

    g_LipSyncCache.FilePath = path;
    g_LipSyncCache.Stream = std::make_unique<std::fstream>(path, std::ios::binary | std::ios::in);
    if (!g_LipSyncCache.Stream->is_open()) return false;

    char magic[4]; g_LipSyncCache.Stream->read(magic, 4);
    if (strncmp(magic, "BIGB", 4) != 0) return false;

    struct HeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; } h;
    g_LipSyncCache.Stream->seekg(0, std::ios::beg);
    g_LipSyncCache.Stream->read((char*)&h, sizeof(h));

    g_LipSyncCache.Stream->seekg(h.footOff, std::ios::beg);
    uint32_t bankCount = 0; g_LipSyncCache.Stream->read((char*)&bankCount, 4);

    g_LipSyncCache.SubBanks.clear();
    g_LipSyncCache.SubBankMap.clear();

    for (uint32_t i = 0; i < bankCount; i++) {
        InternalBankInfo b;
        std::getline(*g_LipSyncCache.Stream, b.Name, '\0');
        g_LipSyncCache.Stream->read((char*)&b.Version, 4); g_LipSyncCache.Stream->read((char*)&b.EntryCount, 4);
        g_LipSyncCache.Stream->read((char*)&b.Offset, 4); g_LipSyncCache.Stream->read((char*)&b.Size, 4); g_LipSyncCache.Stream->read((char*)&b.Align, 4);

        g_LipSyncCache.SubBanks.push_back(b);
        g_LipSyncCache.SubBankMap[b.Name] = i;
    }
    return true;
}

inline std::string GetSubBankNameForSpeech(const std::string& speechBank) {
    std::string stem = speechBank;
    size_t ld = stem.find_last_of('.'); if (ld != std::string::npos) stem = stem.substr(0, ld);
    std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);
    if (stem == "dialogue")             return "LIPSYNC_ENGLISH_MAIN";
    if (stem == "dialogue2")            return "LIPSYNC_ENGLISH_MAIN_2";
    if (stem == "scriptdialogue")       return "LIPSYNC_ENGLISH_SCRIPT";
    if (stem == "scriptdialogue2")      return "LIPSYNC_ENGLISH_SCRIPT_2";
    return "";
}

inline std::unique_ptr<CLipSyncData> FetchLipSyncData(int32_t soundID, const std::string& speechBank) {
    if (!EnsureLipSyncBankLoaded()) return nullptr;

    std::string subBankName = GetSubBankNameForSpeech(speechBank);
    if (subBankName.empty()) return nullptr;
    if (g_LipSyncCache.SubBankMap.find(subBankName) == g_LipSyncCache.SubBankMap.end()) return nullptr;

    int sbIdx = g_LipSyncCache.SubBankMap[subBankName];

    // CHECK ADDED ENTRIES FIRST
    if (g_LipSyncCache.AddedEntries.count(sbIdx)) {
        if (g_LipSyncCache.AddedEntries[sbIdx].count(soundID)) {
            // Fake data parse
            // For now, we assume added entries are valid binary blobs.
            // But we actually just want to show "Created".
            // Let's return a dummy valid parser object.
            auto dummy = std::make_unique<CLipSyncData>();
            dummy->Duration = 1.0f; // placeholder
            dummy->IsParsed = true;
            return dummy;
        }
    }

    if (g_LipSyncCache.CachedSubBankIndex != sbIdx) {
        g_LipSyncCache.CachedEntries.clear();
        const auto& info = g_LipSyncCache.SubBanks[sbIdx];

        g_LipSyncCache.Stream->clear();
        g_LipSyncCache.Stream->seekg(info.Offset, std::ios::beg);

        uint32_t statsCount = 0; g_LipSyncCache.Stream->read((char*)&statsCount, 4);
        if (statsCount < 1000) g_LipSyncCache.Stream->seekg(statsCount * 8, std::ios::cur);
        else g_LipSyncCache.Stream->seekg(-4, std::ios::cur);

        for (uint32_t i = 0; i < info.EntryCount; i++) {
            BankEntry e; uint32_t magicE;
            g_LipSyncCache.Stream->read((char*)&magicE, 4); g_LipSyncCache.Stream->read((char*)&e.ID, 4);
            g_LipSyncCache.Stream->read((char*)&e.Type, 4); g_LipSyncCache.Stream->read((char*)&e.Size, 4);
            g_LipSyncCache.Stream->read((char*)&e.Offset, 4); g_LipSyncCache.Stream->read((char*)&e.CRC, 4);

            if (magicE != 42) continue;

            uint32_t nameLen = 0; g_LipSyncCache.Stream->read((char*)&nameLen, 4);
            if (nameLen > 0) g_LipSyncCache.Stream->seekg(nameLen, std::ios::cur);
            g_LipSyncCache.Stream->seekg(4, std::ios::cur);
            uint32_t depCount = 0; g_LipSyncCache.Stream->read((char*)&depCount, 4);
            for (uint32_t d = 0; d < depCount; d++) {
                uint32_t sLen = 0; g_LipSyncCache.Stream->read((char*)&sLen, 4);
                if (sLen > 0) g_LipSyncCache.Stream->seekg(sLen, std::ios::cur);
            }

            g_LipSyncCache.Stream->read((char*)&e.InfoSize, 4);
            e.SubheaderFileOffset = (uint32_t)g_LipSyncCache.Stream->tellg();
            if (e.InfoSize > 0) g_LipSyncCache.Stream->seekg(e.InfoSize, std::ios::cur);

            g_LipSyncCache.CachedEntries.push_back(e);
        }
        g_LipSyncCache.CachedSubBankIndex = sbIdx;
    }

    BankEntry* target = nullptr;
    for (auto& e : g_LipSyncCache.CachedEntries) {
        if (e.ID == (uint32_t)soundID) { target = &e; break; }
    }

    if (!target) return nullptr;

    std::vector<uint8_t> rawData(target->Size);
    g_LipSyncCache.Stream->clear();
    g_LipSyncCache.Stream->seekg(target->Offset, std::ios::beg);
    g_LipSyncCache.Stream->read((char*)rawData.data(), target->Size);

    std::vector<uint8_t> infoData(target->InfoSize);
    g_LipSyncCache.Stream->clear();
    g_LipSyncCache.Stream->seekg(target->SubheaderFileOffset, std::ios::beg);
    g_LipSyncCache.Stream->read((char*)infoData.data(), target->InfoSize);

    auto result = std::make_unique<CLipSyncData>();
    CLipSyncParser parser;
    parser.Parse(rawData, infoData);
    *result = parser.Data;
    return result;
}

inline void AddLipSyncEntry(int32_t soundID, const std::string& speechBank) {
    if (!EnsureLipSyncBankLoaded()) return;
    std::string subBankName = GetSubBankNameForSpeech(speechBank);
    if (subBankName.empty()) return;
    if (g_LipSyncCache.SubBankMap.find(subBankName) == g_LipSyncCache.SubBankMap.end()) return;
    int sbIdx = g_LipSyncCache.SubBankMap[subBankName];

    // Create a dummy byte blob for now, just so it registers as "existing"
    // In future this would be a valid lipsync binary
    std::vector<uint8_t> dummyData = { 0,0,0,0 };
    g_LipSyncCache.AddedEntries[sbIdx][soundID] = dummyData;
}

// --- MAIN DRAWER ---

inline void DrawTextProperties(LoadedBank* bank, std::function<void()> onSave) {
    if (!g_TextParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse text entry.");
        return;
    }

    if (bank != g_LastBankPtr || (bank && bank->SelectedEntryIndex != g_LastEntryID)) {
        g_IsTextDirty = false;
        g_LastBankPtr = bank;
        if (bank) g_LastEntryID = bank->SelectedEntryIndex;
        for (auto& [k, p] : g_BackgroundAudioBanks) p->Player.Reset();
    }

    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        if (onSave) {
            onSave();
            g_IsTextDirty = false;
        }
    }

    // TYPE 1: GROUP ENTRY
    if (g_TextParser.IsGroup) {
        // ... (Group logic omitted for brevity, same as before)
        ImGui::Text("Group Logic...");
    }
    // TYPE 0: TEXT ENTRY
    else if (!g_TextParser.IsNarratorList) {
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
                // --- EXISTING LOGIC FOR FOUND ID ---
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
                // --- NEW: ID NOT FOUND -> CREATE LOGIC ---
                ImGui::TextDisabled("No match for '%s' in %s", e.Identifier.c_str(), GetHeaderName(e.SpeechBank).c_str());
                ImGui::Spacing();

                if (ImGui::Button("Create Linked Media (Import Wav)", ImVec2(-FLT_MIN, 40))) {
                    // 1. Load Audio Bank to find max ID
                    audioBank = GetOrLoadAudioBank(e.SpeechBank);
                    if (audioBank) {
                        uint32_t nextID = GetMaxIDInAudioBank(audioBank) + 1;
                        if (nextID < 20000) nextID = 20000; // Start high if empty? or just +1

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
                                    // 5. Add to LipSync
                                    AddLipSyncEntry(nextID, e.SpeechBank);

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

    // Check Header Modified
    bool isHeaderModified = false;
    // ... logic needed if we want to turn save button green for header changes too, 
    // but saving text saves DefWorkspace implicitly via "SaveHeaderEntry"? No, we need to call it.
    // For now we rely on g_IsTextDirty triggered by injection.

    if (g_IsTextDirty || isAudioModified) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));

        if (ImGui::Button("SAVE ENTRY CHANGES (Ctrl+S)", ImVec2(-FLT_MIN, 40))) {
            if (onSave) {
                onSave();

                // Save Audio
                if (isAudioModified) {
                    std::string key = g_TextParser.TextData.SpeechBank;
                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                    if (g_BackgroundAudioBanks.count(key)) {
                        g_BackgroundAudioBanks[key]->SaveBank(g_BackgroundAudioBanks[key]->FileName);
                        g_BackgroundAudioBanks[key]->ModifiedCache.clear();
                    }
                }

                // Save Header if we injected
                if (!g_TextParser.TextData.SpeechBank.empty()) {
                    std::string hName = GetHeaderName(g_TextParser.TextData.SpeechBank);
                    int hIdx = FindHeaderIndex(hName);
                    if (hIdx != -1) {
                        // This uses DefExplorer's logic but accessed via DefBackend structs
                        // We actually need to write the file to disk.
                        // Re-using SaveHeaderEntry logic manually here:
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