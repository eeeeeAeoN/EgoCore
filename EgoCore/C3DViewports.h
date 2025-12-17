#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include <algorithm>
#include <vector>

static float g_ViewRot[2] = { 0.5f, 0.5f }; 
static float g_ViewZoom = 1.0f;
static float g_ViewPan[2] = { 0.0f, 0.0f };
static bool g_ShowWireframe = true;
static bool g_ShowSolid = true;

struct RenderTri {
    ImVec2 p0, p1, p2;
    float zDepth;
    float lightIntensity;
};

inline ImVec4 ProjectPoint(const float* worldPos, const ImVec2& center) {
    float x = worldPos[0]; float y = worldPos[1]; float z = worldPos[2];

    float cosY = cosf(g_ViewRot[1]), sinY = sinf(g_ViewRot[1]);
    float cosX = cosf(g_ViewRot[0]), sinX = sinf(g_ViewRot[0]);

    // Rotate Y
    float x1 = x * cosY - z * sinY;
    float z1 = x * sinY + z * cosY;

    // Rotate X
    float y2 = y * cosX - z1 * sinX;
    float z2 = y * sinX + z1 * cosX;

    return ImVec4(center.x + x1 * g_ViewZoom, center.y - y2 * g_ViewZoom, z2, 0.0f);
}

