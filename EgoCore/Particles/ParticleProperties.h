#pragma once
#include "imgui.h"
#include "ParticleParser.h"
#include <string>
#include <cstring>

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
            ImGui::InputInt("SpriteBankIndex", &rs->SpriteBankIndex);
            ImGui::InputInt("TrailBankIndex", &rs->TrailBankIndex);

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
            DrawU32("AccelerationParam", un->AccelerationParam);
            DrawU32("OrientFromGameParam", un->OrientFromGameParam);
        }
        else if (auto* eg = dynamic_cast<CPSCEmitterGeneric*>(comp.get())) {
            DrawU32("EmitterPosParam", eg->EmitterPosParam);
            DrawU32("DirectionParamName", eg->DirectionParamName);
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
            ImGui::InputInt("BankIndex", &rm->BankIndex);
            ImGui::InputInt("TrailBankIndex", &rm->TrailBankIndex);

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
            DrawU32("RenderSizeParam", rm->RenderSizeParam);
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
            DrawU32("LightPositionParam", lgt->LightPositionParam);
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
            ImGui::InputInt("SpriteBankIndex", &ss->SpriteBankIndex);
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
        }
        else if (auto* at = dynamic_cast<CPSCAttractor*>(comp.get())) {
            ImGui::Checkbox("AttractorEnabled", &at->AttractorEnabled);
            ImGui::Checkbox("AttractorUseParamPosition", &at->AttractorUseParamPosition);
            DrawU32("AttractorPositionParam", at->AttractorPositionParam);
            DrawU32("AttractorPositionParamName", at->AttractorPositionParamName);
            ImGui::InputInt("AttractorInfluenceFallOff", &at->AttractorInfluenceFallOff);
            ImGui::DragFloat("AttractorInfluenceRadius", &at->AttractorInfluenceRadius, 0.1f);
            ImGui::DragFloat("AttractorInfluenceForce", &at->AttractorInfluenceForce, 0.1f);

            ImGui::Text("User Points: %d", (int)at->AttractorUserPointsArray.size());
            for (size_t i = 0; i < at->AttractorUserPointsArray.size(); i++) {
                DrawVector3(("Point " + std::to_string(i)).c_str(), at->AttractorUserPointsArray[i]);
            }
        }
        else if (auto* ob = dynamic_cast<CPSCOrbit*>(comp.get())) {
            DrawU32("CentreParam", ob->CentreParam);
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
            ImGui::InputInt("DecalBankIndex", &dr->DecalBankIndex);
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
}