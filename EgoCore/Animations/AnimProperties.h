#pragma once
#include "imgui.h"
#include "AnimParser.h"
#include "GltfAnimImporter.h"
#include "FileDialogs.h"
#include "MeshRenderer.h"
#include "BankBackend.h" 
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

extern struct C3DMeshContent g_ActiveMeshContent;
extern std::string g_BankStatus;
extern ID3D11Device* g_pd3dDevice;
extern ImTextureID g_PlayTexture;
extern ImTextureID g_PauseTexture;
extern ImTextureID g_StopTexture;
extern ImTextureID g_LoopTexture;
extern ImTextureID g_ImportTexture;
extern ImTextureID g_ExportTexture;
extern ImTextureID g_ChangeTexture;


static const ImVec4 kChangeIconTint = ImVec4(0.95f, 0.82f, 0.45f, 1.0f);

static MeshRenderer g_StandaloneRenderer;
static C3DMeshContent g_StandaloneMesh;
static bool g_StandaloneMeshLoaded = false;
static bool g_StandaloneUploadNeeded = false;
static bool g_ShowStandaloneMeshPicker = false;
static float g_StandaloneTime = 0.0f;
static bool g_StandalonePlaying = false;
static bool g_StandaloneLoop = true;
static bool g_StandaloneShowSkeleton = false;
static bool g_StandaloneShowMovementVector = false;
static std::vector<XMMATRIX> g_StandaloneBoneMats;
static std::vector<XMMATRIX> g_StandaloneGlobalMats;
static bool g_ShowStandaloneExportPopup = false;
static bool g_StandaloneExportTextures = true;
static bool g_StandaloneExportAnimation = true;

inline void UpdateStandaloneBones(const C3DAnimationInfo& anim, const C3DMeshContent& mesh, float time, int animType) {
    int boneCount = mesh.BoneCount;
    g_StandaloneBoneMats.resize(boneCount);
    g_StandaloneGlobalMats.resize(boneCount);
    if (boneCount == 0) return;

    std::vector<XMMATRIX> ibm(boneCount), bindGlobal(boneCount), bindLocal(boneCount);
    for (int i = 0; i < boneCount; i++) {
        if ((i + 1) * 64 <= mesh.BoneTransformsRaw.size()) {
            float* raw = (float*)(mesh.BoneTransformsRaw.data() + i * 64);
            XMMATRIX rawMatrix = XMMATRIX(raw);
            rawMatrix.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
            ibm[i] = XMMatrixTranspose(rawMatrix);
            bindGlobal[i] = XMMatrixInverse(nullptr, ibm[i]);
        }
        else {
            ibm[i] = XMMatrixIdentity(); bindGlobal[i] = XMMatrixIdentity();
        }
    }
    for (int i = 0; i < boneCount; i++) {
        int p = mesh.Bones[i].ParentIndex;
        if (p == -1 || p >= boneCount) bindLocal[i] = bindGlobal[i];
        else bindLocal[i] = XMMatrixMultiply(bindGlobal[i], ibm[p]);
    }

    std::vector<XMMATRIX> localTransforms(boneCount);
    auto cleanName = [](const std::string& str) { std::string res; for (char c : str) if (isalnum(c)) res += tolower(c); return res; };

    for (int i = 0; i < boneCount; i++) {
        bool hasAnim = false;
        std::string targetBoneName = i < mesh.BoneNames.size() ? mesh.BoneNames[i] : "";
        std::string cleanTarget = cleanName(targetBoneName);
        if (!cleanTarget.empty()) {
            for (const auto& track : anim.Tracks) {
                if (cleanTarget == cleanName(track.BoneName)) {
                    if (track.FrameCount > 0 && track.SamplesPerSecond > 0) {
                        int frame = (int)(time * track.SamplesPerSecond) % track.FrameCount;
                        if (frame < 0) frame += track.FrameCount;

                        if (animType == 7) {
                            Vec3 p = { 0, 0, 0 }; Vec4 q = { 0, 0, 0, 1 };
                            track.EvaluateFrame(frame, p, q);
                            XMVECTOR vPos = XMVectorSet(p.x, p.y, p.z, 1.0f);
                            XMVECTOR vRot = XMQuaternionConjugate(XMQuaternionNormalize(XMVectorSet(q.x, q.y, q.z, q.w)));
                            XMMATRIX trackMat = XMMatrixRotationQuaternion(vRot) * XMMatrixTranslationFromVector(vPos);
                            localTransforms[i] = XMMatrixMultiply(trackMat, bindLocal[i]);
                        }
                        else {
                            XMVECTOR s_b, r_b, t_b; XMMatrixDecompose(&s_b, &r_b, &t_b, bindLocal[i]);
                            r_b = XMQuaternionConjugate(r_b);
                            Vec3 p = { XMVectorGetX(t_b), XMVectorGetY(t_b), XMVectorGetZ(t_b) };
                            Vec4 q = { XMVectorGetX(r_b), XMVectorGetY(r_b), XMVectorGetZ(r_b), XMVectorGetW(r_b) };
                            track.EvaluateFrame(frame, p, q);
                            XMVECTOR vPos = XMVectorSet(p.x, p.y, p.z, 1.0f);
                            XMVECTOR vRot = XMQuaternionConjugate(XMQuaternionNormalize(XMVectorSet(q.x, q.y, q.z, q.w)));
                            localTransforms[i] = XMMatrixScalingFromVector(s_b) * XMMatrixRotationQuaternion(vRot) * XMMatrixTranslationFromVector(vPos);
                        }
                        hasAnim = true;
                    }
                    break;
                }
            }
        }
        if (!hasAnim) localTransforms[i] = bindLocal[i];
    }

    std::vector<bool> computed(boneCount, false);
    std::function<void(int)> ComputeGlobal = [&](int idx) {
        if (computed[idx]) return;
        int p = mesh.Bones[idx].ParentIndex;
        if (p != -1 && p < boneCount) { ComputeGlobal(p); g_StandaloneGlobalMats[idx] = XMMatrixMultiply(localTransforms[idx], g_StandaloneGlobalMats[p]); }
        else g_StandaloneGlobalMats[idx] = localTransforms[idx];
        computed[idx] = true;
        };

    for (int i = 0; i < boneCount; i++) {
        ComputeGlobal(i);
        g_StandaloneBoneMats[i] = XMMatrixMultiply(ibm[i], g_StandaloneGlobalMats[i]);
    }
}

