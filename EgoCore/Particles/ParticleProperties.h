#pragma once
#include "imgui.h"
#include "ParticleParser.h"
#include "ParticleRenderer.h"
#include "BinaryParser.h"
#include "MeshRenderer.h"
#include "BankBackend.h"
#include <string>
#include <cstring>
#include <algorithm>
#include <cmath>

extern ID3D11Device* g_pd3dDevice;
extern ImTextureID g_PlayTexture;
extern ImTextureID g_PauseTexture;
extern ImTextureID g_StopTexture;
extern ImTextureID g_LoopTexture;
extern ImTextureID g_AddTexture;
extern ImTextureID g_DeleteTexture;

ID3D11ShaderResourceView* LoadTextureForMesh(int textureID);

// --- THEME CONSTANTS ---
static const ImVec4 kGoldAccent = ImVec4(0.95f, 0.82f, 0.45f, 1.0f);
static const ImVec4 kDeleteTint = ImVec4(0.95f, 0.40f, 0.40f, 1.0f);
static const ImVec4 kAddTint    = ImVec4(0.40f, 0.95f, 0.50f, 1.0f);

// --- STATE VARIABLES ---
static bool g_TriggerParticleTexPopup = false;
static bool g_ShowParticleTexPopup = false;
static char g_ParticleTexSearchBuf[128] = "";
static int32_t* g_ParticleTargetTexIDPtr = nullptr;
static int32_t g_ParticlePreviewTexID = -1;

static bool g_ShowParticleMeshPicker = false;
static char g_ParticleMeshSearchBuf[128] = "";
static int32_t* g_ParticleTargetMeshIDPtr = nullptr;
static MeshRenderer g_ParticleMeshRenderer;
static C3DMeshContent g_ParticlePreviewMesh;
static bool g_ParticleMeshUploadNeeded = false;
static bool g_ParticleMeshPreviewOpen = false;

static int32_t g_ParticlePreviewMeshID = -1;
static int32_t g_LoadedParticlePreviewMeshID = -1;

// --- ADD COMPONENT MODAL STATE ---
static bool g_ShowAddComponentModal = false;
static int g_TargetSystemIndexForAdd = -1;
static int g_SelectedNewCompType = 0;

// --- SIMULATOR & VIEWPORT STATE ---
static bool g_ShowParticleRenderer = true;
static ParticleRenderer g_ParticleRenderer;
static ParticleSimulator g_ParticleSimulator;
static const CParticleEmitter* g_LastSimulatedEmitter = nullptr;
static bool g_ParticleSimNeedsReset = true;
static bool g_ParticlePlaying = false;
static float g_ParticleSimTime = 0.0f;
static float g_ParticleSimDuration = 10.0f;
static bool g_ParticleLoop = true;
static float g_ParticlePlaybackSpeed = 1.0f;

// --- ICON BUTTON HELPERS ---
inline bool DrawAddIconButton(const char* id, float size = 18.0f, const char* tooltip = "Add") {
    bool clicked = false;
    if (g_AddTexture) {
        clicked = ImGui::ImageButton(id, g_AddTexture, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), kAddTint);
    }
    else {
        clicked = ImGui::Button((std::string("+##") + id).c_str(), ImVec2(size + 6, size + 2));
    }
    if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
    return clicked;
}

inline bool DrawDeleteIconButton(const char* id, float size = 18.0f, const char* tooltip = "Delete") {
    bool clicked = false;
    if (g_DeleteTexture) {
        clicked = ImGui::ImageButton(id, g_DeleteTexture, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), kDeleteTint);
    }
    else {
        clicked = ImGui::Button((std::string("-##") + id).c_str(), ImVec2(size + 6, size + 2));
    }
    if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
    return clicked;
}

// --- UI HELPERS ---
inline void DrawString(const char* label, std::string& str) {
    char buf[256];
    strncpy_s(buf, str.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText(label, buf, sizeof(buf))) {
        str = buf;
    }
}

inline void DrawU32(const char* label, uint32_t& v, bool hex = false) {
    ImGui::InputScalar(label, ImGuiDataType_U32, &v, NULL, NULL, hex ? "%08X" : "%u");
}

inline void DrawColour(const char* label, CRGBColour& col) {
    float c[4] = { col.R / 255.0f, col.G / 255.0f, col.B / 255.0f, col.A / 255.0f };
    if (ImGui::ColorEdit4(label, c)) {
        col.R = (uint8_t)(c[0] * 255.0f);
        col.G = (uint8_t)(c[1] * 255.0f);
        col.B = (uint8_t)(c[2] * 255.0f);
        col.A = (uint8_t)(c[3] * 255.0f);
    }
}

inline void DrawVector3(const char* label, C3DVector& vec) {
    ImGui::DragFloat3(label, &vec.X, 0.05f);
}

inline void DrawHashParam(const char* label, uint32_t& param) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::SameLine(220);

    ImGui::SetNextItemWidth(90);
    ImGui::InputScalar((std::string("##val_") + label).c_str(), ImGuiDataType_U32, &param);

    ImGui::SameLine();
    char buf[128] = "";
    ImGui::SetNextItemWidth(130);
    if (ImGui::InputTextWithHint((std::string("##txt_") + label).c_str(), "String to Hash...", buf, 128)) {
        if (strlen(buf) > 0) {
            param = BinaryParser::CalculateCRC32_Fable(buf);
        }
    }
}

inline void DrawParticleTexRow(const char* label, int32_t& bankIndex) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::SameLine(220);

    ImGui::SetNextItemWidth(80);
    ImGui::InputInt((std::string("##id_") + label).c_str(), &bankIndex, 0, 0);

    ImGui::SameLine();
    if (DrawAddIconButton((std::string("##btn_") + label).c_str(), 18.0f, "Choose Texture")) {
        g_ParticleTargetTexIDPtr = &bankIndex;
        g_ParticlePreviewTexID = bankIndex;
        g_ParticleTexSearchBuf[0] = '\0';
        g_TriggerParticleTexPopup = true;
    }

    if (bankIndex > 0) {
        ImGui::SameLine();
        ID3D11ShaderResourceView* srv = LoadTextureForMesh(bankIndex);
        if (srv) {
            ImGui::Image((void*)srv, ImVec2(22, 22));
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Image((void*)srv, ImVec2(256, 256));
                ImGui::PushTextWrapPos(256.0f);
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Texture ID: %d", bankIndex);
                auto info = g_ParticleRenderer.GetTextureInfo(bankIndex);
                if (info.Width > 0 && info.Height > 0) {
                    ImGui::Text("Size: %dx%d", info.Width, info.Height);
                    if (info.IsSpriteSheet) {
                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Sprite Sheet: %dx%d (%d frames)", info.FrameWidth, info.FrameHeight, info.FrameCount);
                    }
                    else if (info.FrameCount > 1) {
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Multi-Frame (%d frames)", info.FrameCount);
                    }
                    else {
                        ImGui::TextDisabled("Single Frame");
                    }
                    if (info.IsDXT1) {
                        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "DXT1 (Transparent Black)");
                    }
                }
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
    }
}

inline void DrawParticleMeshRow(const char* label, int32_t& bankIndex) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::SameLine(220);

    ImGui::SetNextItemWidth(80);
    ImGui::InputInt((std::string("##id_") + label).c_str(), &bankIndex, 0, 0);

    ImGui::SameLine();
    if (DrawAddIconButton((std::string("##btn_") + label).c_str(), 18.0f, "Choose Type 4 Mesh")) {
        g_ParticleTargetMeshIDPtr = &bankIndex;
        g_ParticlePreviewMeshID = bankIndex;
        g_LoadedParticlePreviewMeshID = -1;
        g_ParticleMeshSearchBuf[0] = '\0';
        g_ShowParticleMeshPicker = true;
    }
}

