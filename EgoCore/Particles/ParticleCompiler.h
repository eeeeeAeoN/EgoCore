#pragma once
#include "ParticleParser.h"
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>


class CParticleWriteStream {
public:
    std::vector<uint8_t> Data;

    void WriteULONG(uint32_t val) {
        uint8_t bytes[4];
        std::memcpy(bytes, &val, 4);
        Data.insert(Data.end(), bytes, bytes + 4);
    }

    void WriteSLONG(int32_t val) {
        WriteULONG((uint32_t)val);
    }

    void WriteFloat(float val) {
        uint8_t bytes[4];
        std::memcpy(bytes, &val, 4);
        Data.insert(Data.end(), bytes, bytes + 4);
    }

    void WriteEBOOL(bool val) {
        Data.push_back(val ? 1 : 0);
    }

    void WriteColour(const CRGBColour& c) {
        Data.push_back(c.B);
        Data.push_back(c.G);
        Data.push_back(c.R);
        Data.push_back(c.A);
    }

    void Write3DVector(const C3DVector& v) {
        WriteFloat(v.X);
        WriteFloat(v.Y);
        WriteFloat(v.Z);
    }

    void WriteString(const std::string& str) {
        for (char c : str) {
            Data.push_back((uint8_t)c);
        }
        Data.push_back(0);
    }
};

class ParticleCompiler {
private:
    static uint32_t EncodeFloat(float val, float maxFloat, float maxQuant) {
        float packed = (val / maxFloat) * maxQuant;
        return (uint32_t)std::round((std::max)(0.0f, (std::min)(packed, maxQuant)));
    }

    static uint32_t EncodeBiasFloat(float val, float bias, float maxFloat, float maxQuant) {
        float packed = ((val + bias) / maxFloat) * maxQuant;
        return (uint32_t)std::round((std::max)(0.0f, (std::min)(packed, maxQuant)));
    }

