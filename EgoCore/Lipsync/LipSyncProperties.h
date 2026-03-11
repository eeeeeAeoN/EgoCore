#pragma once
#include "imgui.h"
#include "LipSyncParser.h"
#include "BankBackend.h"
#include "ConfigBackend.h"
#include "SpeechAnalyzer.h"
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <fstream>
#include <memory>
#include <filesystem>
#include <cctype> 
#include <cmath>

static CLipSyncParser g_LipSyncParser;

struct AddedEntryData {
    uint32_t Type;
    std::string NamePrefix;
    std::vector<uint8_t> Raw;
    std::vector<uint8_t> Info;
    std::vector<std::string> Dependencies;
};

struct LipSyncState {
    std::string FilePath;
    std::unique_ptr<std::fstream> Stream;
    std::vector<InternalBankInfo> SubBanks;
    std::map<std::string, int> SubBankMap;

    int CachedSubBankIndex = -1;
    std::vector<BankEntry> CachedEntries;

    std::map<int, std::set<uint32_t>> PendingDeletes;
    std::map<int, std::map<uint32_t, AddedEntryData>> PendingAdds;
};

static LipSyncState g_LipSyncState;

inline std::string GetSubBankNameForSpeech(const std::string& speechBank) {
    std::string stem = speechBank;
    size_t ld = stem.find_last_of('.'); if (ld != std::string::npos) stem = stem.substr(0, ld);
    std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);

    std::string targetSuffix = "";
    if (stem == "dialogue") targetSuffix = "_MAIN";
    else if (stem == "dialogue2") targetSuffix = "_MAIN_2";
    else if (stem == "scriptdialogue") targetSuffix = "_SCRIPT";
    else if (stem == "scriptdialogue2") targetSuffix = "_SCRIPT_2";

    // Check loaded memory to match active language
    if (!targetSuffix.empty() && !g_LipSyncState.SubBanks.empty()) {
        for (const auto& sb : g_LipSyncState.SubBanks) {
            if (sb.Name.find(targetSuffix) != std::string::npos) return sb.Name;
        }
    }
    return "LIPSYNC_ENGLISH" + targetSuffix; // Safe fallback
}

inline bool EnsureLipSyncLoaded() {
    if (g_LipSyncState.Stream && g_LipSyncState.Stream->is_open()) return true;

    std::string path = "";
    const char* langs[] = { "English", "French", "Italian", "Chinese", "German", "Korean", "Japanese", "Spanish" };
    for (const char* l : langs) {
        std::string p = g_AppConfig.GameRootPath + "\\Data\\Lang\\" + std::string(l) + "\\dialogue.big";
        if (std::filesystem::exists(p)) { path = p; break; }
    }

    if (path.empty()) return false;

    g_LipSyncState.FilePath = path;
    g_LipSyncState.Stream = std::make_unique<std::fstream>(path, std::ios::binary | std::ios::in);

    char magic[4]; g_LipSyncState.Stream->read(magic, 4);
    if (strncmp(magic, "BIGB", 4) != 0) return false;

    struct HeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; } h;
    g_LipSyncState.Stream->seekg(0, std::ios::beg);
    g_LipSyncState.Stream->read((char*)&h, sizeof(h));

    g_LipSyncState.Stream->seekg(h.footOff, std::ios::beg);
    uint32_t bankCount = 0; g_LipSyncState.Stream->read((char*)&bankCount, 4);

    g_LipSyncState.SubBanks.clear();
    g_LipSyncState.SubBankMap.clear();

    for (uint32_t i = 0; i < bankCount; i++) {
        InternalBankInfo b;
        std::getline(*g_LipSyncState.Stream, b.Name, '\0');
        g_LipSyncState.Stream->read((char*)&b.Version, 4); g_LipSyncState.Stream->read((char*)&b.EntryCount, 4);
        g_LipSyncState.Stream->read((char*)&b.Offset, 4); g_LipSyncState.Stream->read((char*)&b.Size, 4); g_LipSyncState.Stream->read((char*)&b.Align, 4);
        g_LipSyncState.SubBanks.push_back(b);
        g_LipSyncState.SubBankMap[b.Name] = i;
    }
    return true;
}

