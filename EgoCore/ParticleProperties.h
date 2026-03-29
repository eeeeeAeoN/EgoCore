#pragma once
#include "imgui.h"
#include "ParticleParser.h"
#include <string>

// --- DIAGNOSTIC UI HELPERS ---

inline void DrawColour(const char* label, const CRGBColour& col) {
    // ImGui uses 0.0f - 1.0f for colors
    ImVec4 c(col.R / 255.0f, col.G / 255.0f, col.B / 255.0f, col.A / 255.0f);
    ImGui::ColorEdit4(label, (float*)&c, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
    ImGui::SameLine();
    ImGui::TextDisabled("R:%d G:%d B:%d A:%d", col.R, col.G, col.B, col.A);
}

inline void DrawVector3(const char* label, const C3DVector& vec) {
    ImGui::Text("%s:", label);
    ImGui::SameLine();
    ImGui::TextDisabled("(%.3f, %.3f, %.3f)", vec.X, vec.Y, vec.Z);
}

inline void DrawSplineBase(const CPSCSplineBase* spline) {
    if (ImGui::TreeNodeEx("Spline Base Data", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Text("Control Points: %d", (int)spline->ControlPoints.size());
        for (size_t i = 0; i < spline->ControlPoints.size(); i++) {
            DrawVector3(("  Pt " + std::to_string(i)).c_str(), spline->ControlPoints[i]);
        }

        ImGui::Text("Has Random Offset Data: %s", spline->HasRandomOffsetData ? "TRUE" : "FALSE");
        if (spline->HasRandomOffsetData) {
            ImGui::Text("  OffsetOnlyAtStartup: %d | ResetEachFrame: %d", spline->OffsetOnlyAtStartup, spline->ResetEachFrame);
        }

        ImGui::Text("Has Pos Param Data: %s", spline->HasPosParamData ? "TRUE" : "FALSE");
        ImGui::TreePop();
    }
}

// --- COMPONENT INSPECTOR ---

inline void DrawParticleComponent(const std::shared_ptr<CParticleComponent>& comp) {
    ImGui::PushID(comp.get());
    bool isOpen = ImGui::TreeNodeEx(comp->ClassName.c_str(), ImGuiTreeNodeFlags_Framed);
    ImGui::SameLine();
    ImGui::TextDisabled("(ID Hash: %u)", comp->InstanceID);

    if (isOpen) {
        ImGui::Text("Enabled: %s | Visible: %s", comp->Enabled ? "TRUE" : "FALSE", comp->Visible ? "TRUE" : "FALSE");
        ImGui::Separator();

        if (auto* rs = dynamic_cast<CPSCRenderSprite*>(comp.get())) {
            ImGui::Text("SpriteBankIndex: %d | TrailBankIndex: %d", rs->SpriteBankIndex, rs->TrailBankIndex);
            DrawColour("StartColour", rs->StartColour);
            DrawColour("MidColour", rs->MidColour);
            DrawColour("EndColour", rs->EndColour);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Render Settings:");
            ImGui::Text("Render Size: Start %.3f | End %.3f", rs->StartRenderSize, rs->EndRenderSize);
            ImGui::Text("Alpha Fade Min: %.3f | Size Fade Min: %.3f", rs->AlphaFadeMinimum, rs->SizeFadeMinimum);
            ImGui::Text("Flicker Speed: %.3f | Flicker Bias: %.3f", rs->FlickerSpeed, rs->FlickerBias);
            ImGui::Text("Anim Time Secs: %.3f", rs->AnimationTimeSecs);
            ImGui::Text("Trail Width: %.3f | Trail Length: %u", rs->TrailWidth, rs->TrailLengthInteger);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Flags & Modes:");
            ImGui::Text("BlendMode: %u | TrailBlendMode: %u", rs->BlendMode, rs->TrailBlendMode);
            ImGui::Text("UseColours: Start[%d] Mid[%d] End[%d]", rs->FlagUseStartCol, rs->FlagUseMidCol, rs->FlagUseEndCol);
            ImGui::Text("Fades: Alpha[%d] Size[%d] Flicker[%d] ForceAnim[%d]", rs->FlagAlphaFade, rs->FlagSizeFade, rs->FlagFlicker, rs->FlagForceAnimTime);
        }
        else if (auto* un = dynamic_cast<CPSCUpdateNormal*>(comp.get())) {
            ImGui::Text("DecalEmitterName: \"%s\"", un->DecalEmitterName.c_str());
            ImGui::Text("SystemLifeSecs: %.3f | ParticleLifeSecs: %.3f", un->SystemLifeSecs, un->ParticleLifeSecs);
            ImGui::Text("WindFactor: %.3f | GravityFactor: %.3f | AirResistance: %.3f", un->WindFactor, un->GravityFactor, un->AirResistance);
            ImGui::Text("ParticleBounce: %.3f | AccelScale: %.3f", un->ParticleBounce, un->ParticleAccelerationScale);
            DrawVector3("ParticleSystemOffset", un->ParticleSystemOffset);
            DrawVector3("ParticleAcceleration", un->ParticleAcceleration);
            DrawVector3("RotationAxis", un->RotationAxis);
            ImGui::Text("RotationSpeed: Min %.3f | Max %.3f", un->RotationMinAngleSpeed, un->RotationMaxAngleSpeed);
            DrawVector3("RandomisePosSpeed", un->RandomisePosSpeed);
            DrawVector3("RandomisePosScale", un->RandomisePosScale);
            ImGui::TextDisabled("(Plus 18 state flags)");
        }
        else if (auto* eg = dynamic_cast<CPSCEmitterGeneric*>(comp.get())) {
            ImGui::Text("EmitterPosParam: %u | DirectionParamName: %u", eg->EmitterPosParam, eg->DirectionParamName);
            ImGui::Text("Emitter Type: %u | Particles To Start (Base/Rand): %u / %u", eg->EmitterType, eg->NoParticlesToStart, eg->NoParticlesToStartRand);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Fixed-Point Decoded Values:");
            ImGui::Text("Particles Per Sec: %.3f", eg->ParticlesPerSecond);
            ImGui::Text("Speed: Min %.3f | Max %.3f", eg->MinSpeed, eg->MaxSpeed);
            ImGui::Text("Emitter Size: %.3f | Radial Bias: %.3f", eg->EmitterSize, eg->RadialBias);
            ImGui::Text("Life Secs: %.3f | Start Time: %.3f | Timeline Secs: %.3f", eg->EmitterLifeSecs, eg->EmitterStartTime, eg->EmitterTimelineSecs);
            ImGui::Text("Angular Perturbation Int: %u", eg->AngularPerturbationInteger);

            ImGui::Dummy(ImVec2(0, 5));
            DrawVector3("Custom Direction", eg->CustomDirection);
            DrawVector3("Non-Uniform Scaling", eg->NonUniformScaling);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Flags:");
            ImGui::Text("Solid: %d | OppositeDir: %d", eg->Solid, eg->OppositeDirection);
            ImGui::Text("Orientation: XY[%d] XZ[%d] YZ[%d]", eg->OrientationXY, eg->OrientationXZ, eg->OrientationYZ);
            ImGui::Text("Direction Rules: Custom[%d] Outward[%d] Forward[%d] Rand2D[%d] Rand3D[%d] Param[%d]",
                eg->UseCustomDirection, eg->UseOutwardDirection, eg->UseForwardDirection, eg->UseRandom2DDirection, eg->UseRandom3DDirection, eg->UseParamDirection);

            if (eg->HasSpline) {
                ImGui::Separator();
                ImGui::Text("SplineTension: %.3f", eg->SplineTension);
                ImGui::Text("Control Points: %d", (int)eg->SplineControlPoints.size());
            }
        }
        else if (auto* rm = dynamic_cast<CPSCRenderMesh*>(comp.get())) {
            ImGui::Text("BankIndex: %d | TrailBankIndex: %d", rm->BankIndex, rm->TrailBankIndex);
            DrawColour("StartColour", rm->StartColour);
            DrawColour("MidColour", rm->MidColour);
            DrawColour("EndColour", rm->EndColour);
            DrawColour("TrailStartCol", rm->TrailStartColour);
            ImGui::Text("RenderSizeParam: %u", rm->RenderSizeParam);
            ImGui::TextDisabled("(Plus 95 bytes of packed config fields)");
        }
        else if (auto* lgt = dynamic_cast<CPSCLight*>(comp.get())) {
            ImGui::Text("LightLifeSecs: %.3f | RespawnDelay: %.3f", lgt->LightLifeSecs, lgt->LightRespawnDelaySecs);
            ImGui::Text("RenderWorldRadius: Start %.3f | End %.3f", lgt->LightStartRenderWorldRadius, lgt->LightEndRenderWorldRadius);
            DrawColour("LightStartColour", lgt->LightStartColour);
            DrawColour("LightMidColour", lgt->LightMidColour);
            DrawColour("LightEndColour", lgt->LightEndColour);
        }
        else if (auto* sp = dynamic_cast<CPSCSpline*>(comp.get())) {
            ImGui::Text("SplineBounce: %d | ScaleSplineSpeed: %d", sp->SplineBounce, sp->ScaleSplineSpeed);
            ImGui::Text("SplineAnimSpeed: %.3f", sp->SplineAnimSpeed);
            DrawSplineBase(sp);
        }
        else if (auto* ss = dynamic_cast<CPSCSingleSprite*>(comp.get())) {
            ImGui::Text("SpriteBankIndex: %d | AnimTimeSecs: %.3f", ss->SpriteBankIndex, ss->AnimationTimeSecs);
            DrawColour("StartColour", ss->StartColour);
            ImGui::Text("RenderSize: Start %.3f | End %.3f", ss->StartRenderSize, ss->EndRenderSize);
            DrawSplineBase(ss);
        }
        else if (auto* at = dynamic_cast<CPSCAttractor*>(comp.get())) {
            ImGui::Text("InfluenceFallOff: %d", at->AttractorInfluenceFallOff);
            ImGui::Text("InfluenceRadius: %.3f | InfluenceForce: %.3f", at->AttractorInfluenceRadius, at->AttractorInfluenceForce);
            ImGui::Text("User Points: %d", (int)at->AttractorUserPointsArray.size());
        }
        else if (auto* ob = dynamic_cast<CPSCOrbit*>(comp.get())) {
            ImGui::Text("CentreParam: %u", ob->CentreParam);
            ImGui::Text("Orbit Floats Extracted: %d", (int)ob->OrbitsData.size());
        }
        else if (auto* dr = dynamic_cast<CPSCDecalRenderer*>(comp.get())) {
            ImGui::Text("DecalBankIndex: %d | LifeSecs: %.3f", dr->DecalBankIndex, dr->DecalLifeSecs);
            DrawColour("StartColour", dr->StartColour);
            DrawColour("MidColour", dr->MidColour);
            DrawColour("EndColour", dr->EndColour);
            ImGui::Text("MaxPoolSize: %.3f | MaxPoolLife: %.3f", dr->MaxPoolSize, dr->MaxPoolLife);
            ImGui::Text("PoolFrameIncrease: %.3f | StencilWidth: %.3f", dr->PoolFrameIncreaseRate, dr->StencilCubeWidth);
        }
        else {
            ImGui::TextDisabled("Diagnostic view not implemented for this component type yet.");
        }

        ImGui::TreePop();
    }
    ImGui::PopID();
}

// --- MAIN INSPECTOR VIEWS ---

inline void DrawParticleSystem(const CParticleSystem& sys) {
    ImGui::PushID(&sys);
    bool isOpen = ImGui::TreeNodeEx(sys.Name.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen);
    if (isOpen) {
        ImGui::Text("Enabled: %s | ScaleParticles: %s", sys.Enabled ? "TRUE" : "FALSE", sys.ScaleParticles ? "TRUE" : "FALSE");
        DrawVector3("Scale", sys.Scale);

        ImGui::Dummy(ImVec2(0, 5));
        ImGui::TextDisabled("Components (%d):", (int)sys.Components.size());
        for (const auto& comp : sys.Components) {
            DrawParticleComponent(comp);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

inline void DrawParticleProperties(const CParticleEmitter& emitter) {
    ImGui::Text("Emitter Name: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", emitter.Name.c_str());
    ImGui::TextDisabled("Magic: 0x%X", emitter.Magic);
    ImGui::Separator();

    if (ImGui::TreeNodeEx("Base Properties", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Text("Priority: %d", emitter.Priority);
        ImGui::Text("ContinuousEmitter: %s", emitter.ContinuousEmitter ? "TRUE" : "FALSE");
        ImGui::Text("MaxSpawnDistance: %.3f | MaxDrawDistance: %.3f", emitter.MaxSpawnDistance, emitter.MaxDrawDistance);
        ImGui::Text("FadeOutStart: %.3f | FadeInEnd: %.3f | FadeInStart: %.3f", emitter.FadeOutStart, emitter.FadeInEnd, emitter.FadeInStart);

        ImGui::Dummy(ImVec2(0, 5));
        ImGui::TextDisabled("Engine Flags:");
        ImGui::Text("2D: %d | PreWater: %d | Water: %d | ZBufferWrite: %d",
            emitter.Emitter2D, emitter.PreWaterEffect, emitter.WaterEffect, emitter.ZBufferWriteable);
        ImGui::Text("ScreenDisp: %d | ReadZ: %d | DieIfOffscreen: %d",
            emitter.IsScreenDisplacement, emitter.ReadZBuffer, emitter.DieIfOffscreen);
        ImGui::Text("OffscreenUpdate: %d | ClipWeather: %d | Dithering: %d",
            emitter.OffscreenUpdate, emitter.ClipEffectToWeatherMask, emitter.EnableDithering);

        ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::TextDisabled("Particle Systems (%d):", (int)emitter.Systems.size());
    ImGui::Dummy(ImVec2(0, 5));

    for (const auto& sys : emitter.Systems) {
        DrawParticleSystem(sys);
    }
}