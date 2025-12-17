#pragma once
#include "imgui.h"
#include "AnimParser.h"
#include <string>
#include <vector>
#include <functional>

struct AnimUIContext {
    bool ShowHexWindow = false;
    std::vector<uint8_t> HexBuffer;
};

inline void DrawAnimProperties(C3DAnimationInfo& anim, bool isLoaded, AnimUIContext& ctx, std::function<void()> onSave) {
    if (!isLoaded) {
        ImGui::Text("No animation loaded.");
        return;
    }

    if (ImGui::Button("INSPECT ANIM HEX")) {
        ctx.ShowHexWindow = true;
        ctx.HexBuffer = anim.Serialize();
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
    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "--- ANIMATION HEADER DATA ---");

    ImGui::DragFloat("Duration", &anim.Duration, 0.01f);
    ImGui::DragFloat("Non-Looping", &anim.NonLoopingDuration, 0.01f);
    ImGui::DragFloat3("Root Move", &anim.MovementVector.x, 0.1f);
    ImGui::DragFloat("Rotation", &anim.Rotation, 0.01f);

    ImGui::Separator();

    if (ImGui::Button("SAVE TO .BIG", ImVec2(-1, 40))) {
        if (onSave) onSave();
    }
}