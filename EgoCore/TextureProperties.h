#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

inline std::string BytesToHexString(const uint8_t* data, size_t size, size_t bytesPerLine = 16) {
    std::stringstream ss;
    for (size_t i = 0; i < size; i++) {
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)data[i] << " ";
        if ((i + 1) % bytesPerLine == 0 && i != size - 1) {
            ss << "\n";
        }
    }
    return ss.str();
}

inline void DrawTextureProperties() {
    if (!g_TextureParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No texture data available or parse failed.");
        ImGui::TextWrapped("Debug: %s", g_TextureParser.DebugLog.c_str());

        // Still show hex dumps even if parsing failed
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Debug: Hex Dumps (Parse Failed)")) {
            // Metadata Hex Dump
            if (ImGui::TreeNode("Metadata Hex Dump")) {
                if (g_SelectedEntryIndex >= 0 && g_SubheaderCache.count(g_SelectedEntryIndex)) {
                    const auto& metadata = g_SubheaderCache[g_SelectedEntryIndex];
                    std::string hexDump = BytesToHexString(metadata.data(), metadata.size());

                    ImGui::Text("Size: %zu bytes", metadata.size());
                    if (ImGui::Button("Copy Metadata Hex")) {
                        ImGui::SetClipboardText(hexDump.c_str());
                    }

                    ImGui::BeginChild("MetadataHexScroll", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
                    ImGui::TextUnformatted(hexDump.c_str());
                    ImGui::EndChild();
                }
                else {
                    ImGui::TextDisabled("No metadata available.");
                }
                ImGui::TreePop();
            }

            // Pixel Data Hex Dump
            if (ImGui::TreeNode("Pixel Data Hex Dump")) {
                if (!g_CurrentEntryRawData.empty()) {
                    std::string hexDump = BytesToHexString(g_CurrentEntryRawData.data(),
                        std::min<size_t>(g_CurrentEntryRawData.size(), 4096));

                    ImGui::Text("Total Size: %zu bytes (showing first 4KB)", g_CurrentEntryRawData.size());
                    if (ImGui::Button("Copy First 4KB")) {
                        ImGui::SetClipboardText(hexDump.c_str());
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Copy All Pixel Data")) {
                        std::string fullHex = BytesToHexString(g_CurrentEntryRawData.data(), g_CurrentEntryRawData.size());
                        ImGui::SetClipboardText(fullHex.c_str());
                    }

                    ImGui::BeginChild("PixelDataHexScroll", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
                    ImGui::TextUnformatted(hexDump.c_str());
                    ImGui::EndChild();
                }
                else {
                    ImGui::TextDisabled("No pixel data available.");
                }
                ImGui::TreePop();
            }
        }
        return;
    }

    ImGui::TextColored(ImVec4(0, 1, 1, 1), "--- TEXTURE HEADER DATA ---");

    // Display Basic Dimensions
    ImGui::Text("Width:  %d", g_TextureParser.Header.Width);
    ImGui::Text("Height: %d", g_TextureParser.Header.Height);
    ImGui::Text("Depth:  %d", g_TextureParser.Header.Depth);

    ImGui::Separator();

    // Display Format and Transparency
    ImGui::Text("Format: %s", g_TextureParser.GetFormatString().c_str());
    ImGui::Text("Mips:   %d", g_TextureParser.Header.MipmapLevels);

    const char* transTypes[] = { "None", "Alpha", "Boolean", "Interpolated", "Non-DXT1 Boolean" };
    int transIdx = g_TextureParser.Header.TransparencyType;
    if (transIdx >= 0 && transIdx <= 4)
        ImGui::Text("Transparency: %s", transTypes[transIdx]);
    else
        ImGui::Text("Transparency: Unknown (%d)", transIdx);

    ImGui::Separator();

    // Animated Sequence Info
    if (g_TextureParser.Header.FrameCount > 1) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "SEQUENCE DATA");
        ImGui::Text("Frames: %d", g_TextureParser.Header.FrameCount);
        ImGui::Text("Frame Size: %d x %d", g_TextureParser.Header.FrameWidth, g_TextureParser.Header.FrameHeight);
        ImGui::Text("Data Per Frame: %u bytes", g_TextureParser.Header.FrameDataSize);
        ImGui::Separator();
    }

    // Detailed Mip Map Sizes
    if (ImGui::TreeNode("Mip-Map Details")) {
        if (ImGui::BeginTable("MipTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Level");
            ImGui::TableSetupColumn("Compressed Size");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < g_TextureParser.MipSizes.size(); i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Mip %zu", i);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%u bytes", g_TextureParser.MipSizes[i]);
            }
            ImGui::EndTable();
        }
        ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::TextWrapped("Status: %s", g_TextureParser.DebugLog.c_str());

    // === HEX DUMP VIEWER ===
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Debug: Hex Dumps")) {
        // Metadata Hex Dump
        if (ImGui::TreeNode("Metadata Hex Dump")) {
            if (g_SelectedEntryIndex >= 0 && g_SubheaderCache.count(g_SelectedEntryIndex)) {
                const auto& metadata = g_SubheaderCache[g_SelectedEntryIndex];
                std::string hexDump = BytesToHexString(metadata.data(), metadata.size());

                ImGui::Text("Size: %zu bytes", metadata.size());
                if (ImGui::Button("Copy Metadata Hex")) {
                    ImGui::SetClipboardText(hexDump.c_str());
                }

                ImGui::BeginChild("MetadataHexScroll", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextUnformatted(hexDump.c_str());
                ImGui::EndChild();
            }
            else {
                ImGui::TextDisabled("No metadata available.");
            }
            ImGui::TreePop();
        }

        // Pixel Data Hex Dump
        if (ImGui::TreeNode("Pixel Data Hex Dump")) {
            if (!g_CurrentEntryRawData.empty()) {
                std::string hexDump = BytesToHexString(g_CurrentEntryRawData.data(),
                    std::min<size_t>(g_CurrentEntryRawData.size(), 4096)); // Limit to 4KB preview

                ImGui::Text("Total Size: %zu bytes (showing first 4KB)", g_CurrentEntryRawData.size());
                if (ImGui::Button("Copy First 4KB")) {
                    ImGui::SetClipboardText(hexDump.c_str());
                }
                ImGui::SameLine();
                if (ImGui::Button("Copy All Pixel Data")) {
                    std::string fullHex = BytesToHexString(g_CurrentEntryRawData.data(), g_CurrentEntryRawData.size());
                    ImGui::SetClipboardText(fullHex.c_str());
                }

                ImGui::BeginChild("PixelDataHexScroll", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextUnformatted(hexDump.c_str());
                ImGui::EndChild();
            }
            else {
                ImGui::TextDisabled("No pixel data available.");
            }
            ImGui::TreePop();
        }
    }
}