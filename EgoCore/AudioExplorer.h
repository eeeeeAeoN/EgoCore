#pragma once
#include "imgui.h"
#include "FileDialogs.h"
#include "BankBackend.h"
#include <string>
#include <vector>

// Formats time as "MM:SS.ms"
static std::string FormatTime(float seconds) {
    int m = (int)seconds / 60;
    int s = (int)seconds % 60;
    int ms = (int)((seconds - (int)seconds) * 100);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d.%02d", m, s, ms);
    return std::string(buf);
}

static void DrawAudioProperties(LoadedBank* bank) {
    if (!bank || bank->Type != EBankType::Audio || !bank->AudioParser) return;

    // --- HEADER INFO ---
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Fable Audio Bank (.LUT)");
    ImGui::Separator();
    
    // We can pull more info from the selected entry
    if (bank->SelectedEntryIndex >= 0 && bank->SelectedEntryIndex < bank->AudioParser->Entries.size()) {
        const auto& entry = bank->AudioParser->Entries[bank->SelectedEntryIndex];
        
        if (ImGui::BeginTable("AudioMeta", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Sound ID");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d", entry.SoundID);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Offset");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d (0x%X)", entry.Offset, entry.Offset);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Compressed Size");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d bytes", entry.Length);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Format");
            ImGui::TableSetColumnIndex(1); ImGui::Text("Xbox ADPCM (4-bit)");

            ImGui::EndTable();
        }
    }

    ImGui::Dummy(ImVec2(0, 20));
    ImGui::Separator();
    ImGui::Text("Playback Controls");

    // --- PLAYBACK CONTROLS ---
    auto& player = bank->AudioParser->Player;
    float currentT = player.GetCurrentTime();
    float totalT = player.GetTotalDuration();
    float progress = player.GetProgress();

    // 1. Progress Bar / Seeker
    // We use a slider so the user can drag it
    ImGui::PushItemWidth(-1);
    if (ImGui::SliderFloat("##seek", &progress, 0.0f, 1.0f, "")) {
        player.Seek(progress);
    }
    ImGui::PopItemWidth();

    // 2. Time Labels
    ImGui::Text("%s", FormatTime(currentT).c_str());
    ImGui::SameLine();
    
    // Right-align the total time
    float avail = ImGui::GetContentRegionAvail().x;
    std::string totalStr = FormatTime(totalT);
    float textWidth = ImGui::CalcTextSize(totalStr.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - textWidth);
    ImGui::Text("%s", totalStr.c_str());

    ImGui::Dummy(ImVec2(0, 5));

    // 3. Buttons (Play/Pause, Stop)
    // Center the buttons
    float buttonWidth = 100;
    float centerOffset = (ImGui::GetContentRegionAvail().x - (buttonWidth * 2 + 10)) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerOffset);

    // Play/Pause Button
    const char* label = player.IsPlaying() ? "Pause" : "Play";
    if (ImGui::Button(label, ImVec2(buttonWidth, 40))) {
        // If nothing is loaded yet, load it first
        if (totalT == 0.0f) {
            auto pcm = bank->AudioParser->GetDecodedAudio(bank->SelectedEntryIndex);
            if (!pcm.empty()) {
                player.LoadPCM(pcm, 22050);
                player.Play();
            }
        } else {
            player.Toggle();
        }
    }

    ImGui::SameLine();

    // Export Button (Moved here for convenience)
    if (ImGui::Button("Export .WAV", ImVec2(buttonWidth, 40))) {
        auto pcm = bank->AudioParser->GetDecodedAudio(bank->SelectedEntryIndex);
        if (!pcm.empty()) {
            std::string savePath = SaveFileDialog("WAV File\0*.wav\0");
            if (!savePath.empty()) {
                if (savePath.find(".wav") == std::string::npos) savePath += ".wav";
                WriteWavFile(savePath, pcm, 22050, 1);
            }
        }
    }
}