    static void CompileSplineBase(CParticleWriteStream& stream, const CPSCSplineBase* spline) {
        stream.WriteULONG((uint32_t)spline->ControlPoints.size());
        for (const auto& pt : spline->ControlPoints) {
            stream.Write3DVector(pt);
        }

        stream.WriteEBOOL(spline->HasRandomOffsetData);
        if (spline->HasRandomOffsetData) {
            stream.WriteEBOOL(spline->OffsetOnlyAtStartup);
            stream.WriteEBOOL(spline->ResetEachFrame);
            stream.WriteULONG((uint32_t)spline->RandomOffsets.size());
            for (const auto& off : spline->RandomOffsets) {
                stream.Write3DVector(off.Limit);
                stream.Write3DVector(off.Speed);
            }
        }

        stream.WriteEBOOL(spline->HasPosParamData);
        if (spline->HasPosParamData) {
            stream.WriteEBOOL(spline->MaintainSplineShape);
            stream.WriteULONG((uint32_t)spline->PosParams.size());
            for (uint32_t param : spline->PosParams) {
                stream.WriteULONG(param);
            }
        }
    }

public:
    static std::vector<uint8_t> Compile(const CParticleEmitter& emitter) {
        CParticleWriteStream stream;

        stream.WriteULONG(emitter.Magic);
        stream.WriteString(emitter.Name);
        stream.WriteEBOOL(emitter.Emitter2D);
        stream.WriteEBOOL(emitter.PreWaterEffect);
        stream.WriteEBOOL(emitter.WaterEffect);
        stream.WriteEBOOL(emitter.ZBufferWriteable);
        stream.WriteEBOOL(emitter.ContinuousEmitter);
        stream.WriteEBOOL(emitter.IsScreenDisplacement);
        stream.WriteEBOOL(emitter.ReadZBuffer);
        stream.WriteFloat(emitter.MaxSpawnDistance);
        stream.WriteFloat(emitter.MaxDrawDistance);
        stream.WriteFloat(emitter.FadeOutStart);
        stream.WriteFloat(emitter.FadeInEnd);
        stream.WriteFloat(emitter.FadeInStart);
        stream.WriteSLONG(emitter.Priority);
        stream.WriteEBOOL(emitter.DieIfOffscreen);
        stream.WriteEBOOL(emitter.OffscreenUpdate);
        stream.WriteEBOOL(emitter.ClipEffectToWeatherMask);
        stream.WriteEBOOL(emitter.EnableDithering);
        stream.WriteEBOOL(emitter.CalcBoundingSphereOnceOnly);
        stream.WriteULONG((uint32_t)emitter.Systems.size());
        for (const auto& sys : emitter.Systems) {
            stream.WriteString(sys.Name);
            stream.WriteEBOOL(sys.Enabled);
            stream.WriteEBOOL(sys.ScaleParticles);
            stream.Write3DVector(sys.Scale);

            stream.WriteULONG((uint32_t)sys.Components.size());
            for (const auto& comp : sys.Components) {
                stream.WriteString(comp->ClassName);
                stream.WriteULONG(comp->InstanceID);
                stream.WriteEBOOL(comp->Enabled);

                if (auto* rs = dynamic_cast<CPSCRenderSprite*>(comp.get())) {
                    stream.WriteSLONG(rs->SpriteBankIndex);
                    stream.WriteSLONG(rs->TrailBankIndex);
                    stream.WriteColour(rs->StartColour);
                    stream.WriteColour(rs->MidColour);
                    stream.WriteColour(rs->EndColour);
                    stream.WriteULONG(rs->BlendMode);
                    stream.WriteULONG(rs->TrailBlendMode);
                    stream.WriteULONG(rs->BlendOp);
                    stream.WriteULONG(rs->TrailBlendOp);
                    stream.WriteULONG(rs->SpriteFlags);
                    stream.WriteULONG(rs->FadeInEndInteger);
                    stream.WriteULONG(rs->FadeOutBeginInteger);
                    stream.WriteULONG(rs->TrailLengthInteger);
                    stream.WriteULONG(rs->FlickerMinAlphaInteger);
                    stream.WriteULONG(rs->FlickerMinSizeInteger);
                    stream.WriteULONG(rs->NoCrossedSprites);

                    stream.WriteULONG(EncodeFloat(rs->StartRenderSize, 20.0f, 2047.0f));
                    stream.WriteULONG(EncodeFloat(rs->AlphaFadeMinimum, 1.0f, 127.0f));
                    stream.WriteULONG(EncodeFloat(rs->EndRenderSize, 20.0f, 2047.0f));
                    stream.WriteULONG(EncodeBiasFloat(rs->FlickerBias, 1.0f, 2.0f, 255.0f));
                    stream.WriteULONG(EncodeBiasFloat(rs->AnimationTimeSecs, -0.1f, 99.9f, 16383.0f));
                    stream.WriteULONG(EncodeFloat(rs->SizeFadeMinimum, 1.0f, 127.0f));
                    stream.WriteULONG(EncodeFloat(rs->TrailWidth, 10.0f, 1023.0f));
                    stream.WriteULONG(EncodeFloat(rs->FlickerSpeed, 30.0f, 4095.0f));

                    stream.WriteEBOOL(rs->UseStartColour);
                    stream.WriteEBOOL(rs->UseMidColour);
                    stream.WriteEBOOL(rs->UseEndColour);
                    stream.WriteEBOOL(rs->AlphaFadeEnable);
                    stream.WriteEBOOL(rs->SizeFadeEnable);
                    stream.WriteEBOOL(rs->FlickerEnable);
                    stream.WriteEBOOL(rs->ForceAnimationTime);
                }
                else if (auto* eg = dynamic_cast<CPSCEmitterGeneric*>(comp.get())) {
                    stream.WriteULONG(eg->EmitterPosParam);
                    stream.WriteULONG(eg->DirectionParamName);
                    stream.WriteULONG(eg->NoParticlesToStart);
                    stream.WriteULONG(eg->NoParticlesToStartRand);
                    stream.WriteULONG(eg->EmitterType);

                    stream.WriteEBOOL(eg->Solid);
                    stream.WriteEBOOL(eg->UseEmitterLifeSecs);
                    stream.WriteEBOOL(eg->UseEmitterTimelineSecs);
                    stream.WriteEBOOL(eg->OrientationXY);
                    stream.WriteEBOOL(eg->OrientationXZ);
                    stream.WriteEBOOL(eg->OrientationYZ);
                    stream.WriteEBOOL(eg->UseCustomDirection);
                    stream.WriteEBOOL(eg->UseOutwardDirection);
                    stream.WriteEBOOL(eg->UseParamDirection);
                    stream.WriteEBOOL(eg->OppositeDirection);

                    stream.WriteULONG(eg->AngularPerturbationInteger);

                    stream.WriteULONG(EncodeBiasFloat(eg->CustomDirection.X, 1.0f, 2.0f, 255.0f) & 0xFF);

                    stream.WriteULONG(EncodeFloat(eg->ParticlesPerSecond, 100.0f, 16383.0f));
                    stream.WriteULONG(EncodeFloat(eg->EmitterSize, 10.0f, 1023.0f));
                    stream.WriteULONG(EncodeBiasFloat(eg->RadialBias, 5.0f, 10.0f, 1023.0f));
                    stream.WriteULONG(EncodeFloat(eg->MinSpeed, 10.0f, 1023.0f));

                    stream.WriteEBOOL(eg->UseForwardDirection);
                    stream.WriteEBOOL(eg->UseRandom2DDirection);

                    stream.WriteULONG(EncodeBiasFloat(eg->CustomDirection.Y, 1.0f, 2.0f, 255.0f) & 0xFF);
                    stream.WriteULONG(EncodeBiasFloat(eg->CustomDirection.Z, 1.0f, 2.0f, 255.0f) & 0xFF);

                    stream.WriteULONG(EncodeFloat(eg->EmitterTimelineSecs, 30.0f, 32767.0f));
                    stream.WriteEBOOL(eg->UseRandom3DDirection);

                    stream.WriteULONG(EncodeFloat(eg->EmitterLifeSecs, 300.0f, 32767.0f));
                    stream.WriteULONG(EncodeFloat(eg->EmitterStartTime, 300.0f, 32767.0f));
                    stream.WriteULONG(EncodeFloat(eg->MaxSpeed, 10.0f, 1023.0f));

                    stream.WriteULONG(EncodeBiasFloat(eg->NonUniformScaling.X, 10.0f, 20.0f, 2047.0f));
                    stream.WriteULONG(EncodeBiasFloat(eg->NonUniformScaling.Y, 10.0f, 20.0f, 2047.0f));
                    stream.WriteULONG(EncodeBiasFloat(eg->NonUniformScaling.Z, 10.0f, 20.0f, 2047.0f));

                    stream.WriteEBOOL(eg->HasSpline);
                    if (eg->HasSpline) {
                        stream.WriteFloat(eg->SplineTension);
                        stream.WriteULONG((uint32_t)eg->SplineControlPoints.size());
                        for (const auto& pt : eg->SplineControlPoints) {
                            stream.Write3DVector(pt);
                        }
                    }
                }
                else if (auto* un = dynamic_cast<CPSCUpdateNormal*>(comp.get())) {
                    stream.WriteULONG(un->FadeInEndInteger);
                    stream.WriteULONG(un->FadeOutBeginInteger);
                    stream.WriteEBOOL(un->UseParticleLifeSecs);
                    stream.WriteEBOOL(un->UseRandomRotationAxis);
                    stream.WriteEBOOL(un->UseRandomInitialRotation);
                    stream.WriteEBOOL(un->UseAccelerationParam);
                    stream.WriteEBOOL(un->StayWithEmitter);
                    stream.WriteEBOOL(un->UseSystemLifeSecs);
                    stream.WriteEBOOL(un->SystemAlphaFadeEnable);
                    stream.WriteEBOOL(un->EmissionFadeEnable);
                    stream.WriteEBOOL(un->ParticleCollideWithGround);
                    stream.WriteEBOOL(un->ParticleCollideWithAnything);
                    stream.WriteEBOOL(un->ParticleDieOnCollision);
                    stream.WriteEBOOL(un->UseAllAttractors);
                    stream.WriteEBOOL(un->RandomisePosEnable);
                    stream.WriteEBOOL(un->RandomisePosDistVaryEnable);
                    stream.WriteEBOOL(un->SetOrientationFromDirection);
                    stream.WriteEBOOL(un->SetOrientationFromGame);
                    stream.WriteEBOOL(un->ParticleCreateDecal);
                    stream.WriteEBOOL(un->ParticleCreateDecalEmitter);

                    stream.WriteString(un->DecalEmitterName);

                    stream.WriteFloat(un->SystemLifeSecs); stream.WriteFloat(un->ParticleLifeSecs);
                    stream.WriteFloat(un->WindFactor); stream.WriteFloat(un->GravityFactor); stream.WriteFloat(un->AirResistance);
                    stream.WriteFloat(un->ParticleAccelerationScale);
                    stream.WriteFloat(un->InitialRotationX); stream.WriteFloat(un->InitialRotationY); stream.WriteFloat(un->InitialRotationZ);
                    stream.WriteFloat(un->RotationMinAngleSpeed); stream.WriteFloat(un->RotationMaxAngleSpeed); stream.WriteFloat(un->ParticleBounce);
                    stream.WriteFloat(un->SystemAlphaFadeMinimum); stream.WriteFloat(un->RandomisePosDistVaryScale); stream.WriteFloat(un->EmissionFadeMinimum);

                    stream.Write3DVector(un->ParticleSystemOffset); stream.Write3DVector(un->ParticleAcceleration);
                    stream.Write3DVector(un->RotationAxis); stream.Write3DVector(un->RandomisePosSpeed); stream.Write3DVector(un->RandomisePosScale);

                    stream.WriteULONG(un->AccelerationParam); stream.WriteULONG(un->OrientFromGameParam);
                }
                else if (auto* dr = dynamic_cast<CPSCDecalRenderer*>(comp.get())) {
                    stream.WriteFloat(dr->DecalLifeSecs);
                    stream.WriteSLONG(dr->DecalBankIndex);
                    stream.WriteColour(dr->StartColour);
                    stream.WriteColour(dr->MidColour);
                    stream.WriteColour(dr->EndColour);

                    stream.WriteULONG(dr->BlendMode);
                    stream.WriteULONG(dr->BlendOp);
                    stream.WriteULONG(dr->SpriteAlignment);
                    stream.WriteULONG(dr->FadeInEndInteger);
                    stream.WriteULONG(dr->FadeOutBeginInteger);
                    stream.WriteULONG(dr->StartRenderSizeInteger);
                    stream.WriteULONG(dr->AlphaFadeMinimumInteger);
                    stream.WriteULONG(dr->EndRenderSizeInteger);
                    stream.WriteULONG(dr->AnimationTimeSecsInteger);
                    stream.WriteULONG(dr->SizeFadeMinimumInteger);

                    stream.WriteEBOOL(dr->UseStartColour);
                    stream.WriteEBOOL(dr->UseMidColour);
                    stream.WriteEBOOL(dr->UseEndColour);
                    stream.WriteEBOOL(dr->AlphaFadeEnable);
                    stream.WriteEBOOL(dr->SizeFadeEnable);
                    stream.WriteEBOOL(dr->ForceAnimationTime);

                    stream.WriteULONG(dr->FlickerMinAlphaInteger);
                    stream.WriteULONG(dr->FlickerMinSizeInteger);
                    stream.WriteULONG(dr->FlickerBiasInteger);
                    stream.WriteULONG(dr->FlickerSpeedInteger);

                    stream.WriteEBOOL(dr->FlickerEnable);
                    stream.WriteEBOOL(dr->PoolingEnable);
                    stream.WriteEBOOL(dr->PoolIncreaseAlphaEnable);
                    stream.WriteEBOOL(dr->PoolIncreaseSizeEnable);
                    stream.WriteEBOOL(dr->PoolIncreaseLifeEnable);

                    stream.WriteFloat(dr->MaxPoolSize);
                    stream.WriteFloat(dr->MaxPoolAlpha);
                    stream.WriteFloat(dr->MaxPoolLife);
                    stream.WriteEBOOL(dr->PoolIncreaseFrameEnable);
                    stream.WriteFloat(dr->PoolFrameIncreaseRate);
                    stream.WriteFloat(dr->StencilCubeWidth);
                }
                else if (auto* sp = dynamic_cast<CPSCSpline*>(comp.get())) {
                    stream.WriteEBOOL(sp->SplineBounce);
                    stream.WriteEBOOL(sp->ScaleSplineSpeed);
                    stream.WriteFloat(sp->SplineAnimSpeed);
                    CompileSplineBase(stream, sp);
                }
                else if (auto* ss = dynamic_cast<CPSCSingleSprite*>(comp.get())) {
                    stream.WriteSLONG(ss->SpriteBankIndex);
                    stream.WriteColour(ss->StartColour); stream.WriteColour(ss->MidColour); stream.WriteColour(ss->EndColour);
                    stream.WriteFloat(ss->AnimationTimeSecs); stream.WriteFloat(ss->StartRenderSize); stream.WriteFloat(ss->EndRenderSize);
                    stream.WriteSLONG(ss->SpriteAlignment); stream.WriteSLONG(ss->NoCrossedSprites);
                    stream.WriteFloat(ss->InitialAngle);
                    stream.WriteSLONG(ss->FadeInEndInteger); stream.WriteSLONG(ss->FadeOutBeginInteger);
                    stream.WriteFloat(ss->AlphaFadeMinimum); stream.WriteFloat(ss->SizeFadeMinimum);
                    stream.WriteULONG(ss->PositionParam);
                    stream.WriteSLONG(ss->BlendMode); stream.WriteSLONG(ss->BlendOp);
                    stream.WriteSLONG(ss->TrailBlendMode); stream.WriteSLONG(ss->TrailBlendOp); stream.WriteSLONG(ss->TrailBankIndex);
                    stream.WriteFloat(ss->TrailWidth);
                    stream.WriteSLONG(ss->MaxTrailLength); stream.WriteSLONG(ss->StayWithEmitterIntFactor);

                    stream.WriteEBOOL(ss->UseStartColour); stream.WriteEBOOL(ss->UseMidColour); stream.WriteEBOOL(ss->UseEndColour);
                    stream.WriteEBOOL(ss->SelfIlluminating); stream.WriteEBOOL(ss->AlphaFadeEnable); stream.WriteEBOOL(ss->RotateAroundCentre);
                    stream.WriteEBOOL(ss->FaceMe2D); stream.WriteEBOOL(ss->FaceMe3D); stream.WriteEBOOL(ss->CrossedSprites);
                    stream.WriteEBOOL(ss->SizeFadeEnable); stream.WriteEBOOL(ss->ForceAnimationTime); stream.WriteEBOOL(ss->StayWithEmitter);
                    stream.WriteEBOOL(ss->UsePosition); stream.WriteEBOOL(ss->UseSplinePoints);
                    CompileSplineBase(stream, ss);
                }
                else if (auto* ob = dynamic_cast<CPSCOrbit*>(comp.get())) {
                    stream.WriteULONG(ob->CentreParam);
                    for (int i = 0; i < 3; i++) {
                        if (i < ob->OrbitsData.size()) {
                            stream.WriteSLONG(ob->OrbitsData[i].Type);
                            stream.WriteULONG(ob->OrbitsData[i].Enabled ? 1 : 0);
                            stream.WriteFloat(ob->OrbitsData[i].Radius);
                            stream.WriteFloat(ob->OrbitsData[i].Expand);
                            stream.WriteULONG(ob->OrbitsData[i].CycleFlag ? 1 : 0);
                            stream.WriteFloat(ob->OrbitsData[i].CycleTime);
                            stream.WriteFloat(ob->OrbitsData[i].SqueezeScale);
                            stream.WriteFloat(ob->OrbitsData[i].SqueezeAngle);
                            stream.WriteFloat(ob->OrbitsData[i].RotateSpeed);
                            stream.WriteFloat(ob->OrbitsData[i].RotateStart);
                            stream.WriteFloat(ob->OrbitsData[i].RotateSpeedRandom);
                            stream.WriteFloat(ob->OrbitsData[i].RotateStartRandom);
                        }
                    }
                }
                else if (auto* at = dynamic_cast<CPSCAttractor*>(comp.get())) {
                    stream.WriteEBOOL(at->AttractorEnabled);
                    stream.WriteEBOOL(at->AttractorUseParamPosition);
                    stream.WriteSLONG(at->AttractorInfluenceFallOff);
                    stream.WriteULONG(at->AttractorPositionParam);
                    stream.WriteULONG(at->AttractorPositionParamName);
                    stream.WriteFloat(at->AttractorInfluenceRadius);
                    stream.WriteFloat(at->AttractorInfluenceForce);

                    stream.WriteULONG((uint32_t)at->AttractorUserPointsArray.size());
                    for (const auto& pt : at->AttractorUserPointsArray) {
                        stream.Write3DVector(pt);
                    }
                }
                else if (auto* lgt = dynamic_cast<CPSCLight*>(comp.get())) {
                    stream.WriteULONG(lgt->LightPositionParam);
                    stream.WriteFloat(lgt->LightLifeSecs); stream.WriteFloat(lgt->LightRespawnDelaySecs);
                    stream.WriteFloat(lgt->LightStartTime); stream.WriteFloat(lgt->LightTimelineSecs);
                    stream.WriteFloat(lgt->LightStartRenderWorldRadius); stream.WriteFloat(lgt->LightEndRenderWorldRadius);
                    stream.WriteSLONG(lgt->LightAttenuationFactor);
                    stream.WriteSLONG(lgt->LightFadeInEndInteger); stream.WriteSLONG(lgt->LightFadeOutBeginInteger);
                    stream.WriteFloat(lgt->LightWorldRadiusFadeMinimum);
                    stream.WriteColour(lgt->LightColourFadeMinimum);

                    stream.WriteEBOOL(lgt->LightWorldRadiusFadeEnable);
                    stream.WriteEBOOL(lgt->LightUseLifeSecs);
                    stream.WriteEBOOL(lgt->LightEnabled);
                    stream.WriteEBOOL(lgt->LightRespawns);
                    stream.WriteEBOOL(lgt->LightColourFadeEnable);
                    stream.WriteEBOOL(lgt->LightUseStartColour);
                    stream.WriteEBOOL(lgt->LightUseMidColour);
                    stream.WriteEBOOL(lgt->LightUseEndColour);

                    stream.WriteEBOOL(lgt->LightUseFadeColour);
                    stream.WriteEBOOL(lgt->LightUseTimelineSecs);
                    stream.WriteEBOOL(lgt->LightHasInitializedTime);

                    stream.WriteColour(lgt->LightStartColour);
                    stream.WriteColour(lgt->LightMidColour);
                    stream.WriteColour(lgt->LightEndColour);
                }
                else if (auto* rm = dynamic_cast<CPSCRenderMesh*>(comp.get())) {
                    stream.WriteSLONG(rm->BankIndex);
                    stream.WriteSLONG(rm->TrailBankIndex);
                    stream.WriteColour(rm->TrailStartColour);
                    stream.WriteColour(rm->TrailMidColour);
                    stream.WriteColour(rm->TrailEndColour);
                    stream.WriteColour(rm->StartColour);
                    stream.WriteColour(rm->MidColour);
                    stream.WriteColour(rm->EndColour);

                    stream.WriteULONG(rm->BlendMode);
                    stream.WriteULONG(rm->TrailBlendMode);
                    stream.WriteULONG(rm->BlendOp);
                    stream.WriteULONG(rm->TrailBlendOp);
                    stream.WriteULONG(rm->FadeInEndInteger);
                    stream.WriteULONG(rm->FadeOutBeginInteger);
                    stream.WriteULONG(rm->TrailLengthInteger);
                    stream.WriteULONG(rm->FlickerMinAlphaInteger);
                    stream.WriteULONG(rm->FlickerMinSizeInteger);

                    stream.WriteULONG(EncodeFloat(rm->StartRenderSize.X, 20.0f, 2047.0f));
                    stream.WriteULONG(EncodeFloat(rm->StartRenderSize.Y, 20.0f, 2047.0f));
                    stream.WriteULONG(EncodeFloat(rm->StartRenderSize.Z, 20.0f, 2047.0f));

                    stream.WriteEBOOL(rm->CentredOnPos);
                    stream.WriteEBOOL(rm->AlphaFadeEnable);

                    stream.WriteULONG(EncodeFloat(rm->EndRenderSize.X, 20.0f, 2047.0f));
                    stream.WriteULONG(EncodeFloat(rm->EndRenderSize.Y, 20.0f, 2047.0f));
                    stream.WriteULONG(EncodeFloat(rm->EndRenderSize.Z, 20.0f, 2047.0f));

                    stream.WriteEBOOL(rm->SizeFadeEnable);
                    stream.WriteEBOOL(rm->FlickerEnable);
                    stream.WriteEBOOL(rm->TrailUseStartColour);
                    stream.WriteEBOOL(rm->TrailUseMidColour);
                    stream.WriteEBOOL(rm->TrailUseEndColour);
                    stream.WriteEBOOL(rm->UseRenderSizeParam);

                    stream.WriteULONG(EncodeBiasFloat(rm->FlickerBias, 1.0f, 2.0f, 255.0f));
                    stream.WriteULONG(EncodeFloat(rm->FlickerSpeed, 30.0f, 4095.0f));
                    stream.WriteULONG(EncodeFloat(rm->AlphaFadeMinimum, 1.0f, 127.0f));

                    stream.WriteEBOOL(rm->UseStartColour);
                    stream.WriteULONG(EncodeFloat(rm->SizeFadeMinimum, 1.0f, 127.0f));
                    stream.WriteEBOOL(rm->UseMidColour);
                    stream.WriteULONG(EncodeFloat(rm->TrailWidth, 10.0f, 1023.0f));
                    stream.WriteEBOOL(rm->UseEndColour);

                    stream.WriteULONG(rm->RenderSizeParam);
                }

                stream.WriteEBOOL(true);
                stream.Data.back() = 0x7B;
            }

            stream.WriteEBOOL(true);
            stream.Data.back() = 0x26;
        }

        return stream.Data;
    }
};