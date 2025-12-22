#pragma once
#include "imgui.h"
#include "LipSyncParser.h"
#include "BankBackend.h"
#include "ConfigBackend.h"
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <fstream>
#include <memory>
#include <filesystem>

static CLipSyncParser g_LipSyncParser;

// =========================================================
// LIPSYNC BACKEND STATE
// =========================================================
struct AddedEntryData {
    std::vector<uint8_t> Raw;
    std::vector<uint8_t> Info;
};

struct LipSyncState {
    std::string FilePath;
    std::unique_ptr<std::fstream> Stream;
    std::vector<InternalBankInfo> SubBanks;
    std::map<std::string, int> SubBankMap;

    int CachedSubBankIndex = -1;
    std::vector<BankEntry> CachedEntries;

    // Pending Changes [SubBankIdx] -> ...
    std::map<int, std::set<uint32_t>> PendingDeletes;

    // CHANGED: Stores Raw + Info pair
    std::map<int, std::map<uint32_t, AddedEntryData>> PendingAdds;
};

static LipSyncState g_LipSyncState;

// =========================================================
// LIPSYNC BACKEND FUNCTIONS
// =========================================================

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

inline bool EnsureLipSyncLoaded() {
    if (g_LipSyncState.Stream && g_LipSyncState.Stream->is_open()) return true;
    std::string path = g_AppConfig.GameRootPath + "\\Data\\Lang\\English\\dialogue.big";
    if (!std::filesystem::exists(path)) return false;

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

        if (magicE != 42) continue;

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

// --- NEW: Generate binary data AND INFO data ---
inline void AddLipSyncEntry(const std::string& speechBank, uint32_t newID, float duration) {
    if (!EnsureLipSyncLoaded()) return;
    std::string subName = GetSubBankNameForSpeech(speechBank);
    if (subName.empty()) return;
    if (g_LipSyncState.SubBankMap.find(subName) == g_LipSyncState.SubBankMap.end()) return;
    int sbIdx = g_LipSyncState.SubBankMap[subName];

    // 1. RAW DATA
    std::vector<uint8_t> raw;
    raw.push_back(0); // Dictionary Count

    float fpsVal = 43.0f;
    uint32_t frameCount = (uint32_t)std::ceil(duration * fpsVal);
    if (frameCount == 0) frameCount = 1;

    // Write FPS as INT (Parser casts to float)
    uint32_t fpsInt = (uint32_t)fpsVal;
    uint8_t* pFps = (uint8_t*)&fpsInt;
    raw.insert(raw.end(), pFps, pFps + 4);

    uint8_t* pFrames = (uint8_t*)&frameCount;
    raw.insert(raw.end(), pFrames, pFrames + 4);

    for (uint32_t i = 0; i < frameCount; i++) raw.push_back(0);

    // 2. INFO DATA (Metadata)
    // Writing Duration as float (4 bytes)
    std::vector<uint8_t> info;
    uint8_t* pDur = (uint8_t*)&duration;
    info.insert(info.end(), pDur, pDur + 4);

    g_LipSyncState.PendingAdds[sbIdx][newID] = { raw, info };
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

    // Check Pending Adds
    if (g_LipSyncState.PendingAdds[sbIdx].count((uint32_t)soundID)) {
        const auto& entry = g_LipSyncState.PendingAdds[sbIdx][(uint32_t)soundID];

        auto result = std::make_unique<CLipSyncData>();
        CLipSyncParser parser;
        parser.Parse(entry.Raw, entry.Info);
        *result = parser.Data;

        result->IsParsed = true;

        // Force calculation if parser logic didn't pick up Info correctly
        if (result->Duration == 0.0f) {
            // Peek duration from info we just created
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

// =========================================================
// UI HELPERS
// =========================================================

inline std::string GetSymbol(uint8_t id, const std::vector<CLipSyncPhonemeRef>& dict) {
    for (const auto& p : dict) {
        if (p.ID == id) return p.Symbol;
    }
    return "??";
}

inline void RenderLipSyncFrames(const CLipSyncData& d) {
    if (ImGui::BeginChild("FrameStream", ImVec2(0, 300), true)) {
        if (ImGui::BeginTable("FrameTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed, 60);
            ImGui::TableSetupColumn("Active Phonemes", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < d.Frames.size(); i++) {
                const auto& frame = d.Frames[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled("%03zu", i);
                ImGui::TableSetColumnIndex(1);
                if (frame.Keys.empty()) {
                    ImGui::TextDisabled("-");
                }
                else {
                    for (const auto& key : frame.Keys) {
                        std::string symbol = GetSymbol(key.ID, d.Dictionary);
                        float intensity = key.WeightFloat;
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 0.4f + (intensity * 0.6f)),
                            "[%s : %3.0f%%]", symbol.c_str(), intensity * 100.0f);
                        ImGui::SameLine();
                    }
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}

inline void DrawLipSyncProperties() {
    if (!g_LipSyncParser.IsParsed && !g_LipSyncParser.Data.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse LipSync data.");
        return;
    }
    const auto& d = g_LipSyncParser.Data;
    if (ImGui::CollapsingHeader("Header Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("HeaderTable", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Duration"); ImGui::TableSetColumnIndex(1); ImGui::TextColored(ImVec4(0, 1, 1, 1), "%.3f sec", d.Duration);
            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Frames"); ImGui::TableSetColumnIndex(1); ImGui::Text("%u", d.FrameCount);
            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("FPS"); ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f", d.FPS);
            ImGui::EndTable();
        }
    }
    if (ImGui::CollapsingHeader("Animation Frames", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        RenderLipSyncFrames(d);
        ImGui::PopFont();
    }
}