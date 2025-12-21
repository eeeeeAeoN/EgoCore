#pragma once
#include "imgui.h"
#include "BankBackend.h" 
#include "TextParser.h"
#include "BinaryParser.h" 
#include <string>
#include <codecvt> 
#include <vector>
#include <sstream>
#include <iomanip>

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
            if (tempParser.IsParsed && !tempParser.IsGroup && !tempParser.IsNarratorList) {
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

inline void DrawTextProperties(const std::vector<BinaryParser>& loadedBinaries) {
    if (!g_TextParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse text entry.");
        return;
    }

    // --- TYPE 1: GROUP ENTRY ---
    if (g_TextParser.IsGroup) {
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Group Entry (%zu Items)", g_TextParser.GroupData.Items.size());
        ImGui::Separator();

        if (ImGui::BeginTable("GroupTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, 300))) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 150);
            ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin((int)g_TextParser.GroupData.Items.size());
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    const auto& item = g_TextParser.GroupData.Items[i];
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", item.ID);
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%s", item.CachedName.c_str());
                    ImGui::TableSetColumnIndex(2); ImGui::TextWrapped("%s", item.CachedContent.c_str());
                }
            }
            ImGui::EndTable();
        }
        return;
    }

    // --- TYPE 2: NARRATOR LIST ---
    if (g_TextParser.IsNarratorList) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 1.0f, 1.0f), "Narrator List Entry (Type 2)");
        ImGui::Text("Parsed Strings: %zu", g_TextParser.NarratorStrings.size());
        ImGui::Separator();

        // 1. Parsed String List
        if (ImGui::CollapsingHeader("Parsed Strings", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginChild("NarratorStr", ImVec2(0, 300), true)) {
                for (const auto& s : g_TextParser.NarratorStrings) {
                    ImGui::Text("- %s", s.c_str());
                }
            }
            ImGui::EndChild();
        }
        // Removed Hex Dump and Raw Size
        return;
    }

    // --- TYPE 0: TEXT ENTRY ---
    auto& e = g_TextParser.TextData;

    ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "ID: %s", e.Identifier.c_str());
    // Removed CRC Display

    ImGui::Separator();
    if (ImGui::BeginTable("TextMeta", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Value");

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Speaker");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", e.Speaker.c_str());

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Sound Bank");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(ImVec4(1, 0.8f, 0.5f, 1), "%s", e.SpeechBank.c_str());

        ImGui::EndTable();
    }

    // --- MODIFIERS / TAGS ---
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Modifiers / Tags (%zu)", e.Tags.size());
    if (ImGui::BeginTable("TagsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(0, 100))) {
        ImGui::TableSetupColumn("Position", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Tag Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (auto& tag : e.Tags) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", tag.Position);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", tag.Name.c_str());
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