inline void DrawAnimProperties(std::string& entryName, uint32_t entryID, int32_t& entryType, AnimParser& parser, AnimUIContext& ctx, const std::vector<uint8_t>& rawData) {
    if (!parser.Data.IsParsed) { ImGui::Text("No animation loaded or failed to parse."); return; }
    auto& anim = parser.Data;

    static int replaceAnimType = 6;

    auto DrawPropertyRow = [](const char* label, const char* id, float* val) {
        ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(label); ImGui::SameLine(180.0f);
        ImGui::SetNextItemWidth(120.0f); ImGui::DragFloat(id, val, 0.01f);
        };

    static float rightPanelWidth = 546.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float splitterWidth = 4.0f;
    float leftWidth = avail.x - rightPanelWidth - splitterWidth;
    if (leftWidth < 260.0f) leftWidth = 260.0f;

    ImGui::BeginChild("AnimLeftPanel", ImVec2(leftWidth, avail.y), false);
    {
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Overview");
        ImGui::Separator();

        ImGui::Checkbox("Is Cyclic (Looping Animation)", &anim.IsCyclic);
        ImGui::Dummy(ImVec2(0, 5));

        DrawPropertyRow("Duration:", "##dur", &anim.Duration);
        DrawPropertyRow("Non-Looping Duration:", "##nl", &anim.NonLoopingDuration);

        ImGui::Dummy(ImVec2(0, 12));

        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Movement Vector");
        ImGui::Separator();

        if (entryType != 7) {
            DrawPropertyRow("Rotation:", "##rot", &anim.Rotation);
            DrawPropertyRow("X:", "##mvecX", &anim.MovementVector.x);
            DrawPropertyRow("Y:", "##mvecY", &anim.MovementVector.y);
            DrawPropertyRow("Z:", "##mvecZ", &anim.MovementVector.z);

            ImGui::Dummy(ImVec2(0, 5));

            ImGui::SetNextItemAllowOverlap();
            bool openCurve = ImGui::CollapsingHeader("Collision Curve Editor (MVEC)");
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 24);
            {
                AnimTrack* headerTrack = nullptr;
                for (auto& ht : anim.HelperTracks) { if (ht.BoneName == "") { headerTrack = &ht; break; } }
                bool hasCurve = headerTrack && !headerTrack->PositionTrack.empty();

                if (hasCurve) {
                    if (ImGui::Button("-##DelCurve", ImVec2(20, 0))) {
                        anim.HelperTracks.erase(std::remove_if(anim.HelperTracks.begin(), anim.HelperTracks.end(), [](const AnimTrack& t) { return t.BoneName == ""; }), anim.HelperTracks.end());
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete Collision Curve");
                }
                else {
                    if (ImGui::Button("+##AddCurve", ImVec2(20, 0))) {
                        AnimTrack* newTrack = headerTrack;
                        if (!newTrack) {
                            AnimTrack track; track.BoneName = ""; track.BoneIndex = 31450; track.ParentIndex = -1; track.SamplesPerSecond = 30.0f; track.PositionFactor = 1.0f; track.ScalingFactor = 1.0f;
                            anim.HelperTracks.push_back(track); newTrack = &anim.HelperTracks.back();
                        }
                        uint32_t trackFrames = 24; if (!anim.Tracks.empty()) trackFrames = anim.Tracks[0].FrameCount; if (trackFrames < 2) trackFrames = 2;
                        newTrack->FrameCount = trackFrames; newTrack->PositionTrack.clear();
                        for (uint32_t f = 0; f < trackFrames; f++) {
                            float t = (float)f / (float)(trackFrames - 1); newTrack->PositionTrack.push_back({ anim.MovementVector.x * t, anim.MovementVector.y * t, anim.MovementVector.z * t });
                        }
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Generate Linear Curve");
                }
            }

            if (openCurve) {
                AnimTrack* mvecTrack = nullptr;
                for (auto& ht : anim.HelperTracks) { if (ht.BoneName == "") { mvecTrack = &ht; break; } }

                if (!mvecTrack || mvecTrack->PositionTrack.empty()) {
                    ImGui::TextDisabled("No collision curve found. Use the + on the header above to generate one.");
                }
                else {
                    {
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Click anywhere on a graph to edit a frame. Use + to append a keyframe at the end:");
                        std::vector<float> posX, posY, posZ;
                        for (const auto& p : mvecTrack->PositionTrack) { posX.push_back(p.x); posY.push_back(p.y); posZ.push_back(p.z); }

                        auto getBounds = [](float target) {
                            float bMin = (std::min)(0.0f, target); float bMax = (std::max)(0.0f, target);
                            if (std::abs(bMax - bMin) < 0.001f) { bMin = FLT_MAX; bMax = FLT_MAX; } return std::make_pair(bMin, bMax);
                            };
                        auto bX = getBounds(anim.MovementVector.x); auto bY = getBounds(anim.MovementVector.y); auto bZ = getBounds(anim.MovementVector.z);

                        static int selectedMvecFrame = -1;
                        auto AddKeyframeAtEnd = [&]() {
                            Vec3 lastPos = mvecTrack->PositionTrack.back();
                            mvecTrack->PositionTrack.push_back(lastPos);
                            mvecTrack->FrameCount = (uint32_t)mvecTrack->PositionTrack.size();
                            };
                        auto DrawPlotAndCatchClick = [&](const char* label, const char* addId, const std::vector<float>& data, float scaleMin, float scaleMax) {
                            int count = (int)data.size(); ImGui::PlotLines(label, data.data(), count, 0, nullptr, scaleMin, scaleMax, ImVec2(0, 50));
                            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                                float mouseX = ImGui::GetMousePos().x - ImGui::GetItemRectMin().x; float width = ImGui::GetItemRectSize().x;
                                int idx = count > 1 ? (int)std::round((mouseX / width) * (count - 1)) : 0;
                                selectedMvecFrame = std::clamp(idx, 0, count - 1); ImGui::OpenPopup("EditKeyframePopup");
                            }
                            ImGui::SameLine();
                            if (ImGui::Button(addId, ImVec2(20, 0))) AddKeyframeAtEnd();
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Keyframe at End");
                            };

                        DrawPlotAndCatchClick("X Axis", "+##addFrameX", posX, bX.first, bX.second);
                        DrawPlotAndCatchClick("Y Axis", "+##addFrameY", posY, bY.first, bY.second);
                        DrawPlotAndCatchClick("Z Axis", "+##addFrameZ", posZ, bZ.first, bZ.second);

                        if (ImGui::BeginPopup("EditKeyframePopup")) {
                            if (selectedMvecFrame >= 0 && selectedMvecFrame < mvecTrack->PositionTrack.size()) {
                                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Editing Frame %03d", selectedMvecFrame); ImGui::Separator();
                                ImGui::SetNextItemWidth(200);
                                if (ImGui::DragFloat3("XYZ", &mvecTrack->PositionTrack[selectedMvecFrame].x, 0.01f)) {
                                    if (selectedMvecFrame == mvecTrack->PositionTrack.size() - 1) anim.MovementVector = mvecTrack->PositionTrack[selectedMvecFrame];
                                }
                                ImGui::Dummy(ImVec2(0, 5));
                                if (ImGui::Button("Delete This Frame", ImVec2(-1, 0))) {
                                    mvecTrack->PositionTrack.erase(mvecTrack->PositionTrack.begin() + selectedMvecFrame); mvecTrack->FrameCount = (uint32_t)mvecTrack->PositionTrack.size(); ImGui::CloseCurrentPopup();
                                }
                            }
                            ImGui::EndPopup();
                        }
                    }
                }
            }
        }
        else {
            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Delta Animations (Type 7) do not use Root Movement Vectors.");
        }

        ImGui::SetNextItemAllowOverlap();
        bool openTimeEvents = ImGui::CollapsingHeader("Time Events (TMEV)");
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 24);
        if (ImGui::Button("+##AddTmev", ImVec2(20, 0))) anim.TimeEvents.push_back({ "NEW_EVENT", 0.0f });
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Time Event");

        if (openTimeEvents) {
            ImGui::BeginChild("TMEV_Editor", ImVec2(0, 150), true);
            for (size_t i = 0; i < anim.TimeEvents.size(); i++) {
                ImGui::PushID((int)i);
                char tmevBuf[256]; strncpy_s(tmevBuf, anim.TimeEvents[i].Name.c_str(), 255); ImGui::SetNextItemWidth(250);
                if (ImGui::InputText("##evname", tmevBuf, 256)) anim.TimeEvents[i].Name = tmevBuf; ImGui::SameLine(); ImGui::SetNextItemWidth(100);
                ImGui::DragFloat("##evtime", &anim.TimeEvents[i].Time, 0.01f, 0.0f, anim.Duration, "%.2fs"); ImGui::SameLine();
                if (ImGui::Button("X")) { anim.TimeEvents.erase(anim.TimeEvents.begin() + i); i--; } ImGui::PopID();
            }
            ImGui::EndChild();
        }

        ImGui::SetNextItemAllowOverlap();
        bool openMask = ImGui::CollapsingHeader("Partial Animation Bone Mask (AMSK)");
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 24);
        if (anim.BoneMaskBits.empty()) {
            if (ImGui::Button("+##AddMask", ImVec2(20, 0))) {
                uint32_t wordCount = ((uint32_t)anim.Tracks.size() + 31) / 32;
                anim.BoneMaskBits.resize(wordCount, 0xFFFFFFFF); entryType = 9;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Apply Bone Mask\nConverts animation to Partial Animation.");
        }
        else {
            if (ImGui::Button("-##DelMask", ImVec2(20, 0))) { anim.BoneMaskBits.clear(); entryType = 6; }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove Bone Mask\nConverts partial animation to a full body animation.");
        }

        if (openMask) {
            if (anim.BoneMaskBits.empty()) {
                ImGui::TextDisabled("This is a Full Body animation (No Mask). Use the + on the header above to apply a mask.");
            }
            else {
                ImGui::BeginChild("MaskEditor", ImVec2(0, 150), true);
                for (size_t i = 0; i < anim.Tracks.size(); i++) {
                    uint32_t wordIdx = (uint32_t)(i / 32); uint32_t bitIdx = (uint32_t)(i % 32);
                    if (wordIdx >= anim.BoneMaskBits.size()) anim.BoneMaskBits.resize(wordIdx + 1, 0);
                    bool isEnabled = (anim.BoneMaskBits[wordIdx] & (1 << bitIdx)) != 0;
                    if (ImGui::Checkbox((anim.Tracks[i].BoneName + "##mask" + std::to_string(i)).c_str(), &isEnabled)) {
                        if (isEnabled) anim.BoneMaskBits[wordIdx] |= (1 << bitIdx); else anim.BoneMaskBits[wordIdx] &= ~(1 << bitIdx);
                    }
                }
                ImGui::EndChild();
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine(0, 0);
    ImVec2 splitterMin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("AnimVSplitter", ImVec2(splitterWidth, avail.y));
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive()) {
        rightPanelWidth -= ImGui::GetIO().MouseDelta.x;
        if (rightPanelWidth < 340.0f) rightPanelWidth = 340.0f;
        if (rightPanelWidth > avail.x - 260.0f) rightPanelWidth = avail.x - 260.0f;
    }
    // Purely visual divider between the properties and the previewer
    {
        float lineX = splitterMin.x + splitterWidth * 0.5f;
        ImGui::GetWindowDrawList()->AddLine(ImVec2(lineX, splitterMin.y), ImVec2(lineX, splitterMin.y + avail.y), IM_COL32(90, 90, 90, 255), 1.0f);
    }
    ImGui::SameLine(0, 0);

    ImGui::BeginChild("AnimRightPanel", ImVec2(rightPanelWidth, avail.y), false);
    {
        float duration = anim.Duration > 0 ? anim.Duration : 1.0f;
        if (g_StandalonePlaying) {
            g_StandaloneTime += ImGui::GetIO().DeltaTime;
            if (g_StandaloneTime >= duration) {
                if (g_StandaloneLoop) g_StandaloneTime = fmod(g_StandaloneTime, duration);
                else { g_StandaloneTime = duration; g_StandalonePlaying = false; }
            }
        }

        if (g_StandaloneMeshLoaded) {
            if (g_StandaloneUploadNeeded) {
                g_StandaloneRenderer.Initialize(g_pd3dDevice);
                g_StandaloneRenderer.UploadMesh(g_pd3dDevice, g_StandaloneMesh);
                std::vector<MeshRenderer::RenderMaterial> materials;
                int maxMat = 0; for (const auto& m : g_StandaloneMesh.Materials) if (m.ID > maxMat) maxMat = m.ID;
                materials.resize(maxMat + 1);

                for (const auto& m : g_StandaloneMesh.Materials) {
                    materials[m.ID].SelfIllumination = (float)m.SelfIllumination / 255.0f;
                    if (m.DiffuseMapID > 0) {
                        materials[m.ID].Diffuse = LoadTextureForMesh(m.DiffuseMapID);
                    }
                }
                g_StandaloneRenderer.SetMaterials(materials);
                g_StandaloneUploadNeeded = false;
            }
            UpdateStandaloneBones(anim, g_StandaloneMesh, g_StandaloneTime, entryType);
        }

        float animBtnSize = 24.0f;
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.82f, 0.45f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.82f, 0.45f, 0.5f));
        ImVec4 animIconTint = ImVec4(0.95f, 0.82f, 0.45f, 1.0f);

        ImTextureID playPauseTex = g_StandalonePlaying ? g_PauseTexture : g_PlayTexture;
        if (playPauseTex) {
            if (ImGui::ImageButton("##AnimPlayPause", playPauseTex, ImVec2(animBtnSize, animBtnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), animIconTint)) {
                g_StandalonePlaying = !g_StandalonePlaying;
            }
        }
        else if (ImGui::Button(g_StandalonePlaying ? "Pause" : "Play", ImVec2(60, 0))) {
            g_StandalonePlaying = !g_StandalonePlaying;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(g_StandalonePlaying ? "Pause" : "Play");

        ImGui::SameLine();
        if (g_StopTexture) {
            if (ImGui::ImageButton("##AnimStop", g_StopTexture, ImVec2(animBtnSize, animBtnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), animIconTint)) {
                g_StandalonePlaying = false; g_StandaloneTime = 0.0f;
            }
        }
        else if (ImGui::Button("Stop", ImVec2(60, 0))) {
            g_StandalonePlaying = false; g_StandaloneTime = 0.0f;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop");

        ImGui::SameLine();
        ImVec4 loopTint = g_StandaloneLoop ? animIconTint : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        if (g_LoopTexture) {
            if (ImGui::ImageButton("##AnimLoop", g_LoopTexture, ImVec2(animBtnSize, animBtnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), loopTint)) {
                g_StandaloneLoop = !g_StandaloneLoop;
            }
        }
        else if (ImGui::Button(g_StandaloneLoop ? "Loop: On" : "Loop: Off", ImVec2(70, 0))) {
            g_StandaloneLoop = !g_StandaloneLoop;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(g_StandaloneLoop ? "Looping enabled" : "Looping disabled");

        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::TextDisabled("%.2fs / %.2fs", g_StandaloneTime, duration);

        ImVec2 timelineSize = ImVec2(ImGui::GetContentRegionAvail().x, 22.0f);
        ImVec2 tMin = ImGui::GetCursorScreenPos();
        ImVec2 tMax = ImVec2(tMin.x + timelineSize.x, tMin.y + timelineSize.y);

        ImGui::InvisibleButton("##AnimTimeline", timelineSize);
        bool timelineActive = ImGui::IsItemActive();
        bool timelineHovered = ImGui::IsItemHovered();
        if (timelineActive && ImGui::IsMouseDown(0) && timelineSize.x > 0.0f) {
            float t = (ImGui::GetMousePos().x - tMin.x) / timelineSize.x;
            t = std::clamp(t, 0.0f, 1.0f);
            g_StandaloneTime = t * duration;
            g_StandalonePlaying = false;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(tMin, tMax, IM_COL32(40, 40, 40, 255), 3.0f);
        dl->AddRect(tMin, tMax, IM_COL32(90, 90, 90, 255), 3.0f);

        float progress = duration > 0.0f ? std::clamp(g_StandaloneTime / duration, 0.0f, 1.0f) : 0.0f;
        dl->AddRectFilled(tMin, ImVec2(tMin.x + timelineSize.x * progress, tMax.y), IM_COL32(90, 140, 200, 90), 3.0f);

        ImVec2 mousePos = ImGui::GetMousePos();
        const decltype(anim.TimeEvents)::value_type* hoveredEvent = nullptr;
        for (const auto& ev : anim.TimeEvents) {
            float evT = duration > 0.0f ? std::clamp(ev.Time / duration, 0.0f, 1.0f) : 0.0f;
            float evX = tMin.x + timelineSize.x * evT;
            dl->AddLine(ImVec2(evX, tMin.y), ImVec2(evX, tMax.y), IM_COL32(255, 200, 0, 220), 2.0f);

            if (timelineHovered && std::abs(mousePos.x - evX) <= 4.0f && mousePos.y >= tMin.y && mousePos.y <= tMax.y) {
                hoveredEvent = &ev;
            }
        }

        float headX = tMin.x + timelineSize.x * progress;
        dl->AddLine(ImVec2(headX, tMin.y), ImVec2(headX, tMax.y), IM_COL32(255, 255, 255, 255), 2.0f);

        if (hoveredEvent) {
            ImGui::SetTooltip("%s\n%.2fs", hoveredEvent->Name.c_str(), hoveredEvent->Time);
        }
        else if (timelineHovered && timelineSize.x > 0.0f) {
            float hoverT = std::clamp((mousePos.x - tMin.x) / timelineSize.x, 0.0f, 1.0f);
            ImGui::SetTooltip("%.2fs", hoverT * duration);
        }

        ImGui::Dummy(ImVec2(0, 4));

        ImVec2 viewportAvail = ImGui::GetContentRegionAvail();
        if (viewportAvail.y < 60.0f) viewportAvail.y = 60.0f;

        bool openImportPopup = false;

        ImGui::BeginChild("AnimViewport", viewportAvail, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        bool viewportHovered = ImGui::IsWindowHovered();
        static float s_AnimOverlayAlpha = 1.0f;
        float targetAlpha = viewportHovered ? 1.0f : 0.0f;
        s_AnimOverlayAlpha += (targetAlpha - s_AnimOverlayAlpha) * ImGui::GetIO().DeltaTime * 15.0f;
        if (s_AnimOverlayAlpha < 0.0f) s_AnimOverlayAlpha = 0.0f;
        if (s_AnimOverlayAlpha > 1.0f) s_AnimOverlayAlpha = 1.0f;

        ImVec2 pMin = ImGui::GetCursorScreenPos();
        ImVec2 pMax = ImVec2(pMin.x + viewportAvail.x, pMin.y + viewportAvail.y);

        if (!g_StandaloneMeshLoaded) {
            ImVec2 center = ImVec2((pMin.x + pMax.x) * 0.5f, (pMin.y + pMax.y) * 0.5f);
            const char* msg = "Select a mesh to preview this animation";
            ImVec2 textSize = ImGui::CalcTextSize(msg);
            ImGui::SetCursorScreenPos(ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f));
            ImGui::TextDisabled("%s", msg);
        }
        else {
            g_StandaloneRenderer.Resize(g_pd3dDevice, viewportAvail.x, viewportAvail.y);
            ID3D11DeviceContext* pCtx; g_pd3dDevice->GetImmediateContext(&pCtx);
            ID3D11ShaderResourceView* tex = g_StandaloneRenderer.Render(pCtx, viewportAvail.x, viewportAvail.y, false, false, &g_StandaloneBoneMats);
            pCtx->Release();
            if (tex) { ImGui::SetCursorScreenPos(pMin); ImGui::Image((void*)tex, viewportAvail); }
            if (g_StandaloneShowSkeleton && g_StandaloneMesh.BoneCount > 0 && !g_StandaloneGlobalMats.empty()) {
                ImDrawList* sdl = ImGui::GetWindowDrawList();
                sdl->PushClipRect(pMin, pMax, true);

                auto cleanBoneName = [](const std::string& str) { std::string res; for (char c : str) if (isalnum((unsigned char)c)) res += (char)tolower(c); return res; };

                ImVec2 mousePos = ImGui::GetMousePos();
                for (int i = 0; i < g_StandaloneMesh.BoneCount; i++) {
                    XMMATRIX globalMat = g_StandaloneGlobalMats[i];
                    XMFLOAT3 pos(XMVectorGetX(globalMat.r[3]), XMVectorGetY(globalMat.r[3]), XMVectorGetZ(globalMat.r[3]));

                    ImVec2 scrPos;
                    if (g_StandaloneRenderer.ProjectToScreen(pos, scrPos, viewportAvail.x, viewportAvail.y)) {
                        scrPos.x += pMin.x; scrPos.y += pMin.y;

                        ImU32 jointColor = IM_COL32(0, 255, 0, 255);
                        if (!anim.BoneMaskBits.empty()) {
                            std::string targetName = (i < (int)g_StandaloneMesh.BoneNames.size()) ? g_StandaloneMesh.BoneNames[i] : "";
                            std::string cleanTarget = cleanBoneName(targetName);
                            if (!cleanTarget.empty()) {
                                for (size_t t = 0; t < anim.Tracks.size(); t++) {
                                    if (cleanTarget == cleanBoneName(anim.Tracks[t].BoneName)) {
                                        uint32_t wordIdx = (uint32_t)(t / 32); uint32_t bitIdx = (uint32_t)(t % 32);
                                        bool enabled = (wordIdx < anim.BoneMaskBits.size()) && (anim.BoneMaskBits[wordIdx] & (1 << bitIdx)) != 0;
                                        if (!enabled) jointColor = IM_COL32(255, 120, 40, 255);
                                        break;
                                    }
                                }
                            }
                        }

                        sdl->AddCircleFilled(scrPos, 3.0f, jointColor);

                        float dist = sqrtf((mousePos.x - scrPos.x) * (mousePos.x - scrPos.x) + (mousePos.y - scrPos.y) * (mousePos.y - scrPos.y));
                        if (dist < 8.0f) {
                            sdl->AddCircle(scrPos, 6.0f, IM_COL32(255, 255, 0, 255));
                            std::string boneName = (i < (int)g_StandaloneMesh.BoneNames.size()) ? g_StandaloneMesh.BoneNames[i] : "Bone " + std::to_string(i);
                            ImGui::SetTooltip(jointColor == IM_COL32(255, 120, 40, 255) ? "%s (masked out)" : "%s", boneName.c_str());
                        }

                        int parentIdx = g_StandaloneMesh.Bones[i].ParentIndex;
                        if (parentIdx != -1 && parentIdx < g_StandaloneMesh.BoneCount) {
                            XMMATRIX parentMat = g_StandaloneGlobalMats[parentIdx];
                            XMFLOAT3 parentPos(XMVectorGetX(parentMat.r[3]), XMVectorGetY(parentMat.r[3]), XMVectorGetZ(parentMat.r[3]));
                            ImVec2 parentScrPos;
                            if (g_StandaloneRenderer.ProjectToScreen(parentPos, parentScrPos, viewportAvail.x, viewportAvail.y)) {
                                parentScrPos.x += pMin.x; parentScrPos.y += pMin.y;
                                sdl->AddLine(scrPos, parentScrPos, IM_COL32(255, 255, 0, 255), 2.0f);
                            }
                        }
                    }
                }

                sdl->PopClipRect();
            }
            if (g_StandaloneShowMovementVector) {
                ImDrawList* mdl = ImGui::GetWindowDrawList();
                mdl->PushClipRect(pMin, pMax, true);

                ImU32 vecColor = IM_COL32(80, 180, 255, 255);
                XMFLOAT3 originPos(0.0f, 0.0f, 0.0f);
                ImVec2 originScr;
                if (g_StandaloneRenderer.ProjectToScreen(originPos, originScr, viewportAvail.x, viewportAvail.y)) {
                    originScr.x += pMin.x; originScr.y += pMin.y;

                    bool isZero = fabsf(anim.MovementVector.x) < 0.0001f && fabsf(anim.MovementVector.y) < 0.0001f && fabsf(anim.MovementVector.z) < 0.0001f;

                    if (isZero) {
                        mdl->AddCircleFilled(originScr, 5.0f, vecColor);
                        mdl->AddCircle(originScr, 9.0f, vecColor, 0, 1.5f);
                    }
                    else {
                        XMFLOAT3 endPos(anim.MovementVector.x, anim.MovementVector.y, anim.MovementVector.z);
                        ImVec2 endScr;
                        if (g_StandaloneRenderer.ProjectToScreen(endPos, endScr, viewportAvail.x, viewportAvail.y)) {
                            endScr.x += pMin.x; endScr.y += pMin.y;

                            mdl->AddCircleFilled(originScr, 4.0f, vecColor);
                            mdl->AddLine(originScr, endScr, vecColor, 3.0f);

                            ImVec2 dir = ImVec2(endScr.x - originScr.x, endScr.y - originScr.y);
                            float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
                            if (len > 0.001f) {
                                dir.x /= len; dir.y /= len;
                                ImVec2 perp = ImVec2(-dir.y, dir.x);
                                float headLen = 12.0f, headWidth = 6.0f;
                                ImVec2 tipBack = ImVec2(endScr.x - dir.x * headLen, endScr.y - dir.y * headLen);
                                ImVec2 left = ImVec2(tipBack.x + perp.x * headWidth, tipBack.y + perp.y * headWidth);
                                ImVec2 right = ImVec2(tipBack.x - perp.x * headWidth, tipBack.y - perp.y * headWidth);
                                mdl->AddTriangleFilled(endScr, left, right, vecColor);
                            }
                        }
                    }
                }

                mdl->PopClipRect();
            }
            {
                ImGui::SetCursorScreenPos(ImVec2(pMin.x + 10, pMin.y + 10));
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s_AnimOverlayAlpha);
                ImGui::BeginGroup();
                if (g_StandaloneMesh.BoneCount > 0) {
                    ImGui::Checkbox("Skeleton", &g_StandaloneShowSkeleton);
                    ImGui::SameLine();
                }
                ImGui::Checkbox("Movement Vector", &g_StandaloneShowMovementVector);
                ImGui::EndGroup();
                ImGui::PopStyleVar();
            }
        }
        {
            static float s_AnimOverlayW = 220.0f;
            static float s_AnimOverlayH = 24.0f;

            ImGui::SetCursorScreenPos(ImVec2(pMax.x - s_AnimOverlayW - 18, pMax.y - s_AnimOverlayH - 18));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s_AnimOverlayAlpha);
            ImGui::BeginGroup();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.12f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.22f));

            // Import button
            if (g_ImportTexture) {
                if (ImGui::ImageButton("##AnimImport", g_ImportTexture, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), kImportIconTint)) {
                    replaceAnimType = (entryType == 7) ? 7 : 6;
                    openImportPopup = true;
                }
            }
            else if (ImGui::Button("Import")) {
                replaceAnimType = (entryType == 7) ? 7 : 6;
                openImportPopup = true;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Import");

            ImGui::SameLine();

            // Export button
            ImGui::BeginDisabled(!g_StandaloneMeshLoaded);
            if (g_ExportTexture) {
                if (ImGui::ImageButton("##AnimExport", g_ExportTexture, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), kExportIconTint)) {
                    g_ShowStandaloneExportPopup = true;
                }
            }
            else if (ImGui::Button("Export")) {
                g_ShowStandaloneExportPopup = true;
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Export");

            ImGui::SameLine();

            // Change Mesh button (now using texture)
            if (g_ChangeTexture) {
                if (ImGui::ImageButton("##AnimChangeMesh", g_ChangeTexture, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), kChangeIconTint)) {
                    g_ShowStandaloneMeshPicker = true;
                }
            }
            else {
                // fallback to text button if texture not loaded
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.55f, 0.10f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.66f, 0.16f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.45f, 0.08f, 1.0f));
                if (ImGui::Button(g_StandaloneMeshLoaded ? "Change Mesh" : "Select Mesh")) {
                    g_ShowStandaloneMeshPicker = true;
                }
                ImGui::PopStyleColor(3);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(g_StandaloneMeshLoaded ? "Change Mesh" : "Select Mesh");
            }

            ImGui::PopStyleColor(3);  // restore button colors for the three buttons (they share the same style)
            ImGui::EndGroup();
            ImGui::PopStyleVar();     // restore alpha

            ImVec2 brMin = ImGui::GetItemRectMin();
            ImVec2 brMax = ImGui::GetItemRectMax();
            s_AnimOverlayW = brMax.x - brMin.x;
            s_AnimOverlayH = brMax.y - brMin.y;
        }

        ImGui::EndChild();

        if (openImportPopup) {                       
            ImGui::OpenPopup("Import Over Existing"); 
        }                                                  

        if (ImGui::BeginPopupModal("Import Over Existing", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0, 1, 1, 1), "Select target animation type:");
            ImGui::RadioButton("Normal Animation (6)", &replaceAnimType, 6);
            ImGui::RadioButton("Delta Animation (7)", &replaceAnimType, 7);
            ImGui::TextDisabled("Note: Partial Animations (9) are auto-detected via bitmasks.");
            ImGui::Separator();

            if (ImGui::Button("Select File & Import", ImVec2(180, 0))) {
                std::string loadPath = OpenFileDialog("glTF Files\0*.gltf\0All Files\0*.*\0");
                if (!loadPath.empty()) {
                    int importedType = replaceAnimType;
                    std::string err = GltfAnimImporter::Import(loadPath, g_ActiveMeshContent, anim, importedType);
                    if (err.empty()) {
                        if (!anim.BoneMaskBits.empty()) importedType = 9;
                        if (importedType == 7) {
                            anim.MovementVector = { 0.0f, 0.0f, 0.0f };
                            anim.HelperTracks.erase(std::remove_if(anim.HelperTracks.begin(), anim.HelperTracks.end(), [](const AnimTrack& t) { return t.BoneName == ""; }), anim.HelperTracks.end());
                        }
                        entryType = importedType; g_BankStatus = "Transpiled animation successfully! PLEASE SAVE.";
                    }
                    else g_BankStatus = "Import Error: " + err;
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine(); if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (g_ShowStandaloneExportPopup) {
            ImGui::OpenPopup("Export Standalone Options");
        }

        if (ImGui::BeginPopupModal("Export Standalone Options", &g_ShowStandaloneExportPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!g_StandaloneMeshLoaded) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Select a preview mesh first.");
            }
            else {
                ImGui::Checkbox("Export Textures", &g_StandaloneExportTextures);
                ImGui::Checkbox("Export Animation", &g_StandaloneExportAnimation);
            }

            ImGui::Separator();

            ImGui::BeginDisabled(!g_StandaloneMeshLoaded);
            if (ImGui::Button("Export", ImVec2(120, 0))) {
                std::string savePath = SaveFileDialog("glTF Files\0*.gltf\0All Files\0*.*\0");
                if (!savePath.empty()) {
                    if (savePath.length() < 5 || savePath.substr(savePath.length() - 5) != ".gltf") savePath += ".gltf";
                    std::string expDir = savePath.substr(0, savePath.find_last_of("\\/") + 1);

                    std::function<std::string(int)> finalExtFunc = nullptr;
                    if (g_StandaloneExportTextures) {
                        finalExtFunc = [expDir](int id) { return ExtractTextureForGltf(id, expDir); };
                    }
                    const AnimParser* animToExport = g_StandaloneExportAnimation ? &parser : nullptr;
                    int animTypeToExport = g_StandaloneExportAnimation ? entryType : 6;

                    GltfExporter::Export(g_StandaloneMesh, savePath, animToExport, animTypeToExport, finalExtFunc);
                }
                g_ShowStandaloneExportPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                g_ShowStandaloneExportPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (g_ShowStandaloneMeshPicker) ImGui::OpenPopup("Select Mesh for Preview");
        if (ImGui::BeginPopupModal("Select Mesh for Preview", &g_ShowStandaloneMeshPicker, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Select a mesh to preview this animation:");
            ImGui::Separator();

            static char meshFilterBuf[128] = "";
            ImGui::InputTextWithHint("##MeshSearch", "Search animated meshes...", meshFilterBuf, 128);
            ImGui::Separator();

            ImGui::BeginChild("MeshListPreview", ImVec2(400, 300), true);

            std::string filterStr = meshFilterBuf;
            std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

            bool isXboxActive = (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < g_OpenBanks.size() && g_OpenBanks[g_ActiveBankIndex].Type == EBankType::XboxGraphics);

            auto processMeshBank = [&](int bankIdx) {
                for (int j = 0; j < g_OpenBanks[bankIdx].Entries.size(); j++) {
                    int t = g_OpenBanks[bankIdx].Entries[j].Type;
                    if (t == 5) {
                        std::string mName = g_OpenBanks[bankIdx].Entries[j].Name;
                        std::transform(mName.begin(), mName.end(), mName.begin(), ::tolower);
                        if (!filterStr.empty() && mName.find(filterStr) == std::string::npos) continue;

                        bool isMatch = false;
                        std::string aName = anim.ObjectName;
                        std::transform(aName.begin(), aName.end(), aName.begin(), ::tolower);

                        std::string mBase = mName.substr(0, mName.find('_'));
                        std::string aBase = aName.substr(0, aName.find('_'));
                        if (!mBase.empty() && !aBase.empty() && mBase == aBase) isMatch = true;

                        if (isMatch) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1));

                        std::string displayLabel = g_OpenBanks[bankIdx].Entries[j].FriendlyName + (isMatch ? " (Likely Match)" : "");
                        if (isXboxActive) displayLabel += " [Xbox]";

                        if (ImGui::Selectable(displayLabel.c_str())) {
                            auto& b = g_OpenBanks[bankIdx];
                            std::vector<uint8_t> meshRawData;
                            if (b.ModifiedEntryData.count(j)) meshRawData = b.ModifiedEntryData[j];
                            else {
                                b.Stream->clear(); b.Stream->seekg(b.Entries[j].Offset, std::ios::beg);
                                meshRawData.resize(b.Entries[j].Size); b.Stream->read((char*)meshRawData.data(), b.Entries[j].Size);
                            }
                            g_StandaloneMesh = C3DMeshContent();
                            if (b.SubheaderCache.count(j)) g_StandaloneMesh.ParseEntryMetadata(b.SubheaderCache[j]);
                            g_StandaloneMesh.Parse(meshRawData);
                            g_StandaloneUploadNeeded = true;
                            g_StandaloneMeshLoaded = true;
                            g_StandaloneTime = 0.0f;
                            g_StandalonePlaying = true;
                            g_StandaloneLoop = anim.IsCyclic;
                            g_ShowStandaloneMeshPicker = false;
                        }
                        if (isMatch) ImGui::PopStyleColor();
                    }
                }
                };

            if (isXboxActive) {
                processMeshBank(g_ActiveBankIndex);
            }
            else {
                for (int i = 0; i < g_OpenBanks.size(); i++) {
                    if (g_OpenBanks[i].Type == EBankType::Graphics) {
                        processMeshBank(i);
                    }
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) g_ShowStandaloneMeshPicker = false;
            ImGui::EndPopup();
        }
    }
    ImGui::EndChild();
}