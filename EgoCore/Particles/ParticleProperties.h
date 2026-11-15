#pragma once
#include "imgui.h"
#include "ParticleParser.h"
#include "ParticleRenderer.h"
#include "BinaryParser.h"
#include "MeshRenderer.h"
#include "BankBackend.h"
#include <string>
#include <cstring>
#include <cstdint>
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
inline bool g_ShowParticleRenderer = true;
static ParticleRenderer g_ParticleRenderer;
static ParticleSimulator g_ParticleSimulator;
static const CParticleEmitter* g_LastSimulatedEmitter = nullptr;
static bool g_ParticleSimNeedsReset = true;
static bool g_ParticlePlaying = true;
static float g_ParticleSimTime = 0.0f;
static float g_ParticleSimDuration = 10.0f;
static bool g_ParticleLoop = true;
static float g_ParticlePlaybackSpeed = 1.0f;
static uint64_t g_LastEmitterFingerprint = 0;
static uint64_t g_LastStructuralFingerprint = 0;
static bool g_HasEmitterFingerprint = false;

// --- LIVE EDIT FINGERPRINTING ---
namespace ParticleEditSync {
    struct EmitterFingerprint {
        uint64_t Full = 0;
        uint64_t Structural = 0;
    };

    inline void HashBytes(uint64_t& h, const void* data, size_t size) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < size; ++i) {
            h ^= bytes[i];
            h *= 1099511628211ULL;
        }
    }

    template <typename T>
    inline void HashPod(uint64_t& h, const T& val) {
        HashBytes(h, &val, sizeof(T));
    }

    inline void HashFloat(uint64_t& h, float v) {
        if (v == 0.0f) v = 0.0f;
        HashPod(h, v);
    }

    inline void HashBool(uint64_t& h, bool b) {
        uint8_t v = b ? 1 : 0;
        HashPod(h, v);
    }

    inline void HashString(uint64_t& h, const std::string& s) {
        size_t len = s.size();
        HashPod(h, len);
        if (len > 0) HashBytes(h, s.data(), len);
    }

    inline void HashVec3(uint64_t& h, const C3DVector& v) {
        HashFloat(h, v.X);
        HashFloat(h, v.Y);
        HashFloat(h, v.Z);
    }

    inline void HashColour(uint64_t& h, const CRGBColour& c) {
        HashPod(h, c.R);
        HashPod(h, c.G);
        HashPod(h, c.B);
        HashPod(h, c.A);
    }

    inline void HashSplineBase(uint64_t& h, const CPSCSplineBase& sb) {
        HashPod(h, sb.ControlPoints.size());
        for (const auto& pt : sb.ControlPoints) HashVec3(h, pt);
        HashBool(h, sb.HasRandomOffsetData);
        HashBool(h, sb.OffsetOnlyAtStartup);
        HashBool(h, sb.ResetEachFrame);
        HashPod(h, sb.RandomOffsets.size());
        for (const auto& off : sb.RandomOffsets) {
            HashVec3(h, off.Limit);
            HashVec3(h, off.Speed);
        }
        HashBool(h, sb.HasPosParamData);
        HashBool(h, sb.MaintainSplineShape);
        HashPod(h, sb.PosParams.size());
        for (uint32_t p : sb.PosParams) HashPod(h, p);
    }

    inline EmitterFingerprint ComputeFingerprint(const CParticleEmitter& emitter) {
        constexpr uint64_t kFnvOffset = 14695981039346656037ULL;
        uint64_t full = kFnvOffset;
        uint64_t struc = kFnvOffset;

        HashPod(full, emitter.Magic);
        HashString(full, emitter.Name);
        HashBool(full, emitter.Emitter2D);
        HashBool(full, emitter.PreWaterEffect);
        HashBool(full, emitter.WaterEffect);
        HashBool(full, emitter.ZBufferWriteable);
        HashBool(full, emitter.ContinuousEmitter);
        HashBool(full, emitter.IsScreenDisplacement);
        HashBool(full, emitter.ReadZBuffer);
        HashFloat(full, emitter.MaxSpawnDistance);
        HashFloat(full, emitter.MaxDrawDistance);
        HashFloat(full, emitter.FadeOutStart);
        HashFloat(full, emitter.FadeInEnd);
        HashFloat(full, emitter.FadeInStart);
        HashPod(full, emitter.Priority);
        HashBool(full, emitter.DieIfOffscreen);
        HashBool(full, emitter.OffscreenUpdate);
        HashBool(full, emitter.ClipEffectToWeatherMask);
        HashBool(full, emitter.EnableDithering);
        HashBool(full, emitter.CalcBoundingSphereOnceOnly);
        HashPod(full, emitter.Systems.size());

        HashBool(struc, emitter.ContinuousEmitter);
        HashPod(struc, emitter.Systems.size());

        for (const auto& sys : emitter.Systems) {
            HashString(full, sys.Name);
            HashBool(full, sys.Enabled);
            HashBool(full, sys.ScaleParticles);
            HashVec3(full, sys.Scale);
            HashPod(full, sys.Components.size());

            HashBool(struc, sys.Enabled);
            HashBool(struc, sys.ScaleParticles);
            HashVec3(struc, sys.Scale);
            HashPod(struc, sys.Components.size());

            for (const auto& comp : sys.Components) {
                if (!comp) {
                    HashPod(full, uint8_t(0));
                    HashPod(struc, uint8_t(0));
                    continue;
                }
                HashString(full, comp->ClassName);
                HashPod(full, comp->InstanceID);
                HashBool(full, comp->Enabled);
                HashBool(full, comp->Visible);

                HashString(struc, comp->ClassName);
                HashPod(struc, comp->InstanceID);
                HashBool(struc, comp->Enabled);
                HashBool(struc, comp->Visible);

                if (const auto* rs = dynamic_cast<const CPSCRenderSprite*>(comp.get())) {
                    HashPod(full, rs->SpriteBankIndex);
                    HashPod(full, rs->TrailBankIndex);
                    HashColour(full, rs->StartColour);
                    HashColour(full, rs->MidColour);
                    HashColour(full, rs->EndColour);
                    HashPod(full, rs->BlendMode);
                    HashPod(full, rs->TrailBlendMode);
                    HashPod(full, rs->BlendOp);
                    HashPod(full, rs->TrailBlendOp);
                    HashPod(full, rs->SpriteFlags);
                    HashPod(full, rs->FadeInEndInteger);
                    HashPod(full, rs->FadeOutBeginInteger);
                    HashPod(full, rs->TrailLengthInteger);
                    HashPod(full, rs->FlickerMinAlphaInteger);
                    HashPod(full, rs->FlickerMinSizeInteger);
                    HashPod(full, rs->NoCrossedSprites);
                    HashFloat(full, rs->StartRenderSize);
                    HashFloat(full, rs->AlphaFadeMinimum);
                    HashFloat(full, rs->EndRenderSize);
                    HashFloat(full, rs->FlickerBias);
                    HashFloat(full, rs->AnimationTimeSecs);
                    HashFloat(full, rs->SizeFadeMinimum);
                    HashFloat(full, rs->TrailWidth);
                    HashFloat(full, rs->FlickerSpeed);
                    HashBool(full, rs->UseStartColour);
                    HashBool(full, rs->UseMidColour);
                    HashBool(full, rs->UseEndColour);
                    HashBool(full, rs->AlphaFadeEnable);
                    HashBool(full, rs->SizeFadeEnable);
                    HashBool(full, rs->FlickerEnable);
                    HashBool(full, rs->ForceAnimationTime);
                }
                else if (const auto* eg = dynamic_cast<const CPSCEmitterGeneric*>(comp.get())) {
                    HashPod(full, eg->EmitterPosParam);
                    HashPod(full, eg->DirectionParamName);
                    HashPod(full, eg->NoParticlesToStart);
                    HashPod(full, eg->NoParticlesToStartRand);
                    HashPod(full, eg->EmitterType);
                    HashBool(full, eg->Solid);
                    HashBool(full, eg->UseEmitterLifeSecs);
                    HashBool(full, eg->UseEmitterTimelineSecs);
                    HashBool(full, eg->OrientationXY);
                    HashBool(full, eg->OrientationXZ);
                    HashBool(full, eg->OrientationYZ);
                    HashBool(full, eg->UseCustomDirection);
                    HashBool(full, eg->UseOutwardDirection);
                    HashBool(full, eg->UseParamDirection);
                    HashBool(full, eg->OppositeDirection);
                    HashPod(full, eg->AngularPerturbationInteger);
                    HashVec3(full, eg->CustomDirection);
                    HashFloat(full, eg->ParticlesPerSecond);
                    HashFloat(full, eg->EmitterSize);
                    HashFloat(full, eg->RadialBias);
                    HashFloat(full, eg->MinSpeed);
                    HashBool(full, eg->UseForwardDirection);
                    HashBool(full, eg->UseRandom2DDirection);
                    HashFloat(full, eg->EmitterTimelineSecs);
                    HashBool(full, eg->UseRandom3DDirection);
                    HashFloat(full, eg->EmitterLifeSecs);
                    HashFloat(full, eg->EmitterStartTime);
                    HashFloat(full, eg->MaxSpeed);
                    HashVec3(full, eg->NonUniformScaling);
                    HashBool(full, eg->HasSpline);
                    HashFloat(full, eg->SplineTension);
                    HashPod(full, eg->SplineControlPoints.size());
                    for (const auto& pt : eg->SplineControlPoints) HashVec3(full, pt);

                    // Structural / initial / timing properties for CPSCEmitterGeneric
                    HashPod(struc, eg->NoParticlesToStart);
                    HashPod(struc, eg->NoParticlesToStartRand);
                    HashPod(struc, eg->EmitterType);
                    HashBool(struc, eg->Solid);
                    HashBool(struc, eg->UseEmitterLifeSecs);
                    HashBool(struc, eg->UseEmitterTimelineSecs);
                    HashBool(struc, eg->OrientationXY);
                    HashBool(struc, eg->OrientationXZ);
                    HashBool(struc, eg->OrientationYZ);
                    HashBool(struc, eg->UseCustomDirection);
                    HashBool(struc, eg->UseOutwardDirection);
                    HashBool(struc, eg->UseParamDirection);
                    HashBool(struc, eg->OppositeDirection);
                    HashPod(struc, eg->AngularPerturbationInteger);
                    HashVec3(struc, eg->CustomDirection);
                    HashFloat(struc, eg->EmitterSize);
                    HashFloat(struc, eg->RadialBias);
                    HashFloat(struc, eg->MinSpeed);
                    HashBool(struc, eg->UseForwardDirection);
                    HashBool(struc, eg->UseRandom2DDirection);
                    HashFloat(struc, eg->EmitterTimelineSecs);
                    HashBool(struc, eg->UseRandom3DDirection);
                    HashFloat(struc, eg->EmitterLifeSecs);
                    HashFloat(struc, eg->EmitterStartTime);
                    HashFloat(struc, eg->MaxSpeed);
                    HashVec3(struc, eg->NonUniformScaling);
                    HashBool(struc, eg->HasSpline);
                    HashFloat(struc, eg->SplineTension);
                    HashPod(struc, eg->SplineControlPoints.size());
                    for (const auto& pt : eg->SplineControlPoints) HashVec3(struc, pt);
                }
                else if (const auto* un = dynamic_cast<const CPSCUpdateNormal*>(comp.get())) {
                    HashPod(full, un->FadeInEndInteger);
                    HashPod(full, un->FadeOutBeginInteger);
                    HashBool(full, un->UseParticleLifeSecs);
                    HashBool(full, un->UseRandomRotationAxis);
                    HashBool(full, un->UseRandomInitialRotation);
                    HashBool(full, un->UseAccelerationParam);
                    HashBool(full, un->StayWithEmitter);
                    HashBool(full, un->UseSystemLifeSecs);
                    HashBool(full, un->SystemAlphaFadeEnable);
                    HashBool(full, un->EmissionFadeEnable);
                    HashBool(full, un->ParticleCollideWithGround);
                    HashBool(full, un->ParticleCollideWithAnything);
                    HashBool(full, un->ParticleDieOnCollision);
                    HashBool(full, un->UseAllAttractors);
                    HashBool(full, un->RandomisePosEnable);
                    HashBool(full, un->RandomisePosDistVaryEnable);
                    HashBool(full, un->SetOrientationFromDirection);
                    HashBool(full, un->SetOrientationFromGame);
                    HashBool(full, un->ParticleCreateDecal);
                    HashBool(full, un->ParticleCreateDecalEmitter);
                    HashString(full, un->DecalEmitterName);
                    HashFloat(full, un->SystemLifeSecs);
                    HashFloat(full, un->ParticleLifeSecs);
                    HashFloat(full, un->WindFactor);
                    HashFloat(full, un->GravityFactor);
                    HashFloat(full, un->AirResistance);
                    HashFloat(full, un->ParticleAccelerationScale);
                    HashFloat(full, un->InitialRotationX);
                    HashFloat(full, un->InitialRotationY);
                    HashFloat(full, un->InitialRotationZ);
                    HashFloat(full, un->RotationMinAngleSpeed);
                    HashFloat(full, un->RotationMaxAngleSpeed);
                    HashFloat(full, un->ParticleBounce);
                    HashFloat(full, un->SystemAlphaFadeMinimum);
                    HashFloat(full, un->RandomisePosDistVaryScale);
                    HashFloat(full, un->EmissionFadeMinimum);
                    HashVec3(full, un->ParticleSystemOffset);
                    HashVec3(full, un->ParticleAcceleration);
                    HashVec3(full, un->RotationAxis);
                    HashVec3(full, un->RandomisePosSpeed);
                    HashVec3(full, un->RandomisePosScale);
                    HashPod(full, un->AccelerationParam);
                    HashPod(full, un->OrientFromGameParam);

                    // Structural / initial / timing properties for CPSCUpdateNormal
                    HashBool(struc, un->UseParticleLifeSecs);
                    HashBool(struc, un->UseRandomRotationAxis);
                    HashBool(struc, un->UseRandomInitialRotation);
                    HashBool(struc, un->StayWithEmitter);
                    HashBool(struc, un->UseSystemLifeSecs);
                    HashFloat(struc, un->SystemLifeSecs);
                    HashFloat(struc, un->ParticleLifeSecs);
                    HashFloat(struc, un->InitialRotationX);
                    HashFloat(struc, un->InitialRotationY);
                    HashFloat(struc, un->InitialRotationZ);
                    HashFloat(struc, un->RotationMinAngleSpeed);
                    HashFloat(struc, un->RotationMaxAngleSpeed);
                    HashVec3(struc, un->RotationAxis);
                    HashVec3(struc, un->ParticleSystemOffset);
                    HashBool(struc, un->RandomisePosEnable);
                    HashBool(struc, un->RandomisePosDistVaryEnable);
                    HashFloat(struc, un->RandomisePosDistVaryScale);
                    HashVec3(struc, un->RandomisePosSpeed);
                    HashVec3(struc, un->RandomisePosScale);
                }
                else if (const auto* dr = dynamic_cast<const CPSCDecalRenderer*>(comp.get())) {
                    HashFloat(full, dr->DecalLifeSecs);
                    HashPod(full, dr->DecalBankIndex);
                    HashColour(full, dr->StartColour);
                    HashColour(full, dr->MidColour);
                    HashColour(full, dr->EndColour);
                    HashPod(full, dr->BlendMode);
                    HashPod(full, dr->BlendOp);
                    HashPod(full, dr->SpriteAlignment);
                    HashPod(full, dr->FadeInEndInteger);
                    HashPod(full, dr->FadeOutBeginInteger);
                    HashPod(full, dr->StartRenderSizeInteger);
                    HashPod(full, dr->AlphaFadeMinimumInteger);
                    HashPod(full, dr->EndRenderSizeInteger);
                    HashPod(full, dr->AnimationTimeSecsInteger);
                    HashPod(full, dr->SizeFadeMinimumInteger);
                    HashBool(full, dr->UseStartColour);
                    HashBool(full, dr->UseMidColour);
                    HashBool(full, dr->UseEndColour);
                    HashBool(full, dr->AlphaFadeEnable);
                    HashBool(full, dr->SizeFadeEnable);
                    HashBool(full, dr->ForceAnimationTime);
                    HashPod(full, dr->FlickerMinAlphaInteger);
                    HashPod(full, dr->FlickerMinSizeInteger);
                    HashPod(full, dr->FlickerBiasInteger);
                    HashPod(full, dr->FlickerSpeedInteger);
                    HashBool(full, dr->FlickerEnable);
                    HashBool(full, dr->PoolingEnable);
                    HashBool(full, dr->PoolIncreaseAlphaEnable);
                    HashBool(full, dr->PoolIncreaseSizeEnable);
                    HashBool(full, dr->PoolIncreaseLifeEnable);
                    HashFloat(full, dr->MaxPoolSize);
                    HashFloat(full, dr->MaxPoolAlpha);
                    HashFloat(full, dr->MaxPoolLife);
                    HashBool(full, dr->PoolIncreaseFrameEnable);
                    HashFloat(full, dr->PoolFrameIncreaseRate);
                    HashFloat(full, dr->StencilCubeWidth);

                    HashFloat(struc, dr->DecalLifeSecs);
                    HashPod(struc, dr->DecalBankIndex);
                    HashBool(struc, dr->PoolingEnable);
                    HashFloat(struc, dr->MaxPoolLife);
                }
                else if (const auto* sp = dynamic_cast<const CPSCSpline*>(comp.get())) {
                    HashBool(full, sp->SplineBounce);
                    HashBool(full, sp->ScaleSplineSpeed);
                    HashFloat(full, sp->SplineAnimSpeed);
                    HashSplineBase(full, *sp);

                    HashBool(struc, sp->SplineBounce);
                    HashBool(struc, sp->ScaleSplineSpeed);
                    HashFloat(struc, sp->SplineAnimSpeed);
                    HashSplineBase(struc, *sp);
                }
                else if (const auto* ss = dynamic_cast<const CPSCSingleSprite*>(comp.get())) {
                    HashPod(full, ss->SpriteBankIndex);
                    HashColour(full, ss->StartColour);
                    HashColour(full, ss->MidColour);
                    HashColour(full, ss->EndColour);
                    HashFloat(full, ss->AnimationTimeSecs);
                    HashFloat(full, ss->StartRenderSize);
                    HashFloat(full, ss->EndRenderSize);
                    HashPod(full, ss->SpriteAlignment);
                    HashPod(full, ss->NoCrossedSprites);
                    HashFloat(full, ss->InitialAngle);
                    HashPod(full, ss->FadeInEndInteger);
                    HashPod(full, ss->FadeOutBeginInteger);
                    HashFloat(full, ss->AlphaFadeMinimum);
                    HashFloat(full, ss->SizeFadeMinimum);
                    HashPod(full, ss->PositionParam);
                    HashPod(full, ss->BlendMode);
                    HashPod(full, ss->BlendOp);
                    HashPod(full, ss->TrailBlendMode);
                    HashPod(full, ss->TrailBlendOp);
                    HashPod(full, ss->TrailBankIndex);
                    HashFloat(full, ss->TrailWidth);
                    HashPod(full, ss->MaxTrailLength);
                    HashPod(full, ss->StayWithEmitterIntFactor);
                    HashBool(full, ss->UseStartColour);
                    HashBool(full, ss->UseMidColour);
                    HashBool(full, ss->UseEndColour);
                    HashBool(full, ss->SelfIlluminating);
                    HashBool(full, ss->AlphaFadeEnable);
                    HashBool(full, ss->RotateAroundCentre);
                    HashBool(full, ss->FaceMe2D);
                    HashBool(full, ss->FaceMe3D);
                    HashBool(full, ss->CrossedSprites);
                    HashBool(full, ss->SizeFadeEnable);
                    HashBool(full, ss->ForceAnimationTime);
                    HashBool(full, ss->StayWithEmitter);
                    HashBool(full, ss->UsePosition);
                    HashBool(full, ss->UseSplinePoints);
                    HashSplineBase(full, *ss);

                    HashPod(struc, ss->SpriteBankIndex);
                    HashPod(struc, ss->TrailBankIndex);
                    HashFloat(struc, ss->AnimationTimeSecs);
                    HashBool(struc, ss->ForceAnimationTime);
                    HashFloat(struc, ss->InitialAngle);
                    HashBool(struc, ss->StayWithEmitter);
                    HashPod(struc, ss->StayWithEmitterIntFactor);
                    HashBool(struc, ss->UsePosition);
                    HashBool(struc, ss->UseSplinePoints);
                    HashSplineBase(struc, *ss);
                }
                else if (const auto* ob = dynamic_cast<const CPSCOrbit*>(comp.get())) {
                    HashPod(full, ob->CentreParam);
                    HashPod(full, ob->OrbitsData.size());
                    HashPod(struc, ob->CentreParam);
                    HashPod(struc, ob->OrbitsData.size());
                    for (const auto& o : ob->OrbitsData) {
                        HashPod(full, o.Type);
                        HashBool(full, o.Enabled);
                        HashFloat(full, o.Radius);
                        HashFloat(full, o.Expand);
                        HashBool(full, o.CycleFlag);
                        HashFloat(full, o.CycleTime);
                        HashFloat(full, o.SqueezeScale);
                        HashFloat(full, o.SqueezeAngle);
                        HashFloat(full, o.RotateSpeed);
                        HashFloat(full, o.RotateStart);
                        HashFloat(full, o.RotateSpeedRandom);
                        HashFloat(full, o.RotateStartRandom);

                        HashPod(struc, o.Type);
                        HashBool(struc, o.Enabled);
                        HashFloat(struc, o.Radius);
                        HashFloat(struc, o.Expand);
                        HashBool(struc, o.CycleFlag);
                        HashFloat(struc, o.CycleTime);
                        HashFloat(struc, o.SqueezeScale);
                        HashFloat(struc, o.SqueezeAngle);
                        HashFloat(struc, o.RotateSpeed);
                        HashFloat(struc, o.RotateStart);
                        HashFloat(struc, o.RotateSpeedRandom);
                        HashFloat(struc, o.RotateStartRandom);
                    }
                }
                else if (const auto* at = dynamic_cast<const CPSCAttractor*>(comp.get())) {
                    HashBool(full, at->AttractorEnabled);
                    HashBool(full, at->AttractorUseParamPosition);
                    HashPod(full, at->AttractorInfluenceFallOff);
                    HashPod(full, at->AttractorPositionParam);
                    HashPod(full, at->AttractorPositionParamName);
                    HashFloat(full, at->AttractorInfluenceRadius);
                    HashFloat(full, at->AttractorInfluenceForce);
                    HashPod(full, at->AttractorUserPointsArray.size());
                    for (const auto& pt : at->AttractorUserPointsArray) HashVec3(full, pt);

                    HashBool(struc, at->AttractorEnabled);
                    HashBool(struc, at->AttractorUseParamPosition);
                    HashPod(struc, at->AttractorInfluenceFallOff);
                    HashFloat(struc, at->AttractorInfluenceRadius);
                    HashFloat(struc, at->AttractorInfluenceForce);
                    HashPod(struc, at->AttractorUserPointsArray.size());
                    for (const auto& pt : at->AttractorUserPointsArray) HashVec3(struc, pt);
                }
                else if (const auto* lgt = dynamic_cast<const CPSCLight*>(comp.get())) {
                    HashPod(full, lgt->LightPositionParam);
                    HashFloat(full, lgt->LightLifeSecs);
                    HashFloat(full, lgt->LightRespawnDelaySecs);
                    HashFloat(full, lgt->LightStartTime);
                    HashFloat(full, lgt->LightTimelineSecs);
                    HashFloat(full, lgt->LightStartRenderWorldRadius);
                    HashFloat(full, lgt->LightEndRenderWorldRadius);
                    HashPod(full, lgt->LightAttenuationFactor);
                    HashPod(full, lgt->LightFadeInEndInteger);
                    HashPod(full, lgt->LightFadeOutBeginInteger);
                    HashFloat(full, lgt->LightWorldRadiusFadeMinimum);
                    HashColour(full, lgt->LightColourFadeMinimum);
                    HashBool(full, lgt->LightWorldRadiusFadeEnable);
                    HashBool(full, lgt->LightUseLifeSecs);
                    HashBool(full, lgt->LightEnabled);
                    HashBool(full, lgt->LightRespawns);
                    HashBool(full, lgt->LightColourFadeEnable);
                    HashBool(full, lgt->LightUseStartColour);
                    HashBool(full, lgt->LightUseMidColour);
                    HashBool(full, lgt->LightUseEndColour);
                    HashBool(full, lgt->LightUseFadeColour);
                    HashBool(full, lgt->LightUseTimelineSecs);
                    HashBool(full, lgt->LightHasInitializedTime);
                    HashColour(full, lgt->LightStartColour);
                    HashColour(full, lgt->LightMidColour);
                    HashColour(full, lgt->LightEndColour);

                    HashBool(struc, lgt->LightEnabled);
                    HashBool(struc, lgt->LightUseLifeSecs);
                    HashBool(struc, lgt->LightUseTimelineSecs);
                    HashBool(struc, lgt->LightRespawns);
                    HashFloat(struc, lgt->LightLifeSecs);
                    HashFloat(struc, lgt->LightRespawnDelaySecs);
                    HashFloat(struc, lgt->LightStartTime);
                    HashFloat(struc, lgt->LightTimelineSecs);
                }
                else if (const auto* rm = dynamic_cast<const CPSCRenderMesh*>(comp.get())) {
                    HashPod(full, rm->BankIndex);
                    HashPod(full, rm->TrailBankIndex);
                    HashColour(full, rm->TrailStartColour);
                    HashColour(full, rm->TrailMidColour);
                    HashColour(full, rm->TrailEndColour);
                    HashColour(full, rm->StartColour);
                    HashColour(full, rm->MidColour);
                    HashColour(full, rm->EndColour);
                    HashPod(full, rm->BlendMode);
                    HashPod(full, rm->TrailBlendMode);
                    HashPod(full, rm->BlendOp);
                    HashPod(full, rm->TrailBlendOp);
                    HashPod(full, rm->FadeInEndInteger);
                    HashPod(full, rm->FadeOutBeginInteger);
                    HashPod(full, rm->TrailLengthInteger);
                    HashPod(full, rm->FlickerMinAlphaInteger);
                    HashPod(full, rm->FlickerMinSizeInteger);
                    HashVec3(full, rm->StartRenderSize);
                    HashBool(full, rm->CentredOnPos);
                    HashBool(full, rm->AlphaFadeEnable);
                    HashVec3(full, rm->EndRenderSize);
                    HashBool(full, rm->SizeFadeEnable);
                    HashBool(full, rm->FlickerEnable);
                    HashBool(full, rm->TrailUseStartColour);
                    HashBool(full, rm->TrailUseMidColour);
                    HashBool(full, rm->TrailUseEndColour);
                    HashBool(full, rm->UseRenderSizeParam);
                    HashFloat(full, rm->FlickerBias);
                    HashFloat(full, rm->FlickerSpeed);
                    HashFloat(full, rm->AlphaFadeMinimum);
                    HashBool(full, rm->UseStartColour);
                    HashFloat(full, rm->SizeFadeMinimum);
                    HashBool(full, rm->UseMidColour);
                    HashFloat(full, rm->TrailWidth);
                    HashBool(full, rm->UseEndColour);
                    HashPod(full, rm->RenderSizeParam);
                }
            }
        }
        return { full, struc };
    }
}