inline void DrawCookedMeshViewport(const C3DMeshContent& mesh) {
    ImVec2 canvasSize = ImVec2(ImGui::GetContentRegionAvail().x, 400);
    if (canvasSize.x < 50) canvasSize.x = 50;

    ImGui::InvisibleButton("ViewportCanvas", canvasSize);
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilledMultiColor(p0, p1, IM_COL32(50, 50, 60, 255), IM_COL32(50, 50, 60, 255), IM_COL32(20, 20, 25, 255), IM_COL32(20, 20, 25, 255));
    draw_list->AddRect(p0, p1, IM_COL32(100, 100, 100, 255));

    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) { g_ViewRot[1] += ImGui::GetIO().MouseDelta.x * 0.01f; g_ViewRot[0] += ImGui::GetIO().MouseDelta.y * 0.01f; }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) { g_ViewPan[0] += ImGui::GetIO().MouseDelta.x; g_ViewPan[1] += ImGui::GetIO().MouseDelta.y; }
        g_ViewZoom += ImGui::GetIO().MouseWheel * 0.1f * g_ViewZoom;
        if (g_ViewZoom < 0.001f) g_ViewZoom = 0.001f;
    }

    draw_list->PushClipRect(p0, p1, true);
    ImVec2 center = ImVec2(p0.x + canvasSize.x * 0.5f + g_ViewPan[0], p0.y + canvasSize.y * 0.5f + g_ViewPan[1]);

    // Grid Scale
    float meshRadius = (mesh.BoundingSphereRadius > 1.0f) ? mesh.BoundingSphereRadius : 100.0f;
    float gridStep = powf(10.0f, floorf(log10f(meshRadius)));
    if (gridStep < 10.0f) gridStep = 10.0f;

    static bool fitOnce = true;
    if (fitOnce && mesh.BoundingSphereRadius > 0) { g_ViewZoom = (canvasSize.y * 0.4f) / mesh.BoundingSphereRadius; fitOnce = false; }

    // Draw Grid
    ImU32 gridCol = IM_COL32(255, 255, 255, 30);
    int gridLines = 10;
    for (int i = -gridLines; i <= gridLines; i++) {
        float d = i * gridStep; float ext = gridLines * gridStep;
        float pA[3] = { d, 0, -ext }, pB[3] = { d, 0, ext }, pC[3] = { -ext, 0, d }, pD[3] = { ext, 0, d };
        ImVec4 a1 = ProjectPoint(pA, center); ImVec4 b1 = ProjectPoint(pB, center);
        ImVec4 a2 = ProjectPoint(pC, center); ImVec4 b2 = ProjectPoint(pD, center);
        draw_list->AddLine(ImVec2(a1.x, a1.y), ImVec2(b1.x, b1.y), gridCol);
        draw_list->AddLine(ImVec2(a2.x, a2.y), ImVec2(b2.x, b2.y), gridCol);
    }
    float o[3] = { 0,0,0 }, ax[3] = { gridStep,0,0 }, ay[3] = { 0,gridStep,0 }, az[3] = { 0,0,gridStep };
    ImVec4 po = ProjectPoint(o, center), pax = ProjectPoint(ax, center), pay = ProjectPoint(ay, center), paz = ProjectPoint(az, center);
    draw_list->AddLine(ImVec2(po.x, po.y), ImVec2(pax.x, pax.y), IM_COL32(255, 50, 50, 255), 2.0f);
    draw_list->AddLine(ImVec2(po.x, po.y), ImVec2(pay.x, pay.y), IM_COL32(50, 255, 50, 255), 2.0f);
    draw_list->AddLine(ImVec2(po.x, po.y), ImVec2(paz.x, paz.y), IM_COL32(50, 50, 255, 255), 2.0f);

    std::vector<RenderTri> renderList;
    int maxPrims = (mesh.Primitives.size() > 50) ? 50 : (int)mesh.Primitives.size();

    for (int i = 0; i < maxPrims; i++) {
        const auto& prim = mesh.Primitives[i];
        if (prim.VertexBuffer.empty() || prim.IndexBuffer.empty()) continue;

        auto AddTri = [&](uint16_t i0, uint16_t i1, uint16_t i2) {
            if (i0 == i1 || i1 == i2 || i0 == i2) return;
            ImVec4 vProj[3]; float world[3][3]; uint16_t idx[3] = { i0, i1, i2 };

            for (int k = 0; k < 3; k++) {
                uint32_t offset = idx[k] * prim.VertexStride;
                if (offset + 12 > prim.VertexBuffer.size()) return;
                if (prim.IsCompressed) {
                    uint32_t packed = *(uint32_t*)(prim.VertexBuffer.data() + offset);
                    UnpackPOSPACKED3(packed, prim.Compression.Scale, prim.Compression.Offset, world[k][0], world[k][1], world[k][2]);
                }
                else {
                    const float* raw = (const float*)(prim.VertexBuffer.data() + offset);
                    world[k][0] = raw[0]; world[k][1] = raw[1]; world[k][2] = raw[2];
                }
                vProj[k] = ProjectPoint(world[k], center);
            }

            float ax = world[1][0] - world[0][0], ay = world[1][1] - world[0][1], az = world[1][2] - world[0][2];
            float bx = world[2][0] - world[0][0], by = world[2][1] - world[0][1], bz = world[2][2] - world[0][2];
            float nx = ay * bz - az * by, ny = az * bx - ax * bz, nz = ax * by - ay * bx;
            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            if (len > 0.0f) { nx /= len; ny /= len; nz /= len; }
            float dot = nx * 0.577f + ny * 0.577f + nz * (-0.577f);
            if (dot < 0.25f) dot = 0.25f; if (dot > 1.0f) dot = 1.0f;

            RenderTri tri; tri.p0 = ImVec2(vProj[0].x, vProj[0].y); tri.p1 = ImVec2(vProj[1].x, vProj[1].y); tri.p2 = ImVec2(vProj[2].x, vProj[2].y);
            tri.zDepth = (vProj[0].z + vProj[1].z + vProj[2].z) / 3.0f; tri.lightIntensity = dot;
            renderList.push_back(tri);
            };

        bool hasBlocks = !prim.StaticBlocks.empty() || !prim.AnimatedBlocks.empty();
        if (hasBlocks) {
            for (const auto& b : prim.StaticBlocks) {
                if (b.IsStrip) for (uint32_t k = 0; k < b.PrimitiveCount; k++) { if (k % 2) AddTri(prim.IndexBuffer[b.StartIndex + k], prim.IndexBuffer[b.StartIndex + k + 2], prim.IndexBuffer[b.StartIndex + k + 1]); else AddTri(prim.IndexBuffer[b.StartIndex + k], prim.IndexBuffer[b.StartIndex + k + 1], prim.IndexBuffer[b.StartIndex + k + 2]); }
                else for (uint32_t k = 0; k < b.PrimitiveCount; k++) AddTri(prim.IndexBuffer[b.StartIndex + k * 3], prim.IndexBuffer[b.StartIndex + k * 3 + 1], prim.IndexBuffer[b.StartIndex + k * 3 + 2]);
            }
            for (const auto& b : prim.AnimatedBlocks) {
                if (b.IsStrip) for (uint32_t k = 0; k < b.PrimitiveCount; k++) { if (k % 2) AddTri(prim.IndexBuffer[b.StartIndex + k], prim.IndexBuffer[b.StartIndex + k + 2], prim.IndexBuffer[b.StartIndex + k + 1]); else AddTri(prim.IndexBuffer[b.StartIndex + k], prim.IndexBuffer[b.StartIndex + k + 1], prim.IndexBuffer[b.StartIndex + k + 2]); }
                else for (uint32_t k = 0; k < b.PrimitiveCount; k++) AddTri(prim.IndexBuffer[b.StartIndex + k * 3], prim.IndexBuffer[b.StartIndex + k * 3 + 1], prim.IndexBuffer[b.StartIndex + k * 3 + 2]);
            }
        }
        else {
            size_t numTriangles = prim.IndexBuffer.size() / 3;
            for (size_t t = 0; t < numTriangles; t++) AddTri(prim.IndexBuffer[t * 3], prim.IndexBuffer[t * 3 + 1], prim.IndexBuffer[t * 3 + 2]);
        }
    }

    std::sort(renderList.begin(), renderList.end(), [](const RenderTri& a, const RenderTri& b) { return a.zDepth < b.zDepth; });

    for (const auto& tri : renderList) {
        if (g_ShowSolid) {
            int r = (int)(100 * tri.lightIntensity); int g = (int)(140 * tri.lightIntensity); int b = (int)(200 * tri.lightIntensity);
            draw_list->AddTriangleFilled(tri.p0, tri.p1, tri.p2, IM_COL32(r, g, b, 255));
        }
        if (g_ShowWireframe) draw_list->AddTriangle(tri.p0, tri.p1, tri.p2, IM_COL32(0, 0, 0, 60), 1.0f);
    }

    draw_list->PopClipRect();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 40); ImGui::Indent(10);
    if (ImGui::Button("Reset View")) { g_ViewRot[0] = 0.5f; g_ViewRot[1] = 0.5f; fitOnce = true; g_ViewPan[0] = 0; g_ViewPan[1] = 0; }
    ImGui::SameLine(); ImGui::Checkbox("Wire", &g_ShowWireframe); ImGui::SameLine(); ImGui::Checkbox("Solid", &g_ShowSolid);
    ImGui::Unindent(10);
}

