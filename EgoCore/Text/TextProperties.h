#pragma once
#include "imgui.h"
#include "TextBackend.h" 
#include "FileDialogs.h"
#include "LipSyncCompiler.h"
#include <functional> 

bool LoadDialogueBankInBackground();

static bool g_ShowAddGroupItemPopup = false;
static char g_GroupSearchBuf[128] = "";
static bool g_ShowLipSyncAnalysisPopup = false;
static std::string g_PendingWavPath = "";
static std::string g_PendingSpeechBank = "";
static std::string g_PendingIdentifier = "";

struct GroupItemPreview {
    std::string Speaker;
    std::string ContentShort;
    bool Loaded = false;
};
static std::map<uint32_t, GroupItemPreview> g_GroupPreviewCache;

void DeleteLinkedMedia(const std::string& speechBankName, const std::string& identifier);

inline bool TextInputField(const char* label, std::string& str, float width = 0.0f) {
    static char buffer[1024];
    strncpy_s(buffer, sizeof(buffer), str.c_str(), _TRUNCATE);

    if (width != 0.0f) ImGui::SetNextItemWidth(width);

    bool changed = ImGui::InputText(label, buffer, sizeof(buffer));

    if (changed) {
        str = buffer;
    }
    return changed;
}

inline GroupItemPreview GetItemPreview(LoadedBank* bank, uint32_t id) {
    if (g_GroupPreviewCache.count(id) && g_GroupPreviewCache[id].Loaded) {
        return g_GroupPreviewCache[id];
    }

    GroupItemPreview preview;
    preview.Loaded = true;

    int idx = -1;
    for (int i = 0; i < (int)bank->Entries.size(); i++) {
        if (bank->Entries[i].ID == id) {
            idx = i;
            break;
        }
    }

    if (idx != -1) {
        const auto& entry = bank->Entries[idx];
        std::vector<uint8_t> data(entry.Size);

        bank->Stream->clear();
        bank->Stream->seekg(entry.Offset, std::ios::beg);
        bank->Stream->read((char*)data.data(), entry.Size);

        if (data.size() > 4) {
            CTextParser tempParser;
            if (tempParser.Parse(data, entry.Type)) {
                preview.Speaker = tempParser.TextData.Speaker;
                preview.ContentShort = WStringToString(tempParser.TextData.Content);
                if (preview.ContentShort.length() > 100) {
                    preview.ContentShort = preview.ContentShort.substr(0, 97) + "...";
                }
            }
        }
    }

    g_GroupPreviewCache[id] = preview;
    return preview;
}

enum class TagMode { Custom, Attitude, Animation, Camera };

