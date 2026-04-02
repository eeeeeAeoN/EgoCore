#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include <vector>
#include <string>
#include <algorithm>
#include "ModPackageCompiler.h"
#include "ModPackageTracker.h"

inline bool g_ShowModPackageWindow = false;
inline std::vector<StagedModPackageEntry> g_ModPackageEntries;
inline char g_ModNameBuffer[128] = "";

inline void DrawModPackageWindow() {
    if (!g_ShowModPackageWindow) return;

    ImGui::SetNextWindowSize(ImVec2(750, 550), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Create Mod Package", &g_ShowModPackageWindow)) {

        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Mod Name:");
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##modname", g_ModNameBuffer, 128);
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 5));

        float btnWidth = (ImGui::GetContentRegionAvail().x / 2.0f) - 4.0f;

        if (ImGui::Button("Auto-Add Changed Entries", ImVec2(btnWidth, 30))) {
            for (const auto& bank : g_OpenBanks) {
                for (size_t i = 0; i < bank.Entries.size(); ++i) {
                    if (bank.StagedEntries.count(i) || bank.ModifiedEntryData.count(i)) {

                        std::string currentSubBank = (bank.ActiveSubBankIndex >= 0 && bank.ActiveSubBankIndex < bank.SubBanks.size()) ? bank.SubBanks[bank.ActiveSubBankIndex].Name : std::string("N/A");

                        bool exists = false;
                        for (const auto& existing : g_ModPackageEntries) {
                            if (existing.EntryID == bank.Entries[i].ID && existing.BankName == bank.FileName && existing.SubBankName == currentSubBank) {
                                exists = true; break;
                            }
                        }
                        if (!exists) {
                            StagedModPackageEntry staged;
                            staged.EntryID = bank.Entries[i].ID;
                            staged.EntryName = bank.Entries[i].Name;
                            staged.EntryType = bank.Entries[i].Type;
                            staged.TypeName = GetEntryTypeName(bank.Type, bank.Entries[i].Type, bank.FileName);
                            staged.BankName = bank.FileName;
                            staged.SourceFullPath = bank.FullPath;
                            staged.SubBankName = (bank.ActiveSubBankIndex >= 0 && bank.ActiveSubBankIndex < bank.SubBanks.size()) ? bank.SubBanks[bank.ActiveSubBankIndex].Name : std::string("N/A");

                            g_ModPackageEntries.push_back(staged);
                        }
                    }
                }
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Auto-Add Marked Entries", ImVec2(btnWidth, 30))) {
            for (const auto& tracked : ModPackageTracker::g_MarkedEntries) {
                bool exists = false;
                for (const auto& existing : g_ModPackageEntries) {
                    if (existing.EntryName == tracked.EntryName && existing.BankName == tracked.BankName) {
                        exists = true; break;
                    }
                }
                if (!exists) g_ModPackageEntries.push_back(tracked);
            }
        }

        ImGui::Separator();

        if (ImGui::BeginTable("ModPackageTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0, -50))) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 130);
            ImGui::TableSetupColumn("Bank", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableSetupColumn("Sub-Bank", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < g_ModPackageEntries.size(); i++) {
                auto& e = g_ModPackageEntries[i];
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0); ImGui::Text("%u", e.EntryID);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", e.EntryName.c_str());
                ImGui::TableSetColumnIndex(2); ImGui::Text("%s", e.TypeName.c_str());
                ImGui::TableSetColumnIndex(3); ImGui::Text("%s", e.BankName.c_str());
                ImGui::TableSetColumnIndex(4); ImGui::Text("%s", e.SubBankName.c_str());

                ImGui::TableSetColumnIndex(5);
                ImGui::PushID((int)i);
                if (ImGui::Button("X")) {
                    g_ModPackageEntries.erase(g_ModPackageEntries.begin() + i);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BANK_ENTRY_PAYLOAD")) {
                int* data = (int*)payload->Data;
                int bIdx = data[0];
                int eIdx = data[1];

                if (bIdx >= 0 && bIdx < (int)g_OpenBanks.size()) {
                    auto& bank = g_OpenBanks[bIdx];
                    if (eIdx >= 0 && eIdx < (int)bank.Entries.size()) {
                        auto& entry = bank.Entries[eIdx];

                        std::string currentSubBank = (bank.ActiveSubBankIndex >= 0 && bank.ActiveSubBankIndex < bank.SubBanks.size()) ? bank.SubBanks[bank.ActiveSubBankIndex].Name : std::string("N/A");

                        bool exists = false;
                        for (const auto& existing : g_ModPackageEntries) {
                            // --- NEW: Added existing.SubBankName check ---
                            if (existing.EntryID == entry.ID && existing.BankName == bank.FileName && existing.SubBankName == currentSubBank) {
                                exists = true; break;
                            }
                        }

                        if (!exists) {
                            StagedModPackageEntry staged;
                            staged.EntryID = entry.ID;
                            staged.EntryName = entry.Name;
                            staged.EntryType = entry.Type;
                            staged.BankType = bank.Type;
                            staged.TypeName = GetEntryTypeName(bank.Type, entry.Type, bank.FileName);
                            staged.BankName = bank.FileName;
                            staged.SourceFullPath = bank.FullPath;

                            if (bank.ActiveSubBankIndex >= 0 && bank.ActiveSubBankIndex < bank.SubBanks.size())
                                staged.SubBankName = bank.SubBanks[bank.ActiveSubBankIndex].Name;
                            else
                                staged.SubBankName = "N/A";

                            g_ModPackageEntries.push_back(staged);
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 65);
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Warning: Building the package will automatically compile and save all staged entries to your active banks.");

        bool canBuild = !g_ModPackageEntries.empty() && strlen(g_ModNameBuffer) > 0;

        ImGui::BeginDisabled(!canBuild);
        if (ImGui::Button("Build Mod Package", ImVec2(-1, 30))) {

            ModPackageCompiler::BuildPackageStructure(g_ModNameBuffer, g_ModPackageEntries);

            g_SuccessMessage = "Mod folder structure & files created successfully!";
            g_ShowSuccessPopup = true;
        }
        ImGui::EndDisabled();
    }
    ImGui::End();
}