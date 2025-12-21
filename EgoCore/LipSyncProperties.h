#pragma once
#include "imgui.h"
#include "LipSyncParser.h"
#include <string>
#include <vector>
#include <algorithm>

static CLipSyncParser g_LipSyncParser;

// Helper to look up symbol from internal dictionary
inline std::string GetSymbol(uint8_t id, const std::vector<CLipSyncPhonemeRef>& dict) {
    for (const auto& p : dict) {
        if (p.ID == id) return p.Symbol;
    }
    return "??";
}

inline void DrawLipSyncProperties() {
    if (!g_LipSyncParser.IsParsed && !g_LipSyncParser.Data.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse LipSync data.");
        return;
    }

    const auto& d = g_LipSyncParser.Data;

    // --- HEADER INFO ---
    if (ImGui::CollapsingHeader("Header Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("HeaderTable", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Duration");
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(ImVec4(0, 1, 1, 1), "%.3f sec", d.Duration);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Frames");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%u", d.FrameCount);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("FPS");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f", d.FPS);
            ImGui::EndTable();
        }
    }

    // --- ANIMATION FRAMES (Clean View) ---
    if (ImGui::CollapsingHeader("Animation Frames", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginChild("FrameStream", ImVec2(0, 300), true)) {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

            // Simple 2-Column Layout
            if (ImGui::BeginTable("FrameTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed, 60);
                ImGui::TableSetupColumn("Active Phonemes", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < d.Frames.size(); i++) {
                    const auto& frame = d.Frames[i];

                    ImGui::TableNextRow();

                    // Column 1: Frame Number
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("%03zu", i);

                    // Column 2: The Morph Data
                    ImGui::TableSetColumnIndex(1);
                    if (frame.Keys.empty()) {
                        ImGui::TextDisabled("-");
                    }
                    else {
                        for (const auto& key : frame.Keys) {
                            std::string symbol = GetSymbol(key.ID, d.Dictionary);
                            float intensity = key.WeightFloat;

                            // Visual: [AH : 100%]
                            // Green intensity scales with weight
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 0.4f + (intensity * 0.6f)),
                                "[%s : %3.0f%%]", symbol.c_str(), intensity * 100.0f);

                            ImGui::SameLine();
                        }
                    }
                }
                ImGui::EndTable();
            }
            ImGui::PopFont();
        }
        ImGui::EndChild();
    }
}