inline void RenderTagEditor(CTextTag& tag) {
    TagMode mode = TagMode::Custom;
    if (tag.Name.rfind("ANIM:", 0) == 0) mode = TagMode::Animation;
    else if (tag.Name.rfind("CAM:", 0) == 0) mode = TagMode::Camera;
    else if (tag.Name.rfind("CONVERSATION_ATTITUDE_", 0) == 0) mode = TagMode::Attitude;

    if (mode == TagMode::Animation) {
        std::string animName = tag.Name.substr(5);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("ANIM:");
        ImGui::SameLine();
        if (TextInputField("##anim", animName)) {
            tag.Name = "ANIM:" + animName;
            g_IsTextDirty = true;
        }
    }
    else if (mode == TagMode::Attitude) {
        static std::vector<std::string> attitudes;
        if (attitudes.empty()) attitudes = GetEnumMembers("EConversationAttitude");

        ImGui::AlignTextToFramePadding();
        ImGui::Text("MOOD:");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##att", tag.Name.c_str(), (ImGuiComboFlags)0)) {
            for (const auto& a : attitudes) {
                bool isSel = (tag.Name == a);
                if (ImGui::Selectable(a.c_str(), isSel, (ImGuiSelectableFlags)0)) {
                    tag.Name = a;
                    g_IsTextDirty = true;
                }
                if (isSel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
    else if (mode == TagMode::Camera) {
        std::string s = tag.Name.substr(4);
        std::string posP = "SPEAKER", pos = "IN_FRONT_OF_FACE";
        std::string focP = "SPEAKER", foc = "FOCUS_FACE";
        std::string zoom = "NO_ZOOM";

        size_t p1 = s.find('('), p2 = s.find(')');
        if (p1 != std::string::npos && p2 != std::string::npos) {
            std::string g1 = s.substr(p1 + 1, p2 - p1 - 1); size_t c = g1.find(',');
            if (c != std::string::npos) { posP = g1.substr(0, c); pos = g1.substr(c + 1); }
            size_t p3 = s.find('(', p2), p4 = s.find(')', p2 + 1);
            if (p3 != std::string::npos && p4 != std::string::npos) {
                std::string g2 = s.substr(p3 + 1, p4 - p3 - 1); size_t c2 = g2.find(',');
                if (c2 != std::string::npos) { focP = g2.substr(0, c2); foc = g2.substr(c2 + 1); }
                size_t p5 = s.find('(', p4), p6 = s.find(')', p4 + 1);
                if (p5 != std::string::npos && p6 != std::string::npos) zoom = s.substr(p5 + 1, p6 - p5 - 1);
            }
        }

        static std::vector<std::string> protags, camPos, camFoc, camZoom;
        if (protags.empty()) protags = GetEnumMembers("EConversationProtagonist");
        if (camPos.empty()) camPos = GetEnumMembers("EConversationCameraPosition");
        if (camFoc.empty()) camFoc = GetEnumMembers("EConversationCameraFocus");
        if (camZoom.empty()) camZoom = GetEnumMembers("EConversationCameraZoomType");

        bool changed = false;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Pos:"); ImGui::SameLine(); ImGui::SetNextItemWidth(100);
        if (ImGui::BeginCombo("##pp", posP.c_str(), (ImGuiComboFlags)0)) { for (auto& x : protags) if (ImGui::Selectable(x.c_str(), false, (ImGuiSelectableFlags)0)) { posP = x; changed = true; } ImGui::EndCombo(); }
        ImGui::SameLine(); ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo("##p", pos.c_str(), (ImGuiComboFlags)0)) { for (auto& x : camPos) if (ImGui::Selectable(x.c_str(), false, (ImGuiSelectableFlags)0)) { pos = x; changed = true; } ImGui::EndCombo(); }

        ImGui::Text("Look:"); ImGui::SameLine(); ImGui::SetNextItemWidth(100);
        if (ImGui::BeginCombo("##fp", focP.c_str(), (ImGuiComboFlags)0)) { for (auto& x : protags) if (ImGui::Selectable(x.c_str(), false, (ImGuiSelectableFlags)0)) { focP = x; changed = true; } ImGui::EndCombo(); }
        ImGui::SameLine(); ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo("##f", foc.c_str(), (ImGuiComboFlags)0)) { for (auto& x : camFoc) if (ImGui::Selectable(x.c_str(), false, (ImGuiSelectableFlags)0)) { foc = x; changed = true; } ImGui::EndCombo(); }

        ImGui::Text("Zoom:"); ImGui::SameLine(); ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo("##z", zoom.c_str(), (ImGuiComboFlags)0)) { for (auto& x : camZoom) if (ImGui::Selectable(x.c_str(), false, (ImGuiSelectableFlags)0)) { zoom = x; changed = true; } ImGui::EndCombo(); }

        if (changed) { tag.Name = "CAM:(" + posP + "," + pos + ")(" + focP + "," + foc + ")(" + zoom + ")"; g_IsTextDirty = true; }
    }
    else {
        if (TextInputField("##raw", tag.Name)) g_IsTextDirty = true;
    }
}