// --- TOOLTIP HELPER ---
static void PropTooltip(const char* desc) {
    if (!desc || desc[0] == '\0') return;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// --- ICON BUTTON HELPERS ---
inline bool DrawAddIconButton(const char* id, float size = 18.0f, const char* tooltip = "Add") {
    bool clicked = false;
    if (g_AddTexture) {
        clicked = ImGui::ImageButton(id, g_AddTexture, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), kAddTint);
    }
    else {
        clicked = ImGui::Button((std::string("+##") + id).c_str(), ImVec2(size + 6, size + 2));
    }
    PropTooltip(tooltip);
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
    PropTooltip(tooltip);
    return clicked;
}

// --- UI HELPERS ---
inline bool DrawString(const char* label, std::string& str, const char* tooltip = nullptr) {
    char buf[256];
    strncpy_s(buf, str.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    bool edited = ImGui::InputText(label, buf, sizeof(buf));
    if (edited) {
        str = buf;
    }
    PropTooltip(tooltip);
    return edited;
}

inline bool DrawU32(const char* label, uint32_t& v, bool hex = false, const char* tooltip = nullptr) {
    bool edited = ImGui::InputScalar(label, ImGuiDataType_U32, &v, NULL, NULL, hex ? "%08X" : "%u");
    PropTooltip(tooltip);
    return edited;
}

inline bool DrawInt(const char* label, int32_t& v, const char* tooltip = nullptr) {
    bool edited = ImGui::InputInt(label, &v);
    PropTooltip(tooltip);
    return edited;
}

inline bool DrawFloat(const char* label, float& v, float speed = 0.1f, float minVal = 0.0f, float maxVal = 0.0f, const char* tooltip = nullptr) {
    bool edited = ImGui::DragFloat(label, &v, speed, minVal, maxVal);
    PropTooltip(tooltip);
    return edited;
}

inline bool DrawBool(const char* label, bool& v, const char* tooltip = nullptr) {
    bool edited = ImGui::Checkbox(label, &v);
    PropTooltip(tooltip);
    return edited;
}

inline bool DrawColour(const char* label, CRGBColour& col, const char* tooltip = nullptr) {
    float c[4] = { col.R / 255.0f, col.G / 255.0f, col.B / 255.0f, col.A / 255.0f };
    bool edited = ImGui::ColorEdit4(label, c);
    if (edited) {
        col.R = (uint8_t)(c[0] * 255.0f);
        col.G = (uint8_t)(c[1] * 255.0f);
        col.B = (uint8_t)(c[2] * 255.0f);
        col.A = (uint8_t)(c[3] * 255.0f);
    }
    PropTooltip(tooltip);
    return edited;
}

inline bool DrawVector3(const char* label, C3DVector& vec, const char* tooltip = nullptr) {
    bool edited = ImGui::DragFloat3(label, &vec.X, 0.05f);
    PropTooltip(tooltip);
    return edited;
}

inline bool DrawHashParam(const char* label, uint32_t& param, const char* tooltip = nullptr) {
    bool edited = false;
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    PropTooltip(tooltip);
    ImGui::SameLine(220);

    ImGui::SetNextItemWidth(90);
    edited |= ImGui::InputScalar((std::string("##val_") + label).c_str(), ImGuiDataType_U32, &param);
    PropTooltip(tooltip);

    ImGui::SameLine();
    char buf[128] = "";
    ImGui::SetNextItemWidth(130);
    if (ImGui::InputTextWithHint((std::string("##txt_") + label).c_str(), "String to Hash...", buf, 128)) {
        if (strlen(buf) > 0) {
            param = BinaryParser::CalculateCRC32_Fable(buf);
            edited = true;
        }
    }
    PropTooltip(tooltip ? tooltip : "Type a parameter name string to compute its Fable CRC32 hash.");
    return edited;
}

inline bool DrawSpriteFlagsEditor(uint32_t& flags, const char* tooltip = nullptr) {
    bool edited = DrawU32("SpriteFlags (Hex)", flags, true,
        tooltip ? tooltip : "Packed 10-bit sprite flags bitmask (FaceMe3D, SpriteAlignment, RotateAroundCentre, CrossedSprites, !SelfIlluminating).");

    auto flagBit = [&](const char* label, uint32_t mask, const char* bitTooltip, bool invert = false) {
        bool checked = invert ? ((flags & mask) == 0) : ((flags & mask) != 0);
        if (ImGui::Checkbox(label, &checked)) {
            bool setBit = invert ? !checked : checked;
            if (setBit) flags |= mask;
            else flags &= ~mask;
            edited = true;
        }
        PropTooltip(bitTooltip);
    };

    flagBit("FaceMe3D (0x001)", 0x001u,
        "Controls 3D vs 2D billboard facing mode (bit 0x001). Toggles spherical vs cylindrical upright camera facing.");
    flagBit("SpriteAlignment Bit 1 (Full Arc / Crossed 360) (0x002)", 0x002u,
        "Sprite alignment bit 1 (0x002): configures anchor placement and enables full 360-degree distribution for CrossedSprites.");
    flagBit("SpriteAlignment Bit 2 (0x004)", 0x004u,
        "Sprite alignment bit 2 (0x004): secondary anchor offset bit and full 360-degree distribution for CrossedSprites.");
    flagBit("RotateAroundCentre (0x020)", 0x020u,
        "When checked, quads pivot and rotate around their geometric center instead of their alignment edge/base.");
    flagBit("CrossedSprites (0x040)", 0x040u,
        "Renders NoCrossedSprites intersecting vertical quads fanned around the Z axis instead of a single camera billboard.");
    flagBit("SelfIlluminating (!0x080)", 0x080u,
        "Self-illuminated when checked (bit 0x080 clear): ignores dynamic scene/point lights and renders at full brightness.", true);

    return edited;
}

inline bool DrawNoCrossedSpritesSlider(const char* label, int32_t& count, const char* tooltip = nullptr) {
    int v = std::clamp((int)count, 1, 31);
    bool edited = ImGui::SliderInt(label, &v, 1, 31);
    if (edited) {
        count = (int32_t)std::clamp(v, 1, 31);
    }
    PropTooltip(tooltip);
    return edited;
}

inline bool DrawNoCrossedSpritesSlider(const char* label, uint32_t& count, const char* tooltip = nullptr) {
    int v = std::clamp((int)count, 1, 31);
    bool edited = ImGui::SliderInt(label, &v, 1, 31);
    if (edited) {
        count = (uint32_t)std::clamp(v, 1, 31);
    }
    PropTooltip(tooltip);
    return edited;
}

inline bool DrawParticleTexRow(const char* label, int32_t& bankIndex, const char* tooltip = nullptr) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    PropTooltip(tooltip);
    ImGui::SameLine(220);

    ImGui::SetNextItemWidth(80);
    bool edited = ImGui::InputInt((std::string("##id_") + label).c_str(), &bankIndex, 0, 0);
    PropTooltip(tooltip);

    ImGui::SameLine();
    if (DrawAddIconButton((std::string("##btn_") + label).c_str(), 18.0f, "Choose Texture from open bank")) {
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
    return edited;
}

inline bool DrawParticleMeshRow(const char* label, int32_t& bankIndex, const char* tooltip = nullptr) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    PropTooltip(tooltip);
    ImGui::SameLine(220);

    ImGui::SetNextItemWidth(80);
    bool edited = ImGui::InputInt((std::string("##id_") + label).c_str(), &bankIndex, 0, 0);
    PropTooltip(tooltip);

    ImGui::SameLine();
    if (DrawAddIconButton((std::string("##btn_") + label).c_str(), 18.0f, "Choose Type 4 Mesh from open bank")) {
        g_ParticleTargetMeshIDPtr = &bankIndex;
        g_ParticlePreviewMeshID = bankIndex;
        g_LoadedParticlePreviewMeshID = -1;
        g_ParticleMeshSearchBuf[0] = '\0';
        g_ShowParticleMeshPicker = true;
    }
    return edited;
}