inline void LoadLipSyncSubBankEntries(int sbIdx) {
    if (g_LipSyncState.CachedSubBankIndex == sbIdx) return;

    g_LipSyncState.CachedEntries.clear();
    const auto& info = g_LipSyncState.SubBanks[sbIdx];
    g_LipSyncState.Stream->clear();
    g_LipSyncState.Stream->seekg(info.Offset, std::ios::beg);

    uint32_t checkVal = 0; g_LipSyncState.Stream->read((char*)&checkVal, 4);
    if (checkVal == 42) g_LipSyncState.Stream->seekg(-4, std::ios::cur);
    else if (checkVal < 1000) g_LipSyncState.Stream->seekg(checkVal * 8, std::ios::cur);
    else g_LipSyncState.Stream->seekg(-4, std::ios::cur);

    for (uint32_t i = 0; i < info.EntryCount; i++) {
        BankEntry e; uint32_t magicE;
        g_LipSyncState.Stream->read((char*)&magicE, 4); g_LipSyncState.Stream->read((char*)&e.ID, 4);
        g_LipSyncState.Stream->read((char*)&e.Type, 4); g_LipSyncState.Stream->read((char*)&e.Size, 4);
        g_LipSyncState.Stream->read((char*)&e.Offset, 4); g_LipSyncState.Stream->read((char*)&e.CRC, 4);

        if (magicE != 42 && e.Size == 0) continue;

        uint32_t nameLen = 0; g_LipSyncState.Stream->read((char*)&nameLen, 4);
        if (nameLen > 0) g_LipSyncState.Stream->seekg(nameLen, std::ios::cur);
        g_LipSyncState.Stream->seekg(4, std::ios::cur);
        uint32_t depCount = 0; g_LipSyncState.Stream->read((char*)&depCount, 4);
        for (uint32_t d = 0; d < depCount; d++) {
            uint32_t sLen = 0; g_LipSyncState.Stream->read((char*)&sLen, 4);
            if (sLen > 0) g_LipSyncState.Stream->seekg(sLen, std::ios::cur);
        }

        g_LipSyncState.Stream->read((char*)&e.InfoSize, 4);
        e.SubheaderFileOffset = (uint32_t)g_LipSyncState.Stream->tellg();
        if (e.InfoSize > 0) g_LipSyncState.Stream->seekg(e.InfoSize, std::ios::cur);

        g_LipSyncState.CachedEntries.push_back(e);
    }
    g_LipSyncState.CachedSubBankIndex = sbIdx;
}

inline void AddLipSyncEntry(const std::string& speechBank, uint32_t newID, float duration) {
    if (!EnsureLipSyncLoaded()) return;
    std::string subName = GetSubBankNameForSpeech(speechBank);
    if (subName.empty()) return;
    int sbIdx = g_LipSyncState.SubBankMap[subName];

    // Generate Empty Content (4-Byte Info + Sorted Raw)
    auto blob = CLipSyncParser::GenerateEmpty(duration);

    // Prepare Name Prefix
    std::filesystem::path p(speechBank);
    std::string prefix = p.stem().string();
    if (!prefix.empty()) prefix[0] = toupper(prefix[0]);

    // Store
    AddedEntryData ae;
    ae.Type = 1;
    ae.NamePrefix = prefix;
    ae.Raw = blob.Raw;
    ae.Info = blob.Info;
    ae.Dependencies.push_back("SPEAKER_FEMALE1");

    g_LipSyncState.PendingAdds[sbIdx][newID] = ae;
}

