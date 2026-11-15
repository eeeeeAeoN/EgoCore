#pragma once
#include "imgui.h"
#include "MeshRenderer.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <directxmath.h>

using namespace DirectX;

enum class EGizmoTargetKind {
    None = 0,
    BBMHelper,
    BBMDummy,
    MeshHelper,
    MeshDummy,
    MeshGenerator,
    Bone
};

struct MeshGizmoState {
    EGizmoTargetKind Kind = EGizmoTargetKind::None;
    int Index = -1;
    int ActiveAxis = -1; // -1 = none, 0 = X, 1 = Y, 2 = Z, 3 = Uniform center
    int HoverAxis = -1;

    float DragStartT = 0.0f;
    float DragWorldLen = 1.0f;
    ImVec2 DragStartMouse = { 0.0f, 0.0f };
    XMFLOAT3 DragOriginWorld = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 DragAxesWorld[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
    XMFLOAT3 DragStartPos = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 DragStartScale = { 1.0f, 1.0f, 1.0f };
    bool DragIsPhysics = false;

    void ClearSelection() {
        Kind = EGizmoTargetKind::None;
        Index = -1;
        ActiveAxis = -1;
        HoverAxis = -1;
    }

    bool HasSelection() const {
        return Kind != EGizmoTargetKind::None && Index >= 0;
    }
};

namespace MeshGizmo {

    inline float DistPointToSegment2D(const ImVec2& p, const ImVec2& a, const ImVec2& b) {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float lenSq = dx * dx + dy * dy;
        if (lenSq < 1e-6f) {
            float px = p.x - a.x;
            float py = p.y - a.y;
            return sqrtf(px * px + py * py);
        }
        float t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / lenSq;
        t = std::clamp(t, 0.0f, 1.0f);
        float projX = a.x + t * dx;
        float projY = a.y + t * dy;
        float ex = p.x - projX;
        float ey = p.y - projY;
        return sqrtf(ex * ex + ey * ey);
    }

    inline bool ClosestTOnLineToRay(XMVECTOR lineOrigin, XMVECTOR lineDir, XMVECTOR rayOrigin, XMVECTOR rayDir, float& outT, float& outAbsDot) {
        XMVECTOR d0 = XMVector3Normalize(lineDir);
        XMVECTOR d1 = XMVector3Normalize(rayDir);
        XMVECTOR r = XMVectorSubtract(lineOrigin, rayOrigin);

        float b = XMVectorGetX(XMVector3Dot(d0, d1));
        outAbsDot = fabsf(b);
        float denom = 1.0f - b * b;
        if (denom < 1e-4f) return false;

        float d = XMVectorGetX(XMVector3Dot(d0, r));
        float e = XMVectorGetX(XMVector3Dot(d1, r));
        outT = (b * e - d) / denom;
        return std::isfinite(outT);
    }

