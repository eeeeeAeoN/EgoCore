#pragma once
#include "imgui.h"
#include "ParticleParser.h"
#include "BinaryParser.h"
#include "MeshRenderer.h"
#include "BankBackend.h"
#include <string>
#include <cstring>

extern ID3D11Device* g_pd3dDevice;
// Note: Relies on LoadTextureForMesh() from MeshProperties.h

// --- NEW STATE VARIABLES ---
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

// Custom Helper for dynamic Fable CRC32 Hashing
inline void DrawHashParam(const char* label, uint32_t& param) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::SameLine(240);

    ImGui::SetNextItemWidth(100);
    ImGui::InputScalar((std::string("##val_") + label).c_str(), ImGuiDataType_U32, &param);

    ImGui::SameLine();
    char buf[128] = "";
    ImGui::SetNextItemWidth(150);
    if (ImGui::InputTextWithHint((std::string("##txt_") + label).c_str(), "String to Hash...", buf, 128)) {
        if (strlen(buf) > 0) {
            param = BinaryParser::CalculateCRC32_Fable(buf);
        }
    }
}

inline void DrawParticleTexRow(const char* label, int32_t& bankIndex) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::SameLine(240);

    ImGui::SetNextItemWidth(80);
    ImGui::InputInt((std::string("##id_") + label).c_str(), &bankIndex, 0, 0);

    ImGui::SameLine();
    if (ImGui::Button((std::string("+##btn_") + label).c_str(), ImVec2(24, 0))) {
        g_ParticleTargetTexIDPtr = &bankIndex;
        g_ParticlePreviewTexID = bankIndex; // <-- Pre-loads current texture
        g_ParticleTexSearchBuf[0] = '\0';
        g_TriggerParticleTexPopup = true;
    }

    if (bankIndex > 0) {
        ImGui::SameLine();
        ID3D11ShaderResourceView* srv = LoadTextureForMesh(bankIndex);
        if (srv) {
            ImGui::Image((void*)srv, ImVec2(24, 24));
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Image((void*)srv, ImVec2(256, 256));
                ImGui::PushTextWrapPos(256.0f);
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Texture ID: %d", bankIndex);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
    }
}

// Custom Helper for Mesh Selection
inline void DrawParticleMeshRow(const char* label, int32_t& bankIndex) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::SameLine(240);

    ImGui::SetNextItemWidth(80);
    ImGui::InputInt((std::string("##id_") + label).c_str(), &bankIndex, 0, 0);

    ImGui::SameLine();
    if (ImGui::Button((std::string("+##btn_") + label).c_str(), ImVec2(24, 0))) {
        g_ParticleTargetMeshIDPtr = &bankIndex;
        g_ParticlePreviewMeshID = bankIndex; // <-- Sets target mesh
        g_LoadedParticlePreviewMeshID = -1;  // <-- Forces parsing logic to run
        g_ParticleMeshSearchBuf[0] = '\0';
        g_ShowParticleMeshPicker = true;
    }
}

