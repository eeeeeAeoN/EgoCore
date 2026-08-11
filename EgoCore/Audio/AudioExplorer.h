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

extern ImTextureID g_PlayTexture;
extern ImTextureID g_PauseTexture;
extern ImTextureID g_StopTexture;
extern ImTextureID g_LoopTexture;
extern ImTextureID g_ImportTexture;
extern ImTextureID g_ExportTexture;

inline LugParser::ParsedLugEntry g_ActiveAudioEntry;
inline int g_LastSelectedAudioIndex = -1;

inline int g_LugExplorerMode = 0;
inline int g_SelectedScriptIndex = -1;

inline bool g_LutLoopEnabled = false;
inline bool g_LugLoopEnabled = false;

static const ImVec4 kAudioAccent = ImVec4(0.95f, 0.82f, 0.45f, 1.0f);
static const ImVec4 kAudioAccentDim = ImVec4(0.60f, 0.50f, 0.20f, 1.0f);
static const ImVec4 kSectionColor = ImVec4(0.95f, 0.82f, 0.45f, 1.0f);
static const float kAudioLabelColumn = 140.0f;
static const float kAudioLabelColumnNarrow = 88.0f;
static const float kAudioIconSize = 22.0f;
static const float kPlayerSideMargin = 16.0f;
static const float kPlayerTopMargin = 10.0f;
static const float kPlayerBottomMargin = 12.0f;
static const float kFieldsCellPaddingX = 24.0f;
static const float kFieldsCellPaddingY = 6.0f;
static const float kOverviewVerticalMargin = 6.0f;

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

static bool AudioIconButton(const char* id, ImTextureID tex, const char* fallbackLabel, const char* tooltip, bool active = true, ImVec2 size = ImVec2(kAudioIconSize, kAudioIconSize)) {
    ImVec4 tint = active ? kAudioAccent : kAudioAccentDim;
    bool pressed;
    if (tex) {
        pressed = ImGui::ImageButton(id, tex, size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tint);
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Text, tint);
        pressed = ImGui::Button(fallbackLabel, ImVec2(size.x + 40, size.y));
        ImGui::PopStyleColor();
    }
    if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
    return pressed;
}

static void BeginCenteredButtonRow(int numButtons, float buttonSize = kAudioIconSize, float spacing = 12.0f) {
    float totalWidth = numButtons * buttonSize + (numButtons - 1) * spacing;
    float avail = ImGui::GetContentRegionAvail().x;
    float offset = (avail - totalWidth) * 0.5f;
    if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
}

template <typename TPlayer>
static void DrawAudioScrubber(TPlayer& audioPlayer, float height = 20.0f) {
    float currentT = audioPlayer.GetCurrentTime();
    float totalT = audioPlayer.GetTotalDuration();
    float progress = std::clamp(audioPlayer.GetProgress(), 0.0f, 1.0f);

    ImGui::Dummy(ImVec2(0, kPlayerTopMargin));

    float fullWidth = ImGui::GetContentRegionAvail().x;
    float barWidth = fullWidth - kPlayerSideMargin * 2.0f;
    if (barWidth < 40.0f) barWidth = 40.0f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kPlayerSideMargin);

    ImVec2 barSize = ImVec2(barWidth, height);
    ImVec2 pMin = ImGui::GetCursorScreenPos();
    ImVec2 pMax = ImVec2(pMin.x + barSize.x, pMin.y + barSize.y);

    ImGui::InvisibleButton("##AudioScrub", barSize);
    bool active = ImGui::IsItemActive();
    bool hovered = ImGui::IsItemHovered();
    if (active && ImGui::IsMouseDown(0) && barSize.x > 0.0f) {
        float t = std::clamp((ImGui::GetMousePos().x - pMin.x) / barSize.x, 0.0f, 1.0f);
        audioPlayer.Seek(t);
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pMin, pMax, IM_COL32(40, 40, 40, 255), 4.0f);
    dl->AddRect(pMin, pMax, IM_COL32(90, 90, 90, 255), 4.0f);
    if (barSize.x > 0.0f) {
        dl->AddRectFilled(pMin, ImVec2(pMin.x + barSize.x * progress, pMax.y),
            IM_COL32(242, 209, 115, 100), 4.0f);
    }
    float headX = pMin.x + barSize.x * progress;
    dl->AddLine(ImVec2(headX, pMin.y), ImVec2(headX, pMax.y),
        IM_COL32(242, 209, 115, 255), 2.0f);

    if (hovered && barSize.x > 0.0f) {
        float hoverT = std::clamp((ImGui::GetMousePos().x - pMin.x) / barSize.x, 0.0f, 1.0f);
        ImGui::SetTooltip("%s", FormatTime(hoverT * totalT).c_str());
    }

    ImGui::Dummy(ImVec2(0, 4));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kPlayerSideMargin);
    ImGui::TextDisabled("%s / %s", FormatTime(currentT).c_str(), FormatTime(totalT).c_str());
}

