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
static MeshRenderer g_StandaloneRenderer;
static C3DMeshContent g_StandaloneMesh;
static bool g_StandaloneUploadNeeded = false;
static bool g_ShowStandaloneMeshPicker = false;
static bool g_ShowStandalonePreview = false;
static float g_StandaloneTime = 0.0f;
static bool g_StandalonePlaying = true;
static std::vector<XMMATRIX> g_StandaloneBoneMats;
static std::vector<XMMATRIX> g_StandaloneGlobalMats;

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
    if (ImGui::Button("Import from glTF", ImVec2(150, 0))) {
        replaceAnimType = (entryType == 7) ? 7 : 6;
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

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    if (ImGui::Button("Preview Animation (3D)", ImVec2(150, 0))) g_ShowStandaloneMeshPicker = true;
    ImGui::PopStyleColor();

    ImGui::SameLine();

    if (ImGui::Button("Export Uncompressed Binary")) {
        std::string savePath = SaveFileDialog("Binary Files\0*.bin\0All Files\0*.*\0");
        if (!savePath.empty()) {
            std::ofstream out(savePath, std::ios::binary);
            out.write((char*)parser.UncompressedData.data(), parser.UncompressedData.size());
            g_BankStatus = "Exported Uncompressed Binary!";
        }
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
                        std::vector<uint8_t> rawData;
                        if (b.ModifiedEntryData.count(j)) rawData = b.ModifiedEntryData[j];
                        else {
                            b.Stream->clear(); b.Stream->seekg(b.Entries[j].Offset, std::ios::beg);
                            rawData.resize(b.Entries[j].Size); b.Stream->read((char*)rawData.data(), b.Entries[j].Size);
                        }
                        g_StandaloneMesh = C3DMeshContent();
                        if (b.SubheaderCache.count(j)) g_StandaloneMesh.ParseEntryMetadata(b.SubheaderCache[j]);
                        g_StandaloneMesh.Parse(rawData);
                        g_StandaloneUploadNeeded = true;
                        g_ShowStandalonePreview = true;
                        g_ShowStandaloneMeshPicker = false;
                    }
                    if (isMatch) ImGui::PopStyleColor();
                }
            }
            };

        if (isXboxActive) {
            // XBOX: Only scan the local active bank for meshes
            processMeshBank(g_ActiveBankIndex);
        }
        else {
            // PC: Scan all standard graphics banks
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

    static bool s_wasPreviewOpen = false;
    if (g_ShowStandalonePreview) {
        ImGui::OpenPopup("Animation Previewer");
        s_wasPreviewOpen = true;
    }

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("Animation Previewer", &g_ShowStandalonePreview, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        if (g_StandaloneUploadNeeded) {
            g_StandaloneRenderer.Initialize(g_pd3dDevice);
            g_StandaloneRenderer.UploadMesh(g_pd3dDevice, g_StandaloneMesh);
            std::vector<MeshRenderer::RenderMaterial> materials;
            int maxMat = 0; for (const auto& m : g_StandaloneMesh.Materials) if (m.ID > maxMat) maxMat = m.ID;
            materials.resize(maxMat + 1);

            for (const auto& m : g_StandaloneMesh.Materials) {
                // Carry over the self-illumination for the animation previewer
                materials[m.ID].SelfIllumination = (float)m.SelfIllumination / 255.0f;

                // Seamlessly use the cache we set up in MeshProperties to handle Xbox/PC transparently!
                if (m.DiffuseMapID > 0) {
                    materials[m.ID].Diffuse = LoadTextureForMesh(m.DiffuseMapID);
                }
            }
            // Use the new method name
            g_StandaloneRenderer.SetMaterials(materials);
            g_StandaloneTime = 0.0f; g_StandalonePlaying = true; g_StandaloneUploadNeeded = false;
        }

        if (g_StandalonePlaying) {
            g_StandaloneTime += ImGui::GetIO().DeltaTime;
            float d = anim.Duration > 0 ? anim.Duration : 1.0f;
            if (g_StandaloneTime >= d) {
                if (anim.IsCyclic) g_StandaloneTime = fmod(g_StandaloneTime, d);
                else { g_StandaloneTime = d; g_StandalonePlaying = false; }
            }
        }

        UpdateStandaloneBones(anim, g_StandaloneMesh, g_StandaloneTime, entryType);

        if (ImGui::Button(g_StandalonePlaying ? "Pause" : "Play", ImVec2(80, 0))) g_StandalonePlaying = !g_StandalonePlaying;
        ImGui::SameLine(); if (ImGui::Button("Stop", ImVec2(80, 0))) { g_StandalonePlaying = false; g_StandaloneTime = 0.0f; }

        float fps = anim.Tracks.empty() ? 30.0f : anim.Tracks[0].SamplesPerSecond;
        float d = anim.Duration > 0 ? anim.Duration : 1.0f;
        int maxFrames = (int)(d * fps);
        if (maxFrames < 1) maxFrames = 1;
        int currentFrame = (int)(g_StandaloneTime * fps);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        if (ImGui::SliderInt("##FrameScrub", &currentFrame, 0, maxFrames, "%d")) {
            g_StandaloneTime = (float)currentFrame / fps;
            g_StandalonePlaying = false;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("FPS: %.1f", fps);
        ImGui::Separator();

        ImVec2 avail = ImGui::GetContentRegionAvail();
        g_StandaloneRenderer.Resize(g_pd3dDevice, avail.x, avail.y);
        ID3D11DeviceContext* pCtx; g_pd3dDevice->GetImmediateContext(&pCtx);
        ID3D11ShaderResourceView* tex = g_StandaloneRenderer.Render(pCtx, avail.x, avail.y, false, false, &g_StandaloneBoneMats);
        pCtx->Release();
        if (tex) ImGui::Image((void*)tex, avail);

        ImGui::EndPopup();
    }

    if (s_wasPreviewOpen && !g_ShowStandalonePreview) {
        g_StandaloneMesh = C3DMeshContent();
        g_StandaloneRenderer.Release();
        s_wasPreviewOpen = false;
    }


    ImGui::Dummy(ImVec2(0, 5));

    if (ImGui::CollapsingHeader("Animation Metadata", ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGui::Checkbox("Is Cyclic (Looping Animation)", &anim.IsCyclic);

        uint32_t maxFrames = 0;
        for (const auto& t : anim.Tracks) { if (t.FrameCount > maxFrames) maxFrames = t.FrameCount; }
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Total Frames: %u", maxFrames);
        ImGui::Dummy(ImVec2(0, 5));

        auto DrawPropertyRow = [](const char* label, const char* id, float* val) {
            ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(label); ImGui::SameLine(200.0f);
            ImGui::SetNextItemWidth(120.0f); ImGui::DragFloat(id, val, 0.01f);
            };

        DrawPropertyRow("Duration:", "##dur", &anim.Duration);
        DrawPropertyRow("Non-Looping Duration:", "##nl", &anim.NonLoopingDuration);
        DrawPropertyRow("Rotation:", "##rot", &anim.Rotation);

        if (entryType != 7) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 1, 1, 1), "Root Movement Vector (MVEC)");

            DrawPropertyRow("X:", "##mvecX", &anim.MovementVector.x);
            DrawPropertyRow("Y:", "##mvecY", &anim.MovementVector.y);
            DrawPropertyRow("Z:", "##mvecZ", &anim.MovementVector.z);

            if (ImGui::Button("Auto-Calc from Root", ImVec2(150, 0))) {
                if (!anim.Tracks.empty() && !anim.Tracks[0].PositionTrack.empty()) {
                    Vec3 startPos = anim.Tracks[0].PositionTrack.front();
                    Vec3 endPos = anim.Tracks[0].PositionTrack.back();
                    anim.MovementVector.x = endPos.x - startPos.x; anim.MovementVector.y = endPos.y - startPos.y; anim.MovementVector.z = endPos.z - startPos.z;
                }
            }

            ImGui::Dummy(ImVec2(0, 5));

            if (ImGui::CollapsingHeader("Collision Curve Editor (MVEC)", ImGuiTreeNodeFlags_DefaultOpen)) {
                AnimTrack* mvecTrack = nullptr;
                for (auto& ht : anim.HelperTracks) { if (ht.BoneName == "") { mvecTrack = &ht; break; } }

                if (!mvecTrack || mvecTrack->PositionTrack.empty()) {
                    ImGui::TextDisabled(mvecTrack ? "Collision curve track exists, but has zero keyframes." : "No collision curve found in this animation.");
                    if (ImGui::Button("Generate Linear Curve")) {
                        if (!mvecTrack) {
                            AnimTrack newTrack; newTrack.BoneName = ""; newTrack.BoneIndex = 31450; newTrack.ParentIndex = -1; newTrack.SamplesPerSecond = 30.0f; newTrack.PositionFactor = 1.0f; newTrack.ScalingFactor = 1.0f;
                            anim.HelperTracks.push_back(newTrack); mvecTrack = &anim.HelperTracks.back();
                        }
                        uint32_t trackFrames = 24; if (!anim.Tracks.empty()) trackFrames = anim.Tracks[0].FrameCount; if (trackFrames < 2) trackFrames = 2;
                        mvecTrack->FrameCount = trackFrames; mvecTrack->PositionTrack.clear();
                        for (uint32_t f = 0; f < trackFrames; f++) {
                            float t = (float)f / (float)(trackFrames - 1); mvecTrack->PositionTrack.push_back({ anim.MovementVector.x * t, anim.MovementVector.y * t, anim.MovementVector.z * t });
                        }
                    }
                    if (mvecTrack && mvecTrack->PositionTrack.empty()) {
                        ImGui::SameLine(); if (ImGui::Button("Add First Keyframe")) { mvecTrack->PositionTrack.push_back(Vec3{ 0, 0, 0 }); mvecTrack->FrameCount = 1; }
                        ImGui::SameLine(); if (ImGui::Button("Delete Empty Track")) anim.HelperTracks.erase(std::remove_if(anim.HelperTracks.begin(), anim.HelperTracks.end(), [](const AnimTrack& t) { return t.BoneName == ""; }), anim.HelperTracks.end());
                    }
                }
                else {
                    if (ImGui::Button("Delete Curve")) {
                        anim.HelperTracks.erase(std::remove_if(anim.HelperTracks.begin(), anim.HelperTracks.end(), [](const AnimTrack& t) { return t.BoneName == ""; }), anim.HelperTracks.end()); mvecTrack = nullptr;
                    }
                    if (mvecTrack) {
                        ImGui::SameLine(); if (ImGui::Button("Add Keyframe at End")) { Vec3 lastPos = mvecTrack->PositionTrack.back(); mvecTrack->PositionTrack.push_back(lastPos); mvecTrack->FrameCount = (uint32_t)mvecTrack->PositionTrack.size(); }
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Click anywhere on the graphs below to edit a frame:");
                        std::vector<float> posX, posY, posZ;
                        for (const auto& p : mvecTrack->PositionTrack) { posX.push_back(p.x); posY.push_back(p.y); posZ.push_back(p.z); }

                        auto getBounds = [](float target) {
                            float bMin = (std::min)(0.0f, target); float bMax = (std::max)(0.0f, target);
                            if (std::abs(bMax - bMin) < 0.001f) { bMin = FLT_MAX; bMax = FLT_MAX; } return std::make_pair(bMin, bMax);
                            };
                        auto bX = getBounds(anim.MovementVector.x); auto bY = getBounds(anim.MovementVector.y); auto bZ = getBounds(anim.MovementVector.z);

                        static int selectedMvecFrame = -1;
                        auto DrawPlotAndCatchClick = [&](const char* label, const std::vector<float>& data, float scaleMin, float scaleMax) {
                            int count = (int)data.size(); ImGui::PlotLines(label, data.data(), count, 0, nullptr, scaleMin, scaleMax, ImVec2(0, 50));
                            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                                float mouseX = ImGui::GetMousePos().x - ImGui::GetItemRectMin().x; float width = ImGui::GetItemRectSize().x;
                                int idx = count > 1 ? (int)std::round((mouseX / width) * (count - 1)) : 0;
                                selectedMvecFrame = std::clamp(idx, 0, count - 1); ImGui::OpenPopup("EditKeyframePopup");
                            }
                            };

                        DrawPlotAndCatchClick("X Axis", posX, bX.first, bX.second); DrawPlotAndCatchClick("Y Axis", posY, bY.first, bY.second); DrawPlotAndCatchClick("Z Axis", posZ, bZ.first, bZ.second);

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
    }

    ImGui::Dummy(ImVec2(0, 5));

    if (ImGui::CollapsingHeader("Time Events (TMEV)")) {
        if (ImGui::Button("Add New Event")) anim.TimeEvents.push_back({ "NEW_EVENT", 0.0f });
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

    if (ImGui::CollapsingHeader("Partial Animation Bone Mask (AMSK)")) {
        if (anim.BoneMaskBits.empty()) {
            ImGui::TextDisabled("This is a Full Body animation (No Mask).");
            if (ImGui::Button("Convert to Partial Animation")) {
                uint32_t wordCount = ((uint32_t)anim.Tracks.size() + 31) / 32;
                anim.BoneMaskBits.resize(wordCount, 0xFFFFFFFF); entryType = 9;
            }
        }
        else {
            if (ImGui::Button("Clear Mask (Convert to Normal)")) { anim.BoneMaskBits.clear(); entryType = 6; }
            ImGui::SameLine(); if (ImGui::Button("Disable All")) for (auto& word : anim.BoneMaskBits) word = 0;
            ImGui::SameLine(); if (ImGui::Button("Enable All")) for (auto& word : anim.BoneMaskBits) word = 0xFFFFFFFF;

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

    ImGui::Dummy(ImVec2(0, 5));

    if (ImGui::CollapsingHeader("Tracks & Keyframes")) {
        if (ImGui::BeginChild("TracksList", ImVec2(0, 0), true)) {
            for (size_t i = 0; i < anim.Tracks.size(); i++) {
                const auto& track = anim.Tracks[i];
                bool isEnabled = true;
                if (!anim.BoneMaskBits.empty()) {
                    uint32_t word = (uint32_t)(i / 32); uint32_t bit = (uint32_t)(i % 32);
                    if (word < anim.BoneMaskBits.size()) isEnabled = (anim.BoneMaskBits[word] & (1 << bit)) != 0;
                }

                ImVec4 headerCol = isEnabled ? ImVec4(1, 1, 1, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, headerCol);
                bool open = ImGui::CollapsingHeader((track.BoneName + (isEnabled ? "" : " (MASKED OUT)") + "##" + std::to_string(i)).c_str());
                ImGui::PopStyleColor();

                if (open) {
                    ImGui::Indent();
                    ImGui::Text("FPS: %.2f | Frames: %u", track.SamplesPerSecond, track.FrameCount);
                    if (ImGui::BeginTable(("Table" + std::to_string(i)).c_str(), 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 200))) {
                        ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed, 40);
                        ImGui::TableSetupColumn("Pos"); ImGui::TableSetupColumn("Rot"); ImGui::TableHeadersRow();
                        ImGuiListClipper clipper; clipper.Begin(track.FrameCount);
                        while (clipper.Step()) {
                            for (int f = clipper.DisplayStart; f < clipper.DisplayEnd; f++) {
                                Vec3 p = { 0.0f, 0.0f, 0.0f }; Vec4 r = { 0.0f, 0.0f, 0.0f, 1.0f };
                                track.EvaluateFrame(f, p, r);

                                ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("%d", f);
                                ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f,%.2f,%.2f", p.x, p.y, p.z);
                                ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f,%.2f,%.2f,%.2f", r.x, r.y, r.z, r.w);
                            }
                        }
                        ImGui::EndTable();
                    }
                    ImGui::Unindent();
                }
            }
            ImGui::EndChild();
        }
    }
}