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
#include "MetParser.h"

static bool InputString(const char* label, std::string & str, int maxLen = 255) {
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

static void DrawLugAudioProperties(LoadedBank* bank) {
    if (!bank || !bank->LugParserPtr) return;
    auto& lug = bank->LugParserPtr;

    ImGui::TextDisabled("LUG Script Bank (.lug)");
    ImGui::Separator();

    if (bank->SelectedEntryIndex == -1 || bank->SelectedEntryIndex >= lug->Entries.size()) {
        ImGui::Text("Select a sound to view properties.");
        return;
    }

    auto& e = lug->Entries[bank->SelectedEntryIndex];

    float currentT = player.GetCurrentTime();
    float totalT = player.GetTotalDuration();
    float progress = player.GetProgress();

    ImGui::SliderFloat("##seek", &progress, 0.0f, 1.0f, "");
    if (ImGui::IsItemActive()) player.Seek(progress);
    ImGui::SameLine();
    ImGui::Text("%s / %s", FormatTime(currentT).c_str(), FormatTime(totalT).c_str());

    if (ImGui::Button(player.IsPlaying() ? "Pause" : "Play", ImVec2(50, 0))) {
        if (totalT == 0.0f) {
            auto blob = lug->GetAudioBlob(bank->SelectedEntryIndex);
            if (!blob.empty()) player.PlayWav(blob);
        }
        else {
            if (player.IsPlaying()) player.Pause(); else player.Play();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop", ImVec2(50, 0))) player.Stop();

    auto SyncUIList = [&]() {
        bank->Entries.clear();
        bank->FilteredIndices.clear();
        for (size_t i = 0; i < lug->Entries.size(); i++) {
            BankEntry be; be.ID = lug->Entries[i].SoundID; be.Name = lug->Entries[i].Name;
            be.FriendlyName = be.Name; be.Size = lug->Entries[i].Length; be.Offset = lug->Entries[i].Offset;
            be.Dependencies.push_back(lug->Entries[i].FullPath);
            bank->Entries.push_back(be); bank->FilteredIndices.push_back((int)i);
        }
        UpdateFilter(*bank);
        };

    ImGui::SameLine();
    if (ImGui::Button("Export", ImVec2(60, 0))) {
        auto blob = lug->GetAudioBlob(bank->SelectedEntryIndex);
        if (!blob.empty()) {
            std::string p = SaveFileDialog("WAV File\0*.wav\0");
            if (!p.empty()) {
                if (p.find(".wav") == std::string::npos) p += ".wav";
                std::ofstream out(p, std::ios::binary); out.write((char*)blob.data(), blob.size()); out.close();
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import", ImVec2(60, 0))) {
        std::string p = OpenFileDialog("WAV File\0*.wav\0");
        if (!p.empty()) {
            if (lug->ImportWav(bank->SelectedEntryIndex, p)) {
                player.Reset();
                SyncUIList();
                g_SuccessMessage = "WAV file replaced successfully (Memory).\nRecompile to save to disk.";
                g_ShowSuccessPopup = true;
            }
        }
    }

    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Sound ID: %d", e.SoundID);
    if (InputString("Name", e.Name)) lug->IsDirty = true;
    if (InputString("Source Path", e.FullPath)) lug->IsDirty = true;
    if (InputString("Group/Context", e.GroupName)) lug->IsDirty = true;

    ImGui::Separator();
    ImGui::Text("Playback Logic:");

    int prio = (int)e.Priority;
    if (ImGui::InputInt("Priority", &prio)) { e.Priority = (uint32_t)prio; lug->IsDirty = true; }

    int loops = (int)e.LoopCount;
    if (ImGui::InputInt("Loop Count", &loops)) { e.LoopCount = (uint32_t)loops; lug->IsDirty = true; }

    if (ImGui::SliderFloat("Volume", &e.Volume, 0.0f, 1.0f, "%.2f")) { e.ExplicitVolume = true; lug->IsDirty = true; }
    if (ImGui::DragFloat("Pitch", &e.Pitch, 0.01f, 0.1f, 4.0f, "%.2f")) { e.ExplicitPitch = true; lug->IsDirty = true; }
    if (ImGui::DragFloat("Pitch Var", &e.PitchVar, 0.01f, 0.0f, 1.0f, "%.2f")) lug->IsDirty = true;
    if (ImGui::DragFloat("Probability %%", &e.Probability, 0.5f, 0.0f, 100.0f, "%.1f")) lug->IsDirty = true;

    ImGui::Separator();
    ImGui::Text("3D & Distances:");
    if (ImGui::DragFloat("Min Dist", &e.MinDist, 0.5f, 0.0f, 5000.0f)) { e.Flag_UseMinDist = true; lug->IsDirty = true; }
    if (ImGui::DragFloat("Max Dist", &e.MaxDist, 0.5f, 0.0f, 5000.0f)) { e.Flag_UseMaxDist = true; lug->IsDirty = true; }

    ImGui::Text("Flags:");
    if (ImGui::Checkbox("Interruptable", &e.Flag_Interrupt)) lug->IsDirty = true; ImGui::SameLine();
    if (ImGui::Checkbox("Occlusion", &e.Flag_Occlusion)) lug->IsDirty = true; ImGui::SameLine();
    if (ImGui::Checkbox("Reverb", &e.Flag_Reverb)) lug->IsDirty = true;

    if (ImGui::Checkbox("Use MinDist", &e.Flag_UseMinDist)) lug->IsDirty = true; ImGui::SameLine();
    if (ImGui::Checkbox("Use MaxDist", &e.Flag_UseMaxDist)) lug->IsDirty = true;

    ImGui::Separator();
    ImGui::TextDisabled("WAV Info:");
    ImGui::Text("Rate: %d Hz | Channels: %d", e.SampleRate, e.Channels);
    ImGui::Text("Size: %d bytes | Loops: %d - %d", e.Length, e.LoopStart, e.LoopEnd);
}

static void DrawAudioProperties(LoadedBank* bank) {
    if (!bank || bank->Type != EBankType::Audio || !bank->AudioParser) return;
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Fable Audio Bank (.LUT)");
    ImGui::Separator();
    if (bank->SelectedEntryIndex >= 0 && bank->SelectedEntryIndex < bank->AudioParser->Entries.size()) {
        const auto& entry = bank->AudioParser->Entries[bank->SelectedEntryIndex];
        ImGui::Text("ID: %d | Size: %d | Offset: %d", entry.SoundID, entry.Length, entry.Offset);
        auto& p = bank->AudioParser->Player;
        float prog = p.GetProgress();
        ImGui::SliderFloat("##seek", &prog, 0.0f, 1.0f, "");
        if (ImGui::IsItemActive()) p.Seek(prog);
        ImGui::SameLine(); ImGui::Text("%s / %s", FormatTime(p.GetCurrentTime()).c_str(), FormatTime(p.GetTotalDuration()).c_str());
        if (ImGui::Button(p.IsPlaying() ? "Pause" : "Play", ImVec2(80, 0))) {
            if (p.GetTotalDuration() == 0.0f) {
                auto riff = bank->AudioParser->GetRiffBlob(bank->SelectedEntryIndex);
                if (!riff.empty()) p.PlayWav(riff);
            }
            else { if (p.IsPlaying()) p.Pause(); else p.Play(); }
        }
        ImGui::SameLine();
        if (ImGui::Button("Export", ImVec2(80, 0))) {
            auto riff = bank->AudioParser->GetRiffBlob(bank->SelectedEntryIndex);
            if (!riff.empty()) {
                std::string pth = SaveFileDialog("WAV File\0*.wav\0");
                if (!pth.empty()) {
                    if (pth.find(".wav") == std::string::npos) pth += ".wav";
                    std::ofstream out(pth, std::ios::binary);
                    out.write((char*)riff.data(), riff.size());
                    out.close();
                }
            }
        }
    }
}