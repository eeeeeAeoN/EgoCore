#pragma once
#include "imgui.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <filesystem>
#include "ModManagerBackend.h"
#include "ModManagerCompiler.h"

enum class EAppState;
extern EAppState g_CurrentAppState;
extern std::string g_SuccessMessage;
extern bool g_ShowSuccessPopup;
extern ImFont* g_TitleFont;
inline int g_ModToDeleteIndex = -1;
inline bool g_TriggerDeleteModPopup = false;
inline bool g_ShowModPackageWindow = false;
inline std::vector<StagedModPackageEntry> g_ModPackageEntries;
inline char g_ModNameBuffer[128] = "";

extern ImTextureID g_SearchTexture;
static bool s_SearchActive = false;
static char s_SearchText[256] = "";

// Dedicated background for the Mod Manager screen (night forest concept art).
// Load this the same way g_BackgroundTexture is loaded elsewhere (e.g. on startup
// or when entering ModsManager state), pointing at "Assets/ModManagerBackground.png".
extern ID3D11ShaderResourceView* g_ModManagerBgTexture;
extern int g_ModManagerBgWidth;
extern int g_ModManagerBgHeight;

static std::unordered_map<ImGuiID, float> s_CardHoverTimers;

// ModEntry has no single "type" field — it's expressed as a set of booleans
// (IsCoreMod / HasDll / IsAssetMod / IsDefMod / IsTngMod). Build a short display tag from them.
static std::string BuildModTypeString(const ModEntry& mod) {
    std::vector<std::string> tags;
    if (mod.IsCoreMod) tags.push_back("Core Utility");
    if (mod.HasDll) tags.push_back("DLL Hook");
    if (mod.IsAssetMod) tags.push_back("Asset Mod");
    if (mod.IsDefMod) tags.push_back("Definition Mod");
    if (mod.IsTngMod) tags.push_back("World Edit");
    if (tags.empty()) return "Unknown";

    std::string out;
    for (size_t i = 0; i < tags.size(); i++) {
        out += tags[i];
        if (i + 1 < tags.size()) out += ", ";
    }
    return out;
}

// The type color regardless of enabled state — used to preview what the toggle
// switches "on" to, since GetModAccentColor() below collapses disabled mods to gray.
static ImU32 GetModTypeColor(const ModEntry& mod) {
    if (mod.IsCoreMod)  return IM_COL32(219, 68, 68, 255);     // Core — red (highest)
    if (mod.HasDll)     return IM_COL32(151, 79, 255, 255);    // DLL — purple
    if (mod.IsTngMod)   return IM_COL32(90, 200, 120, 255);    // Tng — green (now higher than Def/Asset)
    if (mod.IsAssetMod) return IM_COL32(70, 140, 230, 255);    // Asset — blue
    if (mod.IsDefMod)   return IM_COL32(224, 196, 60, 255);    // Def — yellow
    return IM_COL32(140, 145, 155, 255);                       // Fallback
}

// Card accent color by mod type. A mod can match multiple flags at once (e.g. an
// asset mod that also ships a DLL), so ties resolve by priority: Core > DLL > Asset > Def > Tng.
// Disabled always overrides to neutral gray regardless of type.
static ImU32 GetModAccentColor(const ModEntry& mod) {
    if (!mod.IsEnabled) return IM_COL32(100, 104, 114, 255);   // Disabled — neutral gray
    return GetModTypeColor(mod);
}

// ============================================================================
//  VISUAL HELPERS
// ============================================================================

// Low-level helper for drawing an arbitrary (possibly rotated) gradient quad,
// used by the god ray effect since ImDrawList's built-in rect gradient can't rotate.
static void AddGradientQuad(ImDrawList* dl, ImVec2 p1, ImVec2 p2, ImVec2 p3, ImVec2 p4,
    ImU32 col1, ImU32 col2, ImU32 col3, ImU32 col4) {
    dl->PrimReserve(6, 4);
    ImDrawIdx idx = (ImDrawIdx)dl->_VtxCurrentIdx;
    dl->PrimWriteIdx(idx); dl->PrimWriteIdx(idx + 1); dl->PrimWriteIdx(idx + 2);
    dl->PrimWriteIdx(idx); dl->PrimWriteIdx(idx + 2); dl->PrimWriteIdx(idx + 3);
    ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
    dl->PrimWriteVtx(p1, uv, col1);
    dl->PrimWriteVtx(p2, uv, col2);
    dl->PrimWriteVtx(p3, uv, col3);
    dl->PrimWriteVtx(p4, uv, col4);
}

// Soft moonlit god rays, anchored at the top-right corner, traveling toward the
// bottom-left at a shallow ~30 degree angle. Drawn behind the vignette so the
// screen edges still darken naturally.
static void DrawGodRays(ImVec2 displaySize) {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    float time = (float)ImGui::GetTime();

    // Origin sits just outside the top-right corner so the rays feel like they're
    // coming from an off-screen moon rather than a hard point on the canvas.
    ImVec2 origin = ImVec2(displaySize.x * 1.08f, -displaySize.y * 0.10f);

    // 30 degrees measured down from horizontal, traveling toward the bottom-left.
    const float angleRad = 30.0f * (3.14159265f / 180.0f);
    ImVec2 dir = ImVec2(-cosf(angleRad), sinf(angleRad));
    ImVec2 perp = ImVec2(-dir.y, dir.x);

    float rayLength = sqrtf(displaySize.x * displaySize.x + displaySize.y * displaySize.y) * 1.15f;
    float breathe = 0.80f + 0.20f * sinf(time * 0.30f); // slow, subtle opacity breathing

    struct RayDef { float offsetPerp; float startWidth; float endWidth; float baseAlpha; };
    RayDef rays[3] = {
        { -90.0f,  14.0f, 300.0f, 0.15f },
        {  30.0f,   8.0f, 190.0f, 0.10f },
        { 150.0f,   6.0f, 130.0f, 0.07f },
    };

    const int segments = 8;
    for (const auto& ray : rays) {
        ImVec2 rayOrigin = ImVec2(origin.x + perp.x * ray.offsetPerp, origin.y + perp.y * ray.offsetPerp);

        for (int s = 0; s < segments; s++) {
            float t0 = (float)s / segments;
            float t1 = (float)(s + 1) / segments;

            float w0 = ray.startWidth + (ray.endWidth - ray.startWidth) * t0;
            float w1 = ray.startWidth + (ray.endWidth - ray.startWidth) * t1;

            float a0 = ray.baseAlpha * breathe * (1.0f - t0) * (1.0f - t0);
            float a1 = ray.baseAlpha * breathe * (1.0f - t1) * (1.0f - t1);
            if (s == 0) a0 *= 0.35f; // soften the origin so it isn't a hard edge

            ImVec2 c0 = ImVec2(rayOrigin.x + dir.x * rayLength * t0, rayOrigin.y + dir.y * rayLength * t0);
            ImVec2 c1 = ImVec2(rayOrigin.x + dir.x * rayLength * t1, rayOrigin.y + dir.y * rayLength * t1);

            ImVec2 p1 = ImVec2(c0.x + perp.x * w0 * 0.5f, c0.y + perp.y * w0 * 0.5f);
            ImVec2 p2 = ImVec2(c1.x + perp.x * w1 * 0.5f, c1.y + perp.y * w1 * 0.5f);
            ImVec2 p3 = ImVec2(c1.x - perp.x * w1 * 0.5f, c1.y - perp.y * w1 * 0.5f);
            ImVec2 p4 = ImVec2(c0.x - perp.x * w0 * 0.5f, c0.y - perp.y * w0 * 0.5f);

            // Cool pale moonlight tint to suit a night forest scene.
            ImU32 col0 = IM_COL32(210, 224, 255, (int)(a0 * 255.0f));
            ImU32 col1c = IM_COL32(210, 224, 255, (int)(a1 * 255.0f));

            AddGradientQuad(dl, p1, p2, p3, p4, col0, col1c, col1c, col0);
        }
    }
}

