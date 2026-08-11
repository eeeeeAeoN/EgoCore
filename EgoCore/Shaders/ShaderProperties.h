#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "TextEditor.h"

inline TextEditor g_ShaderTextEditor;
inline int g_LastShaderEntryID = -1;
inline float g_ShaderEditorFontScale = 1.0f;

inline const char* GetShaderTypeLabel(EShaderType type) {
    switch (type) {
    case EShaderType::VertexShader_1_1: return "Vertex Shader (vs_1_1)";
    case EShaderType::VertexShader_2_0: return "Vertex Shader (vs_2_0)";
    case EShaderType::VertexShader_3_0: return "Vertex Shader (vs_3_0)";
    case EShaderType::PixelShader_1_1:  return "Pixel Shader (ps_1_1)";
    case EShaderType::PixelShader_1_4:  return "Pixel Shader (ps_1_4)";
    case EShaderType::PixelShader_2_0:  return "Pixel Shader (ps_2_0)";
    case EShaderType::PixelShader_3_0:  return "Pixel Shader (ps_3_0)";
    default: return "Unknown Bytecode Format";
    }
}

inline void DrawShaderProperties(int currentEntryID) {
    if (!g_ShaderParser.IsParsed) {
        ImGui::TextDisabled("No shader data available or parsed.");
        return;
    }

    const auto& data = g_ShaderParser.Data;

    if (!data.VSConstantLayout.empty()) {
        ImGui::Text("Vertex Layout:"); ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", data.VSConstantLayout.c_str());
        ImGui::Dummy(ImVec2(0, 10));
    }

    ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Constants (%u)", data.ConstantCount);
    ImGui::Separator();

    if (data.ConstantCount == 0) {
        ImGui::TextDisabled("No constants defined for this shader.");
    }
    else {
        for (size_t i = 0; i < data.ConstantNames.size(); ++i) {
            ImGui::Text("%zu:", i); ImGui::SameLine();
            ImGui::TextUnformatted(data.ConstantNames[i].c_str());
        }
    }

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "%s", GetShaderTypeLabel(data.Type));
    ImGui::Separator();

    if (g_LastShaderEntryID != currentEntryID) {
        g_ShaderTextEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::HLSL());
        g_ShaderTextEditor.SetPalette(TextEditor::GetDarkPalette());
        g_ShaderTextEditor.SetReadOnly(false);
        g_ShaderTextEditor.SetText(g_ShaderParser.DecompiledText);
        g_LastShaderEntryID = currentEntryID;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && io.MouseWheel != 0.0f && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
        float step = (io.MouseWheel > 0) ? 0.2f : -0.2f;
        g_ShaderEditorFontScale = std::clamp(g_ShaderEditorFontScale + step, 0.5f, 3.0f);
    }

    extern ImFont* g_CodeFont;
    ImFont* fontToUse = g_CodeFont ? g_CodeFont : ImGui::GetFont();
    float oldScale = fontToUse->Scale;
    fontToUse->Scale = g_ShaderEditorFontScale;

    ImGui::PushFont(fontToUse);
    g_ShaderTextEditor.Render("ShaderAssemblyEditor", ImVec2(0, 0), true);
    ImGui::PopFont();

    fontToUse->Scale = oldScale;
}