inline bool DrawOrbitData(COrbitData& orbit) {
    bool edited = false;
    edited |= DrawInt("Type", orbit.Type,
        "Orbit plane and parent-rotation flags: low bits 0 = XY plane, 1 = XZ plane, 2 = YZ plane; bit 0x04 (Type & 4) = RotateWithParent.");
    edited |= DrawBool("Enabled", orbit.Enabled,
        "Enables this orbit stage in the 3-stage hierarchical orbit chain.");
    edited |= DrawBool("CycleFlag", orbit.CycleFlag,
        "Cycles radius expansion smoothly back and forth over CycleTime instead of expanding once over CycleTime.");
    edited |= DrawFloat("Radius", orbit.Radius, 0.1f, 0.0f, 0.0f,
        "Base orbital radius (in world units) for this orbit stage.");
    edited |= DrawFloat("Expand", orbit.Expand, 0.1f, 0.0f, 0.0f,
        "Radius expansion distance (in world units) applied over CycleTime (or oscillated when CycleFlag is enabled).");
    edited |= DrawFloat("CycleTime", orbit.CycleTime, 0.1f, 0.0f, 0.0f,
        "Duration/period (in seconds) of the orbit radius expansion or oscillation cycle.");
    edited |= DrawFloat("Squeeze Scale", orbit.SqueezeScale, 0.01f, 0.0f, 0.0f,
        "Minor-axis scale factor shaping the orbit into an ellipse (1.0 = circle, < 1.0 = squashed).");
    edited |= DrawFloat("Squeeze Angle", orbit.SqueezeAngle, 0.1f, 0.0f, 0.0f,
        "In-plane rotation angle (radians) orienting the squeezed ellipse axes.");
    edited |= DrawFloat("Rotate Speed", orbit.RotateSpeed, 0.1f, 0.0f, 0.0f,
        "Angular rotation speed (radians/sec) around the orbit center.");
    edited |= DrawFloat("Rotate Start", orbit.RotateStart, 0.1f, 0.0f, 0.0f,
        "Initial starting angle (radians) along the orbit at reset.");
    edited |= DrawFloat("Rotate Speed Rand", orbit.RotateSpeedRandom, 0.1f, 0.0f, 0.0f,
        "Random +/- angular speed variation (radians/sec) added to RotateSpeed at initialization.");
    edited |= DrawFloat("Rotate Start Rand", orbit.RotateStartRandom, 0.1f, 0.0f, 0.0f,
        "Random +/- initial angle variation (radians) added to RotateStart at initialization.");
    return edited;
}

