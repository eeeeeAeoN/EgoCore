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

    // ====================================================================
    // EGOCORE ENGINE LOGIC EDITOR
    // ====================================================================
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "--- FABLE ENGINE LOGIC ---");

    // 1. Cyclic Flag & Durations
    ImGui::Checkbox("Is Cyclic (Looping Animation)", &anim.IsCyclic);
    ImGui::DragFloat("Duration (Seconds)", &anim.Duration, 0.01f);
    ImGui::DragFloat("Non-Looping Duration", &anim.NonLoopingDuration, 0.01f);

    // 2. Movement Vector (MVEC)
    ImGui::Text("Root Movement Vector (MVEC):");
    ImGui::DragFloat3("##mvec_edit", &anim.MovementVector.x, 0.01f);
    ImGui::SameLine();
    if (ImGui::Button("Auto-Calc from Root")) {
        if (!anim.Tracks.empty() && !anim.Tracks[0].PositionTrack.empty()) {
            Vec3 startPos = anim.Tracks[0].PositionTrack.front();
            Vec3 endPos = anim.Tracks[0].PositionTrack.back();
            anim.MovementVector.x = endPos.x - startPos.x;
            anim.MovementVector.y = endPos.y - startPos.y;
            anim.MovementVector.z = endPos.z - startPos.z;
        }
    }

    ImGui::Dummy(ImVec2(0, 5));

    // 3. Time Events (TMEV) Editor
    if (ImGui::CollapsingHeader("Time Events (TMEV)", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Add New Event")) {
            anim.TimeEvents.push_back({ "NEW_EVENT", 0.0f });
        }

        ImGui::BeginChild("TMEV_Editor", ImVec2(0, 150), true);
        for (size_t i = 0; i < anim.TimeEvents.size(); i++) {
            ImGui::PushID((int)i);

            char tmevBuf[256];
            strncpy_s(tmevBuf, anim.TimeEvents[i].Name.c_str(), 255);
            ImGui::SetNextItemWidth(250);
            if (ImGui::InputText("##evname", tmevBuf, 256)) anim.TimeEvents[i].Name = tmevBuf;

            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::DragFloat("##evtime", &anim.TimeEvents[i].Time, 0.01f, 0.0f, anim.Duration, "%.2fs");

            ImGui::SameLine();
            if (ImGui::Button("X")) {
                anim.TimeEvents.erase(anim.TimeEvents.begin() + i);
                i--;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
    }

    // 4. Bone Mask (AMSK) Editor
    if (ImGui::CollapsingHeader("Partial Animation Bone Mask (AMSK)")) {
        if (anim.BoneMaskBits.empty()) {
            ImGui::TextDisabled("This is a Full Body animation (No Mask).");
            if (ImGui::Button("Convert to Partial Animation")) {
                uint32_t wordCount = ((uint32_t)anim.Tracks.size() + 31) / 32;
                anim.BoneMaskBits.resize(wordCount, 0xFFFFFFFF);
            }
        }
        else {
            if (ImGui::Button("Clear Mask")) anim.BoneMaskBits.clear();
            ImGui::SameLine();
            if (ImGui::Button("Disable All")) for (auto& word : anim.BoneMaskBits) word = 0;
            ImGui::SameLine();
            if (ImGui::Button("Enable All")) for (auto& word : anim.BoneMaskBits) word = 0xFFFFFFFF;

            ImGui::BeginChild("MaskEditor", ImVec2(0, 150), true);
            for (size_t i = 0; i < anim.Tracks.size(); i++) {
                uint32_t wordIdx = (uint32_t)(i / 32);
                uint32_t bitIdx = (uint32_t)(i % 32);
                if (wordIdx >= anim.BoneMaskBits.size()) anim.BoneMaskBits.resize(wordIdx + 1, 0);
                bool isEnabled = (anim.BoneMaskBits[wordIdx] & (1 << bitIdx)) != 0;

                if (ImGui::Checkbox((anim.Tracks[i].BoneName + "##mask" + std::to_string(i)).c_str(), &isEnabled)) {
                    if (isEnabled) anim.BoneMaskBits[wordIdx] |= (1 << bitIdx);
                    else anim.BoneMaskBits[wordIdx] &= ~(1 << bitIdx);
                }
            }
            ImGui::EndChild();
        }
    }

    ImGui::Separator();
    // ====================================================================

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "--- TRACKS & KEYFRAMES ---");

    if (ImGui::BeginChild("TracksList", ImVec2(0, 0), true)) {
        for (size_t i = 0; i < anim.Tracks.size(); i++) {
            const auto& track = anim.Tracks[i];

            bool isEnabled = true;
            if (!anim.BoneMaskBits.empty()) {
                uint32_t word = (uint32_t)(i / 32);
                uint32_t bit = (uint32_t)(i % 32);
                if (word < anim.BoneMaskBits.size()) isEnabled = (anim.BoneMaskBits[word] & (1 << bit)) != 0;
            }

            ImVec4 headerCol = isEnabled ? ImVec4(1, 1, 1, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, headerCol);
            bool open = ImGui::CollapsingHeader((track.BoneName + (isEnabled ? "" : " (MASKED OUT)") + "##" + std::to_string(i)).c_str());
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