inline void AddLipSyncEntryFromWav(const std::string& speechBank, uint32_t newID, const std::string& wavPath) {
    if (!EnsureLipSyncLoaded()) return;
    std::string subName = GetSubBankNameForSpeech(speechBank);
    if (subName.empty()) return;
    int sbIdx = g_LipSyncState.SubBankMap[subName];

    // 1. Load WAV Data
    std::vector<int16_t> pcm;
    int sampleRate = 0;
    if (!CSpeechAnalyzer::LoadWav(wavPath, pcm, sampleRate)) {
        // Fallback to empty if load fails
        AddLipSyncEntry(speechBank, newID, 1.0f);
        return;
    }

    // 2. Analyze
    CLipSyncData lsData = CSpeechAnalyzer::AnalyzeWav(pcm, sampleRate);

    // 3. Serialize using Parser (ensures sorting and format)
    CLipSyncParser parser;
    parser.Data = lsData;
    std::vector<uint8_t> newRaw = parser.Recompile();

    // 4. Create Info (4 bytes duration)
    std::vector<uint8_t> newInfo(4);
    memcpy(newInfo.data(), &lsData.Duration, 4);

    // 5. Prepare Prefix
    std::filesystem::path p(speechBank);
    std::string prefix = p.stem().string();
    if (!prefix.empty()) prefix[0] = toupper(prefix[0]);

    // 6. Store
    AddedEntryData ae;
    ae.Type = 1;
    ae.NamePrefix = prefix;
    ae.Raw = newRaw;
    ae.Info = newInfo;
    ae.Dependencies.push_back("SPEAKER_FEMALE1");

    g_LipSyncState.PendingAdds[sbIdx][newID] = ae;
    std::cout << "[LipSync] Auto-generated from WAV for ID " << newID << std::endl;
}

inline void DeleteLipSyncEntry(const std::string& speechBank, uint32_t id) {
    if (!EnsureLipSyncLoaded()) return;
    std::string subName = GetSubBankNameForSpeech(speechBank);
    if (subName.empty()) return;
    if (g_LipSyncState.SubBankMap.find(subName) == g_LipSyncState.SubBankMap.end()) return;
    int sbIdx = g_LipSyncState.SubBankMap[subName];

    g_LipSyncState.PendingDeletes[sbIdx].insert(id);
    if (g_LipSyncState.PendingAdds[sbIdx].count(id)) {
        g_LipSyncState.PendingAdds[sbIdx].erase(id);
    }
}

inline std::unique_ptr<CLipSyncData> FetchLipSyncData(int32_t soundID, const std::string& speechBank) {
    if (!EnsureLipSyncLoaded()) return nullptr;
    std::string subName = GetSubBankNameForSpeech(speechBank);
    if (subName.empty()) return nullptr;
    int sbIdx = g_LipSyncState.SubBankMap[subName];

    if (g_LipSyncState.PendingDeletes[sbIdx].count((uint32_t)soundID)) return nullptr;

    if (g_LipSyncState.PendingAdds[sbIdx].count((uint32_t)soundID)) {
        const auto& entry = g_LipSyncState.PendingAdds[sbIdx][(uint32_t)soundID];
        auto result = std::make_unique<CLipSyncData>();
        CLipSyncParser parser;
        parser.Parse(entry.Raw, entry.Info);
        *result = parser.Data;
        result->IsParsed = true;
        if (result->Duration == 0.0f) {
            if (entry.Info.size() >= 4) {
                float dur = *(float*)entry.Info.data();
                result->Duration = dur;
            }
        }
        return result;
    }

    LoadLipSyncSubBankEntries(sbIdx);
    BankEntry* target = nullptr;
    for (auto& e : g_LipSyncState.CachedEntries) {
        if (e.ID == (uint32_t)soundID) { target = &e; break; }
    }
    if (!target) return nullptr;

    std::vector<uint8_t> rawData(target->Size);
    g_LipSyncState.Stream->clear();
    g_LipSyncState.Stream->seekg(target->Offset, std::ios::beg);
    g_LipSyncState.Stream->read((char*)rawData.data(), target->Size);

    std::vector<uint8_t> infoData(target->InfoSize);
    g_LipSyncState.Stream->clear();
    g_LipSyncState.Stream->seekg(target->SubheaderFileOffset, std::ios::beg);
    g_LipSyncState.Stream->read((char*)infoData.data(), target->InfoSize);

    auto result = std::make_unique<CLipSyncData>();
    CLipSyncParser parser;
    parser.Parse(rawData, infoData);
    *result = parser.Data;
    return result;
}

inline std::string GetSymbol(uint8_t id, const std::vector<CLipSyncPhonemeRef>& dict) {
    for (const auto& p : dict) {
        if (p.ID == id) return p.Symbol;
    }
    return "??";
}