inline void DrawSplineBase(CPSCSplineBase* spline) {
    if (ImGui::TreeNodeEx("Spline Base Data", ImGuiTreeNodeFlags_Framed)) {
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

inline void DrawParticleComponent(std::shared_ptr<CParticleComponent>& comp, bool& requestDelete) {
    ImGui::PushID(comp.get());

    bool isOpen = ImGui::TreeNodeEx(comp->ClassName.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowOverlap);

    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 65);
    if (ImGui::Button("Delete##comp")) {
        requestDelete = true;
    }

    if (isOpen) {
        ImGui::TextDisabled("(ID Hash: %u)", comp->InstanceID);
        ImGui::Checkbox("Enabled", &comp->Enabled);
        ImGui::SameLine();
        ImGui::Checkbox("Visible", &comp->Visible);
        ImGui::Separator();

        if (auto* rs = dynamic_cast<CPSCRenderSprite*>(comp.get())) {
            DrawParticleTexRow("SpriteBankIndex", rs->SpriteBankIndex);
            DrawParticleTexRow("TrailBankIndex", rs->TrailBankIndex);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Colours:");
            DrawColour("StartColour", rs->StartColour);
            DrawColour("MidColour", rs->MidColour);
            DrawColour("EndColour", rs->EndColour);

            ImGui::Checkbox("UseStartColour", &rs->UseStartColour); ImGui::SameLine();
            ImGui::Checkbox("UseMidColour", &rs->UseMidColour); ImGui::SameLine();
            ImGui::Checkbox("UseEndColour", &rs->UseEndColour);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Render Settings:");
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

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Fades & Flickering:");
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

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Lifecycle & Fading:");
            ImGui::Checkbox("UseSystemLifeSecs", &un->UseSystemLifeSecs); ImGui::SameLine();
            ImGui::Checkbox("UseParticleLifeSecs", &un->UseParticleLifeSecs);
            ImGui::Checkbox("SystemAlphaFadeEnable", &un->SystemAlphaFadeEnable); ImGui::SameLine();
            ImGui::Checkbox("EmissionFadeEnable", &un->EmissionFadeEnable);
            ImGui::DragFloat("SystemAlphaFadeMinimum", &un->SystemAlphaFadeMinimum, 0.01f);
            ImGui::DragFloat("EmissionFadeMinimum", &un->EmissionFadeMinimum, 0.01f);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Physics & Interaction:");
            ImGui::Checkbox("ParticleCollideWithGround", &un->ParticleCollideWithGround);
            ImGui::Checkbox("ParticleCollideWithAnything", &un->ParticleCollideWithAnything);
            ImGui::Checkbox("ParticleDieOnCollision", &un->ParticleDieOnCollision);
            ImGui::Checkbox("UseAllAttractors", &un->UseAllAttractors);
            ImGui::Checkbox("StayWithEmitter", &un->StayWithEmitter);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Rotation & Orientation:");
            ImGui::Checkbox("UseRandomRotationAxis", &un->UseRandomRotationAxis);
            ImGui::Checkbox("UseRandomInitialRotation", &un->UseRandomInitialRotation);
            ImGui::Checkbox("SetOrientationFromDirection", &un->SetOrientationFromDirection);
            ImGui::Checkbox("SetOrientationFromGame", &un->SetOrientationFromGame);
            ImGui::DragFloat("InitialRotationX", &un->InitialRotationX, 0.1f);
            ImGui::DragFloat("InitialRotationY", &un->InitialRotationY, 0.1f);
            ImGui::DragFloat("InitialRotationZ", &un->InitialRotationZ, 0.1f);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Randomise Pos:");
            ImGui::Checkbox("RandomisePosEnable", &un->RandomisePosEnable);
            ImGui::Checkbox("RandomisePosDistVaryEnable", &un->RandomisePosDistVaryEnable);
            ImGui::DragFloat("RandomisePosDistVaryScale", &un->RandomisePosDistVaryScale, 0.1f);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Decals & Params:");
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

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Physics & Sizing:");
            ImGui::DragFloat("Particles Per Sec", &eg->ParticlesPerSecond, 0.5f);
            ImGui::DragFloat("Min Speed", &eg->MinSpeed, 0.1f);
            ImGui::DragFloat("Max Speed", &eg->MaxSpeed, 0.1f);
            ImGui::DragFloat("Emitter Size", &eg->EmitterSize, 0.1f);
            ImGui::DragFloat("Radial Bias", &eg->RadialBias, 0.1f);
            DrawVector3("Non-Uniform Scaling", eg->NonUniformScaling);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Lifecycles:");
            ImGui::DragFloat("Emitter Life Secs", &eg->EmitterLifeSecs, 0.1f);
            ImGui::DragFloat("Emitter Start Time", &eg->EmitterStartTime, 0.1f);
            ImGui::DragFloat("Emitter Timeline Secs", &eg->EmitterTimelineSecs, 0.1f);
            ImGui::Checkbox("UseEmitterLifeSecs", &eg->UseEmitterLifeSecs);
            ImGui::Checkbox("UseEmitterTimelineSecs", &eg->UseEmitterTimelineSecs);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Direction & Orientation:");
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

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Mesh Colours:");
            DrawColour("StartColour", rm->StartColour);
            DrawColour("MidColour", rm->MidColour);
            DrawColour("EndColour", rm->EndColour);
            ImGui::Checkbox("UseStartColour", &rm->UseStartColour); ImGui::SameLine();
            ImGui::Checkbox("UseMidColour", &rm->UseMidColour); ImGui::SameLine();
            ImGui::Checkbox("UseEndColour", &rm->UseEndColour);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Trail Colours:");
            DrawColour("TrailStartCol", rm->TrailStartColour);
            DrawColour("TrailMidCol", rm->TrailMidColour);
            DrawColour("TrailEndCol", rm->TrailEndColour);
            ImGui::Checkbox("TrailUseStartColour", &rm->TrailUseStartColour); ImGui::SameLine();
            ImGui::Checkbox("TrailUseMidColour", &rm->TrailUseMidColour); ImGui::SameLine();
            ImGui::Checkbox("TrailUseEndColour", &rm->TrailUseEndColour);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Render Settings:");
            DrawU32("BlendMode", rm->BlendMode);
            DrawU32("TrailBlendMode", rm->TrailBlendMode);
            DrawU32("BlendOp", rm->BlendOp);
            DrawU32("TrailBlendOp", rm->TrailBlendOp);
            DrawVector3("StartRenderSize", rm->StartRenderSize);
            DrawVector3("EndRenderSize", rm->EndRenderSize);

            DrawHashParam("RenderSizeParam", rm->RenderSizeParam);
            ImGui::Checkbox("UseRenderSizeParam", &rm->UseRenderSizeParam);
            ImGui::Checkbox("CentredOnPos", &rm->CentredOnPos);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Fading & Flickering:");
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

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Trail Specs:");
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

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Flags:");
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

            ImGui::Dummy(ImVec2(0, 5));
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

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Colours & Fades:");
            ImGui::Checkbox("UseStartColour", &ss->UseStartColour); ImGui::SameLine();
            ImGui::Checkbox("UseMidColour", &ss->UseMidColour); ImGui::SameLine();
            ImGui::Checkbox("UseEndColour", &ss->UseEndColour);
            ImGui::Checkbox("AlphaFadeEnable", &ss->AlphaFadeEnable); ImGui::SameLine();
            ImGui::Checkbox("SizeFadeEnable", &ss->SizeFadeEnable); ImGui::SameLine();
            ImGui::Checkbox("ForceAnimationTime", &ss->ForceAnimationTime);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Render Flags:");
            ImGui::Checkbox("SelfIlluminating", &ss->SelfIlluminating);
            ImGui::Checkbox("RotateAroundCentre", &ss->RotateAroundCentre);
            ImGui::Checkbox("FaceMe2D", &ss->FaceMe2D);
            ImGui::Checkbox("FaceMe3D", &ss->FaceMe3D);
            ImGui::Checkbox("CrossedSprites", &ss->CrossedSprites);
            ImGui::Checkbox("StayWithEmitter", &ss->StayWithEmitter);
            ImGui::Checkbox("UsePosition", &ss->UsePosition);
            ImGui::Checkbox("UseSplinePoints", &ss->UseSplinePoints);

            ImGui::Dummy(ImVec2(0, 5));
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
                if (ImGui::TreeNodeEx((void*)(intptr_t)i, ImGuiTreeNodeFlags_Framed, "Orbit %d", (int)i)) {
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

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Render Settings:");
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

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Flicker:");
            ImGui::Checkbox("FlickerEnable", &dr->FlickerEnable);
            DrawU32("FlickerMinAlphaInteger", dr->FlickerMinAlphaInteger);
            DrawU32("FlickerMinSizeInteger", dr->FlickerMinSizeInteger);
            DrawU32("FlickerBiasInteger", dr->FlickerBiasInteger);
            DrawU32("FlickerSpeedInteger", dr->FlickerSpeedInteger);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Pooling:");
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
        else {
            ImGui::TextDisabled("Diagnostic view not implemented for this component type yet.");
        }

        ImGui::TreePop();
    }
    ImGui::PopID();
}

inline void DrawParticleSystem(CParticleSystem& sys, bool& requestDelete) {
    ImGui::PushID(&sys);

    bool isOpen = ImGui::TreeNodeEx(sys.Name.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);

    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 65);
    if (ImGui::Button("Delete##sys")) {
        requestDelete = true;
    }

    if (isOpen) {
        DrawString("System Name", sys.Name);
        ImGui::Checkbox("Enabled", &sys.Enabled);
        ImGui::SameLine();
        ImGui::Checkbox("ScaleParticles", &sys.ScaleParticles);
        DrawVector3("Scale", sys.Scale);

        ImGui::Dummy(ImVec2(0, 5));
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Add New Component:");
        static int newCompType = 0;
        const char* compTypes[] = { "CPSCRenderSprite", "CPSCEmitterGeneric", "CPSCUpdateNormal", "CPSCRenderMesh", "CPSCLight", "CPSCSpline", "CPSCSingleSprite", "CPSCOrbit", "CPSCAttractor", "CPSCDecalRenderer" };
        ImGui::Combo("##NewCompCombo", &newCompType, compTypes, IM_ARRAYSIZE(compTypes));
        ImGui::SameLine();
        if (ImGui::Button("Add Component")) {
            auto newComp = CreateComponent(compTypes[newCompType]);
            if (newComp) {
                newComp->ClassName = compTypes[newCompType];

                // GENERATE THE NATIVE CRC32 INSTANCE ID
                std::string hashSource = sys.Name + "_" + newComp->ClassName + "_" + std::to_string(ImGui::GetTime());
                newComp->InstanceID = BinaryParser::CalculateCRC32_Fable(hashSource);

                sys.Components.push_back(newComp);
            }
        }
        ImGui::Separator();

        ImGui::TextDisabled("Components (%d):", (int)sys.Components.size());

        for (size_t i = 0; i < sys.Components.size(); ) {
            bool deleteComp = false;
            DrawParticleComponent(sys.Components[i], deleteComp);
            if (deleteComp) {
                sys.Components.erase(sys.Components.begin() + i);
            }
            else {
                i++;
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

inline void DrawParticleProperties(CParticleEmitter& emitter) {
    ImGui::Text("Emitter Name: ");
    ImGui::SameLine();
    DrawString("##EmitterNameEdit", emitter.Name);
    ImGui::TextDisabled("Magic: 0x%X", emitter.Magic);
    ImGui::Separator();

    if (ImGui::TreeNodeEx("Base Properties", ImGuiTreeNodeFlags_Framed)) {
        ImGui::InputInt("Priority", &emitter.Priority);
        ImGui::Checkbox("ContinuousEmitter", &emitter.ContinuousEmitter);
        ImGui::DragFloat("MaxSpawnDistance", &emitter.MaxSpawnDistance, 1.0f);
        ImGui::DragFloat("MaxDrawDistance", &emitter.MaxDrawDistance, 1.0f);
        ImGui::DragFloat("FadeOutStart", &emitter.FadeOutStart, 1.0f);
        ImGui::DragFloat("FadeInEnd", &emitter.FadeInEnd, 1.0f);
        ImGui::DragFloat("FadeInStart", &emitter.FadeInStart, 1.0f);

        ImGui::Dummy(ImVec2(0, 5));
        ImGui::TextDisabled("Engine Flags:");
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

        ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::TextDisabled("Particle Systems (%d):", (int)emitter.Systems.size());
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100);

    if (ImGui::Button("+ Add System")) {
        CParticleSystem newSys;
        newSys.Name = "System_" + std::to_string(emitter.Systems.size() + 1);
        emitter.Systems.push_back(newSys);
    }

    ImGui::Dummy(ImVec2(0, 5));

    for (size_t i = 0; i < emitter.Systems.size(); ) {
        bool deleteSys = false;
        DrawParticleSystem(emitter.Systems[i], deleteSys);
        if (deleteSys) {
            emitter.Systems.erase(emitter.Systems.begin() + i);
        }
        else {
            i++;
        }
    }

    if (g_TriggerParticleTexPopup) {
        ImGui::OpenPopup("Select Particle Texture");
        g_ShowParticleTexPopup = true;
        g_TriggerParticleTexPopup = false;
        // Removed the g_ParticlePreviewTexID reset here so the currently assigned texture persists
    }

    if (ImGui::BeginPopupModal("Select Particle Texture", &g_ShowParticleTexPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputTextWithHint("##SearchTex", "Search Textures by ID or Name...", g_ParticleTexSearchBuf, 128);
        ImGui::Separator();

        ImGui::BeginChild("TexList", ImVec2(350, 300), true);
        LoadedBank* texBank = nullptr;
        for (auto& b : g_OpenBanks) {
            if (b.Type == EBankType::Textures || b.Type == EBankType::Frontend || b.Type == EBankType::Effects) {
                texBank = &b; break;
            }
        }

        if (texBank) {
            std::string filter = g_ParticleTexSearchBuf;
            std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

            for (const auto& entry : texBank->Entries) {
                std::string nameLower = entry.Name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                std::string idStr = std::to_string(entry.ID);

                if (filter.empty() || nameLower.find(filter) != std::string::npos || idStr.find(filter) != std::string::npos) {
                    bool isSelected = (g_ParticlePreviewTexID == entry.ID);
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
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("TexPreview", ImVec2(256, 300), true);
        if (g_ParticlePreviewTexID > 0) {
            ID3D11ShaderResourceView* srv = LoadTextureForMesh(g_ParticlePreviewTexID);
            if (srv) ImGui::Image((void*)srv, ImVec2(256, 256));
            else ImGui::TextDisabled("Preview Not Available");
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

        // --- NEW: Auto-Load Mesh Logic ---
        // If the ID changed (or was just opened), fetch and parse the mesh immediately
        if (g_ParticlePreviewMeshID > 0 && g_ParticlePreviewMeshID != g_LoadedParticlePreviewMeshID) {
            g_LoadedParticlePreviewMeshID = g_ParticlePreviewMeshID;
            bool found = false;
            for (auto& b : g_OpenBanks) {
                if (b.Type == EBankType::Graphics) {
                    for (int j = 0; j < b.Entries.size(); j++) {
                        if (b.Entries[j].ID == (uint32_t)g_ParticlePreviewMeshID) {
                            std::vector<uint8_t> rawData;
                            if (b.ModifiedEntryData.count(j)) rawData = b.ModifiedEntryData[j];
                            else {
                                b.Stream->clear(); b.Stream->seekg(b.Entries[j].Offset, std::ios::beg);
                                rawData.resize(b.Entries[j].Size); b.Stream->read((char*)rawData.data(), b.Entries[j].Size);
                            }
                            g_ParticlePreviewMesh = C3DMeshContent();
                            if (b.SubheaderCache.count(j)) g_ParticlePreviewMesh.ParseEntryMetadata(b.SubheaderCache[j]);
                            g_ParticlePreviewMesh.Parse(rawData, 4);
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
        // ----------------------------------

        ImGui::BeginChild("MeshList", ImVec2(350, 400), true);
        std::string filterStr = g_ParticleMeshSearchBuf;
        std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

        for (int i = 0; i < g_OpenBanks.size(); i++) {
            if (g_OpenBanks[i].Type == EBankType::Graphics) {
                for (int j = 0; j < g_OpenBanks[i].Entries.size(); j++) {
                    if (g_OpenBanks[i].Entries[j].Type == 4) {
                        std::string mName = g_OpenBanks[i].Entries[j].Name;
                        std::transform(mName.begin(), mName.end(), mName.begin(), ::tolower);
                        std::string idStr = std::to_string(g_OpenBanks[i].Entries[j].ID);

                        if (filterStr.empty() || mName.find(filterStr) != std::string::npos || idStr.find(filterStr) != std::string::npos) {
                            bool isSelected = (g_ParticlePreviewMeshID == g_OpenBanks[i].Entries[j].ID);
                            if (ImGui::Selectable((idStr + " - " + g_OpenBanks[i].Entries[j].FriendlyName).c_str(), isSelected)) {
                                g_ParticlePreviewMeshID = g_OpenBanks[i].Entries[j].ID; // Just update the ID, the block above handles parsing
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
            ImVec2 avail = ImGui::GetContentRegionAvail();
            g_ParticleMeshRenderer.Resize(g_pd3dDevice, avail.x, avail.y);

            ID3D11DeviceContext* pCtx;
            g_pd3dDevice->GetImmediateContext(&pCtx);
            ID3D11ShaderResourceView* tex = g_ParticleMeshRenderer.Render(pCtx, avail.x, avail.y, false);
            pCtx->Release();

            if (tex) ImGui::Image((void*)tex, avail);
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
            g_LoadedParticlePreviewMeshID = -1;       // Reset loader
            g_ParticlePreviewMesh = C3DMeshContent(); // Cleanup Memory
            g_ParticleMeshRenderer.Release();         // Cleanup VRAM
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            g_ShowParticleMeshPicker = false;
            g_ParticleMeshPreviewOpen = false;
            g_LoadedParticlePreviewMeshID = -1;       // Reset loader
            g_ParticlePreviewMesh = C3DMeshContent(); // Cleanup Memory
            g_ParticleMeshRenderer.Release();         // Cleanup VRAM
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}