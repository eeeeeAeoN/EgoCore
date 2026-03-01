#pragma once
#include "imgui.h"
#include "AnimParser.h"
#include <string>
#include <vector>

inline void DrawAnimProperties(std::string& entryName, uint32_t entryID, AnimParser& parser, AnimUIContext& ctx, const std::vector<uint8_t>& rawData) {
    if (!parser.Data.IsParsed) {
        ImGui::Text("No animation loaded or failed to parse.");
        return;
    }

    auto& anim = parser.Data;

    // --- RENAME (Header) ---
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Name:");
    ImGui::SameLine();
    static char nameBuf[256];
    static uint32_t lastID = 0xFFFFFFFF;
    if (lastID != entryID) {
        strncpy_s(nameBuf, entryName.c_str(), sizeof(nameBuf) - 1);
        lastID = entryID;
    }
    ImGui::SetNextItemWidth(400);
    if (ImGui::InputText("##animNameEdit", nameBuf, 256)) entryName = nameBuf;

    ImGui::Separator();

    if (ImGui::Button("INSPECT ANIM HEX")) {
        ctx.ShowHexWindow = true;
        ctx.HexBuffer = rawData;
    }

    if (ctx.ShowHexWindow) {
        if (ImGui::Begin("Anim Hex Inspector", &ctx.ShowHexWindow)) {
            for (size_t i = 0; i < ctx.HexBuffer.size(); i++) {
                ImGui::Text("%02X ", ctx.HexBuffer[i]);
                if ((i + 1) % 8 != 0) ImGui::SameLine();
            }
        }
        ImGui::End();
    }

    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "--- HEADER METADATA ---");
    ImGui::DragFloat("Duration", &anim.Duration, 0.01f);
    ImGui::DragFloat("Non-Looping", &anim.NonLoopingDuration, 0.01f);
    ImGui::DragFloat("Rotation", &anim.Rotation, 0.01f);

    ImGui::Dummy(ImVec2(0, 5));

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "--- PAYLOAD DATA ---");
    ImGui::Text("Target Skeleton: %s", anim.ObjectName.empty() ? "None" : anim.ObjectName.c_str());
    ImGui::Text("Cyclic: %s", anim.IsCyclic ? "Yes" : "No");

    // Partial Animation Info
    if (!anim.BoneMaskBits.empty()) {
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Partial Animation Bone Mask (AMSK) Detected.");
        if (ImGui::TreeNode("View Bitmask")) {
            for (size_t i = 0; i < anim.BoneMaskBits.size(); i++) {
                ImGui::Text("Word %zu: %08X", i, anim.BoneMaskBits[i]);
            }
            ImGui::TreePop();
        }
    }

    ImGui::DragFloat3("Movement Vector", &anim.MovementVector.x, 0.05f);

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "--- TRACKS & KEYFRAMES ---");

    if (ImGui::BeginChild("TracksList", ImVec2(0, 0), true)) {
        for (size_t i = 0; i < anim.Tracks.size(); i++) {
            const auto& track = anim.Tracks[i];
            bool isEnabled = true;
            // For partial animations, check the mask
            if (!anim.BoneMaskBits.empty()) {
                uint32_t word = i / 32;
                uint32_t bit = i % 32;
                if (word < anim.BoneMaskBits.size()) {
                    isEnabled = (anim.BoneMaskBits[word] & (1 << bit)) != 0;
                }
            }

            ImVec4 headerCol = isEnabled ? ImVec4(1, 1, 1, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, headerCol);
            bool open = ImGui::CollapsingHeader((track.BoneName + (isEnabled ? "" : " (DISABLED)") + "##" + std::to_string(i)).c_str());
            ImGui::PopStyleColor();

            if (open) {
                ImGui::Indent();
                ImGui::Text("FPS: %.2f | Frames: %u", track.SamplesPerSecond, track.FrameCount);
                if (ImGui::BeginTable(("Table" + std::to_string(i)).c_str(), 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 200))) {
                    ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Pos");
                    ImGui::TableSetupColumn("Rot");
                    ImGui::TableHeadersRow();
                    ImGuiListClipper clipper;
                    clipper.Begin(track.FrameCount);
                    while (clipper.Step()) {
                        for (int f = clipper.DisplayStart; f < clipper.DisplayEnd; f++) {
                            Vec3 p; Vec4 r; track.EvaluateFrame(f, p, r);
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", f);
                            ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f,%.2f,%.2f", p.x, p.y, p.z);
                            ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f,%.2f,%.2f,%.2f", r.x, r.y, r.z, r.w);
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::Unindent();
            }
        }
    }
    ImGui::EndChild();
}