inline uint8_t GetOrAddPhonemeID(const std::string& symbol, std::vector<CLipSyncPhonemeRef>& dict) {
    for (const auto& p : dict) {
        if (p.Symbol == symbol) return p.ID;
    }
    std::set<uint8_t> used;
    for (const auto& p : dict) used.insert(p.ID);
    uint8_t nextID = 0;
    while (used.count(nextID)) {
        if (nextID == 255) break;
        nextID++;
    }
    CLipSyncPhonemeRef newRef;
    newRef.ID = nextID;
    newRef.Symbol = symbol;
    dict.push_back(newRef);
    return nextID;
}

static const std::vector<std::string> FABLE_PHONEMES = { "AH", "EE", "MM", "OH", "SZ" };

inline void RenderLipSyncFrames(CLipSyncData& d) {
    if (ImGui::Button("+ Add Frame End")) {
        CLipSyncFrame newFrame;
        d.Frames.push_back(newFrame);
        if (d.FPS > 0.0f) d.Duration = (float)d.Frames.size() / d.FPS;
        d.FrameCount = (uint32_t)d.Frames.size();
    }

    if (ImGui::BeginChild("FrameStream", ImVec2(0, 300), true)) {
        if (ImGui::BeginTable("FrameTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Phonemes", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            int frameToDelete = -1;

            for (size_t i = 0; i < d.Frames.size(); i++) {
                auto& frame = d.Frames[i];
                ImGui::PushID((int)i);
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%03zu", i);
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) {
                    frameToDelete = (int)i;
                }

                ImGui::TableSetColumnIndex(1);
                int keyToDelete = -1;
                for (size_t k = 0; k < frame.Keys.size(); k++) {
                    auto& key = frame.Keys[k];
                    ImGui::PushID((int)k);
                    std::string symbol = GetSymbol(key.ID, d.Dictionary);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", symbol.c_str());
                    ImGui::SameLine();
                    float w = key.WeightFloat;
                    ImGui::SetNextItemWidth(100);
                    if (ImGui::SliderFloat("##w", &w, 0.0f, 1.0f, "%.2f")) {
                        key.WeightFloat = w;
                        key.WeightByte = (uint8_t)(w * 255.0f);
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("x")) keyToDelete = (int)k;
                    ImGui::SameLine();
                    ImGui::Dummy(ImVec2(10, 0));
                    ImGui::SameLine();
                    ImGui::PopID();
                }
                if (keyToDelete != -1) {
                    frame.Keys.erase(frame.Keys.begin() + keyToDelete);
                }
                if (frame.Keys.size() < 3) {
                    if (ImGui::Button("+")) {
                        ImGui::OpenPopup("AddPhonemePopup");
                    }
                    if (ImGui::BeginPopup("AddPhonemePopup")) {
                        for (const auto& ph : FABLE_PHONEMES) {
                            if (ImGui::Selectable(ph.c_str())) {
                                uint8_t pid = GetOrAddPhonemeID(ph, d.Dictionary);
                                CLipSyncFrameKey newKey;
                                newKey.ID = pid;
                                newKey.WeightFloat = 1.0f;
                                newKey.WeightByte = 255;
                                frame.Keys.push_back(newKey);
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        ImGui::EndPopup();
                    }
                }
                ImGui::PopID();
            }
            if (frameToDelete != -1) {
                d.Frames.erase(d.Frames.begin() + frameToDelete);
                if (d.FPS > 0.0f) d.Duration = (float)d.Frames.size() / d.FPS;
                d.FrameCount = (uint32_t)d.Frames.size();
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}

inline void DrawLipSyncProperties(LoadedBank* bank, std::function<void()> onSave, std::function<void()> onRecompile) {
    ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "LipSync Bank Controls");
    ImGui::Separator();

    if (!g_LipSyncParser.IsParsed && !g_LipSyncParser.Data.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse LipSync data.");
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button("Save Entry (Memory)", ImVec2(160, 30))) {
        if (onSave) onSave();
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("(Recompile via Sidebar)");
    ImGui::Separator();

    auto& d = g_LipSyncParser.Data;
    if (ImGui::CollapsingHeader("Header Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("HeaderTable", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Duration");
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(ImVec4(0, 1, 1, 1), "%.3f sec", d.Duration);
            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Frames");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%u", d.FrameCount);
            ImGui::EndTable();
        }
    }
    if (ImGui::CollapsingHeader("Animation Frames", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        RenderLipSyncFrames(d);
        ImGui::PopFont();
    }
}