    inline float ComputeGizmoWorldLength(const MeshRenderer& renderer, const XMFLOAT3& worldPos, float screenH, bool isPhysics) {
        if (screenH <= 1.0f) return 0.5f;
        XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, -renderer.CamDist, 0), XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 1, 0, 0));
        XMMATRIX world = XMMatrixTranslation(renderer.CenterOffset.x, renderer.CenterOffset.y, renderer.CenterOffset.z) *
            XMMatrixRotationX(renderer.CamRotX) * XMMatrixRotationY(renderer.CamRotY) *
            XMMatrixTranslation(renderer.CamPan.x, renderer.CamPan.y, 0);
        if (isPhysics) {
            world = XMMatrixRotationX(-XM_PIDIV2) * world;
        }
        XMVECTOR v = XMVector3TransformCoord(XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.0f), world * view);
        float z = fabsf(XMVectorGetZ(v));
        if (!std::isfinite(z) || z < 0.1f) z = renderer.CamDist;
        // 2 * tan(45 deg / 2) = 0.828427f
        return (std::max)(0.05f, z * 0.828427f * (72.0f / screenH));
    }

    inline ImVec2 Normalize2D(const ImVec2& v) {
        float l = sqrtf(v.x * v.x + v.y * v.y);
        if (l < 1e-5f) return ImVec2(1.0f, 0.0f);
        return ImVec2(v.x / l, v.y / l);
    }

    inline float ComputeAxisDragDelta(
        const MeshRenderer& renderer,
        const MeshGizmoState& state,
        int axis,
        const ImVec2& pMin,
        float vw,
        float vh,
        const ImVec2& mousePos
    ) {
        XMVECTOR origin = XMLoadFloat3(&state.DragOriginWorld);
        XMVECTOR axisDir = XMVector3Normalize(XMLoadFloat3(&state.DragAxesWorld[axis]));

        XMVECTOR rayOrig, rayDir;
        float localX = mousePos.x - pMin.x;
        float localY = mousePos.y - pMin.y;
        if (renderer.UnprojectRay(localX, localY, vw, vh, rayOrig, rayDir, state.DragIsPhysics)) {
            float curT = 0.0f, absDot = 0.0f;
            if (ClosestTOnLineToRay(origin, axisDir, rayOrig, rayDir, curT, absDot) && absDot < 0.96f) {
                return curT - state.DragStartT;
            }
        }

        // Screen-space fallback when looking almost directly along the axis
        MeshRenderer& mutableRenderer = const_cast<MeshRenderer&>(renderer);
        ImVec2 s0, s1;
        XMFLOAT3 tip(
            state.DragOriginWorld.x + state.DragAxesWorld[axis].x * state.DragWorldLen,
            state.DragOriginWorld.y + state.DragAxesWorld[axis].y * state.DragWorldLen,
            state.DragOriginWorld.z + state.DragAxesWorld[axis].z * state.DragWorldLen
        );
        if (mutableRenderer.ProjectToScreen(state.DragOriginWorld, s0, vw, vh, state.DragIsPhysics) &&
            mutableRenderer.ProjectToScreen(tip, s1, vw, vh, state.DragIsPhysics)) {
            ImVec2 d(s1.x - s0.x, s1.y - s0.y);
            float pxLen = sqrtf(d.x * d.x + d.y * d.y);
            if (pxLen > 2.0f) {
                ImVec2 n(d.x / pxLen, d.y / pxLen);
                ImVec2 mouseDelta(mousePos.x - state.DragStartMouse.x, mousePos.y - state.DragStartMouse.y);
                float pxProj = mouseDelta.x * n.x + mouseDelta.y * n.y;
                return (pxProj / pxLen) * state.DragWorldLen;
            }
        }
        return 0.0f;
    }

    inline bool DrawAndUpdateAxisGizmo(
        ImDrawList* dl,
        MeshRenderer& renderer,
        const XMFLOAT3& originWorld,
        const XMFLOAT3 axesWorld[3],
        const ImVec2& pMin,
        float vw,
        float vh,
        bool isPhysics,
        bool isScaleMode,
        bool allowViewportInput,
        MeshGizmoState& state,
        const char* statusText = nullptr
    ) {
        bool justStartedDrag = false;
        ImVec2 scrOrigin;
        if (!renderer.ProjectToScreen(originWorld, scrOrigin, vw, vh, isPhysics)) {
            return false;
        }
        scrOrigin.x += pMin.x;
        scrOrigin.y += pMin.y;

        float worldLen = ComputeGizmoWorldLength(renderer, originWorld, vh, isPhysics);
        ImVec2 scrTips[3];
        bool tipVisible[3] = { false, false, false };

        for (int a = 0; a < 3; ++a) {
            XMFLOAT3 tipWorld(
                originWorld.x + axesWorld[a].x * worldLen,
                originWorld.y + axesWorld[a].y * worldLen,
                originWorld.z + axesWorld[a].z * worldLen
            );
            if (renderer.ProjectToScreen(tipWorld, scrTips[a], vw, vh, isPhysics)) {
                scrTips[a].x += pMin.x;
                scrTips[a].y += pMin.y;
                tipVisible[a] = true;
            }
        }

        ImVec2 mousePos = ImGui::GetMousePos();
        state.HoverAxis = -1;

        if (state.ActiveAxis == -1 && allowViewportInput) {
            // Check center handle first in scale mode
            if (isScaleMode) {
                float dx = mousePos.x - scrOrigin.x;
                float dy = mousePos.y - scrOrigin.y;
                if ((dx * dx + dy * dy) <= 9.0f * 9.0f) {
                    state.HoverAxis = 3;
                }
            }
            if (state.HoverAxis == -1) {
                float bestDist = 9.5f;
                for (int a = 0; a < 3; ++a) {
                    if (!tipVisible[a]) continue;
                    float d = DistPointToSegment2D(mousePos, scrOrigin, scrTips[a]);
                    if (d < bestDist) {
                        bestDist = d;
                        state.HoverAxis = a;
                    }
                }
            }
        }

        static const ImU32 baseColors[3] = {
            IM_COL32(235, 70, 70, 255),
            IM_COL32(75, 220, 75, 255),
            IM_COL32(75, 145, 255, 255)
        };
        static const char* axisLabels[3] = { "X", "Y", "Z" };

        for (int a = 0; a < 3; ++a) {
            if (!tipVisible[a]) continue;
            bool highlighted = (state.ActiveAxis == a) || (state.ActiveAxis == -1 && state.HoverAxis == a) || (state.ActiveAxis == 3);
            ImU32 col = highlighted ? IM_COL32(255, 235, 80, 255) : baseColors[a];
            float thickness = highlighted ? 3.5f : 2.5f;

            dl->AddLine(scrOrigin, scrTips[a], col, thickness);

            ImVec2 dir = Normalize2D(ImVec2(scrTips[a].x - scrOrigin.x, scrTips[a].y - scrOrigin.y));
            ImVec2 perp(-dir.y, dir.x);

            if (isScaleMode) {
                float r = highlighted ? 5.5f : 4.5f;
                dl->AddRectFilled(
                    ImVec2(scrTips[a].x - r, scrTips[a].y - r),
                    ImVec2(scrTips[a].x + r, scrTips[a].y + r),
                    col
                );
            }
            else {
                float headLen = highlighted ? 12.0f : 10.0f;
                float headHalfW = highlighted ? 5.5f : 4.5f;
                ImVec2 basePt(scrTips[a].x - dir.x * headLen, scrTips[a].y - dir.y * headLen);
                ImVec2 p1(basePt.x + perp.x * headHalfW, basePt.y + perp.y * headHalfW);
                ImVec2 p2(basePt.x - perp.x * headHalfW, basePt.y - perp.y * headHalfW);
                dl->AddTriangleFilled(scrTips[a], p1, p2, col);
            }

            ImVec2 lblPos(scrTips[a].x + dir.x * 8.0f - 4.0f, scrTips[a].y + dir.y * 8.0f - 7.0f);
            dl->AddText(lblPos, col, axisLabels[a]);
        }

        if (isScaleMode) {
            bool centerHl = (state.ActiveAxis == 3) || (state.ActiveAxis == -1 && state.HoverAxis == 3);
            ImU32 cCol = centerHl ? IM_COL32(255, 235, 80, 255) : IM_COL32(240, 240, 240, 230);
            dl->AddRectFilled(ImVec2(scrOrigin.x - 5.0f, scrOrigin.y - 5.0f), ImVec2(scrOrigin.x + 5.0f, scrOrigin.y + 5.0f), cCol);
            dl->AddRect(ImVec2(scrOrigin.x - 5.0f, scrOrigin.y - 5.0f), ImVec2(scrOrigin.x + 5.0f, scrOrigin.y + 5.0f), IM_COL32(20, 20, 20, 255));
        }
        else {
            dl->AddCircleFilled(scrOrigin, 4.5f, IM_COL32(255, 255, 255, 235));
            dl->AddCircle(scrOrigin, 4.5f, IM_COL32(20, 20, 20, 255));
        }

        if (statusText && statusText[0] != '\0') {
            ImVec2 textSize = ImGui::CalcTextSize(statusText);
            ImVec2 boxMin(scrOrigin.x + 14.0f, scrOrigin.y - textSize.y - 12.0f);
            ImVec2 boxMax(boxMin.x + textSize.x + 10.0f, boxMin.y + textSize.y + 6.0f);
            dl->AddRectFilled(boxMin, boxMax, IM_COL32(18, 18, 22, 215), 4.0f);
            dl->AddRect(boxMin, boxMax, IM_COL32(255, 215, 90, 200), 4.0f);
            dl->AddText(ImVec2(boxMin.x + 5.0f, boxMin.y + 3.0f), IM_COL32(255, 245, 200, 255), statusText);
        }

        // Start drag when an axis/center handle is clicked
        if (allowViewportInput && state.ActiveAxis == -1 && state.HoverAxis != -1 && ImGui::IsMouseClicked(0)) {
            state.ActiveAxis = state.HoverAxis;
            state.DragStartMouse = mousePos;
            state.DragOriginWorld = originWorld;
            state.DragWorldLen = worldLen;
            state.DragIsPhysics = isPhysics;
            for (int a = 0; a < 3; ++a) state.DragAxesWorld[a] = axesWorld[a];

            if (state.ActiveAxis >= 0 && state.ActiveAxis < 3) {
                XMVECTOR rayOrig, rayDir;
                if (renderer.UnprojectRay(mousePos.x - pMin.x, mousePos.y - pMin.y, vw, vh, rayOrig, rayDir, isPhysics)) {
                    float t0 = 0.0f, dot0 = 0.0f;
                    if (ClosestTOnLineToRay(XMLoadFloat3(&originWorld), XMLoadFloat3(&axesWorld[state.ActiveAxis]), rayOrig, rayDir, t0, dot0)) {
                        state.DragStartT = t0;
                    }
                    else {
                        state.DragStartT = 0.0f;
                    }
                }
            }
            justStartedDrag = true;
        }

        if (state.ActiveAxis != -1) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        return justStartedDrag;
    }

}