// Small ImVec4 lerp helper (ImLerp lives in imgui_internal.h, which we don't include)
static ImVec4 LerpColor(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

// Hand-drawn pill toggle switch, styled to match the card system rather than
// looking like a default ImGui checkbox dropped into a stylized card.
static bool DrawModToggle(const char* id, bool* value, ImU32 onColorU32) {
    ImGuiID id_hash = ImGui::GetID(id);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size(38.0f, 20.0f);

    bool clicked = ImGui::InvisibleButton(id, size);
    bool hovered = ImGui::IsItemHovered();
    if (clicked) *value = !*value;

    static std::unordered_map<ImGuiID, float> s_AnimT;
    float& t = s_AnimT[id_hash];
    float target = *value ? 1.0f : 0.0f;
    t += (target - t) * (std::min)(1.0f, ImGui::GetIO().DeltaTime * 14.0f);
    if (fabsf(t - target) < 0.001f) t = target;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    float rounding = size.y * 0.5f;

    ImVec4 offColF(0.20f, 0.21f, 0.25f, 1.0f);
    ImVec4 onColF = ImGui::ColorConvertU32ToFloat4(onColorU32);
    ImVec4 trackColF = LerpColor(offColF, onColF, t);
    if (hovered) { trackColF.x += 0.06f; trackColF.y += 0.06f; trackColF.z += 0.06f; }
    ImU32 trackCol = ImGui::ColorConvertFloat4ToU32(trackColF);

    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), trackCol, rounding);
    dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(0, 0, 0, 130), rounding);

    // Soft glow once mostly "on"
    if (t > 0.05f) {
        ImU32 glowCol = (onColorU32 & 0x00FFFFFF) | ((uint32_t)(t * 90.0f) << 24);
        dl->AddRect(ImVec2(pos.x - 1.5f, pos.y - 1.5f), ImVec2(pos.x + size.x + 1.5f, pos.y + size.y + 1.5f),
            glowCol, rounding + 1.5f, 0, 1.5f);
    }

    float knobR = (size.y * 0.5f) - 2.5f;
    float knobX = pos.x + 2.5f + knobR + (size.x - size.y) * t;
    float knobY = pos.y + size.y * 0.5f;
    dl->AddCircleFilled(ImVec2(knobX, knobY), knobR, IM_COL32(245, 247, 250, 255), 16);

    return clicked;
}

// Small hand-drawn drag grip (2x3 dot grid) to replace the plain ":::" text.
static void DrawDragGrip(ImDrawList* dl, ImVec2 topLeft, ImU32 col) {
    for (int row = 0; row < 3; row++) {
        for (int col_i = 0; col_i < 2; col_i++) {
            ImVec2 c(topLeft.x + col_i * 6.0f, topLeft.y + row * 6.0f);
            dl->AddCircleFilled(c, 1.4f, col, 8);
        }
    }
}

// Accent-colored button matching the card system, used for header actions
// instead of default ImGui button coloring.
static bool DrawAccentButton(const char* label, ImVec2 size, ImVec4 baseCol, ImVec4 hoverCol) {
    ImGui::PushStyleColor(ImGuiCol_Button, baseCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, hoverCol);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    return pressed;
}