inline void DrawTextProperties(LoadedBank* bank, std::function<void()> onSave, std::function<void(std::string, uint32_t, std::string)> onJump) {
    if (!g_TextParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse text entry.");
        return;
    }

    if (bank != g_LastBankPtr || (bank && bank->SelectedEntryIndex != g_LastEntryID)) {
        g_IsTextDirty = false;
        g_LastBankPtr = bank;
        if (bank) g_LastEntryID = bank->SelectedEntryIndex;
        for (auto& [k, p] : g_BackgroundAudioBanks) p->Player.Reset();
        if (g_TextParser.IsParsed) g_OriginalIdentifier = g_TextParser.TextData.Identifier;
    }

    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) { if (onSave) onSave(); }

    if (g_TextParser.IsGroup) {
        int itemToRemove = -1;
        int jumpToID = -1;

        for (int i = 0; i < g_TextParser.GroupData.Items.size(); i++) {
            auto& item = g_TextParser.GroupData.Items[i];
            if (item.CachedName.empty() && bank) item.CachedName = PeekEntryName(bank, item.ID);
            GroupItemPreview preview = GetItemPreview(bank, item.ID);

            ImGui::PushID(i);
            ImGui::BeginChild(("Item_" + std::to_string(i)).c_str(), ImVec2(0, 75), true);

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%d", item.ID);
            ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();
            ImGui::Text("%s", item.CachedName.c_str());

            float avail = ImGui::GetContentRegionAvail().x;
            ImGui::SameLine(avail - 70);

            if (ImGui::Button("Jump")) jumpToID = item.ID;
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("X", ImVec2(25, 0))) itemToRemove = i;
            ImGui::PopStyleColor();

            if (!preview.Speaker.empty()) {
                ImGui::Text("%s:", preview.Speaker.c_str());
                ImGui::SameLine();
            }
            ImGui::TextDisabled("\"%s\"", preview.ContentShort.c_str());

            ImGui::EndChild();
            ImGui::Spacing();
            ImGui::PopID();
        }

        if (ImGui::Button("+ Add Entry", ImVec2(-FLT_MIN, 30))) {
            g_ShowAddGroupItemPopup = true;
            g_GroupSearchBuf[0] = '\0';
            ImGui::OpenPopup("Add Group Item");
        }

        if (ImGui::BeginPopupModal("Add Group Item", &g_ShowAddGroupItemPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Search for an entry to add:");
            ImGui::InputText("##search", g_GroupSearchBuf, 128);
            ImGui::Separator();
            std::string search = g_GroupSearchBuf;
            std::transform(search.begin(), search.end(), search.begin(), ::tolower);

            int suggestionsFound = 0;
            if (bank && !search.empty()) {
                if (ImGui::BeginListBox("##suggestions", ImVec2(400, 150))) {
                    for (const auto& entry : bank->Entries) {
                        if (entry.Type != 0) continue;
                        if (entry.ID == bank->Entries[bank->SelectedEntryIndex].ID) continue;

                        std::string nameLower = entry.Name;
                        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                        std::string idStr = std::to_string(entry.ID);

                        if (nameLower.find(search) != std::string::npos || idStr.find(search) != std::string::npos) {
                            std::string label = entry.Name + " (" + std::to_string(entry.ID) + ")";
                            if (ImGui::Selectable(label.c_str(), false, (ImGuiSelectableFlags)0)) {
                                CTextGroupItem newItem;
                                newItem.ID = entry.ID;
                                newItem.CachedName = entry.Name;
                                g_TextParser.GroupData.Items.push_back(newItem);
                                g_IsTextDirty = true;
                                g_ShowAddGroupItemPopup = false;
                                ImGui::CloseCurrentPopup();
                            }
                            suggestionsFound++;
                            if (suggestionsFound >= 5) break;
                        }
                    }
                    ImGui::EndListBox();
                }
            }
            ImGui::Separator();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { g_ShowAddGroupItemPopup = false; ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        if (itemToRemove != -1) {
            g_TextParser.GroupData.Items.erase(g_TextParser.GroupData.Items.begin() + itemToRemove);
            g_IsTextDirty = true;
        }

        if (jumpToID != -1 && bank && onJump) {
            onJump(bank->FileName, (uint32_t)jumpToID, "");
        }
    }
    else if (g_TextParser.IsNarratorList) {

        int itemToRemove = -1;

        for (int i = 0; i < g_TextParser.NarratorStrings.size(); i++) {
            std::string& str = g_TextParser.NarratorStrings[i];

            ImGui::PushID(i);
            ImGui::BeginChild(("NarrItem_" + std::to_string(i)).c_str(), ImVec2(0, 45), true);

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%d", i);
            ImGui::SameLine();

            float avail = ImGui::GetContentRegionAvail().x;
            float inputWidth = avail - 60;

            if (TextInputField("##val", str, inputWidth)) g_IsTextDirty = true;

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("X", ImVec2(25, 0))) itemToRemove = i;
            ImGui::PopStyleColor();

            ImGui::EndChild();
            ImGui::Spacing();
            ImGui::PopID();
        }

        if (ImGui::Button("+ Add Speaker", ImVec2(-FLT_MIN, 30))) {
            g_TextParser.NarratorStrings.push_back("NEW_SPEAKER");
            g_IsTextDirty = true;
        }

        if (itemToRemove != -1) {
            g_TextParser.NarratorStrings.erase(g_TextParser.NarratorStrings.begin() + itemToRemove);
            g_IsTextDirty = true;
        }
    }
    else {
        CTextEntry& e = g_TextParser.TextData;

        ImGui::Text("Identifier");
        if (TextInputField("##id", e.Identifier, -FLT_MIN)) g_IsTextDirty = true;
        ImGui::Spacing();

        float avail = ImGui::GetContentRegionAvail().x;
        float colWidth = (avail - ImGui::GetStyle().ItemSpacing.x) / 2.0f;

        ImGui::BeginGroup();
        ImGui::Text("Speaker");
        if (TextInputField("##speaker", e.Speaker, colWidth)) g_IsTextDirty = true;
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::Text("Sound Bank");
        ImGui::SetNextItemWidth(colWidth);
        if (g_AvailableSoundBanks.empty()) {
            if (TextInputField("##soundbank", e.SpeechBank)) g_IsTextDirty = true;
        }
        else {
            if (ImGui::BeginCombo("##soundbank", e.SpeechBank.c_str(), (ImGuiComboFlags)0)) {
                for (const auto& sb : g_AvailableSoundBanks) {
                    std::string cleanName = EnforceLugExtension(sb);
                    bool isSelected = (e.SpeechBank == cleanName);
                    if (ImGui::Selectable(cleanName.c_str(), isSelected, (ImGuiSelectableFlags)0)) { e.SpeechBank = cleanName; g_IsTextDirty = true; }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        ImGui::EndGroup();

        ImGui::Spacing(); ImGui::Separator();

        static char contentBuf[8192];
        std::string utf8Content = WStringToString(e.Content);
        strncpy_s(contentBuf, sizeof(contentBuf), utf8Content.c_str(), _TRUNCATE);

        ImGui::Text("Content");
        if (ImGui::InputTextMultiline("##content", contentBuf, sizeof(contentBuf), ImVec2(-FLT_MIN, 100))) {
            e.Content = StringToWString(contentBuf);
            g_IsTextDirty = true;
        }

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Tags / Modifiers")) {
            if (ImGui::BeginTable("TagsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn("Tag Data", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 50);

                int tagToDelete = -1;
                for (int i = 0; i < e.Tags.size(); i++) {
                    ImGui::PushID(i);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); RenderTagEditor(e.Tags[i]);
                    ImGui::TableSetColumnIndex(1); if (ImGui::Button("X")) tagToDelete = i;
                    ImGui::PopID();
                }
                ImGui::EndTable();
                if (tagToDelete != -1) { e.Tags.erase(e.Tags.begin() + tagToDelete); g_IsTextDirty = true; }
            }

            static bool showTagPopup = false;
            if (ImGui::Button("+ Add New Tag")) { showTagPopup = true; ImGui::OpenPopup("New Tag Type"); }

            if (ImGui::BeginPopupModal("New Tag Type", &showTagPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                auto AddT = [&](std::string n) {
                    CTextTag t; t.Position = 0; t.Name = n; e.Tags.push_back(t);
                    g_IsTextDirty = true; showTagPopup = false; ImGui::CloseCurrentPopup();
                    };
                if (ImGui::Button("Attitude / Mood", ImVec2(200, 0))) AddT("CONVERSATION_ATTITUDE_NEUTRAL");
                if (ImGui::Button("Animation (ANIM)", ImVec2(200, 0))) AddT("ANIM:SCRIPT_IDLE");
                if (ImGui::Button("Camera (Detailed)", ImVec2(200, 0))) AddT("CAM:(SPEAKER,IN_FRONT_OF_FACE)(SPEAKER,FOCUS_FACE)(NO_ZOOM)");
                if (ImGui::Button("Custom / Manual", ImVec2(200, 0))) AddT("NEW_TAG");
                ImGui::Separator();
                if (ImGui::Button("Cancel", ImVec2(200, 0))) { showTagPopup = false; ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }
        }

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Linked Media", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool audioFound = false;
            std::shared_ptr<AudioBankParser> audioBank = nullptr;
            int audioIndex = -1;

            if (!e.SpeechBank.empty() && !e.Identifier.empty()) {
                int32_t soundID = ResolveAudioID(e.SpeechBank, e.Identifier);

                if (soundID != -1) {
                    std::string loadPath = e.SpeechBank;
                    if (loadPath.find(".lug") != std::string::npos) loadPath = loadPath.substr(0, loadPath.find(".lug")) + ".lut";
                    audioBank = GetOrLoadAudioBank(loadPath);
                    if (audioBank) {
                        for (int i = 0; i < (int)audioBank->Entries.size(); i++) {
                            if (audioBank->Entries[i].SoundID == (uint32_t)soundID) { audioIndex = i; audioFound = true; break; }
                        }

                        if (audioFound) {
                            if (audioBank->ModifiedCache.count(audioIndex)) ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "[PENDING SAVE]");

                            auto& player = audioBank->Player;
                            float progress = player.GetProgress();

                            ImGui::PushItemWidth(300);
                            if (ImGui::SliderFloat("##seek", &progress, 0.0f, 1.0f, "")) { player.Seek(progress); }
                            ImGui::PopItemWidth();

                            ImGui::SameLine();
                            if (ImGui::Button(player.IsPlaying() ? "Pause" : "Play", ImVec2(50, 0))) {
                                if (player.GetTotalDuration() == 0.0f) {
                                    auto riff = audioBank->GetRiffBlob(audioIndex);
                                    if (!riff.empty()) player.PlayWav(riff);
                                }
                                else { if (player.IsPlaying()) player.Pause(); else player.Play(); }
                            }

                            ImGui::SameLine();
                            if (ImGui::Button("Export", ImVec2(50, 0))) {
                                auto riff = audioBank->GetRiffBlob(audioIndex);
                                if (!riff.empty()) {
                                    std::string savePath = SaveFileDialog("WAV File\0*.wav\0");
                                    if (!savePath.empty()) {
                                        if (savePath.find(".wav") == std::string::npos) savePath += ".wav";
                                        std::ofstream out(savePath, std::ios::binary);
                                        out.write((char*)riff.data(), riff.size());
                                        out.close();
                                    }
                                }
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Import", ImVec2(50, 0))) {
                                std::string openPath = OpenFileDialog("WAV File\0*.wav\0All Files\0*.*\0");
                                if (!openPath.empty()) {
                                    if (audioBank->ImportWav(audioIndex, openPath)) { player.Reset(); g_IsTextDirty = true; }
                                }
                            }

                            ImGui::Dummy(ImVec2(0, 5));
                            if (ImGui::Button("Phonemes", ImVec2(80, 0))) { if (onJump) onJump("dialogue.big", (uint32_t)soundID, e.SpeechBank); }
                            ImGui::SameLine();
                            if (ImGui::Button("Sample", ImVec2(80, 0))) {
                                std::string lut = e.SpeechBank;
                                if (lut.find(".lug") != std::string::npos) lut = lut.substr(0, lut.find(".lug")) + ".lut";
                                else if (lut.find(".") == std::string::npos) lut += ".lut";
                                if (onJump) onJump(lut, (uint32_t)soundID, "");
                            }
                            ImGui::SameLine();
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                            if (ImGui::Button("Remove", ImVec2(80, 0))) {
                                DeleteLinkedMedia(e.SpeechBank, e.Identifier);
                                ImGui::PopStyleColor();
                                return;
                            }
                            ImGui::PopStyleColor();
                        }
                        else ImGui::TextColored(ImVec4(1, 0, 0, 1), "ID %d found in Defs, but not in Audio Bank.", soundID);
                    }
                    else ImGui::TextDisabled("Audio bank not found on disk (.lut missing).");
                }
                else {
                    if (ImGui::Button("Create Linked Media (Import Wav)", ImVec2(300, 30))) {
                        std::string loadPath = e.SpeechBank;
                        if (loadPath.find(".lug") != std::string::npos) loadPath = loadPath.substr(0, loadPath.find(".lug")) + ".lut";
                        audioBank = GetOrLoadAudioBank(loadPath);
                        if (audioBank) {
                            uint32_t nextID = GetNextIDFromHeader(e.SpeechBank);
                            std::string wavPath = OpenFileDialog("WAV File\0*.wav\0");
                            if (!wavPath.empty()) {
                                g_PendingWavPath = wavPath;
                                g_PendingSpeechBank = e.SpeechBank;
                                g_PendingIdentifier = e.Identifier;
                                g_ShowLipSyncAnalysisPopup = true;
                                ImGui::OpenPopup("Analyze LipSync?");
                            }
                        }
                    }
                }
            }
            else ImGui::TextDisabled("Assign a SpeechBank and Identifier to link media.");
        }
    }

    if (ImGui::BeginPopupModal("Analyze LipSync?", &g_ShowLipSyncAnalysisPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Do you want to automatically generate lip-sync animation?");
        ImGui::TextDisabled("This uses spectral analysis to guess phonemes.");
        ImGui::Spacing();

        auto GetAudioBank = [&](std::string bankName) {
            if (bankName.find(".lug") != std::string::npos) bankName = bankName.substr(0, bankName.find(".lug")) + ".lut";
            else if (bankName.find(".") == std::string::npos) bankName += ".lut";
            return GetOrLoadAudioBank(bankName);
            };

        auto SyncLoadedBank = [&](std::string bankName, uint32_t newID, uint32_t size) {
            if (bankName.find(".lug") != std::string::npos) bankName = bankName.substr(0, bankName.find(".lug")) + ".lut";
            else if (bankName.find(".") == std::string::npos) bankName += ".lut";

            std::transform(bankName.begin(), bankName.end(), bankName.begin(), ::tolower);

            for (auto& b : g_OpenBanks) {
                if (b.Type == EBankType::Audio) {
                    std::string bName = b.FileName;
                    std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
                    if (bName == bankName) {
                        BankEntry be;
                        be.ID = newID;
                        be.Name = "Sound ID " + std::to_string(newID);
                        be.FriendlyName = be.Name;
                        be.Size = size;
                        be.Type = 999;
                        be.Offset = 0;
                        b.Entries.push_back(be);
                        UpdateFilter(b);
                        break;
                    }
                }
            }
            };

        if (ImGui::Button("Yes (Auto-Analyze)", ImVec2(160, 0))) {
            uint32_t nextID = GetNextIDFromHeader(g_PendingSpeechBank);

            std::string hName = GetHeaderName(g_PendingSpeechBank);
            int hIdx = FindHeaderIndex(hName);
            if (hIdx != -1) {
                std::string defName = "SND_" + g_PendingIdentifier;
                InjectHeaderDefinition(hIdx, defName, nextID);
            }

            auto audioBank = GetAudioBank(g_PendingSpeechBank);
            if (audioBank && audioBank->AddEntry(nextID, g_PendingWavPath)) {
                uint32_t size = 0;
                if (!audioBank->Entries.empty()) size = audioBank->Entries.back().Length;

                SyncLoadedBank(g_PendingSpeechBank, nextID, size);

                LoadDialogueBankInBackground();

                AddLipSyncEntryFromWav(g_PendingSpeechBank, nextID, g_PendingWavPath);
                g_IsTextDirty = true;
            }

            g_ShowLipSyncAnalysisPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("No (Empty Entry)", ImVec2(160, 0))) {
            uint32_t nextID = GetNextIDFromHeader(g_PendingSpeechBank);

            std::string hName = GetHeaderName(g_PendingSpeechBank);
            int hIdx = FindHeaderIndex(hName);
            if (hIdx != -1) {
                std::string defName = "SND_" + g_PendingIdentifier;
                InjectHeaderDefinition(hIdx, defName, nextID);
            }

            auto audioBank = GetAudioBank(g_PendingSpeechBank);
            if (audioBank && audioBank->AddEntry(nextID, g_PendingWavPath)) {
                uint32_t size = 0;
                if (!audioBank->Entries.empty()) size = audioBank->Entries.back().Length;

                SyncLoadedBank(g_PendingSpeechBank, nextID, size);

                LoadDialogueBankInBackground();

                std::vector<int16_t> pcm; int rate = 0;
                if (CSpeechAnalyzer::LoadWav(g_PendingWavPath, pcm, rate) && rate > 0) {
                    float dur = (float)pcm.size() / rate;
                    AddLipSyncEntry(g_PendingSpeechBank, nextID, dur);
                }
                else {
                    AddLipSyncEntry(g_PendingSpeechBank, nextID, 1.0f);
                }
                g_IsTextDirty = true;
            }

            g_ShowLipSyncAnalysisPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Spacing();
        if (ImGui::Button("Cancel", ImVec2(330, 0))) {
            g_ShowLipSyncAnalysisPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}