#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "TextEditor.h"

inline TextEditor g_ShaderTextEditor;
inline int g_LastShaderEntryID = -1;

inline void DrawShaderProperties(int currentEntryID) {
    if (!g_ShaderParser.IsParsed) {
        ImGui::TextDisabled("No shader data available or parsed.");
        return;
    }

    const auto& data = g_ShaderParser.Data;

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Shader Information");
    ImGui::Separator();

    if (data.Type == EShaderType::VertexShader_1_1) {
        ImGui::Text("Type: "); ImGui::SameLine(); ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Vertex Shader (vs_1_1)");
    }
    else if (data.Type == EShaderType::PixelShader_1_1) {
        ImGui::Text("Type: "); ImGui::SameLine(); ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "Pixel Shader (ps_1_1)");
    }
    else if (data.Type == EShaderType::PixelShader_1_4) {
        ImGui::Text("Type: "); ImGui::SameLine(); ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "Pixel Shader (ps_1_4)");
    }
    else {
        ImGui::Text("Type: "); ImGui::SameLine(); ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Unknown Bytecode Format");
    }

    ImGui::Text("Bytecode Size: %u bytes", data.ByteSize);

    if (!data.VSConstantLayout.empty()) {
        ImGui::Text("Vertex Layout:"); ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", data.VSConstantLayout.c_str());
    }

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Constants (%u)", data.ConstantCount);
    ImGui::Separator();

    if (data.ConstantCount == 0) {
        ImGui::TextDisabled("No constants defined for this shader.");
    }
    else {
        if (ImGui::BeginTable("ConstantsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Constant Name");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < data.ConstantNames.size(); ++i) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%zu", i);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", data.ConstantNames[i].c_str());
            }
            ImGui::EndTable();
        }
    }

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Disassembly");
    ImGui::Separator();

    // Setup the text editor ONLY when the selected shader changes
    if (g_LastShaderEntryID != currentEntryID) {
        g_ShaderTextEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::HLSL());
        g_ShaderTextEditor.SetPalette(TextEditor::GetDarkPalette());
        g_ShaderTextEditor.SetReadOnly(false);
        g_ShaderTextEditor.SetText(g_ShaderParser.DecompiledText);
        g_LastShaderEntryID = currentEntryID;
    }

    // Render the editor in the remaining space
    g_ShaderTextEditor.Render("ShaderAssemblyEditor", ImVec2(0, 0), true);
}