static void AudioPropLabel(const char* label, float columnWidth = kAudioLabelColumn) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(columnWidth);
    ImGui::SetNextItemWidth(-1);
}

static void DrawLugAudioProperties(LoadedBank* bank) {
    if (!bank || !bank->LugParserPtr) return;
    auto& lug = bank->LugParserPtr;

    if (bank->SelectedEntryIndex == -1 || bank->SelectedEntryIndex >= lug->Entries.size()) {
        ImGui::Text("Select a sound to view properties.");
        return;
    }

    if (bank->SelectedEntryIndex != g_LastSelectedAudioIndex) {
        g_ActiveAudioEntry = lug->Entries[bank->SelectedEntryIndex];
        g_LastSelectedAudioIndex = bank->SelectedEntryIndex;
    }
    auto& e = g_ActiveAudioEntry;

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

    if (player.GetTotalDuration() > 0.0f && player.GetProgress() >= 1.0f) {
        if (g_LugLoopEnabled) {
            player.Seek(0.0f);
            player.Play();
        }
        else {
            player.Stop();
        }
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 1.0f, 0.6f, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 1.0f, 0.6f, 0.4f));

    if (AudioIconButton("##LugExport", g_ExportTexture, "Export", "Export WAV")) {
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
    if (AudioIconButton("##LugImport", g_ImportTexture, "Import", "Import WAV (replace)")) {
        std::string p = OpenFileDialog("WAV File\0*.wav\0");
        if (!p.empty()) {
            if (lug->ImportWav(bank->SelectedEntryIndex, p)) {
                player.Reset();
                SyncUIList();

                StagedEntry dummy;
                bank->StagedEntries[bank->SelectedEntryIndex] = dummy;

                g_SuccessMessage = "WAV file replaced successfully (Memory).\nRecompile to save to disk.";
                g_ShowSuccessPopup = true;
            }
        }
    }
    ImGui::PopStyleColor(3);

    DrawAudioScrubber(player);

    ImGui::Dummy(ImVec2(0, 6));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 1.0f, 0.6f, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 1.0f, 0.6f, 0.4f));

    BeginCenteredButtonRow(3);

    ImTextureID playPauseTex = player.IsPlaying() ? g_PauseTexture : g_PlayTexture;
    if (AudioIconButton("##LugPlayPause", playPauseTex, player.IsPlaying() ? "Pause" : "Play", player.IsPlaying() ? "Pause" : "Play")) {
        if (player.GetTotalDuration() == 0.0f) {
            auto blob = lug->GetAudioBlob(bank->SelectedEntryIndex);
            if (!blob.empty()) player.PlayWav(blob);
        }
        else {
            if (player.IsPlaying()) player.Pause(); else player.Play();
        }
    }
    ImGui::SameLine(0.0f, 12.0f);
    if (AudioIconButton("##LugStop", g_StopTexture, "Stop", "Stop")) player.Stop();

    ImGui::SameLine(0.0f, 12.0f);
    if (AudioIconButton("##LugLoop", g_LoopTexture, g_LugLoopEnabled ? "Loop: On" : "Loop: Off",
        g_LugLoopEnabled ? "Looping enabled" : "Looping disabled", g_LugLoopEnabled)) {
        g_LugLoopEnabled = !g_LugLoopEnabled;
    }

    ImGui::PopStyleColor(3);

    ImGui::Dummy(ImVec2(0, kPlayerBottomMargin));

    ImGui::Separator();

    ImGui::Dummy(ImVec2(0, kOverviewVerticalMargin));
    ImGui::Indent(kPlayerSideMargin);
    ImGui::TextColored(kSectionColor, "Overview");
    if (InputString("Name", e.Name)) lug->IsDirty = true;
    if (InputString("Source Path", e.FullPath)) lug->IsDirty = true;
    if (InputString("Group/Context", e.GroupName)) lug->IsDirty = true;
    ImGui::Text("%d Hz   |   %d Channel%s", e.SampleRate, e.Channels, e.Channels == 1 ? "" : "s");
    ImGui::Unindent(kPlayerSideMargin);

    ImGui::Dummy(ImVec2(0, kOverviewVerticalMargin));
    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(kFieldsCellPaddingX, kFieldsCellPaddingY));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kPlayerSideMargin);
    if (ImGui::BeginTable("LugFieldsSplit", 2, ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextColored(kSectionColor, "Playback Logic");

        int prio = (int)e.Priority;
        AudioPropLabel("Priority", kAudioLabelColumnNarrow);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::InputInt("##Priority", &prio)) { e.Priority = (uint32_t)prio; lug->IsDirty = true; }

        int loops = (int)e.LoopCount;
        AudioPropLabel("Loop Count", kAudioLabelColumnNarrow);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::InputInt("##LoopCount", &loops)) { e.LoopCount = (uint32_t)loops; lug->IsDirty = true; }

        AudioPropLabel("Volume", kAudioLabelColumnNarrow);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::SliderFloat("##Volume", &e.Volume, 0.0f, 1.0f, "%.2f")) { e.ExplicitVolume = true; lug->IsDirty = true; }

        AudioPropLabel("Pitch", kAudioLabelColumnNarrow);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Pitch", &e.Pitch, 0.01f, 0.1f, 4.0f, "%.2f")) { e.ExplicitPitch = true; lug->IsDirty = true; }

        AudioPropLabel("Pitch Var", kAudioLabelColumnNarrow);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##PitchVar", &e.PitchVar, 0.01f, 0.0f, 1.0f, "%.2f")) lug->IsDirty = true;

        AudioPropLabel("Probability", kAudioLabelColumnNarrow);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Probability", &e.Probability, 0.5f, 0.0f, 100.0f, "%.1f")) lug->IsDirty = true;

        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(kSectionColor, "Distances");

        AudioPropLabel("Min Dist", kAudioLabelColumnNarrow);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##MinDist", &e.MinDist, 0.5f, 0.0f, 5000.0f, "%.1f")) { e.Flag_UseMinDist = true; lug->IsDirty = true; }
        ImGui::SameLine();
        if (ImGui::Checkbox("Use##UseMinDist", &e.Flag_UseMinDist)) lug->IsDirty = true;

        AudioPropLabel("Max Dist", kAudioLabelColumnNarrow);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##MaxDist", &e.MaxDist, 0.5f, 0.0f, 5000.0f, "%.1f")) { e.Flag_UseMaxDist = true; lug->IsDirty = true; }
        ImGui::SameLine();
        if (ImGui::Checkbox("Use##UseMaxDist", &e.Flag_UseMaxDist)) lug->IsDirty = true;

        ImGui::Dummy(ImVec2(0, 6));
        ImGui::TextColored(kSectionColor, "Flags");
        if (ImGui::Checkbox("Interruptable", &e.Flag_Interrupt)) lug->IsDirty = true;
        if (ImGui::Checkbox("Occlusion", &e.Flag_Occlusion)) lug->IsDirty = true;
        if (ImGui::Checkbox("Reverb", &e.Flag_Reverb)) lug->IsDirty = true;

        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