inline void DrawSplineBase(CPSCSplineBase* spline) {
    if (ImGui::TreeNodeEx("Spline Base Data", ImGuiTreeNodeFlags_None)) {
        ImGui::Text("Control Points: %d", (int)spline->ControlPoints.size());
        for (size_t i = 0; i < spline->ControlPoints.size(); i++) {
            DrawVector3(("  Pt " + std::to_string(i)).c_str(), spline->ControlPoints[i]);
        }

        ImGui::Checkbox("Has Random Offset Data", &spline->HasRandomOffsetData);
        if (spline->HasRandomOffsetData) {
            ImGui::Checkbox("OffsetOnlyAtStartup", &spline->OffsetOnlyAtStartup);
            ImGui::SameLine();
            ImGui::Checkbox("ResetEachFrame", &spline->ResetEachFrame);

            for (size_t i = 0; i < spline->RandomOffsets.size(); i++) {
                ImGui::PushID((int)i);
                ImGui::TextDisabled("Offset %d", (int)i);
                DrawVector3("Limit", spline->RandomOffsets[i].Limit);
                DrawVector3("Speed", spline->RandomOffsets[i].Speed);
                ImGui::PopID();
            }
        }

        ImGui::Checkbox("Has Pos Param Data", &spline->HasPosParamData);
        if (spline->HasPosParamData) {
            ImGui::Checkbox("MaintainSplineShape", &spline->MaintainSplineShape);
            for (size_t i = 0; i < spline->PosParams.size(); i++) {
                DrawU32(("Param " + std::to_string(i)).c_str(), spline->PosParams[i]);
            }
        }
        ImGui::TreePop();
    }
}

