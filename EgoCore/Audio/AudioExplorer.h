#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "FileDialogs.h" // Required for SaveFileDialog
#include <string>
#include <vector>
#include <iostream>
#include <algorithm> // For std::max

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

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Fable Audio Bank (.LUT)");
    ImGui::Separator();

    if (bank->SelectedEntryIndex >= 0 && bank->SelectedEntryIndex < bank->AudioParser->Entries.size()) {
        const auto& entry = bank->AudioParser->Entries[bank->SelectedEntryIndex];

        bool isModified = bank->AudioParser->ModifiedCache.count(entry.SoundID);
        if (isModified) {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "[MODIFIED] ID: %d", entry.SoundID);
        }
        else {
            ImGui::Text("ID: %d", entry.SoundID);
        }

        ImGui::Text("Size: %d bytes", entry.Length);
        ImGui::Text("Offset: %d", entry.Offset);

        // Playback Controls
        auto& player = bank->AudioParser->Player;
        float currentT = player.GetCurrentTime();
        float totalT = player.GetTotalDuration();
        float progress = player.GetProgress();

        ImGui::SliderFloat("##seek", &progress, 0.0f, 1.0f, "");
        if (ImGui::IsItemActive()) player.Seek(progress);

        ImGui::SameLine();
        ImGui::Text("%s / %s", FormatTime(currentT).c_str(), FormatTime(totalT).c_str());

        // Buttons
        // Adjusted width to fit just Play, Exp, Imp
        float buttonWidth = 80;

        // Play/Pause
        const char* label = player.IsPlaying() ? "Pause" : "Play";
        if (ImGui::Button(label, ImVec2(buttonWidth, 0))) {
            if (totalT == 0.0f) {
                auto pcm = bank->AudioParser->GetDecodedAudio(bank->SelectedEntryIndex);
                if (!pcm.empty()) player.PlayPCM(pcm, 22050);
            }
            else {
                if (player.IsPlaying()) player.Pause(); else player.Play();
            }
        }

        ImGui::SameLine();

        // Export
        if (ImGui::Button("Export", ImVec2(buttonWidth, 0))) {
            auto pcm = bank->AudioParser->GetDecodedAudio(bank->SelectedEntryIndex);
            if (!pcm.empty()) {
                std::string savePath = SaveFileDialog("WAV File\0*.wav\0");
                if (!savePath.empty()) {
                    if (savePath.find(".wav") == std::string::npos) savePath += ".wav";
                    WriteWavFile(savePath, pcm, 22050, 1);
                }
            }
        }

        ImGui::SameLine();

        // Import
        if (ImGui::Button("Import", ImVec2(buttonWidth, 0))) {
            std::string openPath = OpenFileDialog("WAV File\0*.wav\0All Files\0*.*\0");
            if (!openPath.empty()) {
                if (bank->AudioParser->ImportWav(bank->SelectedEntryIndex, openPath)) {
                    player.Reset();
                }
            }
        }
    }
    else {
        ImGui::TextDisabled("Select an audio entry to view details.");
    }
}