inline bool DrawSplineBase(CPSCSplineBase* spline) {
    bool edited = false;
    bool treeOpen = ImGui::TreeNodeEx("Spline Base Data", ImGuiTreeNodeFlags_None);
    PropTooltip("Catmull-Rom spline control points, optional per-point random jitter offsets, and optional runtime position parameter bindings.");
    if (treeOpen) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Control Points: %d", (int)spline->ControlPoints.size());
        PropTooltip("Ordered 3D control points defining the Catmull-Rom spline path in local system space.");
        ImGui::SameLine();
        if (DrawAddIconButton("##AddSplineCP", 16.0f, "Add Control Point")) {
            spline->ControlPoints.push_back(C3DVector{ 0.0f, 0.0f, 0.0f });
            edited = true;
        }
        ImGui::PushID("SplineControlPoints");
        for (size_t i = 0; i < spline->ControlPoints.size(); ) {
            ImGui::PushID((int)i);
            edited |= DrawVector3(("Pt " + std::to_string(i)).c_str(), spline->ControlPoints[i],
                "Local (X, Y, Z) coordinates of this spline control point.");
            ImGui::SameLine();
            bool del = DrawDeleteIconButton("##DelSplineCP", 16.0f, "Delete Control Point");
            ImGui::PopID();
            if (del) {
                spline->ControlPoints.erase(spline->ControlPoints.begin() + i);
                edited = true;
            }
            else {
                ++i;
            }
        }
        ImGui::PopID();

        edited |= DrawBool("Has Random Offset Data", spline->HasRandomOffsetData,
            "Enables per-control-point ellipsoidal random jitter offsets.");
        if (spline->HasRandomOffsetData) {
            edited |= DrawBool("OffsetOnlyAtStartup", spline->OffsetOnlyAtStartup,
                "Applies each control point's random offset once at reset/startup instead of updating during playback.");
            ImGui::SameLine();
            edited |= DrawBool("ResetEachFrame", spline->ResetEachFrame,
                "Generates a fresh random offset within Limit every simulation tick instead of drifting smoothly by Speed.");

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Random Offsets: %d", (int)spline->RandomOffsets.size());
            PropTooltip("Per-point ellipsoidal offset bounds (Limit) and drift rates (Speed).");
            ImGui::SameLine();
            if (DrawAddIconButton("##AddSplineRO", 16.0f, "Add Random Offset")) {
                spline->RandomOffsets.push_back(CPSCSplineBase::RandomOffset{});
                edited = true;
            }
            ImGui::PushID("SplineRandomOffsets");
            for (size_t i = 0; i < spline->RandomOffsets.size(); ) {
                ImGui::PushID((int)i);
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Offset %d", (int)i);
                PropTooltip("Random offset parameters applied to the corresponding spline control point.");
                ImGui::SameLine();
                bool del = DrawDeleteIconButton("##DelSplineRO", 16.0f, "Delete Random Offset");
                edited |= DrawVector3("Limit", spline->RandomOffsets[i].Limit,
                    "Per-axis (X, Y, Z) ellipsoidal maximum displacement around the control point's base position.");
                edited |= DrawVector3("Speed", spline->RandomOffsets[i].Speed,
                    "Per-axis (X, Y, Z) wander speed (world units/sec) when smoothly drifting inside Limit.");
                ImGui::PopID();
                if (del) {
                    spline->RandomOffsets.erase(spline->RandomOffsets.begin() + i);
                    edited = true;
                }
                else {
                    ++i;
                }
            }
            ImGui::PopID();
        }

        edited |= DrawBool("Has Pos Param Data", spline->HasPosParamData,
            "Enables binding spline control points to runtime game position parameters (PosParams).");
        if (spline->HasPosParamData) {
            edited |= DrawBool("MaintainSplineShape", spline->MaintainSplineShape,
                "Translates the entire spline rigidly with the bound position parameter rather than moving individual points independently.");
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Pos Params: %d", (int)spline->PosParams.size());
            PropTooltip("CRC32 hashes of runtime game position parameters mapped to spline control points.");
            ImGui::SameLine();
            if (DrawAddIconButton("##AddSplinePP", 16.0f, "Add Pos Param")) {
                spline->PosParams.push_back(0);
                edited = true;
            }
            ImGui::PushID("SplinePosParams");
            for (size_t i = 0; i < spline->PosParams.size(); ) {
                ImGui::PushID((int)i);
                edited |= DrawU32(("Param " + std::to_string(i)).c_str(), spline->PosParams[i], false,
                    "CRC32 hash of a runtime position parameter binding for this spline point.");
                ImGui::SameLine();
                bool del = DrawDeleteIconButton("##DelSplinePP", 16.0f, "Delete Pos Param");
                ImGui::PopID();
                if (del) {
                    spline->PosParams.erase(spline->PosParams.begin() + i);
                    edited = true;
                }
                else {
                    ++i;
                }
            }
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    return edited;
}

inline bool DrawParticleComponent(std::shared_ptr<CParticleComponent>& comp, bool& requestDelete, size_t compIdx, bool* outStructuralEdited = nullptr) {
    if (!comp) return false;
    bool edited = false;
    bool structural = false;
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
        edited = true;
        structural = true;
    }

    if (isOpen) {
        ImGui::Indent(8.0f);
        ImGui::Spacing();

        if (DrawBool("Enabled", comp->Enabled, "Enables or disables simulation and evaluation of this component.")) { edited = true; structural = true; }
        ImGui::SameLine();
        if (DrawBool("Visible", comp->Visible, "Controls whether this component's visual output is drawn in the viewport.")) { edited = true; structural = true; }
        ImGui::SameLine();
        ImGui::TextDisabled("(CRC32: 0x%08X)", comp->InstanceID);
        PropTooltip("Unique Fable CRC32 instance identifier for this component within the system.");
        ImGui::Separator();

        if (auto* rs = dynamic_cast<CPSCRenderSprite*>(comp.get())) {
            edited |= DrawParticleTexRow("SpriteBankIndex", rs->SpriteBankIndex,
                "Texture ID from the open Texture/XboxGraphics bank used for each particle sprite (supports single frames and sprite sheets).");
            edited |= DrawParticleTexRow("TrailBankIndex", rs->TrailBankIndex,
                "Texture ID used for ribbon trails emitted behind moving particles (0 disables trails).");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Colours");
            ImGui::Separator();
            edited |= DrawColour("StartColour", rs->StartColour,
                "Particle RGBA tint color at birth (normalized lifetime t = 0.0), when UseStartColour is checked.");
            edited |= DrawColour("MidColour", rs->MidColour,
                "Particle RGBA tint color at mid-life (normalized lifetime t = 0.5), when UseMidColour is checked.");
            edited |= DrawColour("EndColour", rs->EndColour,
                "Particle RGBA tint color at death (normalized lifetime t = 1.0), when UseEndColour is checked.");

            edited |= DrawBool("UseStartColour", rs->UseStartColour,
                "Includes StartColour at t = 0.0 in the particle color spline."); ImGui::SameLine();
            edited |= DrawBool("UseMidColour", rs->UseMidColour,
                "Includes MidColour as the midpoint keyframe at t = 0.5 in the particle color spline."); ImGui::SameLine();
            edited |= DrawBool("UseEndColour", rs->UseEndColour,
                "Includes EndColour at t = 1.0 in the particle color spline.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Render Settings");
            ImGui::Separator();
            edited |= DrawU32("BlendMode", rs->BlendMode, false,
                "Sprite blending mode (EEnginePrimitiveBlendMode): 0 = Solid, 1 = Boolean Alpha, 2 = Alpha, 3 = Additive, 4 = AddSmooth, 5 = ConstColour.");
            edited |= DrawU32("TrailBlendMode", rs->TrailBlendMode, false,
                "Ribbon trail blending mode: 0 = Solid, 1 = Boolean Alpha, 2 = Alpha, 3 = Additive, 4 = AddSmooth, 5 = ConstColour.");
            edited |= DrawU32("BlendOp", rs->BlendOp, false,
                "Hardware blend operation for sprites: 0 = Add, 1 = Subtract, 2 = Reverse Subtract.");
            edited |= DrawU32("TrailBlendOp", rs->TrailBlendOp, false,
                "Hardware blend operation for ribbon trails: 0 = Add, 1 = Subtract, 2 = Reverse Subtract.");
            if (DrawSpriteFlagsEditor(rs->SpriteFlags)) {
                edited = true;
                if ((rs->SpriteFlags & 0x040u) != 0) {
                    rs->NoCrossedSprites = std::clamp(rs->NoCrossedSprites, 1u, 31u);
                }
            }
            edited |= DrawNoCrossedSpritesSlider("NoCrossedSprites", rs->NoCrossedSprites,
                "Number of intersecting vertical quads emitted around Z when CrossedSprites (0x040) is enabled (1-31).");

            edited |= DrawFloat("Start Render Size", rs->StartRenderSize, 0.1f, 0.0f, 0.0f,
                "Sprite world size (in meters) at particle birth (t = 0.0).");
            edited |= DrawFloat("End Render Size", rs->EndRenderSize, 0.1f, 0.0f, 0.0f,
                "Sprite world size (in meters) at particle death (t = 1.0); linearly interpolated from Start Render Size.");
            edited |= DrawFloat("Anim Time Secs", rs->AnimationTimeSecs, 0.1f, 0.0f, 0.0f,
                "Sprite-sheet animation cycle duration (in seconds) when ForceAnimTime is enabled.");
            edited |= DrawBool("ForceAnimTime", rs->ForceAnimationTime,
                "Loops sprite-sheet frames every Anim Time Secs instead of stretching one animation cycle across particle lifetime.");
            edited |= DrawFloat("Trail Width", rs->TrailWidth, 0.1f, 0.0f, 0.0f,
                "World-space ribbon width (in meters) of particle trails.");
            edited |= DrawU32("Trail Length Int", rs->TrailLengthInteger, false,
                "Trail delay/length in thousandths of a tick-history step (0 = off, 1000 = 1.0 step, > 2000 stretches further back).");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Fades & Flickering");
            ImGui::Separator();
            edited |= DrawU32("FadeInEndInteger", rs->FadeInEndInteger, false,
                "Normalized lifetime (0-1000, in thousandths) where cosine fade-in completes (e.g. 200 = 20% of lifetime).");
            edited |= DrawU32("FadeOutBeginInteger", rs->FadeOutBeginInteger, false,
                "Normalized lifetime (0-1000, in thousandths) where cosine fade-out begins (e.g. 800 = 80% of lifetime).");
            edited |= DrawBool("AlphaFadeEnable", rs->AlphaFadeEnable,
                "Enables smooth cosine fade-in/out on sprite alpha over lifetime."); ImGui::SameLine();
            edited |= DrawBool("SizeFadeEnable", rs->SizeFadeEnable,
                "Enables smooth cosine fade-in/out multiplier on sprite size over lifetime.");
            edited |= DrawFloat("Alpha Fade Minimum", rs->AlphaFadeMinimum, 0.01f, 0.0f, 1.0f,
                "Minimum alpha multiplier (0.0-1.0) at the start and end of the alpha fade curve.");
            edited |= DrawFloat("Size Fade Minimum", rs->SizeFadeMinimum, 0.01f, 0.0f, 0.0f,
                "Minimum size multiplier (0.0-1.0) at the start and end of the size fade curve.");

            edited |= DrawBool("Flicker Enable", rs->FlickerEnable,
                "Modulates sprite alpha and size using 2D Perlin noise over time.");
            edited |= DrawU32("FlickerMinAlphaInteger", rs->FlickerMinAlphaInteger, false,
                "Minimum alpha multiplier during flicker in thousandths (0-1000, where 1000 = no alpha reduction).");
            edited |= DrawU32("FlickerMinSizeInteger", rs->FlickerMinSizeInteger, false,
                "Minimum size multiplier during flicker in thousandths (0-1000, where 1000 = no size reduction).");
            edited |= DrawFloat("Flicker Speed", rs->FlickerSpeed, 0.1f, 0.0f, 0.0f,
                "Frequency multiplier controlling how fast Perlin noise flicker fluctuates.");
            edited |= DrawFloat("Flicker Bias", rs->FlickerBias, 0.01f, 0.0f, 0.0f,
                "Flicker bias (-1.0 to +1.0) shifting noise toward dimmer/smaller (-) or brighter/larger (+) values.");
        }
        else if (auto* un = dynamic_cast<CPSCUpdateNormal*>(comp.get())) {
            edited |= DrawString("DecalEmitterName", un->DecalEmitterName,
                "Name of a secondary decal emitter spawned on particle collision when ParticleCreateDecalEmitter is enabled.");
            if (DrawFloat("SystemLifeSecs", un->SystemLifeSecs, 0.1f, 0.0f, 0.0f,
                "Total active duration (in seconds) of this system when UseSystemLifeSecs is enabled.")) { edited = true; structural = true; }
            if (DrawFloat("ParticleLifeSecs", un->ParticleLifeSecs, 0.1f, 0.0f, 0.0f,
                "Lifetime (in seconds) of each spawned particle when UseParticleLifeSecs is enabled.")) { edited = true; structural = true; }
            edited |= DrawFloat("WindFactor", un->WindFactor, 0.01f, 0.0f, 0.0f,
                "Multiplier for world wind force acting on particles (world wind is zero in standalone preview).");
            edited |= DrawFloat("GravityFactor", un->GravityFactor, 0.01f, 0.0f, 0.0f,
                "Gravity multiplier applied along -Z (-9.82 * GravityFactor m/s^2). Negative values make particles rise.");
            edited |= DrawFloat("AirResistance", un->AirResistance, 0.01f, 0.0f, 0.0f,
                "Exponential velocity drag per second (0.0 = no drag, ~1.0 = rapid damping).");
            edited |= DrawFloat("ParticleBounce", un->ParticleBounce, 0.01f, 0.0f, 0.0f,
                "Restitution (0.0-1.0) controlling vertical speed retained when bouncing off the Z = 0 ground plane.");
            edited |= DrawFloat("AccelScale", un->ParticleAccelerationScale, 0.01f, 0.0f, 0.0f,
                "Scalar multiplier applied to the ParticleAcceleration vector.");
            if (DrawVector3("ParticleSystemOffset", un->ParticleSystemOffset,
                "Local (X, Y, Z) offset of this system's origin relative to the emitter center.")) { edited = true; structural = true; }
            edited |= DrawVector3("ParticleAcceleration", un->ParticleAcceleration,
                "Constant acceleration vector (X, Y, Z) applied to particles each tick (scaled by AccelScale).");
            edited |= DrawBool("UseAccelerationParam", un->UseAccelerationParam,
                "Reads particle acceleration from the runtime AccelerationParam instead of the static vector.");
            if (DrawVector3("RotationAxis", un->RotationAxis,
                "Local 3D axis (X, Y, Z) around which mesh particles spin when UseRandomRotationAxis is disabled.")) { edited = true; structural = true; }
            if (DrawFloat("RotationMinAngleSpeed", un->RotationMinAngleSpeed, 0.1f, 0.0f, 0.0f,
                "Minimum angular spin speed (radians/sec) assigned at particle birth.")) { edited = true; structural = true; }
            if (DrawFloat("RotationMaxAngleSpeed", un->RotationMaxAngleSpeed, 0.1f, 0.0f, 0.0f,
                "Maximum angular spin speed (radians/sec) assigned at particle birth.")) { edited = true; structural = true; }
            if (DrawVector3("RandomisePosSpeed", un->RandomisePosSpeed,
                "Per-axis (X, Y, Z) speed of Perlin noise position wobble when RandomisePosEnable is enabled.")) { edited = true; structural = true; }
            if (DrawVector3("RandomisePosScale", un->RandomisePosScale,
                "Per-axis (X, Y, Z) amplitude (in world units) of Perlin noise position displacement.")) { edited = true; structural = true; }
            edited |= DrawU32("FadeInEndInteger", un->FadeInEndInteger, false,
                "System-level normalized time (0-1000) where system alpha and emission rate finish fading in.");
            edited |= DrawU32("FadeOutBeginInteger", un->FadeOutBeginInteger, false,
                "System-level normalized time (0-1000) where system alpha and emission rate begin fading out.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Lifecycle & Fading");
            ImGui::Separator();
            if (DrawBool("UseSystemLifeSecs", un->UseSystemLifeSecs,
                "Enforces SystemLifeSecs as a finite duration for emission and system-level fading.")) { edited = true; structural = true; }
            ImGui::SameLine();
            if (DrawBool("UseParticleLifeSecs", un->UseParticleLifeSecs,
                "Enforces ParticleLifeSecs as each particle's lifespan (unchecked lets particles persist long-term).")) { edited = true; structural = true; }
            edited |= DrawBool("SystemAlphaFadeEnable", un->SystemAlphaFadeEnable,
                "Applies the system fade curve to the overall alpha of all renderers in this system."); ImGui::SameLine();
            edited |= DrawBool("EmissionFadeEnable", un->EmissionFadeEnable,
                "Modulates continuous particle emission rate over SystemLifeSecs using the system fade curve.");
            edited |= DrawFloat("SystemAlphaFadeMinimum", un->SystemAlphaFadeMinimum, 0.01f, 0.0f, 0.0f,
                "Minimum system-wide alpha multiplier (0.0-1.0) at the start/end of the system fade curve.");
            edited |= DrawFloat("EmissionFadeMinimum", un->EmissionFadeMinimum, 0.01f, 0.0f, 0.0f,
                "Minimum emission rate multiplier (0.0-1.0) at the start/end of the system fade curve.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Physics & Interaction");
            ImGui::Separator();
            edited |= DrawBool("ParticleCollideWithGround", un->ParticleCollideWithGround,
                "Collides particles against the ground plane (Z = 0), bouncing or triggering decals/death.");
            edited |= DrawBool("ParticleCollideWithAnything", un->ParticleCollideWithAnything,
                "Enables collision against world/entity geometry in-game (preview simulates Z = 0 ground only).");
            edited |= DrawBool("ParticleDieOnCollision", un->ParticleDieOnCollision,
                "Destroys particles immediately on collision instead of bouncing.");
            edited |= DrawBool("UseAllAttractors", un->UseAllAttractors,
                "Applies forces from all enclosing CPSCAttractor points simultaneously; unchecked uses only the smallest-radius attractor.");
            if (DrawBool("StayWithEmitter", un->StayWithEmitter,
                "Translates active particles along with emitter origin movement (from CPSCOrbit or CPSCSpline).")) { edited = true; structural = true; }

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Rotation & Orientation");
            ImGui::Separator();
            if (DrawBool("UseRandomRotationAxis", un->UseRandomRotationAxis,
                "Selects a random 3D spin axis per particle instead of using RotationAxis.")) { edited = true; structural = true; }
            if (DrawBool("UseRandomInitialRotation", un->UseRandomInitialRotation,
                "Randomizes each particle's initial angle and 3D orientation at birth instead of using InitialRotationX/Y/Z.")) { edited = true; structural = true; }
            edited |= DrawBool("SetOrientationFromDirection", un->SetOrientationFromDirection,
                "Continuously aligns 3D mesh particle orientation to face the particle's velocity vector.");
            edited |= DrawBool("SetOrientationFromGame", un->SetOrientationFromGame,
                "Drives particle orientation from the runtime game parameter OrientFromGameParam.");
            if (DrawFloat("InitialRotationX", un->InitialRotationX, 0.1f, 0.0f, 0.0f,
                "Initial X-axis rotation (radians) at birth; also sets initial 2D sprite roll angle.")) { edited = true; structural = true; }
            if (DrawFloat("InitialRotationY", un->InitialRotationY, 0.1f, 0.0f, 0.0f,
                "Initial Y-axis rotation (radians) at birth for 3D mesh particles.")) { edited = true; structural = true; }
            if (DrawFloat("InitialRotationZ", un->InitialRotationZ, 0.1f, 0.0f, 0.0f,
                "Initial Z-axis rotation (radians) at birth for 3D mesh particles.")) { edited = true; structural = true; }

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Randomise Position");
            ImGui::Separator();
            if (DrawBool("RandomisePosEnable", un->RandomisePosEnable,
                "Adds smooth 3D Perlin noise turbulence to particle positions over their lifetime.")) { edited = true; structural = true; }
            if (DrawBool("RandomisePosDistVaryEnable", un->RandomisePosDistVaryEnable,
                "Varies RandomisePos turbulence amplitude by distance/age using RandomisePosDistVaryScale.")) { edited = true; structural = true; }
            if (DrawFloat("RandomisePosDistVaryScale", un->RandomisePosDistVaryScale, 0.1f, 0.0f, 0.0f,
                "Scale factor applied to RandomisePos displacement when RandomisePosDistVaryEnable is checked.")) { edited = true; structural = true; }

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Decals & Parameters");
            ImGui::Separator();
            edited |= DrawBool("ParticleCreateDecal", un->ParticleCreateDecal,
                "Spawns a ground decal via this system's CPSCDecalRenderer whenever a particle impacts Z = 0.");
            edited |= DrawBool("ParticleCreateDecalEmitter", un->ParticleCreateDecalEmitter,
                "Spawns the named DecalEmitterName sub-emitter on particle collision.");
            edited |= DrawHashParam("AccelerationParam", un->AccelerationParam,
                "CRC32 hash of the runtime game parameter supplying dynamic particle acceleration.");
            edited |= DrawHashParam("OrientFromGameParam", un->OrientFromGameParam,
                "CRC32 hash of the runtime game parameter supplying particle orientation.");
        }
        else if (auto* eg = dynamic_cast<CPSCEmitterGeneric*>(comp.get())) {
            edited |= DrawHashParam("EmitterPosParam", eg->EmitterPosParam,
                "CRC32 hash of a runtime position source (e.g. \"System Offset\", component .Centre, orbit .Primary/.Secondary/.Tertiary, or spline).");
            edited |= DrawHashParam("DirectionParamName", eg->DirectionParamName,
                "CRC32 hash of a runtime game parameter supplying emission direction when UseParamDirection is enabled.");
            if (DrawU32("EmitterType", eg->EmitterType, false,
                "Emitter spawn shape: 0 = Point, 1 = 2D Disc / Circle, 2 = 3D Sphere, 3 = Spline (requires HasSpline).")) { edited = true; structural = true; }
            if (DrawU32("NoParticlesToStart", eg->NoParticlesToStart, false,
                "Fixed number of particles spawned in an immediate initial burst when the emitter starts.")) { edited = true; structural = true; }
            if (DrawU32("NoParticlesToStartRand", eg->NoParticlesToStartRand, false,
                "Maximum additional random particles (0..N) added to the initial burst count.")) { edited = true; structural = true; }

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Physics & Sizing");
            ImGui::Separator();
            edited |= DrawFloat("Particles Per Sec", eg->ParticlesPerSecond, 0.5f, 0.0f, 0.0f,
                "Continuous spawn rate (particles per second) while emission is active.");
            if (DrawFloat("Min Speed", eg->MinSpeed, 0.1f, 0.0f, 0.0f,
                "Minimum initial particle launch speed (m/s) along the emission direction.")) { edited = true; structural = true; }
            if (DrawFloat("Max Speed", eg->MaxSpeed, 0.1f, 0.0f, 0.0f,
                "Maximum initial particle launch speed (m/s) along the emission direction.")) { edited = true; structural = true; }
            if (DrawFloat("Emitter Size", eg->EmitterSize, 0.1f, 0.0f, 0.0f,
                "Diameter of the 2D Disc or 3D Sphere spawn volume (radius = Emitter Size * 0.5).")) { edited = true; structural = true; }
            if (DrawFloat("Radial Bias", eg->RadialBias, 0.1f, 0.0f, 0.0f,
                "Controls radial spawn density in Solid emitters (0 = uniform, positive = bias toward center, negative = bias toward outer rim).")) { edited = true; structural = true; }
            if (DrawVector3("Non-Uniform Scaling", eg->NonUniformScaling,
                "Per-axis (X, Y, Z) stretch multiplier applied to the emitter spawn shape.")) { edited = true; structural = true; }

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Lifecycles");
            ImGui::Separator();
            if (DrawFloat("Emitter Life Secs", eg->EmitterLifeSecs, 0.1f, 0.0f, 0.0f,
                "Duration (in seconds) that continuous emission stays active when UseEmitterLifeSecs is enabled.")) { edited = true; structural = true; }
            if (DrawFloat("Emitter Start Time", eg->EmitterStartTime, 0.1f, 0.0f, 0.0f,
                "Delay (in seconds) from system start before continuous emission begins.")) { edited = true; structural = true; }
            if (DrawFloat("Emitter Timeline Secs", eg->EmitterTimelineSecs, 0.1f, 0.0f, 0.0f,
                "Loop period (in seconds) for repeating EmitterStartTime / EmitterLifeSecs when UseEmitterTimelineSecs is enabled.")) { edited = true; structural = true; }
            if (DrawBool("UseEmitterLifeSecs", eg->UseEmitterLifeSecs,
                "Limits continuous emission to Emitter Life Secs after Emitter Start Time.")) { edited = true; structural = true; }
            if (DrawBool("UseEmitterTimelineSecs", eg->UseEmitterTimelineSecs,
                "Wraps emitter clock modulo Emitter Timeline Secs so active emission windows repeat periodically.")) { edited = true; structural = true; }

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Direction & Orientation");
            ImGui::Separator();
            if (DrawBool("Solid", eg->Solid,
                "Spawns particles throughout the interior volume of a Disc or Sphere rather than only on its outer perimeter/surface.")) { edited = true; structural = true; }
            if (DrawU32("Angular Perturbation Int", eg->AngularPerturbationInteger, false,
                "Random directional cone spread in thousandths of a perpendicular unit vector (0 = exact direction, higher = wider spread).")) { edited = true; structural = true; }
            if (DrawBool("OrientationXY", eg->OrientationXY,
                "Orients the emitter disc/shape in the horizontal XY plane (Z-up).")) { edited = true; structural = true; }
            ImGui::SameLine();
            if (DrawBool("OrientationXZ", eg->OrientationXZ,
                "Rotates the emitter shape into the vertical XZ plane.")) { edited = true; structural = true; }
            ImGui::SameLine();
            if (DrawBool("OrientationYZ", eg->OrientationYZ,
                "Rotates the emitter shape into the vertical YZ plane.")) { edited = true; structural = true; }

            if (DrawBool("UseOutwardDirection", eg->UseOutwardDirection,
                "Launches particles radially outward from the emitter center/surface normal.")) { edited = true; structural = true; }
            if (DrawBool("UseForwardDirection", eg->UseForwardDirection,
                "Launches particles along the emitter or orbit forward tangent vector.")) { edited = true; structural = true; }
            if (DrawBool("OppositeDirection", eg->OppositeDirection,
                "Negates (reverses) the final emission direction vector.")) { edited = true; structural = true; }
            if (DrawBool("UseParamDirection", eg->UseParamDirection,
                "Reads emission direction from the runtime game parameter specified by DirectionParamName.")) { edited = true; structural = true; }

            if (DrawBool("UseCustomDirection", eg->UseCustomDirection,
                "Launches particles along the explicit Custom Direction Vector.")) { edited = true; structural = true; }
            if (DrawBool("UseRandom2DDirection", eg->UseRandom2DDirection,
                "Picks a random 2D unit direction within the emitter's orientation plane.")) { edited = true; structural = true; }
            if (DrawBool("UseRandom3DDirection", eg->UseRandom3DDirection,
                "Picks a random isotropic 3D unit direction on the unit sphere.")) { edited = true; structural = true; }
            if (DrawVector3("Custom Direction Vector", eg->CustomDirection,
                "Explicit (X, Y, Z) launch direction used when UseCustomDirection is checked.")) { edited = true; structural = true; }

            if (DrawBool("HasSpline", eg->HasSpline,
                "Enables spline-shaped particle spawning along SplineControlPoints (used with EmitterType = 3).")) { edited = true; structural = true; }
            if (eg->HasSpline) {
                ImGui::Separator();
                ImGui::TextDisabled("Spline Data:");
                PropTooltip("Catmull-Rom spline geometry used when EmitterType = 3.");
                if (DrawFloat("SplineTension", eg->SplineTension, 0.05f, 0.0f, 0.0f,
                    "Catmull-Rom curve tension controlling tightness of the emitter spawn spline.")) { edited = true; structural = true; }
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Control Points: %d", (int)eg->SplineControlPoints.size());
                PropTooltip("Ordered 3D points along which particles spawn when EmitterType = 3 and HasSpline is enabled.");
                ImGui::SameLine();
                if (DrawAddIconButton("##AddEmitterSplineCP", 16.0f, "Add Spline Control Point")) {
                    eg->SplineControlPoints.push_back(C3DVector{ 0.0f, 0.0f, 0.0f });
                    edited = true;
                    structural = true;
                }
                ImGui::PushID("EmitterSplineControlPoints");
                for (size_t i = 0; i < eg->SplineControlPoints.size(); ) {
                    ImGui::PushID((int)i);
                    if (DrawVector3(("Pt " + std::to_string(i)).c_str(), eg->SplineControlPoints[i],
                        "Local (X, Y, Z) coordinates of this emitter spawn spline point.")) {
                        edited = true;
                        structural = true;
                    }
                    ImGui::SameLine();
                    bool del = DrawDeleteIconButton("##DelEmitterSplineCP", 16.0f, "Delete Control Point");
                    ImGui::PopID();
                    if (del) {
                        eg->SplineControlPoints.erase(eg->SplineControlPoints.begin() + i);
                        edited = true;
                        structural = true;
                    }
                    else {
                        ++i;
                    }
                }
                ImGui::PopID();
            }
        }
        else if (auto* rm = dynamic_cast<CPSCRenderMesh*>(comp.get())) {
            edited |= DrawParticleMeshRow("BankIndex", rm->BankIndex,
                "Type 4 3D mesh ID from the open Graphics/XboxGraphics bank rendered for each particle.");
            edited |= DrawParticleTexRow("TrailBankIndex", rm->TrailBankIndex,
                "Texture ID for ribbon trails drawn behind moving mesh particles (0 disables trails).");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Mesh Colours");
            ImGui::Separator();
            edited |= DrawColour("StartColour", rm->StartColour,
                "Mesh RGBA tint color at particle birth (t = 0.0).");
            edited |= DrawColour("MidColour", rm->MidColour,
                "Mesh RGBA tint color at mid-life (t = 0.5).");
            edited |= DrawColour("EndColour", rm->EndColour,
                "Mesh RGBA tint color at particle death (t = 1.0).");
            edited |= DrawBool("UseStartColour", rm->UseStartColour,
                "Includes StartColour at t = 0.0 in the mesh color spline."); ImGui::SameLine();
            edited |= DrawBool("UseMidColour", rm->UseMidColour,
                "Includes MidColour at t = 0.5 in the mesh color spline."); ImGui::SameLine();
            edited |= DrawBool("UseEndColour", rm->UseEndColour,
                "Includes EndColour at t = 1.0 in the mesh color spline.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Trail Colours");
            ImGui::Separator();
            edited |= DrawColour("TrailStartCol", rm->TrailStartColour,
                "Mesh ribbon trail RGBA color at particle birth (t = 0.0).");
            edited |= DrawColour("TrailMidCol", rm->TrailMidColour,
                "Mesh ribbon trail RGBA color at mid-life (t = 0.5).");
            edited |= DrawColour("TrailEndCol", rm->TrailEndColour,
                "Mesh ribbon trail RGBA color at particle death (t = 1.0).");
            edited |= DrawBool("TrailUseStartColour", rm->TrailUseStartColour,
                "Includes TrailStartCol in the mesh trail color spline."); ImGui::SameLine();
            edited |= DrawBool("TrailUseMidColour", rm->TrailUseMidColour,
                "Includes TrailMidCol at t = 0.5 in the mesh trail color spline."); ImGui::SameLine();
            edited |= DrawBool("TrailUseEndColour", rm->TrailUseEndColour,
                "Includes TrailEndCol at t = 1.0 in the mesh trail color spline.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Render Settings");
            ImGui::Separator();
            edited |= DrawU32("BlendMode", rm->BlendMode, false,
                "Mesh blending mode: 0 = Solid, 1 = Boolean Alpha, 2 = Alpha, 3 = Additive, 4 = AddSmooth, 5 = ConstColour.");
            edited |= DrawU32("TrailBlendMode", rm->TrailBlendMode, false,
                "Mesh trail blending mode: 0 = Solid, 1 = Boolean Alpha, 2 = Alpha, 3 = Additive, 4 = AddSmooth, 5 = ConstColour.");
            edited |= DrawU32("BlendOp", rm->BlendOp, false,
                "Hardware blend operation for meshes: 0 = Add, 1 = Subtract, 2 = Reverse Subtract.");
            edited |= DrawU32("TrailBlendOp", rm->TrailBlendOp, false,
                "Hardware blend operation for mesh trails: 0 = Add, 1 = Subtract, 2 = Reverse Subtract.");
            edited |= DrawVector3("StartRenderSize", rm->StartRenderSize,
                "Per-axis (X, Y, Z) mesh scale at particle birth (t = 0.0).");
            edited |= DrawVector3("EndRenderSize", rm->EndRenderSize,
                "Per-axis (X, Y, Z) mesh scale at particle death (t = 1.0); linearly interpolated from StartRenderSize.");

            edited |= DrawHashParam("RenderSizeParam", rm->RenderSizeParam,
                "CRC32 hash of a runtime game parameter overriding mesh render scale when UseRenderSizeParam is checked.");
            edited |= DrawBool("UseRenderSizeParam", rm->UseRenderSizeParam,
                "Uses RenderSizeParam from the game instead of StartRenderSize/EndRenderSize.");
            edited |= DrawBool("CentredOnPos", rm->CentredOnPos,
                "Centers the mesh's bounding box on the particle position rather than using its authored pivot origin.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Fading & Flickering");
            ImGui::Separator();
            edited |= DrawU32("FadeInEndInteger", rm->FadeInEndInteger, false,
                "Normalized lifetime (0-1000) where cosine fade-in finishes.");
            edited |= DrawU32("FadeOutBeginInteger", rm->FadeOutBeginInteger, false,
                "Normalized lifetime (0-1000) where cosine fade-out begins.");
            edited |= DrawBool("AlphaFadeEnable", rm->AlphaFadeEnable,
                "Enables smooth cosine fade-in/out on mesh alpha over lifetime."); ImGui::SameLine();
            edited |= DrawBool("SizeFadeEnable", rm->SizeFadeEnable,
                "Enables smooth cosine fade-in/out multiplier on mesh scale over lifetime.");
            edited |= DrawFloat("AlphaFadeMinimum", rm->AlphaFadeMinimum, 0.01f, 0.0f, 0.0f,
                "Minimum alpha multiplier (0.0-1.0) at the ends of the fade curve.");
            edited |= DrawFloat("SizeFadeMinimum", rm->SizeFadeMinimum, 0.01f, 0.0f, 0.0f,
                "Minimum size multiplier (0.0-1.0) at the ends of the fade curve.");

            edited |= DrawBool("FlickerEnable", rm->FlickerEnable,
                "Modulates mesh alpha and scale using 2D Perlin noise over time.");
            edited |= DrawU32("FlickerMinAlphaInteger", rm->FlickerMinAlphaInteger, false,
                "Minimum alpha multiplier during flicker in thousandths (0-1000).");
            edited |= DrawU32("FlickerMinSizeInteger", rm->FlickerMinSizeInteger, false,
                "Minimum scale multiplier during flicker in thousandths (0-1000).");
            edited |= DrawFloat("FlickerBias", rm->FlickerBias, 0.01f, 0.0f, 0.0f,
                "Flicker bias (-1.0 to +1.0) shifting noise toward dimmer/smaller (-) or brighter/larger (+) values.");
            edited |= DrawFloat("FlickerSpeed", rm->FlickerSpeed, 0.1f, 0.0f, 0.0f,
                "Frequency multiplier controlling how fast Perlin noise flicker fluctuates.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Trail Specs");
            ImGui::Separator();
            edited |= DrawU32("TrailLengthInt", rm->TrailLengthInteger, false,
                "Trail delay/length in thousandths of a tick-history step (0 disables trails).");
            edited |= DrawFloat("TrailWidth", rm->TrailWidth, 0.1f, 0.0f, 0.0f,
                "World-space ribbon width (in meters) of the trail behind each mesh particle.");
        }
        else if (auto* lgt = dynamic_cast<CPSCLight*>(comp.get())) {
            edited |= DrawHashParam("LightPositionParam", lgt->LightPositionParam,
                "CRC32 hash of a runtime position source (e.g. \"System Offset\", .Centre, orbit .Primary/.Secondary/.Tertiary, or spline).");
            if (DrawFloat("LightLifeSecs", lgt->LightLifeSecs, 0.1f, 0.0f, 0.0f,
                "Active duration (in seconds) of the point light before extinguishing or respawning (when LightUseLifeSecs is enabled).")) { edited = true; structural = true; }
            if (DrawFloat("RespawnDelaySecs", lgt->LightRespawnDelaySecs, 0.1f, 0.0f, 0.0f,
                "Wait time (in seconds) before the light turns back on when LightRespawns is enabled.")) { edited = true; structural = true; }
            if (DrawFloat("LightStartTime", lgt->LightStartTime, 0.1f, 0.0f, 0.0f,
                "Initial delay (in seconds) after system start before the light activates.")) { edited = true; structural = true; }
            if (DrawFloat("LightTimelineSecs", lgt->LightTimelineSecs, 0.1f, 0.0f, 0.0f,
                "Loop period (in seconds) for wrapping light timing when LightUseTimelineSecs is enabled.")) { edited = true; structural = true; }
            edited |= DrawFloat("StartRenderWorldRadius", lgt->LightStartRenderWorldRadius, 0.1f, 0.0f, 0.0f,
                "Light illumination radius (in meters) at the start of its lifecycle.");
            edited |= DrawFloat("EndRenderWorldRadius", lgt->LightEndRenderWorldRadius, 0.1f, 0.0f, 0.0f,
                "Light illumination radius (in meters) at the end of its lifecycle.");
            edited |= DrawInt("LightAttenuationFactor", lgt->LightAttenuationFactor,
                "Falloff factor controlling how quickly light intensity attenuates with distance from the center.");
            edited |= DrawInt("FadeInEndInteger", lgt->LightFadeInEndInteger,
                "Normalized light lifetime (0-1000) where radius/color fade-in completes.");
            edited |= DrawInt("FadeOutBeginInteger", lgt->LightFadeOutBeginInteger,
                "Normalized light lifetime (0-1000) where radius/color fade-out begins.");
            edited |= DrawFloat("WorldRadiusFadeMinimum", lgt->LightWorldRadiusFadeMinimum, 0.01f, 0.0f, 0.0f,
                "Minimum radius multiplier (0.0-1.0) at the ends of the light radius fade curve.");
            edited |= DrawColour("ColourFadeMinimum", lgt->LightColourFadeMinimum,
                "Minimum RGBA tint/intensity color used at the ends of the color fade curve when LightUseFadeColour is enabled.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Flags");
            ImGui::Separator();
            if (DrawBool("LightEnabled", lgt->LightEnabled,
                "Enables or disables this dynamic point light.")) { edited = true; structural = true; }
            if (DrawBool("LightRespawns", lgt->LightRespawns,
                "Automatically reactivates the light after RespawnDelaySecs once LightLifeSecs ends.")) { edited = true; structural = true; }
            edited |= DrawBool("LightWorldRadiusFadeEnable", lgt->LightWorldRadiusFadeEnable,
                "Applies cosine fade-in/out to the light's world radius over its lifetime.");
            edited |= DrawBool("LightColourFadeEnable", lgt->LightColourFadeEnable,
                "Applies cosine fade-in/out to the light's color/intensity over its lifetime.");
            if (DrawBool("LightUseLifeSecs", lgt->LightUseLifeSecs,
                "Limits how long the light stays lit to LightLifeSecs (unchecked keeps it lit continuously).")) { edited = true; structural = true; }
            if (DrawBool("LightUseTimelineSecs", lgt->LightUseTimelineSecs,
                "Wraps the light's clock modulo LightTimelineSecs for periodic pulses.")) { edited = true; structural = true; }
            edited |= DrawBool("LightUseStartColour", lgt->LightUseStartColour,
                "Includes LightStartColour at t = 0.0 in the light color spline.");
            edited |= DrawBool("LightUseMidColour", lgt->LightUseMidColour,
                "Includes LightMidColour at t = 0.5 in the light color spline.");
            edited |= DrawBool("LightUseEndColour", lgt->LightUseEndColour,
                "Includes LightEndColour at t = 1.0 in the light color spline.");
            edited |= DrawBool("LightUseFadeColour", lgt->LightUseFadeColour,
                "Fades toward ColourFadeMinimum at the ends of the fade curve instead of fading to zero intensity.");
            edited |= DrawBool("LightHasInitializedTime", lgt->LightHasInitializedTime,
                "Internal/serialized state flag indicating whether initial light timing has been set.");

            ImGui::Dummy(ImVec2(0, 4));
            edited |= DrawColour("LightStartColour", lgt->LightStartColour,
                "Point light RGB color and intensity (alpha) at start (t = 0.0).");
            edited |= DrawColour("LightMidColour", lgt->LightMidColour,
                "Point light RGB color and intensity (alpha) at mid-life (t = 0.5).");
            edited |= DrawColour("LightEndColour", lgt->LightEndColour,
                "Point light RGB color and intensity (alpha) at end (t = 1.0).");
        }
        else if (auto* sp = dynamic_cast<CPSCSpline*>(comp.get())) {
            if (DrawBool("SplineBounce", sp->SplineBounce,
                "Ping-pongs back and forth along the spline instead of wrapping from end to start.")) { edited = true; structural = true; }
            if (DrawBool("ScaleSplineSpeed", sp->ScaleSplineSpeed,
                "Normalizes traversal speed by total spline arc length so movement occurs in world units/sec.")) { edited = true; structural = true; }
            if (DrawFloat("SplineAnimSpeed", sp->SplineAnimSpeed, 0.1f, 0.0f, 0.0f,
                "Speed along the spline (normalized progress/sec, or world units/sec when ScaleSplineSpeed is checked).")) { edited = true; structural = true; }
            if (DrawSplineBase(sp)) { edited = true; structural = true; }
        }
        else if (auto* ss = dynamic_cast<CPSCSingleSprite*>(comp.get())) {
            if (DrawParticleTexRow("SpriteBankIndex", ss->SpriteBankIndex,
                "Texture ID for this persistent single sprite (0 disables sprite rendering; never falls back to glow).")) { edited = true; structural = true; }
            if (DrawFloat("AnimTimeSecs", ss->AnimationTimeSecs, 0.1f, 0.0f, 0.0f,
                "Duration (in seconds) of the single sprite's color, size, fade, and sprite-sheet animation cycle.")) { edited = true; structural = true; }
            if (DrawBool("ForceAnimationTime", ss->ForceAnimationTime,
                "Drives sprite-sheet frame animation independently using AnimTimeSecs.")) { edited = true; structural = true; }
            if (DrawFloat("InitialAngle", ss->InitialAngle, 0.05f, 0.0f, 0.0f,
                "Fixed/initial roll angle (in radians) of the single sprite quad.")) { edited = true; structural = true; }

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Colours & Fades");
            ImGui::Separator();
            edited |= DrawColour("StartColour", ss->StartColour,
                "Single sprite RGBA color at cycle start (t = 0.0).");
            edited |= DrawColour("MidColour", ss->MidColour,
                "Single sprite RGBA color at mid-cycle (t = 0.5).");
            edited |= DrawColour("EndColour", ss->EndColour,
                "Single sprite RGBA color at cycle end (t = 1.0).");
            edited |= DrawBool("UseStartColour", ss->UseStartColour,
                "Includes StartColour at t = 0.0 in the color spline."); ImGui::SameLine();
            edited |= DrawBool("UseMidColour", ss->UseMidColour,
                "Includes MidColour at t = 0.5 in the color spline."); ImGui::SameLine();
            edited |= DrawBool("UseEndColour", ss->UseEndColour,
                "Includes EndColour at t = 1.0 in the color spline.");

            edited |= DrawFloat("StartRenderSize", ss->StartRenderSize, 0.1f, 0.0f, 0.0f,
                "Sprite size (in meters) at cycle start (t = 0.0).");
            edited |= DrawFloat("EndRenderSize", ss->EndRenderSize, 0.1f, 0.0f, 0.0f,
                "Sprite size (in meters) at cycle end (t = 1.0).");
            edited |= DrawBool("AlphaFadeEnable", ss->AlphaFadeEnable,
                "Enables cosine alpha fade-in/out over the animation cycle."); ImGui::SameLine();
            edited |= DrawBool("SizeFadeEnable", ss->SizeFadeEnable,
                "Enables cosine size fade-in/out multiplier over the animation cycle.");
            edited |= DrawInt("FadeInEndInteger", ss->FadeInEndInteger,
                "Normalized cycle time (0-1000) where cosine fade-in completes.");
            edited |= DrawInt("FadeOutBeginInteger", ss->FadeOutBeginInteger,
                "Normalized cycle time (0-1000) where cosine fade-out begins.");
            edited |= DrawFloat("AlphaFadeMinimum", ss->AlphaFadeMinimum, 0.01f, 0.0f, 1.0f,
                "Minimum alpha multiplier (0.0-1.0) at the ends of the fade curve.");
            edited |= DrawFloat("SizeFadeMinimum", ss->SizeFadeMinimum, 0.01f, 0.0f, 0.0f,
                "Minimum size multiplier (0.0-1.0) at the ends of the fade curve.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Render & Blend Settings");
            ImGui::Separator();
            edited |= DrawInt("BlendMode", ss->BlendMode,
                "Sprite blending mode: 0 = Solid, 1 = Boolean Alpha, 2 = Alpha, 3 = Additive, 4 = AddSmooth, 5 = ConstColour.");
            edited |= DrawInt("BlendOp", ss->BlendOp,
                "Hardware blend operation for the sprite: 0 = Add, 1 = Subtract, 2 = Reverse Subtract.");
            edited |= DrawInt("SpriteAlignment", ss->SpriteAlignment,
                "Anchor/alignment bitfield (low 2 bits sync with SpriteFlags 0x002/0x004 for anchor offset and 360-degree crossed arc).");

            uint32_t ssFlags = 0;
            if (ss->FaceMe3D) ssFlags |= 0x001u;
            if (ss->SpriteAlignment & 1) ssFlags |= 0x002u;
            if (ss->SpriteAlignment & 2) ssFlags |= 0x004u;
            if (ss->RotateAroundCentre) ssFlags |= 0x020u;
            if (ss->CrossedSprites) ssFlags |= 0x040u;
            if (!ss->SelfIlluminating) ssFlags |= 0x080u;
            if (DrawSpriteFlagsEditor(ssFlags)) {
                ss->FaceMe3D = (ssFlags & 0x001u) != 0;
                ss->SpriteAlignment = (ss->SpriteAlignment & ~3) | (int32_t)((ssFlags >> 1) & 3u);
                ss->RotateAroundCentre = (ssFlags & 0x020u) != 0;
                ss->CrossedSprites = (ssFlags & 0x040u) != 0;
                ss->SelfIlluminating = (ssFlags & 0x080u) == 0;
                if (ss->CrossedSprites) {
                    ss->NoCrossedSprites = std::clamp(ss->NoCrossedSprites, 1, 31);
                }
                edited = true;
            }
            edited |= DrawBool("FaceMe2D", ss->FaceMe2D,
                "Forces cylindrical/screen-aligned 2D billboard facing rather than 3D spherical facing.");
            edited |= DrawNoCrossedSpritesSlider("NoCrossedSprites", ss->NoCrossedSprites,
                "Number of intersecting vertical quads emitted when CrossedSprites is enabled (1-31).");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Position & Attachment");
            ImGui::Separator();
            if (DrawBool("StayWithEmitter", ss->StayWithEmitter,
                "Locks the single sprite to follow emitter origin movement.")) { edited = true; structural = true; }
            if (DrawInt("StayWithEmitterIntFactor", ss->StayWithEmitterIntFactor,
                "Follow strength in thousandths (1000 = 100% locked to emitter movement).")) { edited = true; structural = true; }
            if (DrawBool("UsePosition", ss->UsePosition,
                "Positions the single sprite from PositionParam instead of ControlPoints[0].")) { edited = true; structural = true; }
            ImGui::SameLine();
            if (DrawBool("UseSplinePoints", ss->UseSplinePoints,
                "Uses raw spline ControlPoints directly (instead of Catmull-Rom interpolation) for trail/path geometry.")) { edited = true; structural = true; }
            edited |= DrawHashParam("PositionParam", ss->PositionParam,
                "CRC32 hash of a runtime position source (e.g. \"System Offset\", .Centre, orbit .Primary/.Secondary/.Tertiary) used when UsePosition is checked.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Trail Settings");
            ImGui::Separator();
            if (DrawParticleTexRow("TrailBankIndex", ss->TrailBankIndex,
                "Texture ID for the ribbon trail along the single sprite spline/history (0 disables trail).")) { edited = true; structural = true; }
            edited |= DrawInt("TrailBlendMode", ss->TrailBlendMode,
                "Trail blending mode: 0 = Solid, 1 = Boolean Alpha, 2 = Alpha, 3 = Additive, 4 = AddSmooth, 5 = ConstColour.");
            edited |= DrawInt("TrailBlendOp", ss->TrailBlendOp,
                "Hardware blend operation for the trail: 0 = Add, 1 = Subtract, 2 = Reverse Subtract.");
            edited |= DrawFloat("TrailWidth", ss->TrailWidth, 0.1f, 0.0f, 0.0f,
                "World-space ribbon width (in meters) of the single sprite trail.");
            edited |= DrawInt("MaxTrailLength", ss->MaxTrailLength,
                "Maximum trail length/delay factor in thousandths (must be > 0 to render the trail).");

            if (DrawSplineBase(ss)) { edited = true; structural = true; }
        }
        else if (auto* at = dynamic_cast<CPSCAttractor*>(comp.get())) {
            if (DrawBool("AttractorEnabled", at->AttractorEnabled,
                "Enables or disables this attractor's force field.")) { edited = true; structural = true; }
            if (DrawBool("AttractorUseParamPosition", at->AttractorUseParamPosition,
                "Positions the attractor from AttractorPositionParam instead of static User Points.")) { edited = true; structural = true; }
            edited |= DrawHashParam("AttractorPositionParam", at->AttractorPositionParam,
                "CRC32 hash of the runtime position parameter used when AttractorUseParamPosition is checked.");
            edited |= DrawHashParam("AttractorPositionParamName", at->AttractorPositionParamName,
                "CRC32 hash of the named parameter binding paired with AttractorPositionParam.");
            if (DrawInt("AttractorInfluenceFallOff", at->AttractorInfluenceFallOff,
                "Distance falloff exponent: values > 5 use (FallOff - 5); values <= 5 use (FallOff / 5.0), where 5 = linear and 0 = uniform force inside radius.")) { edited = true; structural = true; }
            if (DrawFloat("AttractorInfluenceRadius", at->AttractorInfluenceRadius, 0.1f, 0.0f, 0.0f,
                "Maximum influence radius (in meters) around each attractor point.")) { edited = true; structural = true; }
            if (DrawFloat("AttractorInfluenceForce", at->AttractorInfluenceForce, 0.1f, 0.0f, 0.0f,
                "Acceleration strength applied to particles inside the radius (positive pulls inward, negative repels outward).")) { edited = true; structural = true; }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("User Points: %d", (int)at->AttractorUserPointsArray.size());
            PropTooltip("Local (X, Y, Z) attractor target points (if empty, attracts toward system origin (0, 0, 0)).");
            ImGui::SameLine();
            if (DrawAddIconButton("##AddAttractorPt", 16.0f, "Add Attractor Point")) {
                at->AttractorUserPointsArray.push_back(C3DVector{ 0.0f, 0.0f, 0.0f });
                edited = true;
                structural = true;
            }
            ImGui::PushID("AttractorUserPoints");
            for (size_t i = 0; i < at->AttractorUserPointsArray.size(); ) {
                ImGui::PushID((int)i);
                if (DrawVector3(("Point " + std::to_string(i)).c_str(), at->AttractorUserPointsArray[i],
                    "Local (X, Y, Z) coordinates of this attractor point.")) {
                    edited = true;
                    structural = true;
                }
                ImGui::SameLine();
                bool del = DrawDeleteIconButton("##DelAttractorPt", 16.0f, "Delete Attractor Point");
                ImGui::PopID();
                if (del) {
                    at->AttractorUserPointsArray.erase(at->AttractorUserPointsArray.begin() + i);
                    edited = true;
                    structural = true;
                }
                else {
                    ++i;
                }
            }
            ImGui::PopID();
        }
        else if (auto* ob = dynamic_cast<CPSCOrbit*>(comp.get())) {
            if (ob->OrbitsData.size() < 3) {
                ob->OrbitsData.resize(3);
                edited = true;
                structural = true;
            }
            if (DrawHashParam("CentreParam", ob->CentreParam,
                "CRC32 hash of the runtime center position parameter around which the orbit stages revolve.")) { edited = true; structural = true; }
            ImGui::Text("Orbits Count: %d", (int)ob->OrbitsData.size());
            PropTooltip("Fixed 3-stage orbit hierarchy (Primary, Secondary, Tertiary).");

            ImGui::PushID("OrbitStages");
            for (size_t i = 0; i < ob->OrbitsData.size(); ++i) {
                auto& orbit = ob->OrbitsData[i];
                ImGui::PushID((int)i);
                ImGui::SetNextItemAllowOverlap();
                bool stageOpen = ImGui::TreeNodeEx((void*)(intptr_t)i, ImGuiTreeNodeFlags_AllowOverlap, "Orbit %d%s", (int)i, orbit.Enabled ? " [Enabled]" : "");
                PropTooltip("Orbit hierarchy stage (0 = Primary, 1 = Secondary, 2 = Tertiary).");
                ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 26);
                if (DrawDeleteIconButton("##ResetOrbitStage", 16.0f, "Reset Orbit Stage to Default")) {
                    orbit = COrbitData{};
                    edited = true;
                    structural = true;
                }

                if (stageOpen) {
                    if (DrawOrbitData(orbit)) {
                        edited = true;
                        structural = true;
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::PopID();
        }
        else if (auto* dr = dynamic_cast<CPSCDecalRenderer*>(comp.get())) {
            if (DrawParticleTexRow("DecalBankIndex", dr->DecalBankIndex,
                "Texture ID from the open Texture/XboxGraphics bank projected onto the ground plane (Z = 0).")) { edited = true; structural = true; }
            if (DrawFloat("DecalLifeSecs", dr->DecalLifeSecs, 0.1f, 0.0f, 0.0f,
                "Lifetime (in seconds) of each spawned ground decal before expiring.")) { edited = true; structural = true; }
            edited |= DrawColour("StartColour", dr->StartColour,
                "Decal RGBA tint color at spawn (t = 0.0).");
            edited |= DrawColour("MidColour", dr->MidColour,
                "Decal RGBA tint color at mid-life (t = 0.5).");
            edited |= DrawColour("EndColour", dr->EndColour,
                "Decal RGBA tint color at expiry (t = 1.0).");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Render Settings");
            ImGui::Separator();
            edited |= DrawU32("BlendMode", dr->BlendMode, false,
                "Decal blending mode: 0 = Solid, 1 = Boolean Alpha, 2 = Alpha, 3 = Additive, 4 = AddSmooth, 5 = ConstColour.");
            edited |= DrawU32("BlendOp", dr->BlendOp, false,
                "Hardware blend operation for decals: 0 = Add, 1 = Subtract, 2 = Reverse Subtract.");
            edited |= DrawU32("SpriteAlignment", dr->SpriteAlignment, false,
                "Decal orientation/alignment flags.");
            edited |= DrawU32("FadeInEndInteger", dr->FadeInEndInteger, false,
                "Normalized decal lifetime (0-1000) where cosine fade-in completes.");
            edited |= DrawU32("FadeOutBeginInteger", dr->FadeOutBeginInteger, false,
                "Normalized decal lifetime (0-1000) where cosine fade-out begins.");
            edited |= DrawU32("StartRenderSizeInteger", dr->StartRenderSizeInteger, false,
                "Quantized decal start size (raw / 2047 * 20.0 meters; 102 ~= 1.0m).");
            edited |= DrawU32("EndRenderSizeInteger", dr->EndRenderSizeInteger, false,
                "Quantized decal end size (raw / 2047 * 20.0 meters; 102 ~= 1.0m).");
            edited |= DrawU32("AlphaFadeMinimumInteger", dr->AlphaFadeMinimumInteger, false,
                "Quantized minimum alpha multiplier (raw / 127.0; 127 = 1.0).");
            edited |= DrawU32("SizeFadeMinimumInteger", dr->SizeFadeMinimumInteger, false,
                "Quantized minimum size multiplier (raw / 127.0; 127 = 1.0).");
            edited |= DrawU32("AnimationTimeSecsInt", dr->AnimationTimeSecsInteger, false,
                "Quantized sprite-sheet animation cycle period (raw / 16383 * 99.9 + 0.1 seconds).");

            edited |= DrawBool("UseStartColour", dr->UseStartColour,
                "Includes StartColour at t = 0.0 in the decal color spline."); ImGui::SameLine();
            edited |= DrawBool("UseMidColour", dr->UseMidColour,
                "Includes MidColour at t = 0.5 in the decal color spline."); ImGui::SameLine();
            edited |= DrawBool("UseEndColour", dr->UseEndColour,
                "Includes EndColour at t = 1.0 in the decal color spline.");
            edited |= DrawBool("AlphaFadeEnable", dr->AlphaFadeEnable,
                "Enables cosine fade-in/out on decal alpha over DecalLifeSecs."); ImGui::SameLine();
            edited |= DrawBool("SizeFadeEnable", dr->SizeFadeEnable,
                "Enables cosine fade-in/out multiplier on decal size over DecalLifeSecs."); ImGui::SameLine();
            edited |= DrawBool("ForceAnimationTime", dr->ForceAnimationTime,
                "Loops decal sprite-sheet frames using AnimationTimeSecsInt instead of matching DecalLifeSecs.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Flicker");
            ImGui::Separator();
            edited |= DrawBool("FlickerEnable", dr->FlickerEnable,
                "Modulates decal alpha and size with 2D Perlin noise over time.");
            edited |= DrawU32("FlickerMinAlphaInteger", dr->FlickerMinAlphaInteger, false,
                "Minimum alpha multiplier during flicker in thousandths (0-1000).");
            edited |= DrawU32("FlickerMinSizeInteger", dr->FlickerMinSizeInteger, false,
                "Minimum size multiplier during flicker in thousandths (0-1000).");
            edited |= DrawU32("FlickerBiasInteger", dr->FlickerBiasInteger, false,
                "Quantized flicker bias (raw / 255 * 2.0 - 1.0; 128 ~= 0.0).");
            edited |= DrawU32("FlickerSpeedInteger", dr->FlickerSpeedInteger, false,
                "Quantized flicker speed (raw / 4095 * 30.0 Hz; 137 ~= 1.0 Hz).");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextColored(kGoldAccent, "Pooling");
            ImGui::Separator();
            if (DrawBool("PoolingEnable", dr->PoolingEnable,
                "Merges nearby ground decals (within StencilCubeWidth * 0.5) into a single accumulating pool instead of spawning duplicates.")) { edited = true; structural = true; }
            edited |= DrawBool("PoolIncreaseAlphaEnable", dr->PoolIncreaseAlphaEnable,
                "Increases an existing pooled decal's alpha multiplier when another hit lands inside it.");
            edited |= DrawBool("PoolIncreaseSizeEnable", dr->PoolIncreaseSizeEnable,
                "Grows an existing pooled decal's size multiplier when another hit lands inside it.");
            edited |= DrawBool("PoolIncreaseLifeEnable", dr->PoolIncreaseLifeEnable,
                "Extends an existing pooled decal's lifetime when new hits accumulate in the pool.");
            edited |= DrawBool("PoolIncreaseFrameEnable", dr->PoolIncreaseFrameEnable,
                "Advances a pooled decal's sprite-sheet frame by PoolFrameIncreaseRate on each hit.");
            edited |= DrawFloat("MaxPoolSize", dr->MaxPoolSize, 0.1f, 0.0f, 0.0f,
                "Maximum size multiplier cap for pooled decals when PoolIncreaseSizeEnable is enabled.");
            edited |= DrawFloat("MaxPoolAlpha", dr->MaxPoolAlpha, 0.01f, 0.0f, 0.0f,
                "Maximum alpha multiplier cap for pooled decals when PoolIncreaseAlphaEnable is enabled.");
            if (DrawFloat("MaxPoolLife", dr->MaxPoolLife, 0.1f, 0.0f, 0.0f,
                "Maximum lifetime cap (in seconds) for pooled decals when PoolIncreaseLifeEnable is enabled.")) { edited = true; structural = true; }
            edited |= DrawFloat("PoolFrameIncreaseRate", dr->PoolFrameIncreaseRate, 0.1f, 0.0f, 0.0f,
                "Frame offset increment applied per pooled hit when PoolIncreaseFrameEnable is enabled.");
            edited |= DrawFloat("StencilCubeWidth", dr->StencilCubeWidth, 0.1f, 0.0f, 0.0f,
                "World-space width of the decal projection box and pooling merge radius (width * 0.5).");
        }

        ImGui::Spacing();
        ImGui::Unindent(8.0f);
        ImGui::TreePop();
    }
    ImGui::PopID();
    if (outStructuralEdited && structural) {
        *outStructuralEdited = true;
    }
    return edited;
}

inline void DrawParticleEmitter(CParticleEmitter& emitter, bool& anyEdited, bool& structuralEdited) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Emitter Name:");
    PropTooltip("Name identifier of this particle emitter asset.");
    ImGui::SameLine(110.0f);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    anyEdited |= DrawString("##EmitterNameEdit", emitter.Name,
        "Name identifier of this particle emitter asset.");

    ImGui::Dummy(ImVec2(0, 3));

    // Base Properties - starts COLLAPSED
    bool baseOpen = ImGui::CollapsingHeader("Emitter Base Properties", ImGuiTreeNodeFlags_None);
    PropTooltip("Emitter-level sorting priority, camera distance culling/fades, and engine render-pass flags.");
    if (baseOpen) {
        anyEdited |= DrawInt("Priority", emitter.Priority,
            "Render sort priority relative to other active particle emitters.");
        if (DrawBool("ContinuousEmitter", emitter.ContinuousEmitter,
            "Keeps the emitter running continuously in the world rather than playing once and stopping.")) {
            anyEdited = true;
            structuralEdited = true;
        }
        anyEdited |= DrawFloat("MaxSpawnDistance", emitter.MaxSpawnDistance, 1.0f, 0.0f, 0.0f,
            "Maximum camera distance (in meters) within which new particles are spawned.");
        anyEdited |= DrawFloat("MaxDrawDistance", emitter.MaxDrawDistance, 1.0f, 0.0f, 0.0f,
            "Maximum camera distance (in meters) before the emitter is culled from rendering.");
        anyEdited |= DrawFloat("FadeOutStart", emitter.FadeOutStart, 1.0f, 0.0f, 0.0f,
            "Camera distance (in meters) where far-distance alpha fade-out begins toward MaxDrawDistance.");
        anyEdited |= DrawFloat("FadeInEnd", emitter.FadeInEnd, 1.0f, 0.0f, 0.0f,
            "Near-camera distance (in meters) where near fade-in reaches full opacity (if > 0).");
        anyEdited |= DrawFloat("FadeInStart", emitter.FadeInStart, 1.0f, 0.0f, 0.0f,
            "Near-camera distance (in meters) where near fade-in starts from zero opacity.");

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::TextColored(kGoldAccent, "Engine Flags");
        ImGui::Separator();
        anyEdited |= DrawBool("Emitter2D", emitter.Emitter2D,
            "Marks this emitter as a 2D/screen-space effect instead of a 3D world-space effect."); ImGui::SameLine();
        anyEdited |= DrawBool("PreWaterEffect", emitter.PreWaterEffect,
            "Renders in the pre-water pass before water surfaces so water refracts/occludes it."); ImGui::SameLine();
        anyEdited |= DrawBool("WaterEffect", emitter.WaterEffect,
            "Renders as part of the water effect pass.");

        anyEdited |= DrawBool("ZBufferWriteable", emitter.ZBufferWriteable,
            "Writes particle depth to the Z-buffer (normally disabled for translucent/additive sprites)."); ImGui::SameLine();
        anyEdited |= DrawBool("ReadZBuffer", emitter.ReadZBuffer,
            "Enables depth testing against the scene Z-buffer so solid world geometry occludes particles."); ImGui::SameLine();
        anyEdited |= DrawBool("IsScreenDisplacement", emitter.IsScreenDisplacement,
            "Draws into the screen-space refraction/distortion buffer (heat haze, shockwaves) rather than normal color.");

        anyEdited |= DrawBool("DieIfOffscreen", emitter.DieIfOffscreen,
            "Stops/destroys the emitter if its bounds leave the camera view frustum."); ImGui::SameLine();
        anyEdited |= DrawBool("OffscreenUpdate", emitter.OffscreenUpdate,
            "Continues ticking particle simulation even while the emitter is outside the camera frustum."); ImGui::SameLine();
        anyEdited |= DrawBool("ClipEffectToWeatherMask", emitter.ClipEffectToWeatherMask,
            "Clips weather particles against the engine's interior/shelter weather mask.");

        anyEdited |= DrawBool("EnableDithering", emitter.EnableDithering,
            "Enables dithered alpha/transparency falloff on this emitter's primitives."); ImGui::SameLine();
        anyEdited |= DrawBool("CalcBoundingSphereOnceOnly", emitter.CalcBoundingSphereOnceOnly,
            "Computes the culling bounding sphere once at startup instead of updating it every frame.");
    }
}

inline bool DrawParticleSystem(CParticleSystem& sys, bool& structuralEdited) {
    bool edited = false;

    ImGui::TextColored(kGoldAccent, "System Configuration");
    ImGui::Separator();

    edited |= DrawString("System Name", sys.Name,
        "Display name of this particle system layer within the emitter.");
    if (DrawBool("Enabled", sys.Enabled,
        "Toggles simulation and rendering for this particle system and all of its components.")) {
        edited = true;
        structuralEdited = true;
    }
    ImGui::SameLine();
    if (DrawBool("Scale Particles", sys.ScaleParticles,
        "When checked, multiplies rendered sprite/mesh sizes and trail widths by the system's Scale vector.")) {
        edited = true;
        structuralEdited = true;
    }
    if (DrawVector3("Scale", sys.Scale,
        "Per-axis (X, Y, Z) scale multiplier applied to spawn volume and (if Scale Particles is checked) rendered sizes.")) {
        edited = true;
        structuralEdited = true;
    }

    return edited;
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

    bool anyEdited = false;
    bool structuralEdited = false;

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
        PropTooltip(g_ShowParticleRenderer ? "Collapse Preview Viewport" : "Expand Preview Viewport");

        ImGui::Separator();

        DrawParticleEmitter(emitter, anyEdited, structuralEdited);

        ImGui::Dummy(ImVec2(0, 8));

        // Particle Systems Section Header with [+] button
        ImGui::TextColored(kGoldAccent, "Particle Systems (%d)", (int)emitter.Systems.size());
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 26);

        if (DrawAddIconButton("##AddSysTop", 18.0f, "Add New Particle System")) {
            CParticleSystem newSys;
            newSys.Name = "System_" + std::to_string(emitter.Systems.size() + 1);
            emitter.Systems.push_back(newSys);
            anyEdited = true;
            structuralEdited = true;
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
                anyEdited = true;
                structuralEdited = true;
            }

            if (isSysOpen) {
                ImGui::Indent(12.0f);
                ImGui::Spacing();

                anyEdited |= DrawParticleSystem(sys, structuralEdited);

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
                    bool compStruct = false;
                    anyEdited |= DrawParticleComponent(sys.Components[c], deleteComp, c, &compStruct);
                    structuralEdited |= compStruct;
                    if (deleteComp) {
                        sys.Components.erase(sys.Components.begin() + c);
                        anyEdited = true;
                        structuralEdited = true;
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
            const ParticleEditSync::EmitterFingerprint fp = ParticleEditSync::ComputeFingerprint(emitter);

            // Simulation lifecycle and live property sync
            if (&emitter != g_LastSimulatedEmitter || g_ParticleSimNeedsReset || emitter.Systems.size() != g_ParticleSimulator.SystemStates.size()) {
                g_ParticleSimTime = 0.0f;
                g_ParticleSimulator.Reset(emitter);
                g_LastSimulatedEmitter = &emitter;
                g_ParticleSimNeedsReset = false;
                g_LastEmitterFingerprint = fp.Full;
                g_LastStructuralFingerprint = fp.Structural;
                g_HasEmitterFingerprint = true;
            }
            else {
                const bool fullChanged = !g_HasEmitterFingerprint || anyEdited || (fp.Full != g_LastEmitterFingerprint);
                const bool structChanged = !g_HasEmitterFingerprint || structuralEdited || (fp.Structural != g_LastStructuralFingerprint);

                if (!g_ParticlePlaying) {
                    if (fullChanged) {
                        g_ParticleSimulator.Seek(emitter, g_ParticleSimulator.CurrentSimTime);
                    }
                }
                else {
                    if (structChanged) {
                        g_ParticleSimulator.Seek(emitter, g_ParticleSimulator.CurrentSimTime);
                    }
                }

                g_LastEmitterFingerprint = fp.Full;
                g_LastStructuralFingerprint = fp.Structural;
                g_HasEmitterFingerprint = true;
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

            // Controls in top-left
            ImGui::SetCursorScreenPos(ImVec2(pMin.x + 8, pMin.y + 6));
            const bool gridOn = g_ParticleRenderer.ShowGrid;
            if (gridOn) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.32f, 0.28f, 0.14f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.39f, 0.20f, 0.95f));
                ImGui::PushStyleColor(ImGuiCol_Text, kGoldAccent);
            }
            if (ImGui::SmallButton(gridOn ? "Grid: ON" : "Grid: OFF")) {
                bool next = !g_ParticleRenderer.ShowGrid;
                g_ParticleRenderer.ShowGrid = next;
                g_ParticleRenderer.ShowAxes = next;
            }
            if (gridOn) {
                ImGui::PopStyleColor(3);
            }
            PropTooltip("Toggle ground reference grid and world axes (X=Red, Y=Green, Z=Blue).");

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
        PropTooltip("Select a Fable particle component type to add to the selected system.");

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

        ImGui::BeginChild("TexList", ImVec2(420, 300), true);
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
        ImGui::BeginChild("TexPreview", ImVec2(308, 350), true);
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

        ImGui::BeginChild("MeshList", ImVec2(420, 400), true);
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
        ImGui::BeginChild("MeshPreviewView", ImVec2(480, 400), true);

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