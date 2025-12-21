#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include <string>
#include <vector>
#include "FileDialogs.h"

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

        bool isModified = bank->AudioParser->ModifiedCache.count(bank->SelectedEntryIndex);
        if (isModified) {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "[MODIFIED IN MEMORY]");
        }

        if (ImGui::BeginTable("AudioMeta", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Sound ID");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d", entry.SoundID);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Original Size");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d bytes", entry.Length);

            ImGui::EndTable();
        }
    }

    ImGui::Dummy(ImVec2(0, 20));
    ImGui::Separator();

    // --- PLAYBACK CONTROLS ---
    auto& player = bank->AudioParser->Player;
    float currentT = player.GetCurrentTime();
    float totalT = player.GetTotalDuration();
    float progress = player.GetProgress();

    ImGui::PushItemWidth(-1);
    if (ImGui::SliderFloat("##seek", &progress, 0.0f, 1.0f, "")) {
        player.Seek(progress);
    }
    ImGui::PopItemWidth();

    ImGui::Text("%s", FormatTime(currentT).c_str());
    ImGui::SameLine();
    float avail = ImGui::GetContentRegionAvail().x;
    std::string totalStr = FormatTime(totalT);
    float textWidth = ImGui::CalcTextSize(totalStr.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - textWidth);
    ImGui::Text("%s", totalStr.c_str());

    ImGui::Dummy(ImVec2(0, 5));

    // BUTTONS
    float buttonWidth = 100;
    // Center 3 buttons (Play, Export, Import)
    float centerOffset = (ImGui::GetContentRegionAvail().x - (buttonWidth * 3 + 20)) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerOffset);

    // 1. Play
    const char* label = player.IsPlaying() ? "Pause" : "Play";
    if (ImGui::Button(label, ImVec2(buttonWidth, 40))) {
        if (totalT == 0.0f) {
            auto pcm = bank->AudioParser->GetDecodedAudio(bank->SelectedEntryIndex);
            if (!pcm.empty()) {
                player.PlayPCM(pcm, 22050); // Default 22050
            }
        }
        else {
            if (player.IsPlaying()) player.Pause(); else player.Play();
        }
    }

    ImGui::SameLine();

    // 2. Export
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

    ImGui::SameLine();

    // 3. Import (NEW)
    if (ImGui::Button("Import .WAV", ImVec2(buttonWidth, 40))) {
        std::string openPath = OpenFileDialog("WAV File\0*.wav\0All Files\0*.*\0");
        if (!openPath.empty()) {
            bool ok = bank->AudioParser->ImportWav(bank->SelectedEntryIndex, openPath);
            if (ok) {
                player.Reset();
            }
        }
    }
}