inline void DrawModManagerWindow() {
    static int s_SelectedModIndex = -1;
    ImGuiIO& io = ImGui::GetIO();

    // Reset layout viewport
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImVec2 displaySize = io.DisplaySize;

    // --- 1. STATIC BACKGROUND, GOD RAYS & VIGNETTE ---
    if (g_ModManagerBgTexture && g_ModManagerBgWidth > 0 && g_ModManagerBgHeight > 0) {
        ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();

        float scaledWidth = displaySize.x;
        float aspect = (float)g_ModManagerBgHeight / (float)g_ModManagerBgWidth;
        float scaledHeight = scaledWidth * aspect;

        // Render static background image (night forest concept art)
        bgDrawList->AddImage((ImTextureID)g_ModManagerBgTexture, ImVec2(0.0f, 0.0f), ImVec2(scaledWidth, scaledHeight));

        // Background Dark Dimming
        bgDrawList->AddRectFilled(ImVec2(0, 0), displaySize, IM_COL32(10, 12, 16, 130));

        // Moonlit god rays, top-right toward bottom-left, drawn on top of the dim
        // layer but underneath the vignette so the frame edges still darken cleanly.
        DrawGodRays(displaySize);

        // Soft Static Vignette Edge Fades
        float vignetteSize = 220.0f;
        ImU32 colDark = IM_COL32(0, 0, 0, 240);
        ImU32 colClear = IM_COL32(0, 0, 0, 0);

        bgDrawList->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(displaySize.x, vignetteSize), colDark, colDark, colClear, colClear);
        bgDrawList->AddRectFilledMultiColor(ImVec2(0, displaySize.y - vignetteSize), displaySize, colClear, colClear, colDark, colDark);
        bgDrawList->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(vignetteSize, displaySize.y), colDark, colClear, colClear, colDark);
        bgDrawList->AddRectFilledMultiColor(ImVec2(displaySize.x - vignetteSize, 0), displaySize, colClear, colDark, colDark, colClear);
    }

    // --- 2. MAIN WINDOW CONTAINER ---
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f)); // Fully transparent base
    if (ImGui::Begin("Mod Manager UI", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {

        // HEADER BAR
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.08f, 0.11f, 0.85f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::BeginChild("##ModManagerHeader", ImVec2(0, 56.0f), true);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 headerPos = ImGui::GetWindowPos();
        ImVec2 headerSize = ImGui::GetWindowSize();

        // Arcane gradient line at top
        drawList->AddRectFilledMultiColor(
            headerPos, ImVec2(headerPos.x + headerSize.x, headerPos.y + 3.0f),
            IM_COL32(242, 193, 78, 255), IM_COL32(138, 79, 255, 255),
            IM_COL32(138, 79, 255, 255), IM_COL32(242, 193, 78, 255)
        );

        ImGui::SetCursorPosY(12.0f);
        ImGui::Indent(16.0f);

        // Title
        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "MOD MANAGER");
        if (g_TitleFont) ImGui::PopFont();

        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
        ImGui::TextDisabled("| Drag mods to adjust priority load order");
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);

        const float btnSize = 24.0f;
        const float inputWidth = 180.0f;
        const float spacing = 8.0f;

        float searchAreaWidth = 0.0f;
        if (g_SearchTexture) searchAreaWidth += btnSize + spacing;
        else searchAreaWidth += ImGui::CalcTextSize("Search").x + spacing;

        if (s_SearchActive) {
            searchAreaWidth += inputWidth + spacing;
        }

        float returnBtnWidth = 140.0f;
        float returnBtnX = headerSize.x - returnBtnWidth - 16.0f; 

        float searchAreaX = returnBtnX - searchAreaWidth - 8.0f; 
        if (searchAreaX < 20.0f) searchAreaX = 20.0f; 

        ImGui::SameLine(searchAreaX);

        if (g_SearchTexture) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.30f));

            if (ImGui::ImageButton("##SearchBtn", g_SearchTexture, ImVec2(btnSize, btnSize))) {
                s_SearchActive = !s_SearchActive;
                if (!s_SearchActive) {
                    s_SearchText[0] = '\0';
                }
            }
            ImGui::PopStyleColor(3);
        }
        else {
            if (ImGui::Button("Search")) {
                s_SearchActive = !s_SearchActive;
                if (!s_SearchActive) {
                    s_SearchText[0] = '\0';
                }
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(s_SearchActive ? "Hide search" : "Search mods");
        }

        if (s_SearchActive) {
            ImGui::SameLine(0, spacing);

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.13f, 0.16f, 0.80f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.19f, 0.23f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.22f, 0.23f, 0.28f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.22f, 0.28f, 0.50f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

            ImGui::SetNextItemWidth(inputWidth);
            ImGui::InputText("##SearchInput", s_SearchText, sizeof(s_SearchText));

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(4);

            if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                s_SearchActive = false;
                s_SearchText[0] = '\0';
            }
        }

        ImGui::SameLine(returnBtnX);
        ImVec2 btnPos = ImGui::GetCursorScreenPos();
        ImVec2 returnBtnSize(returnBtnWidth, 32.0f);

        bool btnHovered = ImGui::IsMouseHoveringRect(btnPos, ImVec2(btnPos.x + returnBtnSize.x, btnPos.y + returnBtnSize.y));
        if (btnHovered) {
            for (int g = 3; g >= 1; g--) {
                float expand = (float)g * 1.5f;
                float glowAlpha = (1.0f - (float)g / 4.0f) * 0.40f;
                ImU32 glowCol = IM_COL32(138, 79, 255, (uint32_t)(glowAlpha * 255.0f));
                drawList->AddRect(
                    ImVec2(btnPos.x - expand, btnPos.y - expand),
                    ImVec2(btnPos.x + returnBtnSize.x + expand, btnPos.y + returnBtnSize.y + expand),
                    glowCol, 6.0f + expand, 0, 1.2f
                );
            }
        }

        ImGui::PushID("ReturnToHubBtn");
        if (DrawAccentButton("##ReturnToHub", returnBtnSize,
            ImVec4(0.20f, 0.22f, 0.28f, 0.90f),
            ImVec4(0.54f, 0.31f, 1.00f, 0.85f))) {
            g_CurrentAppState = EAppState::Frontend;
        }
        ImGui::PopID();

        ImVec2 textSize = ImGui::CalcTextSize("Main Menu");
        drawList->AddText(
            ImVec2(btnPos.x + (returnBtnSize.x - textSize.x) * 0.5f,
                btnPos.y + (returnBtnSize.y - textSize.y) * 0.5f),
            IM_COL32(255, 255, 255, 255), "Main Menu"
        );

        ImGui::Unindent(16.0f);
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 6.0f));

        // --- 3. TWO-COLUMN LAYOUT (LEFT: MOD CARDS, RIGHT: INSPECTOR) ---
        float availWidth = ImGui::GetContentRegionAvail().x;
        float leftWidth = (s_SelectedModIndex >= 0 && s_SelectedModIndex < (int)ModManagerBackend::g_LoadedMods.size())
            ? availWidth * 0.58f : availWidth;

        // --- LEFT COLUMN: CARD-BASED LOAD ORDER LIST ---
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.06f, 0.08f, 0.85f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));

        ImGui::BeginChild("##ModCardsList", ImVec2(leftWidth, 0), true);

        if (ModManagerBackend::g_LoadedMods.empty()) {
            ImVec2 emptyPos = ImGui::GetCursorScreenPos();
            ImVec2 emptySize(ImGui::GetContentRegionAvail().x, 70.0f);
            ImGui::SetCursorScreenPos(ImVec2(emptyPos.x, emptyPos.y + ImGui::GetWindowHeight() * 0.35f));
            emptyPos = ImGui::GetCursorScreenPos();

            ImDrawList* emptyDraw = ImGui::GetWindowDrawList();
            ImU32 dashCol = IM_COL32(90, 95, 108, 140);
            ImVec2 pMin = emptyPos, pMax = ImVec2(emptyPos.x + emptySize.x, emptyPos.y + emptySize.y);

            float dashLen = 8.0f, gapLen = 5.0f;
            for (float x = pMin.x; x < pMax.x; x += dashLen + gapLen) {
                float xEnd = (std::min)(x + dashLen, pMax.x);
                emptyDraw->AddLine(ImVec2(x, pMin.y), ImVec2(xEnd, pMin.y), dashCol, 1.5f);
                emptyDraw->AddLine(ImVec2(x, pMax.y), ImVec2(xEnd, pMax.y), dashCol, 1.5f);
            }
            for (float y = pMin.y; y < pMax.y; y += dashLen + gapLen) {
                float yEnd = (std::min)(y + dashLen, pMax.y);
                emptyDraw->AddLine(ImVec2(pMin.x, y), ImVec2(pMin.x, yEnd), dashCol, 1.5f);
                emptyDraw->AddLine(ImVec2(pMax.x, y), ImVec2(pMax.x, yEnd), dashCol, 1.5f);
            }

            const char* msg = "No mods detected in Mods folder.";
            ImVec2 txtSize = ImGui::CalcTextSize(msg);
            ImGui::SetCursorScreenPos(ImVec2(pMin.x + (emptySize.x - txtSize.x) * 0.5f, pMin.y + (emptySize.y - txtSize.y) * 0.5f));
            ImGui::TextDisabled("%s", msg);
        }

        int draggedIdx = -1;
        if (const ImGuiPayload* activePayload = ImGui::GetDragDropPayload()) {
            if (activePayload->IsDataType("DND_MOD_ORDER")) {
                draggedIdx = *(const int*)activePayload->Data;
            }
        }

        std::string searchLower = s_SearchText;
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

        int displayedIndex = 0;
        for (int i = 0; i < (int)ModManagerBackend::g_LoadedMods.size(); ++i) {
            auto& mod = ModManagerBackend::g_LoadedMods[i];

            std::string modNameLower = mod.Name;
            std::transform(modNameLower.begin(), modNameLower.end(), modNameLower.begin(), ::tolower);
            if (!searchLower.empty() && modNameLower.find(searchLower) == std::string::npos) {
                continue;
            }

            ImGui::PushID(i);

            ImVec2 cardPos = ImGui::GetCursorScreenPos();
            float cardWidth = ImGui::GetContentRegionAvail().x;
            ImVec2 cardSize(cardWidth, 56.0f);

            bool isSelected = (s_SelectedModIndex == i);
            bool isBeingDragged = false;

            ImGui::SetNextItemAllowOverlap();

            bool clicked = ImGui::InvisibleButton("##CardSelect", cardSize);
            bool hovered = ImGui::IsItemHovered();
            bool held = ImGui::IsItemActive();

            ImGuiID cardId = ImGui::GetItemID();
            float& hoverTime = s_CardHoverTimers[cardId];
            if (hovered) {
                hoverTime += ImGui::GetIO().DeltaTime;
            }
            else {
                hoverTime = 0.0f;
            }
            float subtextAlpha = 0.0f;
            if (hoverTime > 1.0f) {
                subtextAlpha = (std::min)(1.0f, (hoverTime - 1.0f) / 0.25f);
            }

            if (clicked) {
                s_SelectedModIndex = (s_SelectedModIndex == i) ? -1 : i;
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                isBeingDragged = true;
                ImGui::SetDragDropPayload("DND_MOD_ORDER", &i, sizeof(int));
                ImGui::Text("Moving %s", mod.Name.c_str());
                ImGui::EndDragDropSource();
            }

            bool showInsertAbove = false, showInsertBelow = false;
            if (ImGui::IsItemActive() && hovered) {
                const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
                if (activePayload && activePayload->IsDataType("DND_MOD_ORDER")) {
                    int draggedIdx = *(const int*)activePayload->Data;
                    if (draggedIdx != -1 && draggedIdx != i) {
                        float relY = (ImGui::GetMousePos().y - cardPos.y) / cardSize.y;
                        if (relY < 0.5f) showInsertAbove = true; else showInsertBelow = true;
                    }
                }
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_MOD_ORDER")) {
                    int sourceIdx = *(const int*)payload->Data;
                    if (sourceIdx != i && sourceIdx >= 0 && sourceIdx < (int)ModManagerBackend::g_LoadedMods.size()) {

                        bool wasAsset = ModManagerBackend::g_LoadedMods[sourceIdx].IsAssetMod;
                        bool wasDef = ModManagerBackend::g_LoadedMods[sourceIdx].IsDefMod;
                        bool wasTng = ModManagerBackend::g_LoadedMods[sourceIdx].IsTngMod;

                        std::swap(ModManagerBackend::g_LoadedMods[sourceIdx], ModManagerBackend::g_LoadedMods[i]);

                        ModManagerBackend::SaveLoadOrder();
                        ModManagerBackend::UpdateGlobalModsIni();

                        if (wasAsset) g_AppConfig.ModSystemDirty = true;
                        if (wasDef) g_AppConfig.DefSystemDirty = true;
                        if (wasTng) g_AppConfig.TngSystemDirty = true;
                        SaveConfig();
                    }
                }
                ImGui::EndDragDropTarget();
            }

            float offsetY = (hovered && !held && !isBeingDragged) ? -2.0f : (held ? 1.0f : 0.0f);
            ImVec2 pMin = ImVec2(cardPos.x, cardPos.y + offsetY);
            ImVec2 pMax = ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y + offsetY);
            float rounding = 8.0f;

            ImDrawList* cardDraw = ImGui::GetWindowDrawList();
            ImU32 accentCol = GetModAccentColor(mod);
            ImU32 bgCol = isSelected ? IM_COL32(24, 28, 38, 240) : (hovered ? IM_COL32(18, 21, 29, 230) : IM_COL32(12, 14, 19, 210));
            ImU32 borderCol = isSelected ? IM_COL32(242, 193, 78, 220) : (hovered ? accentCol : IM_COL32(40, 45, 58, 160));

            if (isBeingDragged) {
                bgCol = (bgCol & 0x00FFFFFF) | 0x55000000;
                borderCol = (borderCol & 0x00FFFFFF) | 0x55000000;
            }

            if ((hovered || isSelected) && !isBeingDragged) {
                for (int g = 3; g >= 1; g--) {
                    float expand = (float)g * 1.5f;
                    float glowAlpha = (1.0f - (float)g / 4.0f) * 0.35f;
                    ImU32 glowCol = (borderCol & 0x00FFFFFF) | ((uint32_t)(glowAlpha * 255.0f) << 24);
                    cardDraw->AddRect(
                        ImVec2(pMin.x - expand, pMin.y - expand),
                        ImVec2(pMax.x + expand, pMax.y + expand),
                        glowCol, rounding + expand, 0, 1.2f
                    );
                }
            }

            cardDraw->AddRectFilled(pMin, pMax, bgCol, rounding);

            uint32_t alphaLeft = hovered ? 0x66000000 : 0x33000000;
            ImU32 gradLeft = (accentCol & 0x00FFFFFF) | alphaLeft;
            ImU32 gradRight = IM_COL32(0, 0, 0, 0);
            ImVec2 gradMin = ImVec2(pMin.x + 1.0f, pMin.y + 1.0f);
            ImVec2 gradMax = ImVec2(pMax.x - 1.0f, pMax.y - 1.0f);
            cardDraw->AddRectFilledMultiColor(gradMin, gradMax, gradLeft, gradRight, gradRight, gradLeft);

            cardDraw->AddRect(pMin, pMax, borderCol, rounding, 0, isSelected ? 1.8f : 1.0f);

            DrawDragGrip(cardDraw, ImVec2(pMin.x + 14.0f, pMin.y + 20.0f), IM_COL32(150, 155, 165, 200));

            ImGui::SetCursorScreenPos(ImVec2(pMin.x + 34.0f, pMin.y + 18.0f));
            ImGui::TextColored(ImVec4(0.85f, 0.70f, 0.30f, 0.90f), "#%d", displayedIndex + 1);

            ImFont* font = ImGui::GetFont();
            float nameFontSize = ImGui::GetFontSize();
            float subFontSize = ImGui::GetFontSize() * 0.9f;

            std::string nameStr = mod.Name;
            std::string typeStr = "Type: " + BuildModTypeString(mod);

            ImVec2 nameSize = font->CalcTextSizeA(nameFontSize, FLT_MAX, 0.0f, nameStr.c_str());
            float centerY = pMin.y + (cardSize.y - nameSize.y) * 0.5f;
            float topY = pMin.y + 8.0f;
            float nameY = centerY + (topY - centerY) * subtextAlpha;

            ImVec2 namePos = ImVec2(pMin.x + 78.0f, nameY);
            ImU32 nameColor = mod.IsEnabled ? IM_COL32(243, 243, 250, 255) : IM_COL32(140, 148, 166, 255);
            cardDraw->AddText(font, nameFontSize, namePos, nameColor, nameStr.c_str());

            if (subtextAlpha > 0.01f) {
                ImVec2 subSize = font->CalcTextSizeA(subFontSize, FLT_MAX, 0.0f, typeStr.c_str());
                float subY = nameY + nameSize.y + 4.0f;
                ImVec2 subPos = ImVec2(pMin.x + 78.0f, subY);
                ImU32 subColor = IM_COL32(140, 148, 166, (uint32_t)(subtextAlpha * 255 * 0.9f));
                cardDraw->AddText(font, subFontSize, subPos, subColor, typeStr.c_str());
            }

            ImGui::SetCursorScreenPos(ImVec2(pMax.x - 54.0f, pMin.y + (cardSize.y - 20.0f) * 0.5f));
            bool wasEnabled = mod.IsEnabled;
            if (DrawModToggle("##ActiveToggle", &mod.IsEnabled, GetModTypeColor(mod)) && wasEnabled != mod.IsEnabled) {
                ModManagerBackend::UpdateGlobalModsIni();
                ModManagerBackend::SaveLoadOrder();

                if (mod.IsAssetMod) g_AppConfig.ModSystemDirty = true;
                if (mod.IsDefMod)   g_AppConfig.DefSystemDirty = true;
                if (mod.IsTngMod)   g_AppConfig.TngSystemDirty = true;
                SaveConfig();
            }

            if (showInsertAbove || showInsertBelow) {
                float lineY = showInsertAbove ? (pMin.y - 3.0f) : (pMax.y + 3.0f);
                ImU32 lineCol = IM_COL32(242, 193, 78, 230);
                ImU32 lineGlow = IM_COL32(242, 193, 78, 60);
                cardDraw->AddRectFilled(ImVec2(pMin.x, lineY - 4.0f), ImVec2(pMax.x, lineY + 4.0f), lineGlow, 3.0f);
                cardDraw->AddRectFilled(ImVec2(pMin.x, lineY - 1.5f), ImVec2(pMax.x, lineY + 1.5f), lineCol, 2.0f);
            }

            ImGui::SetCursorScreenPos(ImVec2(cardPos.x, cardPos.y + cardSize.y));
            ImGui::Dummy(ImVec2(0, 6.0f));
            ImGui::PopID();

            displayedIndex++;
        }

        if (s_SelectedModIndex != -1) {
            bool visible = false;
            if (s_SelectedModIndex < (int)ModManagerBackend::g_LoadedMods.size()) {
                auto& mod = ModManagerBackend::g_LoadedMods[s_SelectedModIndex];
                std::string modNameLower = mod.Name;
                std::transform(modNameLower.begin(), modNameLower.end(), modNameLower.begin(), ::tolower);
                if (searchLower.empty() || modNameLower.find(searchLower) != std::string::npos) {
                    visible = true;
                }
            }
            if (!visible) {
                s_SelectedModIndex = -1;
            }
        }

        ImGui::EndChild();

        // --- RIGHT COLUMN: MOD SETTINGS INSPECTOR ("detail card") ---
        if (s_SelectedModIndex >= 0 && s_SelectedModIndex < (int)ModManagerBackend::g_LoadedMods.size()) {
            ImGui::SameLine(0, 10.0f);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.10f, 0.90f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
            ImGui::BeginChild("##ModInspector", ImVec2(0, 0), true);
            auto& mod = ModManagerBackend::g_LoadedMods[s_SelectedModIndex];

            // Gradient header strip, matching the main header bar
            ImDrawList* inspDraw = ImGui::GetWindowDrawList();
            ImVec2 inspPos = ImGui::GetWindowPos();
            ImVec2 inspSize = ImGui::GetWindowSize();
            inspDraw->AddRectFilledMultiColor(
                inspPos, ImVec2(inspPos.x + inspSize.x, inspPos.y + 3.0f),
                IM_COL32(138, 79, 255, 255), IM_COL32(242, 193, 78, 255),
                IM_COL32(242, 193, 78, 255), IM_COL32(138, 79, 255, 255)
            );
            ImGui::Dummy(ImVec2(0, 4.0f));

            // Inspector Header
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "MOD DETAILS");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 6.0f));

            ImGui::Text("Name:"); ImGui::SameLine(100.0f);
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.95f, 1.0f), "%s", mod.Name.c_str());

            ImGui::Text("Type:"); ImGui::SameLine(100.0f);
            ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "%s", BuildModTypeString(mod).c_str());

            ImGui::Text("Path:"); ImGui::SameLine(100.0f);
            ImGui::TextDisabled("%s", mod.ModFolderPath.c_str());

            ImGui::Dummy(ImVec2(0, 10.0f));
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 0.9f), "DESCRIPTION");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 6.0f));
            ImGui::TextWrapped("%s", mod.Description.empty() ? "No description provided." : mod.Description.c_str());

            ImGui::Dummy(ImVec2(0, 10.0f));
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 0.9f), "SETTINGS OVERRIDES");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 6.0f));

            bool hasSettingsIni = !mod.SettingsIniPath.empty() && std::filesystem::exists(mod.SettingsIniPath);
            if (hasSettingsIni) {
                ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 0.9f), "CONFIGURATION");
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 8));

                struct IniSection {
                    std::string Name;
                    std::unordered_map<std::string, std::string> KeyValues;
                };

                static std::unordered_map<std::string, std::vector<IniSection>> s_IniData;
                static std::unordered_map<std::string, bool> s_IniLoaded;
                static std::unordered_map<std::string, std::array<char, 256>> s_EditBuffers;

                std::string modKey = mod.ModFolderPath;

                if (!s_IniLoaded[modKey]) {
                    std::ifstream file(mod.SettingsIniPath);
                    std::vector<IniSection> sections;
                    IniSection current;
                    current.Name = "General";

                    if (file) {
                        std::string line;
                        while (std::getline(file, line)) {
                            size_t start = line.find_first_not_of(" \t\r\n");
                            if (start == std::string::npos) continue;
                            line.erase(0, start);
                            size_t end = line.find_last_not_of(" \t\r\n");
                            if (end != std::string::npos) line.erase(end + 1);

                            if (line.empty() || line[0] == ';' || line[0] == '#') continue;

                            if (line[0] == '[' && line.back() == ']') {
                                std::string sectionName = line.substr(1, line.size() - 2);
                                sectionName.erase(0, sectionName.find_first_not_of(" \t"));
                                sectionName.erase(sectionName.find_last_not_of(" \t") + 1);

                                if (!current.KeyValues.empty()) {
                                    sections.push_back(current);
                                }
                                current.Name = sectionName;
                                current.KeyValues.clear();
                            }
                            else {
                                size_t eq = line.find('=');
                                if (eq != std::string::npos) {
                                    std::string key = line.substr(0, eq);
                                    std::string value = line.substr(eq + 1);
                                    key.erase(key.find_last_not_of(" \t") + 1);
                                    value.erase(0, value.find_first_not_of(" \t"));
                                    current.KeyValues[key] = value;
                                }
                            }
                        }
                        if (!current.KeyValues.empty()) {
                            sections.push_back(current);
                        }
                    }
                    s_IniData[modKey] = sections;
                    s_IniLoaded[modKey] = true;
                    s_EditBuffers.clear();
                }

                auto& sections = s_IniData[modKey];
                bool changed = false;

                for (auto& section : sections) {
                    ImGui::SetWindowFontScale(1.15f);
                    ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "%s", section.Name.c_str());
                    ImGui::SetWindowFontScale(1.0f);

                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    float width = ImGui::GetContentRegionAvail().x;
                    dl->AddLine(pos, ImVec2(pos.x + width, pos.y), IM_COL32(242, 193, 78, 60), 1.0f);
                    ImGui::Dummy(ImVec2(0, 8));

                    ImGui::Indent(20.0f);
                    for (auto& kv : section.KeyValues) {
                        const std::string& key = kv.first;
                        std::string& value = kv.second;

                        std::string bufKey = section.Name + "|" + key;
                        auto& buf = s_EditBuffers[bufKey];
                        if (buf[0] == '\0') {
                            strncpy_s(buf.data(), buf.size(), value.c_str(), buf.size() - 1);
                        }

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%s", key.c_str());
                        ImGui::SameLine(150.0f);
                        ImGui::SetNextItemWidth(-1);

                        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.13f, 0.16f, 0.90f));
                        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.19f, 0.23f, 0.95f));
                        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.22f, 0.23f, 0.28f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.22f, 0.28f, 0.50f));
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

                        if (ImGui::InputText(("##" + bufKey).c_str(), buf.data(), buf.size())) {
                            value = buf.data();
                            changed = true;
                        }

                        ImGui::PopStyleVar(2);
                        ImGui::PopStyleColor(4);

                        ImGui::Dummy(ImVec2(0, 2));
                    }
                    ImGui::Unindent(20.0f);
                    ImGui::Dummy(ImVec2(0, 6));
                }

                if (changed) {
                    std::string newContent;
                    for (const auto& section : sections) {
                        newContent += "[" + section.Name + "]\n";
                        for (const auto& kv : section.KeyValues) {
                            newContent += kv.first + "=" + kv.second + "\n";
                        }
                        newContent += "\n";
                    }

                    std::ofstream file(mod.SettingsIniPath);
                    if (file) {
                        file << newContent;

                        if (mod.IsAssetMod) g_AppConfig.ModSystemDirty = true;
                        if (mod.IsDefMod)   g_AppConfig.DefSystemDirty = true;
                        if (mod.IsTngMod)   g_AppConfig.TngSystemDirty = true;
                        SaveConfig();
                    }
                }
            }
            else {
                ImGui::TextDisabled("No configurable .ini settings file found for this mod.");
            }

            ImGui::Dummy(ImVec2(0, 20.0f));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 6.0f));

            ImGui::BeginDisabled(mod.IsCoreMod);
            if (DrawAccentButton(mod.IsCoreMod ? "Delete Mod (unavailable for core mods)" : "Delete Mod", ImVec2(-1, 30),
                ImVec4(0.55f, 0.14f, 0.14f, 0.85f), ImVec4(0.80f, 0.22f, 0.22f, 1.00f))) {
                g_ModToDeleteIndex = s_SelectedModIndex;
                g_TriggerDeleteModPopup = true;
            }
            ImGui::EndDisabled();

            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    // --- DELETE CONFIRMATION ---
    if (g_TriggerDeleteModPopup) {
        ImGui::OpenPopup("ConfirmDeleteMod");
        g_TriggerDeleteModPopup = false;
    }

    // Modal Styling matched to Frontend
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.95f, 0.76f, 0.31f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("ConfirmDeleteMod", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        bool validTarget = g_ModToDeleteIndex >= 0 && g_ModToDeleteIndex < (int)ModManagerBackend::g_LoadedMods.size();

        ImDrawList* modalDraw = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

        // Outer Gold Glow Aura
        for (int i = 3; i >= 1; i--) {
            float expand = (float)i * 1.8f;
            float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
            ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
            modalDraw->AddRect(
                ImVec2(winPos.x - expand, winPos.y - expand),
                ImVec2(winMax.x + expand, winMax.y + expand),
                glowCol, 10.0f + expand, 0, 1.2f
            );
        }

        // Danger/Arcane Header Gradient Banner
        ImVec2 headerMin = winPos;
        ImVec2 headerMax = ImVec2(winMax.x, winPos.y + 58.0f);
        modalDraw->AddRectFilledMultiColor(
            headerMin, headerMax,
            IM_COL32(219, 68, 68, 40),   // Top-Left Red
            IM_COL32(242, 193, 78, 20),  // Top-Right Gold
            IM_COL32(0, 0, 0, 0),        // Bottom-Right
            IM_COL32(0, 0, 0, 0)         // Bottom-Left
        );

        if (g_TitleFont) ImGui::PushFont(g_TitleFont);
        ImGui::TextColored(ImVec4(0.92f, 0.40f, 0.40f, 1.0f), "DELETE MOD");
        if (g_TitleFont) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 4.0f));

        // Gold Accent Separator Line
        modalDraw->AddLine(
            ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
            ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
            IM_COL32(242, 193, 78, 110), 1.2f
        );
        ImGui::Dummy(ImVec2(0, 12.0f));

        if (validTarget) {
            ImGui::TextWrapped("Delete \"%s\" permanently?", ModManagerBackend::g_LoadedMods[g_ModToDeleteIndex].Name.c_str());
            ImGui::Dummy(ImVec2(0, 4.0f));
            ImGui::TextColored(ImVec4(0.95f, 0.65f, 0.30f, 1.0f), "This removes the mod's folder from disk.\nThis cannot be undone.");
        }
        else {
            ImGui::TextDisabled("Nothing selected to delete.");
        }

        ImGui::Dummy(ImVec2(0, 16.0f));

        ImGui::BeginDisabled(!validTarget);
        if (DrawAccentButton("Delete", ImVec2(150, 32), ImVec4(0.55f, 0.14f, 0.14f, 0.85f), ImVec4(0.80f, 0.22f, 0.22f, 1.00f))) {
            // ... (keep original deletion and compile trigger logic) ...

            bool wasDef = ModManagerBackend::g_LoadedMods[g_ModToDeleteIndex].IsDefMod;
            ModManagerBackend::DeleteMod(g_ModToDeleteIndex);

            g_AppConfig.ModSystemDirty = true;
            if (wasDef) g_AppConfig.DefSystemDirty = true;
            SaveConfig();

            if (s_SelectedModIndex == g_ModToDeleteIndex) s_SelectedModIndex = -1;
            else if (s_SelectedModIndex > g_ModToDeleteIndex) s_SelectedModIndex -= 1;

            g_ModToDeleteIndex = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0, 10.0f);
        if (DrawAccentButton("Cancel", ImVec2(120, 32), ImVec4(0.18f, 0.19f, 0.24f, 0.90f), ImVec4(0.28f, 0.30f, 0.38f, 1.00f))) {
            g_ModToDeleteIndex = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    ImGui::End();
    ImGui::PopStyleColor();
}

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
                            staged.Category = EModAssetCategory::BankEntry;
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

                ImGui::TableSetColumnIndex(0);
                if (e.Category == EModAssetCategory::BankEntry) ImGui::Text("%u", e.EntryID);
                else ImGui::TextDisabled("TEXT");

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

            // --- 1. HANDLE BANK ENTRIES (Existing) ---
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BANK_ENTRY_PAYLOAD")) {

                // 0. COMPILER TRIGGER: Force all unsaved entries to recompile to update definitions before pulling
                for (auto& b : g_OpenBanks) {
                    if (!b.StagedEntries.empty() || !b.ModifiedEntryData.empty()) {
                        if (b.Type == EBankType::Audio) SaveAudioBank(&b);
                        else SaveBigBank(&b);
                    }
                }

                int* data = (int*)payload->Data;
                int bIdx = data[0];
                int eIdx = data[1];

                if (bIdx >= 0 && bIdx < (int)g_OpenBanks.size()) {
                    auto& bank = g_OpenBanks[bIdx];
                    if (eIdx >= 0 && eIdx < (int)bank.Entries.size()) {
                        auto& entry = bank.Entries[eIdx];

                        std::string currentSubBank = (bank.ActiveSubBankIndex >= 0 && bank.ActiveSubBankIndex < bank.SubBanks.size()) ? bank.SubBanks[bank.ActiveSubBankIndex].Name : std::string("N/A");

                        // 1. THE LOGIC GATES: Prevent RAW Dependency Injection
                        std::string lowerBank = bank.FileName;
                        std::transform(lowerBank.begin(), lowerBank.end(), lowerBank.begin(), ::tolower);
                        std::string lowerSubBank = currentSubBank;
                        std::transform(lowerSubBank.begin(), lowerSubBank.end(), lowerSubBank.begin(), ::tolower);

                        // Gate A: Audio/LipSync Lock
                        bool isDisallowedNewAudio = false;
                        if (lowerBank == "dialogue.lut" || (lowerSubBank.find("lipsync_") != std::string::npos && lowerSubBank.find("_main") != std::string::npos && lowerSubBank.find("_2") == std::string::npos)) {
                            if (entry.ID > 12134) isDisallowedNewAudio = true;
                        }
                        else if (lowerBank == "dialogue2.lut" || lowerSubBank.find("_main_2") != std::string::npos) {
                            if (entry.ID > 1) isDisallowedNewAudio = true;
                        }
                        else if (lowerBank == "scriptdialogue.lut" || (lowerSubBank.find("_script") != std::string::npos && lowerSubBank.find("_2") == std::string::npos)) {
                            if (entry.ID > 5310) isDisallowedNewAudio = true;
                        }
                        else if (lowerBank == "scriptdialogue2.lut" || lowerSubBank.find("_script_2") != std::string::npos) {
                            if (entry.ID > 3060) isDisallowedNewAudio = true;
                        }

                        if (isDisallowedNewAudio) {
                            g_SuccessMessage = "Directly adding new audio/lipsync entries is locked!\nDrag the matching Text.big entry instead to auto-link dependencies.";
                            g_ShowSuccessPopup = true;
                            goto EndOfBankDropLogic;
                        }

                        // Gate B: Streaming Font GlyphData Lock
                        if (bank.Type == EBankType::Fonts && entry.Type == 2) {
                            g_SuccessMessage = "GlyphData cannot be added manually!\nDrag a Streaming Font entry instead to auto-link its GlyphData.";
                            g_ShowSuccessPopup = true;
                            goto EndOfBankDropLogic;
                        }

                        // Helper lambda to add entries to avoid duplicate code
                        auto addEntryToPackage = [&](uint32_t id, const std::string& name, int32_t type, EBankType bType, const std::string& bName, const std::string& sbName, const std::string& fullPath) {
                            bool exists = false;
                            for (const auto& existing : g_ModPackageEntries) {
                                if (existing.EntryID == id && existing.BankName == bName && existing.SubBankName == sbName) {
                                    exists = true; break;
                                }
                            }
                            if (!exists) {
                                StagedModPackageEntry staged;
                                staged.Category = EModAssetCategory::BankEntry; // Ensure it tracks as a binary asset
                                staged.EntryID = id; staged.EntryName = name; staged.EntryType = type;
                                staged.BankType = bType; staged.BankName = bName; staged.SubBankName = sbName;
                                staged.SourceFullPath = fullPath;
                                staged.TypeName = GetEntryTypeName(bType, type, bName);
                                g_ModPackageEntries.push_back(staged);
                            }
                            };

                        // Add the primary dragged entry
                        addEntryToPackage(entry.ID, entry.Name, entry.Type, bank.Type, bank.FileName, currentSubBank, bank.FullPath);

                        // 2. THE AUDIO CASCADE: Auto-Pull Linked Audio using Text Entry's SpeechBank
                        if (bank.Type == EBankType::Text && entry.Type == 0) {
                            std::string expectedSND = "SND_" + entry.Name;
                            std::transform(expectedSND.begin(), expectedSND.end(), expectedSND.begin(), ::toupper);

                            // --- Fetch the SpeechBank directly from the Text Entry Data ---
                            std::string speechBank = "";
                            if (bank.ModifiedEntryData.count(eIdx)) {
                                CTextParser p; p.Parse(bank.ModifiedEntryData[eIdx], 0);
                                if (p.IsParsed) speechBank = p.TextData.SpeechBank;
                            }
                            else {
                                bank.Stream->clear();
                                bank.Stream->seekg(entry.Offset, std::ios::beg);
                                std::vector<uint8_t> buf(entry.Size);
                                bank.Stream->read((char*)buf.data(), entry.Size);
                                CTextParser p; p.Parse(buf, 0);
                                if (p.IsParsed) speechBank = p.TextData.SpeechBank;
                            }

                            std::transform(speechBank.begin(), speechBank.end(), speechBank.begin(), ::tolower);

                            if (!speechBank.empty()) {
                                std::string targetHeader = "";
                                std::string targetLutName = "";
                                std::string targetSubBankKeyword = "";

                                // Map the specific .lug declaration to the true engine files
                                if (speechBank == "dialogue.lug") { targetHeader = "dialoguesnds.h"; targetLutName = "dialogue.lut"; targetSubBankKeyword = "_main"; }
                                else if (speechBank == "dialogue2.lug") { targetHeader = "dialoguesnds2.h"; targetLutName = "dialogue2.lut"; targetSubBankKeyword = "_main_2"; }
                                else if (speechBank == "scriptdialogue.lug") { targetHeader = "scriptdialoguesnds.h"; targetLutName = "scriptdialogue.lut"; targetSubBankKeyword = "_script"; }
                                else if (speechBank == "scriptdialogue2.lug") { targetHeader = "scriptdialoguesnds2.h"; targetLutName = "scriptdialogue2.lut"; targetSubBankKeyword = "_script_2"; }

                                if (!targetHeader.empty()) {
                                    uint32_t targetAudioID = 0;
                                    std::regex idRegex(expectedSND + R"(\s*=\s*(\d+))");

                                    // Fast targeted scan - we only look inside the specific header now
                                    for (const auto& enumEntry : g_DefWorkspace.AllEnums) {
                                        if (fs::path(enumEntry.FilePath).filename().string() == targetHeader) {
                                            std::smatch match;
                                            if (std::regex_search(enumEntry.FullContent, match, idRegex)) {
                                                targetAudioID = std::stoul(match[1].str());
                                            }
                                            break;
                                        }
                                    }

                                    if (targetAudioID > 0) {
                                        auto loadMissingMediaBank = [&](const std::string& bName) {
                                            bool found = false;
                                            for (auto& b : g_OpenBanks) {
                                                std::string lowerB = b.FileName;
                                                std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(), ::tolower);
                                                if (lowerB == bName) { found = true; break; }
                                            }
                                            if (!found) {
                                                const char* langs[] = { "English", "French", "Italian", "Chinese", "German", "Korean", "Japanese", "Spanish" };
                                                for (const char* l : langs) {
                                                    std::string p = g_AppConfig.GameRootPath + "\\Data\\Lang\\" + std::string(l) + "\\" + bName;
                                                    if (fs::exists(p)) { LoadBank(p); break; }
                                                }
                                            }
                                            };

                                        loadMissingMediaBank(targetLutName);
                                        loadMissingMediaBank("dialogue.big");

                                        for (auto& openBank : g_OpenBanks) {
                                            // Stage the .lut Audio Entry
                                            if (openBank.Type == EBankType::Audio && openBank.FileName.find(".lut") != std::string::npos) {
                                                std::string lowerB = openBank.FileName;
                                                std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(), ::tolower);
                                                if (lowerB == targetLutName) {
                                                    for (auto& depEntry : openBank.Entries) {
                                                        if (depEntry.ID == targetAudioID) {
                                                            addEntryToPackage(depEntry.ID, depEntry.Name, depEntry.Type, openBank.Type, openBank.FileName, "N/A", openBank.FullPath);
                                                        }
                                                    }
                                                }
                                            }
                                            // Stage the dialogue.big LipSync Entry
                                            else if (openBank.Type == EBankType::Dialogue) {
                                                int targetSbIdx = -1;
                                                for (int s = 0; s < openBank.SubBanks.size(); s++) {
                                                    std::string lowerSub = openBank.SubBanks[s].Name;
                                                    std::transform(lowerSub.begin(), lowerSub.end(), lowerSub.begin(), ::tolower);

                                                    // Ensure strict matching so _MAIN doesn't accidentally trigger _MAIN_2
                                                    if (targetSubBankKeyword == "_main" && lowerSub.find("_main_2") != std::string::npos) continue;
                                                    if (targetSubBankKeyword == "_script" && lowerSub.find("_script_2") != std::string::npos) continue;

                                                    if (lowerSub.find(targetSubBankKeyword) != std::string::npos) {
                                                        targetSbIdx = s; break;
                                                    }
                                                }

                                                if (targetSbIdx != -1) {
                                                    // CRITICAL: Force load the correct sub-bank into RAM before we scan it for packaging
                                                    if (openBank.ActiveSubBankIndex != targetSbIdx) {
                                                        LoadSubBankEntries(&openBank, targetSbIdx);
                                                    }

                                                    std::string depSubBank = openBank.SubBanks[openBank.ActiveSubBankIndex].Name;
                                                    for (auto& depEntry : openBank.Entries) {
                                                        if (depEntry.ID == targetAudioID) {
                                                            addEntryToPackage(depEntry.ID, depEntry.Name, depEntry.Type, openBank.Type, openBank.FileName, depSubBank, openBank.FullPath);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // 3. THE FONT CASCADE: Auto-Pull GlyphData for Streaming Fonts
                        if (bank.Type == EBankType::Fonts && lowerSubBank.find("streaming") != std::string::npos && entry.Type != 2) {
                            // Find the matching GlyphData (Type 2) in this exact subbank
                            for (auto& depEntry : bank.Entries) {
                                if (depEntry.Type == 2) {
                                    addEntryToPackage(depEntry.ID, depEntry.Name, depEntry.Type, bank.Type, bank.FileName, currentSubBank, bank.FullPath);
                                }
                            }
                        }
                    }
                }
            EndOfBankDropLogic:;
            }

            // --- 2. HANDLE DEFINITION ENTRIES ---
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DEF_ENTRY_PAYLOAD")) {
                DefDragPayload* data = (DefDragPayload*)payload->Data;
                auto& entry = g_DefWorkspace.CategorizedDefs[data->Category][data->EntryIndex];

                bool exists = false;
                for (const auto& existing : g_ModPackageEntries) {
                    if (existing.EntryName == entry.Name && existing.Category == EModAssetCategory::Definition) {
                        exists = true; break;
                    }
                }
                if (!exists) {
                    StagedModPackageEntry staged;
                    staged.Category = EModAssetCategory::Definition;
                    staged.EntryName = entry.Name;
                    staged.TypeName = "Definition";
                    staged.BankName = std::filesystem::path(entry.SourceFile).filename().string();
                    staged.SubBankName = data->Category;
                    staged.SourceFullPath = entry.SourceFile;
                    g_ModPackageEntries.push_back(staged);
                }
            }

            // --- 3. HANDLE HEADER ENTRIES ---
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HEADER_ENTRY_PAYLOAD")) {
                HeaderDragPayload* data = (HeaderDragPayload*)payload->Data;
                auto& entry = g_DefWorkspace.AllEnums[data->EnumIndex];

                std::vector<std::string> blockedEnums = {
                    "SNDS", "ESNDS", "EMeshType2", "EAnimType2", "ELipSync", "ELipSync2",
                    "ELipSync3", "ELipSync4", "EGuiGraphicBank", "EParticleEmitter",
                    "EFrontEndGraphicBank", "EEngineGraphic"
                };

                if (std::find(blockedEnums.begin(), blockedEnums.end(), entry.Name) != blockedEnums.end()) {
                    g_SuccessMessage = "This header is auto-generated by the asset builder and cannot be staged manually!";
                    g_ShowSuccessPopup = true;
                }
                else {

                    bool exists = false;
                    for (const auto& existing : g_ModPackageEntries) {
                        if (existing.EntryName == entry.Name && existing.Category == EModAssetCategory::Header) {
                            exists = true; break;
                        }
                    }
                    if (!exists) {
                        StagedModPackageEntry staged;
                        staged.Category = EModAssetCategory::Header;
                        staged.EntryName = entry.Name;
                        staged.TypeName = "C++ Header";
                        staged.BankName = std::filesystem::path(entry.FilePath).filename().string();
                        staged.SubBankName = "Defs";
                        staged.SourceFullPath = entry.FilePath;
                        g_ModPackageEntries.push_back(staged);
                    }
                }
            }

            // --- 4. HANDLE ANIMATION EVENT ENTRIES ---
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EVENT_ENTRY_PAYLOAD")) {
                EventDragPayload* data = (EventDragPayload*)payload->Data;
                EventFile* file = (data->FileType == 0) ? &g_EventWorkspace.SoundEvents : &g_EventWorkspace.GameEvents;
                auto& ev = file->Events[data->EventIndex];

                bool exists = false;
                for (const auto& existing : g_ModPackageEntries) {
                    if (existing.EntryName == ev.AnimName && existing.Category == EModAssetCategory::AnimationEvent) {
                        exists = true; break;
                    }
                }
                if (!exists) {
                    StagedModPackageEntry staged;
                    staged.Category = EModAssetCategory::AnimationEvent;
                    staged.EntryName = ev.AnimName;
                    staged.TypeName = (data->FileType == 0) ? "Sound Event" : "Game Event";
                    staged.BankName = file->FileName;
                    staged.SubBankName = "Misc";
                    staged.SourceFullPath = file->FilePath;
                    g_ModPackageEntries.push_back(staged);
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 60);
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Warning: Building the package will automatically compile and save all staged entries to your active banks.");

        bool canBuild = !g_ModPackageEntries.empty() && strlen(g_ModNameBuffer) > 0;

        ImGui::BeginDisabled(!canBuild);
        if (ImGui::Button("Build Mod Package", ImVec2(-1, 30))) {

            ModManagerCompiler::BuildPackageStructure(g_ModNameBuffer, g_ModPackageEntries);

            g_SuccessMessage = "Mod folder structure & files created successfully!";
            g_ShowSuccessPopup = true;
        }
        ImGui::EndDisabled();

        if (g_ShowSuccessPopup) {
            ImGui::OpenPopup("Success");
            g_ShowSuccessPopup = false;
        }

        if (ImGui::BeginPopupModal("Success", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", g_SuccessMessage.c_str());
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}