inline void DrawBBMMeshViewport(const CBBMParser& bbm) {
    ImVec2 canvasSize = ImVec2(ImGui::GetContentRegionAvail().x, 400);
    if (canvasSize.x < 50) canvasSize.x = 50;
    ImGui::InvisibleButton("BBMCanvas", canvasSize);
    ImVec2 p0 = ImGui::GetItemRectMin(); ImVec2 p1 = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilledMultiColor(p0, p1, IM_COL32(50, 60, 50, 255), IM_COL32(50, 60, 50, 255), IM_COL32(20, 25, 20, 255), IM_COL32(20, 25, 20, 255));
    draw_list->AddRect(p0, p1, IM_COL32(100, 100, 100, 255));

    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) { g_ViewRot[1] += ImGui::GetIO().MouseDelta.x * 0.01f; g_ViewRot[0] += ImGui::GetIO().MouseDelta.y * 0.01f; }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) { g_ViewPan[0] += ImGui::GetIO().MouseDelta.x; g_ViewPan[1] += ImGui::GetIO().MouseDelta.y; }
        g_ViewZoom += ImGui::GetIO().MouseWheel * 0.1f * g_ViewZoom; if (g_ViewZoom < 0.001f) g_ViewZoom = 0.001f;
    }
    draw_list->PushClipRect(p0, p1, true);
    ImVec2 center = ImVec2(p0.x + canvasSize.x * 0.5f + g_ViewPan[0], p0.y + canvasSize.y * 0.5f + g_ViewPan[1]);

    static float bbmRadius = 10.0f; static bool fitOnceBBM = true;
    if (fitOnceBBM && !bbm.ParsedVertices.empty()) {
        float min[3] = { 1e9,1e9,1e9 }, max[3] = { -1e9,-1e9,-1e9 };
        for (const auto& v : bbm.ParsedVertices) {
            if (v.Position.x < min[0]) min[0] = v.Position.x; if (v.Position.x > max[0]) max[0] = v.Position.x;
            if (v.Position.y < min[1]) min[1] = v.Position.y; if (v.Position.y > max[1]) max[1] = v.Position.y;
            if (v.Position.z < min[2]) min[2] = v.Position.z; if (v.Position.z > max[2]) max[2] = v.Position.z;
        }
        float dx = max[0] - min[0], dy = max[1] - min[1], dz = max[2] - min[2];
        bbmRadius = sqrtf(dx * dx + dy * dy + dz * dz) * 0.5f; if (bbmRadius < 1.0f) bbmRadius = 1.0f;
        g_ViewZoom = (canvasSize.y * 0.4f) / bbmRadius; fitOnceBBM = false;
    }
    float gridStep = powf(10.0f, floorf(log10f(bbmRadius))); if (gridStep < 1.0f) gridStep = 1.0f;

    // Draw Grid (Same logic as above)
    ImU32 gridCol = IM_COL32(255, 255, 255, 30);
    for (int i = -10; i <= 10; i++) {
        float d = i * gridStep; float ext = 10 * gridStep;
        float pA[3] = { d, 0, -ext }, pB[3] = { d, 0, ext }, pC[3] = { -ext, 0, d }, pD[3] = { ext, 0, d };
        ImVec4 a1 = ProjectPoint(pA, center); ImVec4 b1 = ProjectPoint(pB, center);
        ImVec4 a2 = ProjectPoint(pC, center); ImVec4 b2 = ProjectPoint(pD, center);
        draw_list->AddLine(ImVec2(a1.x, a1.y), ImVec2(b1.x, b1.y), gridCol);
        draw_list->AddLine(ImVec2(a2.x, a2.y), ImVec2(b2.x, b2.y), gridCol);
    }

    std::vector<RenderTri> renderList;
    if (!bbm.ParsedIndices.empty()) {
        size_t numTriangles = bbm.ParsedIndices.size() / 3;
        size_t step = (numTriangles > 5000) ? 2 : 1;
        for (size_t t = 0; t < numTriangles; t += step) {
            uint16_t idx[3] = { bbm.ParsedIndices[t * 3], bbm.ParsedIndices[t * 3 + 1], bbm.ParsedIndices[t * 3 + 2] };
            if (idx[0] >= bbm.ParsedVertices.size() || idx[1] >= bbm.ParsedVertices.size() || idx[2] >= bbm.ParsedVertices.size()) continue;

            float world[3][3]; ImVec4 vProj[3];
            for (int k = 0; k < 3; k++) {
                const auto& v = bbm.ParsedVertices[idx[k]];
                world[k][0] = v.Position.x; world[k][1] = v.Position.y; world[k][2] = v.Position.z;
                vProj[k] = ProjectPoint(world[k], center);
            }
            float ax = world[1][0] - world[0][0], ay = world[1][1] - world[0][1], az = world[1][2] - world[0][2];
            float bx = world[2][0] - world[0][0], by = world[2][1] - world[0][1], bz = world[2][2] - world[0][2];
            float nx = ay * bz - az * by, ny = az * bx - ax * bz, nz = ax * by - ay * bx;
            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            if (len > 0.0f) { nx /= len; ny /= len; nz /= len; }
            float dot = nx * 0.577f + ny * 0.577f + nz * (-0.577f);
            if (dot < 0.25f) dot = 0.25f; if (dot > 1.0f) dot = 1.0f;

            RenderTri tri; tri.p0 = ImVec2(vProj[0].x, vProj[0].y); tri.p1 = ImVec2(vProj[1].x, vProj[1].y); tri.p2 = ImVec2(vProj[2].x, vProj[2].y);
            tri.zDepth = (vProj[0].z + vProj[1].z + vProj[2].z) / 3.0f; tri.lightIntensity = dot;
            renderList.push_back(tri);
        }
    }
    std::sort(renderList.begin(), renderList.end(), [](const RenderTri& a, const RenderTri& b) { return a.zDepth < b.zDepth; });

    for (const auto& tri : renderList) {
        if (g_ShowSolid) {
            int r = (int)(100 * tri.lightIntensity); int g = (int)(200 * tri.lightIntensity); int b = (int)(100 * tri.lightIntensity);
            draw_list->AddTriangleFilled(tri.p0, tri.p1, tri.p2, IM_COL32(r, g, b, 255));
        }
        if (g_ShowWireframe) draw_list->AddTriangle(tri.p0, tri.p1, tri.p2, IM_COL32(0, 0, 0, 60), 1.0f);
    }
    draw_list->PopClipRect();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 40); ImGui::Indent(10);
    if (ImGui::Button("Reset View##BBM")) { g_ViewRot[0] = 0.5f; g_ViewRot[1] = 0.5f; fitOnceBBM = true; g_ViewPan[0] = 0; g_ViewPan[1] = 0; }
    ImGui::SameLine(); ImGui::Checkbox("Wire##BBM", &g_ShowWireframe); ImGui::SameLine(); ImGui::Checkbox("Solid##BBM", &g_ShowSolid);
    ImGui::Unindent(10);
}