inline void DrawParticleComponent(std::shared_ptr<CParticleComponent>& comp, bool& requestDelete, size_t compIdx) {
    if (!comp) return;
    ImGui::PushID(comp.get());

    const char* badge = "[Modifier]";
    if (comp->ClassName == "CPSCEmitterGeneric") badge = "[Emitter]";
    else if (comp->ClassName == "CPSCUpdateNormal") badge = "[Physics]";
    else if (comp->ClassName == "CPSCRenderSprite" || comp->ClassName == "CPSCSingleSprite" || comp->ClassName == "CPSCRenderMesh") badge = "[Render]";

    ImGui::SetNextItemAllowOverlap();
    std::string headerLabel = std::string(badge) + " " + comp->ClassName;
    bool isOpen = ImGui::TreeNodeEx((void*)comp.get(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowOverlap, "%s", headerLabel.c_str());

    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 26);
    if (DrawDeleteIconButton(("##DelComp_" + std::to_string(compIdx)).c_str(), 16.0f, "Delete Component")) {
        requestDelete = true;
    }

    if (isOpen) {
        ImGui::Indent(8.0f);
        ImGui::Spacing();

        ImGui::Checkbox("Enabled", &comp->Enabled);
        ImGui::SameLine();
        ImGui::Checkbox("Visible", &comp->Visible);
        ImGui::SameLine();
        ImGui::TextDisabled("(CRC32: 0x%08X)", comp->InstanceID);
        ImGui::Separator();

        if (auto* rs = dynamic_cast<CPSCRenderSprite*>(comp.get())) {
            DrawParticleTexRow("SpriteBankIndex", rs->SpriteBankIndex);
            DrawParticleTexRow("TrailBankIndex", rs->TrailBankIndex);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Colours");
            ImGui::Separator();
            DrawColour("StartColour", rs->StartColour);
            DrawColour("MidColour", rs->MidColour);
            DrawColour("EndColour", rs->EndColour);

            ImGui::Checkbox("UseStartColour", &rs->UseStartColour); ImGui::SameLine();
            ImGui::Checkbox("UseMidColour", &rs->UseMidColour); ImGui::SameLine();
            ImGui::Checkbox("UseEndColour", &rs->UseEndColour);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Render Settings");
            ImGui::Separator();
            DrawU32("BlendMode", rs->BlendMode);
            DrawU32("TrailBlendMode", rs->TrailBlendMode);
            DrawU32("BlendOp", rs->BlendOp);
            DrawU32("TrailBlendOp", rs->TrailBlendOp);
            DrawU32("SpriteFlags (Hex)", rs->SpriteFlags, true);
            DrawU32("NoCrossedSprites", rs->NoCrossedSprites);

            ImGui::DragFloat("Start Render Size", &rs->StartRenderSize, 0.1f);
            ImGui::DragFloat("End Render Size", &rs->EndRenderSize, 0.1f);
            ImGui::DragFloat("Anim Time Secs", &rs->AnimationTimeSecs, 0.1f);
            ImGui::Checkbox("ForceAnimTime", &rs->ForceAnimationTime);
            ImGui::DragFloat("Trail Width", &rs->TrailWidth, 0.1f);
            DrawU32("Trail Length Int", rs->TrailLengthInteger);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Fades & Flickering");
            ImGui::Separator();
            DrawU32("FadeInEndInteger", rs->FadeInEndInteger);
            DrawU32("FadeOutBeginInteger", rs->FadeOutBeginInteger);
            ImGui::Checkbox("AlphaFadeEnable", &rs->AlphaFadeEnable); ImGui::SameLine();
            ImGui::Checkbox("SizeFadeEnable", &rs->SizeFadeEnable);
            ImGui::DragFloat("Alpha Fade Minimum", &rs->AlphaFadeMinimum, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Size Fade Minimum", &rs->SizeFadeMinimum, 0.01f);

            ImGui::Checkbox("Flicker Enable", &rs->FlickerEnable);
            DrawU32("FlickerMinAlphaInteger", rs->FlickerMinAlphaInteger);
            DrawU32("FlickerMinSizeInteger", rs->FlickerMinSizeInteger);
            ImGui::DragFloat("Flicker Speed", &rs->FlickerSpeed, 0.1f);
            ImGui::DragFloat("Flicker Bias", &rs->FlickerBias, 0.01f);
        }
        else if (auto* un = dynamic_cast<CPSCUpdateNormal*>(comp.get())) {
            DrawString("DecalEmitterName", un->DecalEmitterName);
            ImGui::DragFloat("SystemLifeSecs", &un->SystemLifeSecs, 0.1f);
            ImGui::DragFloat("ParticleLifeSecs", &un->ParticleLifeSecs, 0.1f);
            ImGui::DragFloat("WindFactor", &un->WindFactor, 0.01f);
            ImGui::DragFloat("GravityFactor", &un->GravityFactor, 0.01f);
            ImGui::DragFloat("AirResistance", &un->AirResistance, 0.01f);
            ImGui::DragFloat("ParticleBounce", &un->ParticleBounce, 0.01f);
            ImGui::DragFloat("AccelScale", &un->ParticleAccelerationScale, 0.01f);
            DrawVector3("ParticleSystemOffset", un->ParticleSystemOffset);
            DrawVector3("ParticleAcceleration", un->ParticleAcceleration);
            DrawVector3("RotationAxis", un->RotationAxis);
            ImGui::DragFloat("RotationMinAngleSpeed", &un->RotationMinAngleSpeed, 0.1f);
            ImGui::DragFloat("RotationMaxAngleSpeed", &un->RotationMaxAngleSpeed, 0.1f);
            DrawVector3("RandomisePosSpeed", un->RandomisePosSpeed);
            DrawVector3("RandomisePosScale", un->RandomisePosScale);
            DrawU32("FadeInEndInteger", un->FadeInEndInteger);
            DrawU32("FadeOutBeginInteger", un->FadeOutBeginInteger);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Lifecycle & Fading");
            ImGui::Separator();
            ImGui::Checkbox("UseSystemLifeSecs", &un->UseSystemLifeSecs); ImGui::SameLine();
            ImGui::Checkbox("UseParticleLifeSecs", &un->UseParticleLifeSecs);
            ImGui::Checkbox("SystemAlphaFadeEnable", &un->SystemAlphaFadeEnable); ImGui::SameLine();
            ImGui::Checkbox("EmissionFadeEnable", &un->EmissionFadeEnable);
            ImGui::DragFloat("SystemAlphaFadeMinimum", &un->SystemAlphaFadeMinimum, 0.01f);
            ImGui::DragFloat("EmissionFadeMinimum", &un->EmissionFadeMinimum, 0.01f);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Physics & Interaction");
            ImGui::Separator();
            ImGui::Checkbox("ParticleCollideWithGround", &un->ParticleCollideWithGround);
            ImGui::Checkbox("ParticleCollideWithAnything", &un->ParticleCollideWithAnything);
            ImGui::Checkbox("ParticleDieOnCollision", &un->ParticleDieOnCollision);
            ImGui::Checkbox("UseAllAttractors", &un->UseAllAttractors);
            ImGui::Checkbox("StayWithEmitter", &un->StayWithEmitter);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Rotation & Orientation");
            ImGui::Separator();
            ImGui::Checkbox("UseRandomRotationAxis", &un->UseRandomRotationAxis);
            ImGui::Checkbox("UseRandomInitialRotation", &un->UseRandomInitialRotation);
            ImGui::Checkbox("SetOrientationFromDirection", &un->SetOrientationFromDirection);
            ImGui::Checkbox("SetOrientationFromGame", &un->SetOrientationFromGame);
            ImGui::DragFloat("InitialRotationX", &un->InitialRotationX, 0.1f);
            ImGui::DragFloat("InitialRotationY", &un->InitialRotationY, 0.1f);
            ImGui::DragFloat("InitialRotationZ", &un->InitialRotationZ, 0.1f);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Randomise Position");
            ImGui::Separator();
            ImGui::Checkbox("RandomisePosEnable", &un->RandomisePosEnable);
            ImGui::Checkbox("RandomisePosDistVaryEnable", &un->RandomisePosDistVaryEnable);
            ImGui::DragFloat("RandomisePosDistVaryScale", &un->RandomisePosDistVaryScale, 0.1f);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Decals & Parameters");
            ImGui::Separator();
            ImGui::Checkbox("ParticleCreateDecal", &un->ParticleCreateDecal);
            ImGui::Checkbox("ParticleCreateDecalEmitter", &un->ParticleCreateDecalEmitter);
            DrawHashParam("AccelerationParam", un->AccelerationParam);
            DrawHashParam("OrientFromGameParam", un->OrientFromGameParam);
        }
        else if (auto* eg = dynamic_cast<CPSCEmitterGeneric*>(comp.get())) {
            DrawHashParam("EmitterPosParam", eg->EmitterPosParam);
            DrawHashParam("DirectionParamName", eg->DirectionParamName);
            DrawU32("EmitterType", eg->EmitterType);
            DrawU32("NoParticlesToStart", eg->NoParticlesToStart);
            DrawU32("NoParticlesToStartRand", eg->NoParticlesToStartRand);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Physics & Sizing");
            ImGui::Separator();
            ImGui::DragFloat("Particles Per Sec", &eg->ParticlesPerSecond, 0.5f);
            ImGui::DragFloat("Min Speed", &eg->MinSpeed, 0.1f);
            ImGui::DragFloat("Max Speed", &eg->MaxSpeed, 0.1f);
            ImGui::DragFloat("Emitter Size", &eg->EmitterSize, 0.1f);
            ImGui::DragFloat("Radial Bias", &eg->RadialBias, 0.1f);
            DrawVector3("Non-Uniform Scaling", eg->NonUniformScaling);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Lifecycles");
            ImGui::Separator();
            ImGui::DragFloat("Emitter Life Secs", &eg->EmitterLifeSecs, 0.1f);
            ImGui::DragFloat("Emitter Start Time", &eg->EmitterStartTime, 0.1f);
            ImGui::DragFloat("Emitter Timeline Secs", &eg->EmitterTimelineSecs, 0.1f);
            ImGui::Checkbox("UseEmitterLifeSecs", &eg->UseEmitterLifeSecs);
            ImGui::Checkbox("UseEmitterTimelineSecs", &eg->UseEmitterTimelineSecs);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Direction & Orientation");
            ImGui::Separator();
            ImGui::Checkbox("Solid", &eg->Solid);
            DrawU32("Angular Perturbation Int", eg->AngularPerturbationInteger);
            ImGui::Checkbox("OrientationXY", &eg->OrientationXY); ImGui::SameLine();
            ImGui::Checkbox("OrientationXZ", &eg->OrientationXZ); ImGui::SameLine();
            ImGui::Checkbox("OrientationYZ", &eg->OrientationYZ);

            ImGui::Checkbox("UseOutwardDirection", &eg->UseOutwardDirection);
            ImGui::Checkbox("UseForwardDirection", &eg->UseForwardDirection);
            ImGui::Checkbox("OppositeDirection", &eg->OppositeDirection);
            ImGui::Checkbox("UseParamDirection", &eg->UseParamDirection);

            ImGui::Checkbox("UseCustomDirection", &eg->UseCustomDirection);
            ImGui::Checkbox("UseRandom2DDirection", &eg->UseRandom2DDirection);
            ImGui::Checkbox("UseRandom3DDirection", &eg->UseRandom3DDirection);
            DrawVector3("Custom Direction Vector", eg->CustomDirection);

            ImGui::Checkbox("HasSpline", &eg->HasSpline);
            if (eg->HasSpline) {
                ImGui::Separator();
                ImGui::TextDisabled("Spline Data:");
                ImGui::DragFloat("SplineTension", &eg->SplineTension, 0.05f);
                ImGui::Text("Control Points: %d", (int)eg->SplineControlPoints.size());
                for (size_t i = 0; i < eg->SplineControlPoints.size(); i++) {
                    DrawVector3(("  Pt " + std::to_string(i)).c_str(), eg->SplineControlPoints[i]);
                }
            }
        }
        else if (auto* rm = dynamic_cast<CPSCRenderMesh*>(comp.get())) {
            DrawParticleMeshRow("BankIndex", rm->BankIndex);
            DrawParticleTexRow("TrailBankIndex", rm->TrailBankIndex);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Mesh Colours");
            ImGui::Separator();
            DrawColour("StartColour", rm->StartColour);
            DrawColour("MidColour", rm->MidColour);
            DrawColour("EndColour", rm->EndColour);
            ImGui::Checkbox("UseStartColour", &rm->UseStartColour); ImGui::SameLine();
            ImGui::Checkbox("UseMidColour", &rm->UseMidColour); ImGui::SameLine();
            ImGui::Checkbox("UseEndColour", &rm->UseEndColour);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Trail Colours");
            ImGui::Separator();
            DrawColour("TrailStartCol", rm->TrailStartColour);
            DrawColour("TrailMidCol", rm->TrailMidColour);
            DrawColour("TrailEndCol", rm->TrailEndColour);
            ImGui::Checkbox("TrailUseStartColour", &rm->TrailUseStartColour); ImGui::SameLine();
            ImGui::Checkbox("TrailUseMidColour", &rm->TrailUseMidColour); ImGui::SameLine();
            ImGui::Checkbox("TrailUseEndColour", &rm->TrailUseEndColour);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Render Settings");
            ImGui::Separator();
            DrawU32("BlendMode", rm->BlendMode);
            DrawU32("TrailBlendMode", rm->TrailBlendMode);
            DrawU32("BlendOp", rm->BlendOp);
            DrawU32("TrailBlendOp", rm->TrailBlendOp);
            DrawVector3("StartRenderSize", rm->StartRenderSize);
            DrawVector3("EndRenderSize", rm->EndRenderSize);

            DrawHashParam("RenderSizeParam", rm->RenderSizeParam);
            ImGui::Checkbox("UseRenderSizeParam", &rm->UseRenderSizeParam);
            ImGui::Checkbox("CentredOnPos", &rm->CentredOnPos);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Fading & Flickering");
            ImGui::Separator();
            DrawU32("FadeInEndInteger", rm->FadeInEndInteger);
            DrawU32("FadeOutBeginInteger", rm->FadeOutBeginInteger);
            ImGui::Checkbox("AlphaFadeEnable", &rm->AlphaFadeEnable); ImGui::SameLine();
            ImGui::Checkbox("SizeFadeEnable", &rm->SizeFadeEnable);
            ImGui::DragFloat("AlphaFadeMinimum", &rm->AlphaFadeMinimum, 0.01f);
            ImGui::DragFloat("SizeFadeMinimum", &rm->SizeFadeMinimum, 0.01f);

            ImGui::Checkbox("FlickerEnable", &rm->FlickerEnable);
            DrawU32("FlickerMinAlphaInteger", rm->FlickerMinAlphaInteger);
            DrawU32("FlickerMinSizeInteger", rm->FlickerMinSizeInteger);
            ImGui::DragFloat("FlickerBias", &rm->FlickerBias, 0.01f);
            ImGui::DragFloat("FlickerSpeed", &rm->FlickerSpeed, 0.1f);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Trail Specs");
            ImGui::Separator();
            DrawU32("TrailLengthInt", rm->TrailLengthInteger);
            ImGui::DragFloat("TrailWidth", &rm->TrailWidth, 0.1f);
        }
        else if (auto* lgt = dynamic_cast<CPSCLight*>(comp.get())) {
            DrawHashParam("LightPositionParam", lgt->LightPositionParam);
            ImGui::DragFloat("LightLifeSecs", &lgt->LightLifeSecs, 0.1f);
            ImGui::DragFloat("RespawnDelaySecs", &lgt->LightRespawnDelaySecs, 0.1f);
            ImGui::DragFloat("LightStartTime", &lgt->LightStartTime, 0.1f);
            ImGui::DragFloat("LightTimelineSecs", &lgt->LightTimelineSecs, 0.1f);
            ImGui::DragFloat("StartRenderWorldRadius", &lgt->LightStartRenderWorldRadius, 0.1f);
            ImGui::DragFloat("EndRenderWorldRadius", &lgt->LightEndRenderWorldRadius, 0.1f);
            ImGui::InputInt("LightAttenuationFactor", &lgt->LightAttenuationFactor);
            ImGui::InputInt("FadeInEndInteger", &lgt->LightFadeInEndInteger);
            ImGui::InputInt("FadeOutBeginInteger", &lgt->LightFadeOutBeginInteger);
            ImGui::DragFloat("WorldRadiusFadeMinimum", &lgt->LightWorldRadiusFadeMinimum, 0.01f);
            DrawColour("ColourFadeMinimum", lgt->LightColourFadeMinimum);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Flags");
            ImGui::Separator();
            ImGui::Checkbox("LightEnabled", &lgt->LightEnabled);
            ImGui::Checkbox("LightRespawns", &lgt->LightRespawns);
            ImGui::Checkbox("LightWorldRadiusFadeEnable", &lgt->LightWorldRadiusFadeEnable);
            ImGui::Checkbox("LightColourFadeEnable", &lgt->LightColourFadeEnable);
            ImGui::Checkbox("LightUseLifeSecs", &lgt->LightUseLifeSecs);
            ImGui::Checkbox("LightUseTimelineSecs", &lgt->LightUseTimelineSecs);
            ImGui::Checkbox("LightUseStartColour", &lgt->LightUseStartColour);
            ImGui::Checkbox("LightUseMidColour", &lgt->LightUseMidColour);
            ImGui::Checkbox("LightUseEndColour", &lgt->LightUseEndColour);
            ImGui::Checkbox("LightUseFadeColour", &lgt->LightUseFadeColour);
            ImGui::Checkbox("LightHasInitializedTime", &lgt->LightHasInitializedTime);

            ImGui::Dummy(ImVec2(0, 4));
            DrawColour("LightStartColour", lgt->LightStartColour);
            DrawColour("LightMidColour", lgt->LightMidColour);
            DrawColour("LightEndColour", lgt->LightEndColour);
        }
        else if (auto* sp = dynamic_cast<CPSCSpline*>(comp.get())) {
            ImGui::Checkbox("SplineBounce", &sp->SplineBounce);
            ImGui::Checkbox("ScaleSplineSpeed", &sp->ScaleSplineSpeed);
            ImGui::DragFloat("SplineAnimSpeed", &sp->SplineAnimSpeed, 0.1f);
            DrawSplineBase(sp);
        }
        else if (auto* ss = dynamic_cast<CPSCSingleSprite*>(comp.get())) {
            DrawParticleTexRow("SpriteBankIndex", ss->SpriteBankIndex);
            ImGui::DragFloat("AnimTimeSecs", &ss->AnimationTimeSecs, 0.1f);
            DrawColour("StartColour", ss->StartColour);
            DrawColour("MidColour", ss->MidColour);
            DrawColour("EndColour", ss->EndColour);
            ImGui::DragFloat("StartRenderSize", &ss->StartRenderSize, 0.1f);
            ImGui::DragFloat("EndRenderSize", &ss->EndRenderSize, 0.1f);
            DrawSplineBase(ss);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Colours & Fades");
            ImGui::Separator();
            ImGui::Checkbox("UseStartColour", &ss->UseStartColour); ImGui::SameLine();
            ImGui::Checkbox("UseMidColour", &ss->UseMidColour); ImGui::SameLine();
            ImGui::Checkbox("UseEndColour", &ss->UseEndColour);
            ImGui::Checkbox("AlphaFadeEnable", &ss->AlphaFadeEnable); ImGui::SameLine();
            ImGui::Checkbox("SizeFadeEnable", &ss->SizeFadeEnable); ImGui::SameLine();
            ImGui::Checkbox("ForceAnimationTime", &ss->ForceAnimationTime);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Render Flags");
            ImGui::Separator();
            ImGui::Checkbox("SelfIlluminating", &ss->SelfIlluminating);
            ImGui::Checkbox("RotateAroundCentre", &ss->RotateAroundCentre);
            ImGui::Checkbox("FaceMe2D", &ss->FaceMe2D);
            ImGui::Checkbox("FaceMe3D", &ss->FaceMe3D);
            ImGui::Checkbox("CrossedSprites", &ss->CrossedSprites);
            ImGui::Checkbox("StayWithEmitter", &ss->StayWithEmitter);
            ImGui::Checkbox("UsePosition", &ss->UsePosition);
            ImGui::Checkbox("UseSplinePoints", &ss->UseSplinePoints);

            ImGui::Dummy(ImVec2(0, 4));
            DrawHashParam("PositionParam", ss->PositionParam);
            DrawParticleTexRow("TrailBankIndex", ss->TrailBankIndex);
            ImGui::DragFloat("TrailWidth", &ss->TrailWidth, 0.1f);
        }
        else if (auto* at = dynamic_cast<CPSCAttractor*>(comp.get())) {
            ImGui::Checkbox("AttractorEnabled", &at->AttractorEnabled);
            ImGui::Checkbox("AttractorUseParamPosition", &at->AttractorUseParamPosition);
            DrawHashParam("AttractorPositionParam", at->AttractorPositionParam);
            DrawHashParam("AttractorPositionParamName", at->AttractorPositionParamName);
            ImGui::InputInt("AttractorInfluenceFallOff", &at->AttractorInfluenceFallOff);
            ImGui::DragFloat("AttractorInfluenceRadius", &at->AttractorInfluenceRadius, 0.1f);
            ImGui::DragFloat("AttractorInfluenceForce", &at->AttractorInfluenceForce, 0.1f);

            ImGui::Text("User Points: %d", (int)at->AttractorUserPointsArray.size());
            for (size_t i = 0; i < at->AttractorUserPointsArray.size(); i++) {
                DrawVector3(("Point " + std::to_string(i)).c_str(), at->AttractorUserPointsArray[i]);
            }
        }
        else if (auto* ob = dynamic_cast<CPSCOrbit*>(comp.get())) {
            DrawHashParam("CentreParam", ob->CentreParam);
            ImGui::Text("Orbits Count: %d", (int)ob->OrbitsData.size());

            for (size_t i = 0; i < ob->OrbitsData.size(); i++) {
                auto& orbit = ob->OrbitsData[i];
                if (ImGui::TreeNodeEx((void*)(intptr_t)i, ImGuiTreeNodeFlags_None, "Orbit %d", (int)i)) {
                    ImGui::InputInt("Type", &orbit.Type);
                    ImGui::Checkbox("Enabled", &orbit.Enabled);
                    ImGui::Checkbox("CycleFlag", &orbit.CycleFlag);
                    ImGui::DragFloat("Radius", &orbit.Radius, 0.1f);
                    ImGui::DragFloat("Expand", &orbit.Expand, 0.1f);
                    ImGui::DragFloat("CycleTime", &orbit.CycleTime, 0.1f);
                    ImGui::DragFloat("Squeeze Scale", &orbit.SqueezeScale, 0.01f);
                    ImGui::DragFloat("Squeeze Angle", &orbit.SqueezeAngle, 0.1f);
                    ImGui::DragFloat("Rotate Speed", &orbit.RotateSpeed, 0.1f);
                    ImGui::DragFloat("Rotate Start", &orbit.RotateStart, 0.1f);
                    ImGui::DragFloat("Rotate Speed Rand", &orbit.RotateSpeedRandom, 0.1f);
                    ImGui::DragFloat("Rotate Start Rand", &orbit.RotateStartRandom, 0.1f);
                    ImGui::TreePop();
                }
            }
        }
        else if (auto* dr = dynamic_cast<CPSCDecalRenderer*>(comp.get())) {
            DrawParticleTexRow("DecalBankIndex", dr->DecalBankIndex);
            ImGui::DragFloat("DecalLifeSecs", &dr->DecalLifeSecs, 0.1f);
            DrawColour("StartColour", dr->StartColour);
            DrawColour("MidColour", dr->MidColour);
            DrawColour("EndColour", dr->EndColour);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Render Settings");
            ImGui::Separator();
            DrawU32("BlendMode", dr->BlendMode);
            DrawU32("BlendOp", dr->BlendOp);
            DrawU32("SpriteAlignment", dr->SpriteAlignment);
            DrawU32("FadeInEndInteger", dr->FadeInEndInteger);
            DrawU32("FadeOutBeginInteger", dr->FadeOutBeginInteger);
            DrawU32("StartRenderSizeInteger", dr->StartRenderSizeInteger);
            DrawU32("EndRenderSizeInteger", dr->EndRenderSizeInteger);
            DrawU32("AlphaFadeMinimumInteger", dr->AlphaFadeMinimumInteger);
            DrawU32("SizeFadeMinimumInteger", dr->SizeFadeMinimumInteger);
            DrawU32("AnimationTimeSecsInt", dr->AnimationTimeSecsInteger);

            ImGui::Checkbox("UseStartColour", &dr->UseStartColour); ImGui::SameLine();
            ImGui::Checkbox("UseMidColour", &dr->UseMidColour); ImGui::SameLine();
            ImGui::Checkbox("UseEndColour", &dr->UseEndColour);
            ImGui::Checkbox("AlphaFadeEnable", &dr->AlphaFadeEnable); ImGui::SameLine();
            ImGui::Checkbox("SizeFadeEnable", &dr->SizeFadeEnable); ImGui::SameLine();
            ImGui::Checkbox("ForceAnimationTime", &dr->ForceAnimationTime);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Flicker");
            ImGui::Separator();
            ImGui::Checkbox("FlickerEnable", &dr->FlickerEnable);
            DrawU32("FlickerMinAlphaInteger", dr->FlickerMinAlphaInteger);
            DrawU32("FlickerMinSizeInteger", dr->FlickerMinSizeInteger);
            DrawU32("FlickerBiasInteger", dr->FlickerBiasInteger);
            DrawU32("FlickerSpeedInteger", dr->FlickerSpeedInteger);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Pooling");
            ImGui::Separator();
            ImGui::Checkbox("PoolingEnable", &dr->PoolingEnable);
            ImGui::Checkbox("PoolIncreaseAlphaEnable", &dr->PoolIncreaseAlphaEnable);
            ImGui::Checkbox("PoolIncreaseSizeEnable", &dr->PoolIncreaseSizeEnable);
            ImGui::Checkbox("PoolIncreaseLifeEnable", &dr->PoolIncreaseLifeEnable);
            ImGui::Checkbox("PoolIncreaseFrameEnable", &dr->PoolIncreaseFrameEnable);
            ImGui::DragFloat("MaxPoolSize", &dr->MaxPoolSize, 0.1f);
            ImGui::DragFloat("MaxPoolAlpha", &dr->MaxPoolAlpha, 0.01f);
            ImGui::DragFloat("MaxPoolLife", &dr->MaxPoolLife, 0.1f);
            ImGui::DragFloat("PoolFrameIncreaseRate", &dr->PoolFrameIncreaseRate, 0.1f);
            ImGui::DragFloat("StencilCubeWidth", &dr->StencilCubeWidth, 0.1f);
        }

        ImGui::Spacing();
        ImGui::Unindent(8.0f);
        ImGui::TreePop();
    }
    ImGui::PopID();
}

inline void DrawParticleProperties(CParticleEmitter& emitter) {
    static float rightPanelWidth = 460.0f; // 15% narrower than 540.0f
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float splitterWidth = 4.0f;
    float leftWidth = avail.x;
    if (g_ShowParticleRenderer) {
        leftWidth = avail.x - rightPanelWidth - splitterWidth;
        if (leftWidth < 280.0f) leftWidth = 280.0f;
    }

    // ==========================================
    // LEFT PANEL: PROPERTIES & HIERARCHY
    // ==========================================
    ImGui::BeginChild("ParticleLeftPanel", ImVec2(leftWidth, avail.y), false);
    {
        ImGui::TextColored(kGoldAccent, "Emitter Overview");
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 24.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Button(g_ShowParticleRenderer ? ">##HeaderToggle" : "<##HeaderToggle", ImVec2(24, 20))) {
            g_ShowParticleRenderer = !g_ShowParticleRenderer;
        }
        ImGui::PopStyleVar(2);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(g_ShowParticleRenderer ? "Collapse Viewport" : "Expand Viewport");

        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Emitter Name:");
        ImGui::SameLine(110.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        DrawString("##EmitterNameEdit", emitter.Name);

        ImGui::Dummy(ImVec2(0, 3));

        // Base Properties - starts COLLAPSED
        if (ImGui::CollapsingHeader("Emitter Base Properties", ImGuiTreeNodeFlags_None)) {
            ImGui::InputInt("Priority", &emitter.Priority);
            ImGui::Checkbox("ContinuousEmitter", &emitter.ContinuousEmitter);
            ImGui::DragFloat("MaxSpawnDistance", &emitter.MaxSpawnDistance, 1.0f);
            ImGui::DragFloat("MaxDrawDistance", &emitter.MaxDrawDistance, 1.0f);
            ImGui::DragFloat("FadeOutStart", &emitter.FadeOutStart, 1.0f);
            ImGui::DragFloat("FadeInEnd", &emitter.FadeInEnd, 1.0f);
            ImGui::DragFloat("FadeInStart", &emitter.FadeInStart, 1.0f);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Engine Flags");
            ImGui::Separator();
            ImGui::Checkbox("Emitter2D", &emitter.Emitter2D); ImGui::SameLine();
            ImGui::Checkbox("PreWaterEffect", &emitter.PreWaterEffect); ImGui::SameLine();
            ImGui::Checkbox("WaterEffect", &emitter.WaterEffect);

            ImGui::Checkbox("ZBufferWriteable", &emitter.ZBufferWriteable); ImGui::SameLine();
            ImGui::Checkbox("ReadZBuffer", &emitter.ReadZBuffer); ImGui::SameLine();
            ImGui::Checkbox("IsScreenDisplacement", &emitter.IsScreenDisplacement);

            ImGui::Checkbox("DieIfOffscreen", &emitter.DieIfOffscreen); ImGui::SameLine();
            ImGui::Checkbox("OffscreenUpdate", &emitter.OffscreenUpdate); ImGui::SameLine();
            ImGui::Checkbox("ClipEffectToWeatherMask", &emitter.ClipEffectToWeatherMask);

            ImGui::Checkbox("EnableDithering", &emitter.EnableDithering); ImGui::SameLine();
            ImGui::Checkbox("CalcBoundingSphereOnceOnly", &emitter.CalcBoundingSphereOnceOnly);
        }

        ImGui::Dummy(ImVec2(0, 8));

        // Particle Systems Section Header with [+] button
        ImGui::TextColored(kGoldAccent, "Particle Systems (%d)", (int)emitter.Systems.size());
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 26);

        if (DrawAddIconButton("##AddSysTop", 18.0f, "Add New Particle System")) {
            CParticleSystem newSys;
            newSys.Name = "System_" + std::to_string(emitter.Systems.size() + 1);
            emitter.Systems.push_back(newSys);
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Systems list
        for (size_t i = 0; i < emitter.Systems.size(); ) {
            bool deleteSys = false;
            auto& sys = emitter.Systems[i];

            ImGui::PushID((int)i);
            ImGui::SetNextItemAllowOverlap();

            // System tabs appear COLLAPSED by default (no DefaultOpen flag)
            // Removed component count from tab label as requested
            std::string sysLabel = sys.Name;
            bool isSysOpen = ImGui::TreeNodeEx((void*)&sys, ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowOverlap, "%s", sysLabel.c_str());

            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 26);
            if (DrawDeleteIconButton(("##DelSys_" + std::to_string(i)).c_str(), 16.0f, "Delete Particle System")) {
                deleteSys = true;
            }

            if (isSysOpen) {
                ImGui::Indent(12.0f);
                ImGui::Spacing();

                ImGui::TextColored(kGoldAccent, "System Configuration");
                ImGui::Separator();

                DrawString("System Name", sys.Name);
                ImGui::Checkbox("Enabled", &sys.Enabled);
                ImGui::SameLine();
                ImGui::Checkbox("Scale Particles", &sys.ScaleParticles);
                DrawVector3("Scale", sys.Scale);

                ImGui::Dummy(ImVec2(0, 6));

                // Components Section Header with [+] button on the same line
                ImGui::TextColored(kGoldAccent, "Components (%d)", (int)sys.Components.size());
                ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 26);
                if (DrawAddIconButton(("##AddCompBtnTop_" + std::to_string(i)).c_str(), 18.0f, "Add Component to System")) {
                    g_TargetSystemIndexForAdd = (int)i;
                    g_SelectedNewCompType = 0;
                    g_ShowAddComponentModal = true;
                }

                ImGui::Separator();
                ImGui::Spacing();

                // Components hierarchy
                for (size_t c = 0; c < sys.Components.size(); ) {
                    bool deleteComp = false;
                    DrawParticleComponent(sys.Components[c], deleteComp, c);
                    if (deleteComp) {
                        sys.Components.erase(sys.Components.begin() + c);
                    }
                    else {
                        c++;
                    }
                }

                ImGui::Spacing();
                ImGui::Unindent(12.0f);
                ImGui::TreePop();
            }

            ImGui::PopID();

            if (deleteSys) {
                emitter.Systems.erase(emitter.Systems.begin() + i);
            }
            else {
                i++;
            }
        }
    }
    ImGui::EndChild();

    // ==========================================
    // RIGHT PANEL (VIEWPORT & PLAYER) - TOGGLEABLE
    // ==========================================
    if (g_ShowParticleRenderer) {
        ImGui::SameLine(0, 0);

        // VERTICAL SPLITTER
        ImVec2 splitterMin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("ParticleVSplitter", ImVec2(splitterWidth, avail.y));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsItemActive()) {
            rightPanelWidth -= ImGui::GetIO().MouseDelta.x;
            if (rightPanelWidth < 260.0f) rightPanelWidth = 260.0f;
            if (rightPanelWidth > avail.x - 280.0f) rightPanelWidth = avail.x - 280.0f;
        }
        {
            float lineX = splitterMin.x + splitterWidth * 0.5f;
            ImGui::GetWindowDrawList()->AddLine(ImVec2(lineX, splitterMin.y), ImVec2(lineX, splitterMin.y + avail.y), IM_COL32(90, 90, 90, 255), 1.0f);
        }

        ImGui::SameLine(0, 0);

        // RIGHT PANEL: PLAYER TRANSPORT & 3D VIEWPORT
        ImGui::BeginChild("ParticleRightPanel", ImVec2(rightPanelWidth, avail.y), false);
        {
            // Simulation lifecycle and time advancement
            if (&emitter != g_LastSimulatedEmitter || g_ParticleSimNeedsReset || emitter.Systems.size() != g_ParticleSimulator.SystemStates.size()) {
                g_ParticleSimTime = 0.0f;
                g_ParticleSimulator.Reset(emitter);
                g_LastSimulatedEmitter = &emitter;
                g_ParticleSimNeedsReset = false;
            }

            if (g_ParticlePlaying) {
                float dt = ImGui::GetIO().DeltaTime * g_ParticlePlaybackSpeed;
                g_ParticleSimTime += dt;
                if (g_ParticleSimDuration > 0.0f && g_ParticleSimTime >= g_ParticleSimDuration) {
                    if (g_ParticleLoop) {
                        g_ParticleSimTime = 0.0f;
                        g_ParticleSimulator.Reset(emitter);
                    }
                    else {
                        g_ParticleSimTime = g_ParticleSimDuration;
                        g_ParticlePlaying = false;
                    }
                }
                else {
                    g_ParticleSimulator.Update(emitter, dt);
                }
            }

            // --- TRANSPORT BAR ---
            float btnSize = 24.0f;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.82f, 0.45f, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.82f, 0.45f, 0.5f));

            // Play/Pause button
            ImTextureID playPauseTex = g_ParticlePlaying ? g_PauseTexture : g_PlayTexture;
            if (playPauseTex) {
                if (ImGui::ImageButton("##ParticlePlayPause", playPauseTex, ImVec2(btnSize, btnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), kGoldAccent)) {
                    g_ParticlePlaying = !g_ParticlePlaying;
                }
            }
            else {
                if (ImGui::Button(g_ParticlePlaying ? "Pause" : "Play", ImVec2(55, 0))) {
                    g_ParticlePlaying = !g_ParticlePlaying;
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(g_ParticlePlaying ? "Pause" : "Play");

            ImGui::SameLine();
            // Stop / Reset button
            if (g_StopTexture) {
                if (ImGui::ImageButton("##ParticleStop", g_StopTexture, ImVec2(btnSize, btnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), kGoldAccent)) {
                    g_ParticlePlaying = false;
                    g_ParticleSimTime = 0.0f;
                    g_ParticleSimulator.Reset(emitter);
                }
            }
            else {
                if (ImGui::Button("Reset", ImVec2(50, 0))) {
                    g_ParticlePlaying = false;
                    g_ParticleSimTime = 0.0f;
                    g_ParticleSimulator.Reset(emitter);
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset Simulation");

            ImGui::SameLine();
            // Loop toggle button
            ImVec4 loopTint = g_ParticleLoop ? kGoldAccent : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            if (g_LoopTexture) {
                if (ImGui::ImageButton("##ParticleLoop", g_LoopTexture, ImVec2(btnSize, btnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), loopTint)) {
                    g_ParticleLoop = !g_ParticleLoop;
                }
            }
            else {
                if (ImGui::Button(g_ParticleLoop ? "Loop" : "Once", ImVec2(50, 0))) {
                    g_ParticleLoop = !g_ParticleLoop;
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(g_ParticleLoop ? "Looping Enabled" : "Single Run");

            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%.2fs /", g_ParticleSimTime);

            ImGui::SameLine();
            ImGui::SetNextItemWidth(52.0f);
            ImGui::DragFloat("##SimDuration", &g_ParticleSimDuration, 0.5f, 0.5f, 300.0f, "%.1fs");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Duration (Drag to adjust)");

            ImGui::SameLine();
            ImGui::SetNextItemWidth(65.0f);
            static const char* speedLabels[] = { "0.25x", "0.5x", "1.0x", "2.0x" };
            static const float speedValues[] = { 0.25f, 0.5f, 1.0f, 2.0f };
            static int currentSpeedIdx = 2;
            if (ImGui::Combo("##SimSpeed", &currentSpeedIdx, speedLabels, IM_ARRAYSIZE(speedLabels))) {
                g_ParticlePlaybackSpeed = speedValues[currentSpeedIdx];
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Simulation Speed");

            // Scrubber Timeline Bar
            ImVec2 timelineSize = ImVec2(ImGui::GetContentRegionAvail().x, 20.0f);
            ImVec2 tMin = ImGui::GetCursorScreenPos();
            ImVec2 tMax = ImVec2(tMin.x + timelineSize.x, tMin.y + timelineSize.y);

            ImGui::InvisibleButton("##ParticleTimeline", timelineSize);
            bool timelineActive = ImGui::IsItemActive();
            bool timelineHovered = ImGui::IsItemHovered();
            if (timelineActive && ImGui::IsMouseDown(0) && timelineSize.x > 0.0f) {
                float t = (ImGui::GetMousePos().x - tMin.x) / timelineSize.x;
                t = std::clamp(t, 0.0f, 1.0f);
                g_ParticleSimTime = t * g_ParticleSimDuration;
                g_ParticlePlaying = false;
                g_ParticleSimulator.Seek(emitter, g_ParticleSimTime);
            }

            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(tMin, tMax, IM_COL32(35, 35, 38, 255), 3.0f);
            dl->AddRect(tMin, tMax, IM_COL32(80, 80, 85, 255), 3.0f);

            float progress = (g_ParticleSimDuration > 0.0f) ? std::clamp(g_ParticleSimTime / g_ParticleSimDuration, 0.0f, 1.0f) : 0.0f;
            dl->AddRectFilled(tMin, ImVec2(tMin.x + timelineSize.x * progress, tMax.y), IM_COL32(90, 140, 200, 100), 3.0f);

            float headX = tMin.x + timelineSize.x * progress;
            dl->AddLine(ImVec2(headX, tMin.y), ImVec2(headX, tMax.y), IM_COL32(255, 255, 255, 255), 2.0f);

            if (timelineHovered && timelineSize.x > 0.0f) {
                float hoverT = std::clamp((ImGui::GetMousePos().x - tMin.x) / timelineSize.x, 0.0f, 1.0f);
                ImGui::SetTooltip("%.2fs", hoverT * g_ParticleSimDuration);
            }

            ImGui::Dummy(ImVec2(0, 4));

            // --- 3D VIEWPORT CHILD ---
            ImVec2 viewportAvail = ImGui::GetContentRegionAvail();
            if (viewportAvail.y < 80.0f) viewportAvail.y = 80.0f;

            ImGui::BeginChild("ParticleViewport", viewportAvail, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            bool viewportHovered = ImGui::IsWindowHovered();

            if (g_pd3dDevice) {
                g_ParticleRenderer.Initialize(g_pd3dDevice);
                g_ParticleRenderer.Resize(g_pd3dDevice, viewportAvail.x, viewportAvail.y);

                ID3D11DeviceContext* pCtx = nullptr;
                g_pd3dDevice->GetImmediateContext(&pCtx);
                ID3D11ShaderResourceView* tex = g_ParticleRenderer.Render(pCtx, viewportAvail.x, viewportAvail.y, viewportHovered, &emitter, &g_ParticleSimulator);
                pCtx->Release();

                if (tex) {
                    ImGui::Image((void*)tex, viewportAvail);
                }
            }

            // Viewport HUD Overlays
            ImVec2 pMin = ImGui::GetItemRectMin();
            ImVec2 pMax = ImGui::GetItemRectMax();

            // Controls in top-left
            ImGui::SetCursorScreenPos(ImVec2(pMin.x + 8, pMin.y + 6));
            if (ImGui::SmallButton(g_ParticleRenderer.ShowGrid ? "Grid" : "Grid")) {
                g_ParticleRenderer.ShowGrid = !g_ParticleRenderer.ShowGrid;
            }

            // Watermark text centered at bottom of viewport
            const char* wipText = "Work in progress - might not represent the VFX 100% accurately.";
            ImVec2 textSize = ImGui::CalcTextSize(wipText);
            ImGui::SetCursorScreenPos(ImVec2(pMin.x + (pMax.x - pMin.x - textSize.x) * 0.5f, pMax.y - textSize.y - 8.0f));
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 0.65f), "%s", wipText);

            ImGui::EndChild();
        }
        ImGui::EndChild();
    }

    // ==========================================
    // MODALS: ADD COMPONENT TO SYSTEM
    // ==========================================
    if (g_ShowAddComponentModal) {
        ImGui::OpenPopup("Add Component to System");
    }

    if (ImGui::BeginPopupModal("Add Component to System", &g_ShowAddComponentModal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(kGoldAccent, "Select Component Type:");
        ImGui::Spacing();

        static const char* s_CompTypes[] = {
            "CPSCRenderSprite",
            "CPSCEmitterGeneric",
            "CPSCUpdateNormal",
            "CPSCRenderMesh",
            "CPSCLight",
            "CPSCSpline",
            "CPSCSingleSprite",
            "CPSCOrbit",
            "CPSCAttractor",
            "CPSCDecalRenderer"
        };
        static const char* s_CompDescriptions[] = {
            "Sprite billboard renderer (quads, textures, blend modes)",
            "Generic emitter (spawn rates, shapes, velocities)",
            "Standard physics updater (gravity, drag, rotation, bounce)",
            "3D mesh particle renderer (Type 4 graphics)",
            "Dynamic point light emitter",
            "Spline trajectory controller",
            "Single persistent billboard sprite",
            "Orbital trajectory modifier",
            "Positional gravitational/force attractor",
            "Ground/wall decal renderer"
        };

        ImGui::SetNextItemWidth(340.0f);
        ImGui::Combo("Components", &g_SelectedNewCompType, s_CompTypes, IM_ARRAYSIZE(s_CompTypes));

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushTextWrapPos(340.0f);
        ImGui::TextDisabled("%s", s_CompDescriptions[g_SelectedNewCompType]);
        ImGui::PopTextWrapPos();

        ImGui::Dummy(ImVec2(0, 6));
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Add", ImVec2(100, 0))) {
            if (g_TargetSystemIndexForAdd >= 0 && g_TargetSystemIndexForAdd < (int)emitter.Systems.size()) {
                auto& targetSys = emitter.Systems[g_TargetSystemIndexForAdd];
                auto newComp = CreateComponent(s_CompTypes[g_SelectedNewCompType]);
                if (newComp) {
                    newComp->ClassName = s_CompTypes[g_SelectedNewCompType];
                    std::string hashSource = targetSys.Name + "_" + newComp->ClassName + "_" + std::to_string(ImGui::GetTime());
                    newComp->InstanceID = BinaryParser::CalculateCRC32_Fable(hashSource);
                    targetSys.Components.push_back(newComp);
                }
            }
            g_ShowAddComponentModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            g_ShowAddComponentModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // ==========================================
    // MODALS: TEXTURE PICKER & MESH PICKER
    // ==========================================
    if (g_TriggerParticleTexPopup) {
        ImGui::OpenPopup("Select Particle Texture");
        g_ShowParticleTexPopup = true;
        g_TriggerParticleTexPopup = false;
    }

    if (ImGui::BeginPopupModal("Select Particle Texture", &g_ShowParticleTexPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputTextWithHint("##SearchTex", "Search Textures by ID or Name...", g_ParticleTexSearchBuf, 128);
        ImGui::Separator();

        ImGui::BeginChild("TexList", ImVec2(350, 300), true);
        LoadedBank* texBank = nullptr;
        bool isXbox = false;
        if (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < (int)g_OpenBanks.size()) {
            const auto& activeBank = g_OpenBanks[g_ActiveBankIndex];
            if (activeBank.Type == EBankType::Effects) {
                if (activeBank.ActiveSubBankIndex >= 0 && activeBank.ActiveSubBankIndex < (int)activeBank.SubBanks.size()) {
                    const std::string& sbName = activeBank.SubBanks[activeBank.ActiveSubBankIndex].Name;
                    if (sbName == "PARTICLE_MAIN" || (sbName.find("PARTICLE_MAIN") != std::string::npos && sbName.find("_PC") == std::string::npos)) {
                        isXbox = true;
                    }
                }
            }
        }
        if (!isXbox) {
            bool hasXboxGraphics = false;
            bool hasPcTextures = false;
            for (const auto& b : g_OpenBanks) {
                if (b.Type == EBankType::XboxGraphics) hasXboxGraphics = true;
                if (b.Type == EBankType::Textures) hasPcTextures = true;
            }
            if (hasXboxGraphics && !hasPcTextures) isXbox = true;
        }

        std::string filter = g_ParticleTexSearchBuf;
        std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

        if (isXbox) {
            bool hasAny = false;
            for (int bIdx = 0; bIdx < (int)g_OpenBanks.size(); ++bIdx) {
                if (g_OpenBanks[bIdx].Type == EBankType::XboxGraphics) {
                    EnsureXboxTextureCache(bIdx);
                    for (const auto& pair : g_XboxTexCache[bIdx]) {
                        hasAny = true;
                        uint32_t tID = pair.first;
                        const auto& meta = pair.second;
                        std::string nameLower = meta.Name;
                        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                        std::string idStr = std::to_string(tID);

                        if (filter.empty() || nameLower.find(filter) != std::string::npos || idStr.find(filter) != std::string::npos) {
                            bool isSelected = (g_ParticlePreviewTexID == (int32_t)tID);
                            if (ImGui::Selectable((idStr + " - " + meta.Name).c_str(), isSelected)) {
                                g_ParticlePreviewTexID = (int32_t)tID;
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                    }
                }
            }
            if (!hasAny) {
                ImGui::TextDisabled("No Xbox Graphics Bank open!");
            }
        }
        else {
            for (auto& b : g_OpenBanks) {
                if (b.Type == EBankType::Textures || b.Type == EBankType::Frontend) {
                    texBank = &b; break;
                }
            }

            if (texBank) {
                for (const auto& entry : texBank->Entries) {
                    std::string nameLower = entry.Name;
                    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                    std::string idStr = std::to_string(entry.ID);

                    if (filter.empty() || nameLower.find(filter) != std::string::npos || idStr.find(filter) != std::string::npos) {
                        bool isSelected = (g_ParticlePreviewTexID == (int32_t)entry.ID);
                        if (ImGui::Selectable((idStr + " - " + entry.Name).c_str(), isSelected)) {
                            g_ParticlePreviewTexID = entry.ID;
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                }
            }
            else {
                ImGui::TextDisabled("No Texture Bank open!");
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("TexPreview", ImVec2(256, 350), true);
        if (g_ParticlePreviewTexID > 0) {
            ID3D11ShaderResourceView* srv = LoadTextureForMesh(g_ParticlePreviewTexID);
            if (srv) ImGui::Image((void*)srv, ImVec2(256, 256));
            else ImGui::TextDisabled("Preview Not Available");

            auto info = g_ParticleRenderer.GetTextureInfo(g_ParticlePreviewTexID);
            if (info.Width > 0 && info.Height > 0) {
                ImGui::Spacing();
                ImGui::Text("Size: %dx%d", info.Width, info.Height);
                if (info.IsSpriteSheet) {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Sprite Sheet: %dx%d (%d frames)", 
                                       info.FrameWidth, info.FrameHeight, info.FrameCount);
                }
                else if (info.FrameCount > 1) {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Multi-Frame (%d frames)", info.FrameCount);
                }
                else {
                    ImGui::TextDisabled("Single Frame");
                }
                if (info.IsDXT1) {
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Format: DXT1 (Transparent Black)");
                }
            }
        }
        else {
            ImGui::TextDisabled("No Texture Selected");
        }
        ImGui::EndChild();

        ImGui::Separator();
        if (ImGui::Button("Choose", ImVec2(120, 0))) {
            if (g_ParticleTargetTexIDPtr && g_ParticlePreviewTexID >= 0) {
                *g_ParticleTargetTexIDPtr = g_ParticlePreviewTexID;
            }
            g_ShowParticleTexPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            g_ShowParticleTexPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Mesh Picker Modal
    if (g_ShowParticleMeshPicker) {
        ImGui::OpenPopup("Select Particle Mesh");
    }

    if (ImGui::BeginPopupModal("Select Particle Mesh", &g_ShowParticleMeshPicker, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputTextWithHint("##MeshSearch", "Search Type 4 Meshes...", g_ParticleMeshSearchBuf, 128);
        ImGui::Separator();

        if (g_ParticlePreviewMeshID > 0 && g_ParticlePreviewMeshID != g_LoadedParticlePreviewMeshID) {
            g_LoadedParticlePreviewMeshID = g_ParticlePreviewMeshID;
            bool found = false;
            for (auto& b : g_OpenBanks) {
                if (b.Type == EBankType::Graphics || (b.Type == EBankType::XboxGraphics && IsGraphicsSubBank(&b))) {
                    for (size_t j = 0; j < b.Entries.size(); j++) {
                        if (b.Entries[j].ID == (uint32_t)g_ParticlePreviewMeshID) {
                            std::vector<uint8_t> rawData;
                            if (b.ModifiedEntryData.count((int)j)) rawData = b.ModifiedEntryData[(int)j];
                            else if (b.Stream && b.Stream->is_open()) {
                                b.Stream->clear(); b.Stream->seekg(b.Entries[j].Offset, std::ios::beg);
                                rawData.resize(b.Entries[j].Size); b.Stream->read((char*)rawData.data(), b.Entries[j].Size);
                            }
                            g_ParticlePreviewMesh = C3DMeshContent();
                            if (b.SubheaderCache.count((int)j)) g_ParticlePreviewMesh.ParseEntryMetadata(b.SubheaderCache[(int)j]);
                            g_ParticlePreviewMesh.Parse(rawData, b.Entries[j].Type);
                            g_ParticleMeshUploadNeeded = true;
                            found = true;
                            break;
                        }
                    }
                }
                if (found) break;
            }
            if (!found) {
                g_ParticleMeshPreviewOpen = false;
            }
        }

        ImGui::BeginChild("MeshList", ImVec2(350, 400), true);
        std::string filterStr = g_ParticleMeshSearchBuf;
        std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

        for (size_t i = 0; i < g_OpenBanks.size(); i++) {
            if (g_OpenBanks[i].Type == EBankType::Graphics || (g_OpenBanks[i].Type == EBankType::XboxGraphics && IsGraphicsSubBank(&g_OpenBanks[i]))) {
                for (size_t j = 0; j < g_OpenBanks[i].Entries.size(); j++) {
                    if (IsSupportedMesh(g_OpenBanks[i].Entries[j].Type)) {
                        std::string mName = g_OpenBanks[i].Entries[j].Name;
                        std::transform(mName.begin(), mName.end(), mName.begin(), ::tolower);
                        std::string idStr = std::to_string(g_OpenBanks[i].Entries[j].ID);

                        if (filterStr.empty() || mName.find(filterStr) != std::string::npos || idStr.find(filterStr) != std::string::npos) {
                            bool isSelected = (g_ParticlePreviewMeshID == (int32_t)g_OpenBanks[i].Entries[j].ID);
                            if (ImGui::Selectable((idStr + " - " + g_OpenBanks[i].Entries[j].FriendlyName).c_str(), isSelected)) {
                                g_ParticlePreviewMeshID = g_OpenBanks[i].Entries[j].ID;
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                    }
                }
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("MeshPreviewView", ImVec2(400, 400), true);

        if (g_ParticleMeshUploadNeeded && g_pd3dDevice) {
            g_ParticleMeshRenderer.Initialize(g_pd3dDevice);
            g_ParticleMeshRenderer.UploadMesh(g_pd3dDevice, g_ParticlePreviewMesh);

            std::vector<MeshRenderer::RenderMaterial> materials;
            int maxMat = 0;
            for (const auto& m : g_ParticlePreviewMesh.Materials) if (m.ID > maxMat) maxMat = m.ID;
            materials.resize(maxMat + 1);

            for (const auto& m : g_ParticlePreviewMesh.Materials) {
                if (m.DiffuseMapID > 0) materials[m.ID].Diffuse = LoadTextureForMesh(m.DiffuseMapID);
                materials[m.ID].SelfIllumination = (float)m.SelfIllumination / 255.0f;
            }
            g_ParticleMeshRenderer.SetMaterials(materials);

            g_ParticleMeshUploadNeeded = false;
            g_ParticleMeshPreviewOpen = true;
        }

        if (g_ParticleMeshPreviewOpen && g_ParticlePreviewMesh.IsParsed) {
            ImVec2 mAvail = ImGui::GetContentRegionAvail();
            g_ParticleMeshRenderer.Resize(g_pd3dDevice, mAvail.x, mAvail.y);

            ID3D11DeviceContext* pCtx;
            g_pd3dDevice->GetImmediateContext(&pCtx);
            ID3D11ShaderResourceView* tex = g_ParticleMeshRenderer.Render(pCtx, mAvail.x, mAvail.y, false);
            pCtx->Release();

            if (tex) ImGui::Image((void*)tex, mAvail);
        }
        else {
            ImGui::TextDisabled("Select a particle mesh to preview.");
        }
        ImGui::EndChild();

        ImGui::Separator();
        if (ImGui::Button("Choose", ImVec2(120, 0))) {
            if (g_ParticleTargetMeshIDPtr && g_ParticlePreviewMeshID >= 0) {
                *g_ParticleTargetMeshIDPtr = g_ParticlePreviewMeshID;
            }
            g_ShowParticleMeshPicker = false;
            g_ParticleMeshPreviewOpen = false;
            g_LoadedParticlePreviewMeshID = -1;
            g_ParticlePreviewMesh = C3DMeshContent();
            g_ParticleMeshRenderer.Release();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            g_ShowParticleMeshPicker = false;
            g_ParticleMeshPreviewOpen = false;
            g_LoadedParticlePreviewMeshID = -1;
            g_ParticlePreviewMesh = C3DMeshContent();
            g_ParticleMeshRenderer.Release();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}