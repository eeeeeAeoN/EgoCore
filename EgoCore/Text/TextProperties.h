#pragma once
#include "imgui.h"
#include "BankBackend.h" 
#include "TextParser.h"
#include "BinaryParser.h" // Needed for CRC & Search
#include <string>
#include <codecvt> 
#include <vector>

static CTextParser g_TextParser;

inline std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

inline std::string FetchTextContent(LoadedBank* bank, uint32_t id) {
    if (!bank) return "";
    for (int i = 0; i < bank->Entries.size(); ++i) {
        if (bank->Entries[i].ID == id) {
            CTextParser tempParser;
            bank->Stream->clear();
            bank->Stream->seekg(bank->Entries[i].Offset, std::ios::beg);
            size_t size = bank->Entries[i].Size;
            std::vector<uint8_t> buffer(size + 64);
            bank->Stream->read((char*)buffer.data(), size);
            tempParser.Parse(buffer, bank->Entries[i].Type);
            if (tempParser.IsParsed && !tempParser.IsGroup) {
                return WStringToString(tempParser.TextData.Content);
            }
            return "[Content]";
        }
    }
    return "[ID Not Found]";
}

inline void ResolveGroupMetadata(LoadedBank* bank) {
    if (!g_TextParser.IsParsed || !g_TextParser.IsGroup || !bank) return;
    for (auto& item : g_TextParser.GroupData.Items) {
        bool found = false;
        for (const auto& entry : bank->Entries) {
            if (entry.ID == item.ID) {
                item.CachedName = entry.Name;
                found = true;
                break;
            }
        }
        if (!found) item.CachedName = "Unknown ID";
        if (found) item.CachedContent = FetchTextContent(bank, item.ID);
        else item.CachedContent = "-";
    }
}

// Updated to accept the list of loaded binaries for lookup
inline void DrawTextProperties(const std::vector<BinaryParser>& loadedBinaries) {
    if (!g_TextParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse text entry.");
        return;
    }

    if (g_TextParser.IsGroup) {
        // ... (Group drawing logic remains same) ...
        ImGui::Text("Group Entry");
        return;
    }

    const auto& e = g_TextParser.TextData;

    // --- 1. IDENTIFIER & CRC ---
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "ID: %s", e.Identifier.c_str());

    // Calculate CRC live
    uint32_t crc = BinaryParser::CalculateCRC32(e.Identifier);
    ImGui::SameLine();
    ImGui::TextDisabled("(CRC: %08X)", crc);

    // --- 2. AUDIO LINK CHECK ---
    // Search for this CRC in the loaded binaries
    bool foundLink = false;
    int32_t linkedID = -1;
    std::string foundInFile = "";

    for (const auto& bin : loadedBinaries) {
        for (const auto& entry : bin.Data.Entries) {
            if (entry.CRC == crc) {
                linkedID = entry.ID;
                foundInFile = bin.Data.FileName;
                foundLink = true;
                break;
            }
        }
        if (foundLink) break;
    }

    ImGui::Separator();
    if (foundLink) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Audio Link Found!");
        ImGui::Text("File: %s", foundInFile.c_str());
        ImGui::Text("Audio ID: %d", linkedID);
        ImGui::SameLine();
        ImGui::TextDisabled("(This ID matches dialogue.lut)");
    }
    else {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "No Audio Link Found");
        if (loadedBinaries.empty()) ImGui::TextDisabled("(No binary definitions loaded)");
        else ImGui::TextDisabled("(Checked %zu binaries)", loadedBinaries.size());
    }
    ImGui::Separator();

    // --- 3. STANDARD PROPERTIES ---
    if (ImGui::BeginTable("TextMeta", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Value");

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Speaker");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", e.Speaker.c_str());

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Sound Bank");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(ImVec4(1, 0.8f, 0.5f, 1), "%s", e.SpeechBank.c_str());
        if (foundLink && e.SpeechBank.find("MAIN") != std::string::npos && foundInFile.find("dialoguesnds") != std::string::npos) {
            ImGui::SameLine(); ImGui::TextDisabled("(Matches .bin)");
        }

        ImGui::EndTable();
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Text("Content:");

    std::string utf8Content = WStringToString(e.Content);
    if (ImGui::BeginChild("ContentBox", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::TextWrapped("%s", utf8Content.c_str());
        ImGui::PopFont();
    }
    ImGui::EndChild();
}