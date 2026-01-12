#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "imgui.h"
#include "BankBackend.h"
#include "FileDialogs.h" 
#include <string>
#include <vector>
#include <iostream>
#include <algorithm> 
#include <fstream>
#include <cstring> 

// --- HELPER FOR STRINGS ---
static bool InputString(const char* label, std::string& str, int maxLen = 255) {
    static char buf[1024];
    strncpy_s(buf, str.c_str(), _TRUNCATE);
    if (ImGui::InputText(label, buf, maxLen)) {
        str = buf;
        return true;
    }
    return false;
}

static std::string FormatTime(float seconds) {
    int m = (int)seconds / 60;
    int s = (int)seconds % 60;
    int ms = (int)((seconds - (int)seconds) * 100);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d.%02d", m, s, ms);
    return std::string(buf);
}

// --- LUG (.LUG) PROPERTIES & EDITING ---
static void DrawLugAudioProperties(LoadedBank* bank) {
    if (!bank || !bank->LugParserPtr) return;
    auto& lug = bank->LugParserPtr;

    ImGui::TextDisabled("LUG Audio Bank (.lug)");
    ImGui::SameLine();

    if (ImGui::Button("Save / Recompile Bank", ImVec2(180, 0))) {
        if (lug->Save(lug->FileName)) {
            g_SuccessMessage = "Bank recompiled successfully!";
            g_ShowSuccessPopup = true;
        }
    }

    ImGui::Separator();

    if (bank->SelectedEntryIndex == -1) {
        ImGui::Text("Select a sound to view properties.");
        return;
    }

    if (bank->SelectedEntryIndex >= lug->Entries.size()) return;
    auto& e = lug->Entries[bank->SelectedEntryIndex];

    // --- PLAYER UI (Unified) ---
    float currentT = player.GetCurrentTime();
    float totalT = player.GetTotalDuration();
    float progress = player.GetProgress();

    ImGui::SliderFloat("##seek", &progress, 0.0f, 1.0f, "");
    if (ImGui::IsItemActive()) player.Seek(progress);
    ImGui::SameLine();
    ImGui::Text("%s / %s", FormatTime(currentT).c_str(), FormatTime(totalT).c_str());

    if (ImGui::Button(player.IsPlaying() ? "Pause" : "Play", ImVec2(80, 0))) {
        if (totalT == 0.0f) {
            auto blob = lug->GetAudioBlob(bank->SelectedEntryIndex);
            if (!blob.empty()) player.PlayWav(blob);
        }
        else {
            if (player.IsPlaying()) player.Pause(); else player.Play();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop", ImVec2(80, 0))) {
        player.Stop();
    }

    ImGui::SameLine();
    if (ImGui::Button("Export", ImVec2(80, 0))) {
        auto blob = lug->GetAudioBlob(bank->SelectedEntryIndex);
        if (!blob.empty()) {
            std::string p = SaveFileDialog("WAV File\0*.wav\0");
            if (!p.empty()) {
                if (p.find(".wav") == std::string::npos) p += ".wav";
                std::ofstream out(p, std::ios::binary);
                out.write((char*)blob.data(), blob.size());
                out.close();
            }
        }
    }

    ImGui::Separator();

    // --- EDITING CONTROLS ---
    if (ImGui::Button("Clone Entry")) {
        lug->CloneEntry(bank->SelectedEntryIndex);
        bank->SelectedEntryIndex = (int)lug->Entries.size() - 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Entry")) {
        lug->DeleteEntry(bank->SelectedEntryIndex);
        bank->SelectedEntryIndex = -1;
        return;
    }

    ImGui::Separator();

    // --- PROPERTIES ---
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Sound ID: %d", e.SoundID);
    InputString("Name", e.Name);
    InputString("Source Path", e.FullPath);
    InputString("Group/Context", e.GroupName);

    ImGui::Separator();
    ImGui::Text("Playback Logic:");

    int sType = (int)e.SoundType;
    if (ImGui::InputInt("Sound Type", &sType)) e.SoundType = (uint32_t)sType;

    int prio = (int)e.Priority;
    if (ImGui::InputInt("Priority", &prio)) e.Priority = (uint32_t)prio;

    int vol = (int)e.Volume;
    if (ImGui::InputInt("Volume", &vol)) e.Volume = (uint32_t)vol;

    ImGui::DragFloat("Min Dist", &e.MinDist, 0.5f, 0.0f, 1000.0f);
    ImGui::DragFloat("Max Dist", &e.MaxDist, 0.5f, 0.0f, 1000.0f);

    int prob = (int)e.Probability;
    if (ImGui::InputInt("Probability", &prob)) e.Probability = (uint32_t)prob;

    int inst = (int)e.InstanceLimit;
    if (ImGui::InputInt("Max Instances", &inst)) e.InstanceLimit = (uint32_t)inst;

    ImGui::Separator();
    ImGui::TextDisabled("WAV Info:");
    ImGui::Text("Rate: %d Hz", e.SampleRate);
    ImGui::Text("Channels: %d", e.Channels);
    ImGui::Text("Format Tag: %d", e.FormatTag);
    ImGui::Text("Size: %d bytes", e.Length);
}

// --- STANDARD BANK (.LUT) PLAYER ---
static void DrawAudioProperties(LoadedBank* bank) {
    if (!bank || bank->Type != EBankType::Audio || !bank->AudioParser) return;

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Fable Audio Bank (.LUT)");
    ImGui::Separator();

    if (bank->SelectedEntryIndex >= 0 && bank->SelectedEntryIndex < bank->AudioParser->Entries.size()) {
        const auto& entry = bank->AudioParser->Entries[bank->SelectedEntryIndex];
        ImGui::Text("ID: %d", entry.SoundID);
        ImGui::Text("Size: %d bytes", entry.Length);
        ImGui::Text("Offset: %d", entry.Offset);

        auto& player = bank->AudioParser->Player;
        float currentT = player.GetCurrentTime();
        float totalT = player.GetTotalDuration();
        float progress = player.GetProgress();

        ImGui::SliderFloat("##seek", &progress, 0.0f, 1.0f, "");
        if (ImGui::IsItemActive()) player.Seek(progress);
        ImGui::SameLine();
        ImGui::Text("%s / %s", FormatTime(currentT).c_str(), FormatTime(totalT).c_str());

        if (ImGui::Button(player.IsPlaying() ? "Pause" : "Play", ImVec2(80, 0))) {
            if (totalT == 0.0f) {
                auto pcm = bank->AudioParser->GetDecodedAudio(bank->SelectedEntryIndex);
                if (!pcm.empty()) player.PlayPCM(pcm, 22050);
            }
            else {
                if (player.IsPlaying()) player.Pause(); else player.Play();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Export", ImVec2(80, 0))) {
            auto pcm = bank->AudioParser->GetDecodedAudio(bank->SelectedEntryIndex);
            if (!pcm.empty()) {
                std::string p = SaveFileDialog("WAV File\0*.wav\0");
                if (!p.empty()) {
                    if (p.find(".wav") == std::string::npos) p += ".wav";
                    WriteWavFile(p, pcm, 22050, 1);
                }
            }
        }
    }
    else {
        ImGui::TextDisabled("Select an audio entry to view details.");
    }
}