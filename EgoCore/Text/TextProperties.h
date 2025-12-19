#pragma once
#include "imgui.h"
#include "BankBackend.h" 
#include "TextParser.h"
#include <string>
#include <codecvt> 

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
            else if (tempParser.IsGroup) {
                return "[Group Container]";
            }
            return "[Error Parsing]";
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
        if (found) {
            item.CachedContent = FetchTextContent(bank, item.ID);
        }
        else {
            item.CachedContent = "-";
        }
    }
}

inline void DrawTextProperties() {
    if (!g_TextParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse text entry.");
        return;
    }

    if (g_TextParser.IsGroup) {
        const auto& group = g_TextParser.GroupData;
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "GROUP: Contains %zu entries", group.Items.size());
        ImGui::Separator();

        if (ImGui::BeginTable("GroupTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0, 400))) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Name Key", ImGuiTableColumnFlags_WidthFixed, 200);
            ImGui::TableSetupColumn("Content Preview", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (const auto& item : group.Items) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%u", item.ID);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", item.CachedName.c_str());
                ImGui::TableSetColumnIndex(2); ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "\"%s\"", item.CachedContent.c_str());
            }
            ImGui::EndTable();
        }
        return;
    }

    const auto& e = g_TextParser.TextData;

    ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "ID: %s", e.Identifier.c_str());
    ImGui::Separator();

    if (ImGui::BeginTable("TextMeta", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Value");

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Speaker");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", e.Speaker.c_str());

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Sound File");
        ImGui::TableSetColumnIndex(1); ImGui::TextColored(ImVec4(1, 0.8f, 0.5f, 1), "%s", e.SpeechBank.c_str());
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

    if (!e.Tags.empty()) {
        ImGui::Separator(); ImGui::Text("Tags:");
        if (ImGui::BeginTable("TextTags", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Position/Value"); ImGui::TableSetupColumn("Name"); ImGui::TableHeadersRow();
            for (const auto& tag : e.Tags) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%d", tag.Position);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", tag.Name.c_str());
            }
            ImGui::EndTable();
        }
    }
}