static void DrawLugScriptProperties(LoadedBank* bank) {
    if (!bank || !bank->LugParserPtr) return;
    auto& lug = bank->LugParserPtr;

    ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "Event Script Editor");
    ImGui::Separator();

    if (g_SelectedScriptIndex < 0 || g_SelectedScriptIndex >= lug->Scripts.size()) {
        ImGui::Text("Select an Event Script to view its mapped sounds.");
        return;
    }

    auto& script = lug->Scripts[g_SelectedScriptIndex];

    if (InputString("Trigger Event", script.Name, 255)) {
        lug->IsDirty = true;
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Mapped Sound IDs (%d)", script.SoundIDs.size());

    auto GetSoundInfo = [&](uint32_t id, std::string& outName, int& outInternalIndex) {
        outName = "Unknown Sound";
        outInternalIndex = -1;
        for (int i = 0; i < (int)lug->Entries.size(); i++) {
            if (lug->Entries[i].SoundID == id) {
                outName = lug->Entries[i].Name;
                outInternalIndex = i;
                break;
            }
        }
        };

    if (ImGui::BeginTable("ScriptSoundsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 300))) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Sound Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < script.SoundIDs.size(); i++) {
            ImGui::PushID((int)i);
            ImGui::TableNextRow();

            uint32_t currentID = script.SoundIDs[i];
            std::string soundName;
            int internalIdx;
            GetSoundInfo(currentID, soundName, internalIdx);

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", currentID);

            ImGui::TableSetColumnIndex(1);
            if (internalIdx == -1) ImGui::TextColored(ImVec4(1, 0, 0, 1), "[Missing/Ghost] %d", currentID);
            else ImGui::Text("%s", soundName.c_str());

            ImGui::TableSetColumnIndex(2);
            if (internalIdx != -1) {
                if (ImGui::Button("Play", ImVec2(40, 0))) {
                    auto blob = lug->GetAudioBlob(internalIdx);
                    if (!blob.empty()) player.PlayWav(blob);
                }
                ImGui::SameLine();
            }
            else {
                ImGui::Dummy(ImVec2(40, 0)); ImGui::SameLine();
            }

            if (ImGui::Button("X", ImVec2(24, 0))) {
                script.SoundIDs.erase(script.SoundIDs.begin() + i);
                lug->IsDirty = true;
                i--;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::Dummy(ImVec2(0, 5));
    static int s_NewIDToAdd = 0;
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("##newID", &s_NewIDToAdd, 0);
    ImGui::SameLine();
    if (ImGui::Button("Add Sound ID to Pool") && s_NewIDToAdd > 0) {
        script.SoundIDs.push_back((uint32_t)s_NewIDToAdd);
        lug->IsDirty = true;
    }
}

static void DrawAudioProperties(LoadedBank* bank) {
    if (!bank || bank->Type != EBankType::Audio || !bank->AudioParser) return;

    if (bank->SelectedEntryIndex < 0 || bank->SelectedEntryIndex >= (int)bank->AudioParser->Entries.size()) {
        ImGui::Text("Select a sound to view properties.");
        return;
    }

    auto& p = bank->AudioParser->Player;

    if (p.GetTotalDuration() > 0.0f && p.GetProgress() >= 1.0f) {
        if (g_LutLoopEnabled) {
            p.Seek(0.0f);
            p.Play();
        }
        else {
            p.Stop();
        }
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 1.0f, 0.6f, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 1.0f, 0.6f, 0.4f));

    if (AudioIconButton("##LutExport", g_ExportTexture, "Export", "Export WAV")) {
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
    ImGui::SameLine();

    if (AudioIconButton("##LutImport", g_ImportTexture, "Import", "Import WAV (replace)")) {
        std::string importPath = OpenFileDialog("WAV File\0*.wav\0");
        if (!importPath.empty()) {
            if (bank->AudioParser->ImportWav(bank->SelectedEntryIndex, importPath)) {
                p.Reset();

                g_SuccessMessage = "WAV file replaced successfully.\nRecompile to save to disk.";
                g_ShowSuccessPopup = true;
            }
        }
    }

    ImGui::PopStyleColor(3);
    DrawAudioScrubber(p);

    ImGui::Dummy(ImVec2(0, 6));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 1.0f, 0.6f, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 1.0f, 0.6f, 0.4f));

    BeginCenteredButtonRow(3);

    ImTextureID playPauseTex = p.IsPlaying() ? g_PauseTexture : g_PlayTexture;
    if (AudioIconButton("##LutPlayPause", playPauseTex, p.IsPlaying() ? "Pause" : "Play", p.IsPlaying() ? "Pause" : "Play")) {
        if (p.GetTotalDuration() == 0.0f) {
            auto riff = bank->AudioParser->GetRiffBlob(bank->SelectedEntryIndex);
            if (!riff.empty()) p.PlayWav(riff);
        }
        else {
            if (p.IsPlaying()) p.Pause(); else p.Play();
        }
    }
    ImGui::SameLine(0.0f, 12.0f);
    if (AudioIconButton("##LutStop", g_StopTexture, "Stop", "Stop")) {
        p.Stop();
    }
    ImGui::SameLine(0.0f, 12.0f);
    if (AudioIconButton("##LutLoop", g_LoopTexture, g_LutLoopEnabled ? "Loop: On" : "Loop: Off",
        g_LutLoopEnabled ? "Looping enabled" : "Looping disabled", g_LutLoopEnabled)) {
        g_LutLoopEnabled = !g_LutLoopEnabled;
    }

    ImGui::PopStyleColor(3);

    ImGui::Dummy(ImVec2(0, kPlayerBottomMargin));
}