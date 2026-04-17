#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cstring>


struct C3DVector { float X, Y, Z; };
struct CRGBColour { uint8_t B, G, R, A; };

struct COrbitData {
    int32_t Type;
    bool Enabled;
    float Radius;
    float Expand;
    bool CycleFlag;
    float CycleTime;
    float SqueezeScale;
    float SqueezeAngle;
    float RotateSpeed;
    float RotateStart;
    float RotateSpeedRandom;
    float RotateStartRandom;
};

class CParticleStream {
public:
    const std::vector<uint8_t>& Data;
    size_t Offset;

    CParticleStream(const std::vector<uint8_t>& data, size_t startOffset = 0)
        : Data(data), Offset(startOffset) {
    }

    uint32_t ReadULONG() {
        if (Offset + 4 > Data.size()) return 0;
        uint32_t val;
        std::memcpy(&val, &Data[Offset], 4);
        Offset += 4;
        return val;
    }

    int32_t ReadSLONG() { return (int32_t)ReadULONG(); }

    float ReadFloat() {
        if (Offset + 4 > Data.size()) return 0.0f;
        float val;
        std::memcpy(&val, &Data[Offset], 4);
        Offset += 4;
        return val;
    }

    bool ReadEBOOL() {
        if (Offset + 1 > Data.size()) return false;
        bool val = Data[Offset] != 0;
        Offset += 1;
        return val;
    }

    CRGBColour ReadColour() {
        if (Offset + 4 > Data.size()) return { 0,0,0,0 };
        CRGBColour c = { Data[Offset], Data[Offset + 1], Data[Offset + 2], Data[Offset + 3] };
        Offset += 4;
        return c;
    }

    C3DVector Read3DVector() {
        return { ReadFloat(), ReadFloat(), ReadFloat() };
    }

    std::string ReadString() {
        std::string str = "";
        while (Offset < Data.size() && Data[Offset] != '\0') {
            str += (char)Data[Offset];
            Offset++;
        }
        Offset++;
        return str;
    }
};

struct CParticleComponent {
    std::string ClassName;
    uint32_t InstanceID = 0;
    bool Enabled = true;
    bool Visible = true;

    virtual ~CParticleComponent() = default;
    virtual void Parse(CParticleStream& stream) = 0;
};

struct CPSCSplineBase {
    std::vector<C3DVector> ControlPoints;

    bool HasRandomOffsetData = false;
    bool OffsetOnlyAtStartup = false, ResetEachFrame = false;
    struct RandomOffset { C3DVector Limit, Speed; };
    std::vector<RandomOffset> RandomOffsets;

    bool HasPosParamData = false;
    bool MaintainSplineShape = false;
    std::vector<uint32_t> PosParams;

    void ParseSplineBase(CParticleStream& stream) {
        uint32_t pointCount = stream.ReadULONG();
        for (uint32_t i = 0; i < pointCount; i++) {
            ControlPoints.push_back(stream.Read3DVector());
        }

        HasRandomOffsetData = stream.ReadEBOOL();
        if (HasRandomOffsetData) {
            OffsetOnlyAtStartup = stream.ReadEBOOL();
            ResetEachFrame = stream.ReadEBOOL();
            uint32_t offsetCount = stream.ReadULONG();
            for (uint32_t i = 0; i < offsetCount; i++) {
                RandomOffset off;
                off.Limit = stream.Read3DVector();
                off.Speed = stream.Read3DVector();
                RandomOffsets.push_back(off);
            }
        }

        HasPosParamData = stream.ReadEBOOL();
        if (HasPosParamData) {
            MaintainSplineShape = stream.ReadEBOOL();
            uint32_t paramCount = stream.ReadULONG();
            for (uint32_t i = 0; i < paramCount; i++) {
                PosParams.push_back(stream.ReadULONG());
            }
        }
    }
};

struct CPSCRenderSprite : public CParticleComponent {
    int32_t SpriteBankIndex, TrailBankIndex;
    CRGBColour StartColour, MidColour, EndColour;

    uint32_t BlendMode, TrailBlendMode, BlendOp, TrailBlendOp;
    uint32_t SpriteFlags, FadeInEndInteger, FadeOutBeginInteger;
    uint32_t TrailLengthInteger, FlickerMinAlphaInteger, FlickerMinSizeInteger, NoCrossedSprites;

    float StartRenderSize, AlphaFadeMinimum, EndRenderSize;
    float FlickerBias, AnimationTimeSecs, SizeFadeMinimum;
    float TrailWidth, FlickerSpeed;

    bool UseStartColour, UseMidColour, UseEndColour;
    bool AlphaFadeEnable, SizeFadeEnable, FlickerEnable, ForceAnimationTime;

    void Parse(CParticleStream& stream) override {
        SpriteBankIndex = stream.ReadSLONG();
        TrailBankIndex = stream.ReadSLONG();
        StartColour = stream.ReadColour();
        MidColour = stream.ReadColour();
        EndColour = stream.ReadColour();

        BlendMode = stream.ReadULONG();
        TrailBlendMode = stream.ReadULONG();
        BlendOp = stream.ReadULONG();
        TrailBlendOp = stream.ReadULONG();
        SpriteFlags = stream.ReadULONG();
        FadeInEndInteger = stream.ReadULONG();
        FadeOutBeginInteger = stream.ReadULONG();

        TrailLengthInteger = stream.ReadULONG();
        FlickerMinAlphaInteger = stream.ReadULONG();
        FlickerMinSizeInteger = stream.ReadULONG();
        NoCrossedSprites = stream.ReadULONG();

        StartRenderSize = stream.ReadULONG() / 2047.0f * 20.0f;
        AlphaFadeMinimum = stream.ReadULONG() / 127.0f;
        EndRenderSize = stream.ReadULONG() / 2047.0f * 20.0f;
        FlickerBias = stream.ReadULONG() / 255.0f * 2.0f - 1.0f;
        AnimationTimeSecs = stream.ReadULONG() / 16383.0f * 99.9f + 0.1f;
        SizeFadeMinimum = stream.ReadULONG() / 127.0f;
        TrailWidth = stream.ReadULONG() / 1023.0f * 10.0f;
        FlickerSpeed = stream.ReadULONG() / 4095.0f * 30.0f;

        UseStartColour = stream.ReadEBOOL();
        UseMidColour = stream.ReadEBOOL();
        UseEndColour = stream.ReadEBOOL();
        AlphaFadeEnable = stream.ReadEBOOL();
        SizeFadeEnable = stream.ReadEBOOL();
        FlickerEnable = stream.ReadEBOOL();
        ForceAnimationTime = stream.ReadEBOOL();
    }
};

struct CPSCEmitterGeneric : public CParticleComponent {
    uint32_t EmitterPosParam, DirectionParamName;
    uint32_t NoParticlesToStart, NoParticlesToStartRand, EmitterType;

    bool Solid, UseEmitterLifeSecs, UseEmitterTimelineSecs;
    bool OrientationXY, OrientationXZ, OrientationYZ;
    bool UseCustomDirection, UseOutwardDirection, UseParamDirection, OppositeDirection;

    uint32_t AngularPerturbationInteger;

    C3DVector CustomDirection;
    float ParticlesPerSecond, EmitterSize, RadialBias, MinSpeed;

    bool UseForwardDirection, UseRandom2DDirection;

    float EmitterTimelineSecs;
    bool UseRandom3DDirection;

    float EmitterLifeSecs, EmitterStartTime, MaxSpeed;
    C3DVector NonUniformScaling;

    bool HasSpline;
    float SplineTension;
    std::vector<C3DVector> SplineControlPoints;

    void Parse(CParticleStream& stream) override {
        EmitterPosParam = stream.ReadULONG();
        DirectionParamName = stream.ReadULONG();
        NoParticlesToStart = stream.ReadULONG();
        NoParticlesToStartRand = stream.ReadULONG();
        EmitterType = stream.ReadULONG();

        Solid = stream.ReadEBOOL();
        UseEmitterLifeSecs = stream.ReadEBOOL();
        UseEmitterTimelineSecs = stream.ReadEBOOL();
        OrientationXY = stream.ReadEBOOL();
        OrientationXZ = stream.ReadEBOOL();
        OrientationYZ = stream.ReadEBOOL();
        UseCustomDirection = stream.ReadEBOOL();
        UseOutwardDirection = stream.ReadEBOOL();
        UseParamDirection = stream.ReadEBOOL();
        OppositeDirection = stream.ReadEBOOL();

        AngularPerturbationInteger = stream.ReadULONG();

        CustomDirection.X = (stream.ReadULONG() & 0xFF) / 255.0f * 2.0f - 1.0f;

        ParticlesPerSecond = stream.ReadULONG() / 16383.0f * 100.0f;
        EmitterSize = stream.ReadULONG() / 1023.0f * 10.0f;
        RadialBias = stream.ReadULONG() / 1023.0f * 10.0f - 5.0f;
        MinSpeed = stream.ReadULONG() / 1023.0f * 10.0f;

        UseForwardDirection = stream.ReadEBOOL();
        UseRandom2DDirection = stream.ReadEBOOL();

        CustomDirection.Y = (stream.ReadULONG() & 0xFF) / 255.0f * 2.0f - 1.0f;
        CustomDirection.Z = (stream.ReadULONG() & 0xFF) / 255.0f * 2.0f - 1.0f;

        EmitterTimelineSecs = stream.ReadULONG() / 32767.0f * 30.0f;
        UseRandom3DDirection = stream.ReadEBOOL();

        EmitterLifeSecs = stream.ReadULONG() / 32767.0f * 300.0f;
        EmitterStartTime = stream.ReadULONG() / 32767.0f * 300.0f;
        MaxSpeed = stream.ReadULONG() / 1023.0f * 10.0f;

        NonUniformScaling.X = stream.ReadULONG() / 2047.0f * 20.0f - 10.0f;
        NonUniformScaling.Y = stream.ReadULONG() / 2047.0f * 20.0f - 10.0f;
        NonUniformScaling.Z = stream.ReadULONG() / 2047.0f * 20.0f - 10.0f;

        HasSpline = stream.ReadEBOOL();
        if (HasSpline) {
            SplineTension = stream.ReadFloat();
            uint32_t count = stream.ReadULONG();
            for (uint32_t i = 0; i < count; i++) {
                SplineControlPoints.push_back(stream.Read3DVector());
            }
        }
    }
};

struct CPSCUpdateNormal : public CParticleComponent {
    uint32_t FadeInEndInteger, FadeOutBeginInteger;
    bool UseParticleLifeSecs, UseRandomRotationAxis, UseRandomInitialRotation, UseAccelerationParam;
    bool StayWithEmitter, UseSystemLifeSecs, SystemAlphaFadeEnable, EmissionFadeEnable;
    bool ParticleCollideWithGround, ParticleCollideWithAnything, ParticleDieOnCollision;
    bool UseAllAttractors, RandomisePosEnable, RandomisePosDistVaryEnable;
    bool SetOrientationFromDirection, SetOrientationFromGame, ParticleCreateDecal, ParticleCreateDecalEmitter;
    std::string DecalEmitterName;
    float SystemLifeSecs, ParticleLifeSecs, WindFactor, GravityFactor, AirResistance, ParticleAccelerationScale;
    float InitialRotationX, InitialRotationY, InitialRotationZ;
    float RotationMinAngleSpeed, RotationMaxAngleSpeed, ParticleBounce;
    float SystemAlphaFadeMinimum, RandomisePosDistVaryScale, EmissionFadeMinimum;
    C3DVector ParticleSystemOffset, ParticleAcceleration, RotationAxis, RandomisePosSpeed, RandomisePosScale;
    uint32_t AccelerationParam, OrientFromGameParam;

    void Parse(CParticleStream& stream) override {
        FadeInEndInteger = stream.ReadULONG();
        FadeOutBeginInteger = stream.ReadULONG();

        UseParticleLifeSecs = stream.ReadEBOOL();
        UseRandomRotationAxis = stream.ReadEBOOL();
        UseRandomInitialRotation = stream.ReadEBOOL();
        UseAccelerationParam = stream.ReadEBOOL();
        StayWithEmitter = stream.ReadEBOOL();
        UseSystemLifeSecs = stream.ReadEBOOL();
        SystemAlphaFadeEnable = stream.ReadEBOOL();
        EmissionFadeEnable = stream.ReadEBOOL();
        ParticleCollideWithGround = stream.ReadEBOOL();
        ParticleCollideWithAnything = stream.ReadEBOOL();
        ParticleDieOnCollision = stream.ReadEBOOL();
        UseAllAttractors = stream.ReadEBOOL();
        RandomisePosEnable = stream.ReadEBOOL();
        RandomisePosDistVaryEnable = stream.ReadEBOOL();
        SetOrientationFromDirection = stream.ReadEBOOL();
        SetOrientationFromGame = stream.ReadEBOOL();
        ParticleCreateDecal = stream.ReadEBOOL();
        ParticleCreateDecalEmitter = stream.ReadEBOOL();

        DecalEmitterName = stream.ReadString();

        SystemLifeSecs = stream.ReadFloat(); ParticleLifeSecs = stream.ReadFloat();
        WindFactor = stream.ReadFloat(); GravityFactor = stream.ReadFloat(); AirResistance = stream.ReadFloat();
        ParticleAccelerationScale = stream.ReadFloat();
        InitialRotationX = stream.ReadFloat(); InitialRotationY = stream.ReadFloat(); InitialRotationZ = stream.ReadFloat();
        RotationMinAngleSpeed = stream.ReadFloat(); RotationMaxAngleSpeed = stream.ReadFloat(); ParticleBounce = stream.ReadFloat();
        SystemAlphaFadeMinimum = stream.ReadFloat(); RandomisePosDistVaryScale = stream.ReadFloat(); EmissionFadeMinimum = stream.ReadFloat();

        ParticleSystemOffset = stream.Read3DVector(); ParticleAcceleration = stream.Read3DVector();
        RotationAxis = stream.Read3DVector(); RandomisePosSpeed = stream.Read3DVector(); RandomisePosScale = stream.Read3DVector();

        AccelerationParam = stream.ReadULONG(); OrientFromGameParam = stream.ReadULONG();
    }
};

struct CPSCDecalRenderer : public CParticleComponent {
    float DecalLifeSecs;
    int32_t DecalBankIndex;
    CRGBColour StartColour, MidColour, EndColour;

    uint32_t BlendMode, BlendOp, SpriteAlignment, FadeInEndInteger, FadeOutBeginInteger;
    uint32_t StartRenderSizeInteger, AlphaFadeMinimumInteger, EndRenderSizeInteger;
    uint32_t AnimationTimeSecsInteger, SizeFadeMinimumInteger;

    bool UseStartColour, UseMidColour, UseEndColour;
    bool AlphaFadeEnable, SizeFadeEnable, ForceAnimationTime;

    uint32_t FlickerMinAlphaInteger, FlickerMinSizeInteger, FlickerBiasInteger, FlickerSpeedInteger;

    bool FlickerEnable, PoolingEnable, PoolIncreaseAlphaEnable;
    bool PoolIncreaseSizeEnable, PoolIncreaseLifeEnable;

    float MaxPoolSize, MaxPoolAlpha, MaxPoolLife;
    bool PoolIncreaseFrameEnable;
    float PoolFrameIncreaseRate, StencilCubeWidth;

    void Parse(CParticleStream& stream) override {
        DecalLifeSecs = stream.ReadFloat();
        DecalBankIndex = stream.ReadSLONG();
        StartColour = stream.ReadColour();
        MidColour = stream.ReadColour();
        EndColour = stream.ReadColour();

        BlendMode = stream.ReadULONG();
        BlendOp = stream.ReadULONG();
        SpriteAlignment = stream.ReadULONG();
        FadeInEndInteger = stream.ReadULONG();
        FadeOutBeginInteger = stream.ReadULONG();
        StartRenderSizeInteger = stream.ReadULONG();
        AlphaFadeMinimumInteger = stream.ReadULONG();
        EndRenderSizeInteger = stream.ReadULONG();
        AnimationTimeSecsInteger = stream.ReadULONG();
        SizeFadeMinimumInteger = stream.ReadULONG();

        UseStartColour = stream.ReadEBOOL();
        UseMidColour = stream.ReadEBOOL();
        UseEndColour = stream.ReadEBOOL();
        AlphaFadeEnable = stream.ReadEBOOL();
        SizeFadeEnable = stream.ReadEBOOL();
        ForceAnimationTime = stream.ReadEBOOL();

        FlickerMinAlphaInteger = stream.ReadULONG();
        FlickerMinSizeInteger = stream.ReadULONG();
        FlickerBiasInteger = stream.ReadULONG();
        FlickerSpeedInteger = stream.ReadULONG();

        FlickerEnable = stream.ReadEBOOL();
        PoolingEnable = stream.ReadEBOOL();
        PoolIncreaseAlphaEnable = stream.ReadEBOOL();
        PoolIncreaseSizeEnable = stream.ReadEBOOL();
        PoolIncreaseLifeEnable = stream.ReadEBOOL();

        MaxPoolSize = stream.ReadFloat();
        MaxPoolAlpha = stream.ReadFloat();
        MaxPoolLife = stream.ReadFloat();
        PoolIncreaseFrameEnable = stream.ReadEBOOL();
        PoolFrameIncreaseRate = stream.ReadFloat();
        StencilCubeWidth = stream.ReadFloat();
    }
};

struct CPSCSpline : public CParticleComponent, public CPSCSplineBase {
    bool SplineBounce, ScaleSplineSpeed;
    float SplineAnimSpeed;

    void Parse(CParticleStream& stream) override {
        SplineBounce = stream.ReadEBOOL();
        ScaleSplineSpeed = stream.ReadEBOOL();
        SplineAnimSpeed = stream.ReadFloat();
        ParseSplineBase(stream);
    }
};

struct CPSCSingleSprite : public CParticleComponent, public CPSCSplineBase {
    int32_t SpriteBankIndex;
    CRGBColour StartColour, MidColour, EndColour;
    float AnimationTimeSecs, StartRenderSize, EndRenderSize;
    int32_t SpriteAlignment, NoCrossedSprites;
    float InitialAngle;
    int32_t FadeInEndInteger, FadeOutBeginInteger;
    float AlphaFadeMinimum, SizeFadeMinimum;
    uint32_t PositionParam;
    int32_t BlendMode, BlendOp, TrailBlendMode, TrailBlendOp, TrailBankIndex;
    float TrailWidth;
    int32_t MaxTrailLength, StayWithEmitterIntFactor;

    bool UseStartColour, UseMidColour, UseEndColour;
    bool SelfIlluminating, AlphaFadeEnable, RotateAroundCentre;
    bool FaceMe2D, FaceMe3D, CrossedSprites;
    bool SizeFadeEnable, ForceAnimationTime, StayWithEmitter;
    bool UsePosition, UseSplinePoints;

    void Parse(CParticleStream& stream) override {
        SpriteBankIndex = stream.ReadSLONG();
        StartColour = stream.ReadColour(); MidColour = stream.ReadColour(); EndColour = stream.ReadColour();
        AnimationTimeSecs = stream.ReadFloat(); StartRenderSize = stream.ReadFloat(); EndRenderSize = stream.ReadFloat();
        SpriteAlignment = stream.ReadSLONG(); NoCrossedSprites = stream.ReadSLONG();
        InitialAngle = stream.ReadFloat();
        FadeInEndInteger = stream.ReadSLONG(); FadeOutBeginInteger = stream.ReadSLONG();
        AlphaFadeMinimum = stream.ReadFloat(); SizeFadeMinimum = stream.ReadFloat();
        PositionParam = stream.ReadULONG();
        BlendMode = stream.ReadSLONG(); BlendOp = stream.ReadSLONG();
        TrailBlendMode = stream.ReadSLONG(); TrailBlendOp = stream.ReadSLONG(); TrailBankIndex = stream.ReadSLONG();
        TrailWidth = stream.ReadFloat();
        MaxTrailLength = stream.ReadSLONG(); StayWithEmitterIntFactor = stream.ReadSLONG();
        UseStartColour = stream.ReadEBOOL();
        UseMidColour = stream.ReadEBOOL();
        UseEndColour = stream.ReadEBOOL();
        SelfIlluminating = stream.ReadEBOOL();
        AlphaFadeEnable = stream.ReadEBOOL();
        RotateAroundCentre = stream.ReadEBOOL();
        FaceMe2D = stream.ReadEBOOL();
        FaceMe3D = stream.ReadEBOOL();
        CrossedSprites = stream.ReadEBOOL();
        SizeFadeEnable = stream.ReadEBOOL();
        ForceAnimationTime = stream.ReadEBOOL();
        StayWithEmitter = stream.ReadEBOOL();

        UsePosition = stream.ReadEBOOL(); UseSplinePoints = stream.ReadEBOOL();
        ParseSplineBase(stream);
    }
};

struct CPSCOrbit : public CParticleComponent {
    uint32_t CentreParam;
    std::vector<COrbitData> OrbitsData;

    void Parse(CParticleStream& stream) override {
        CentreParam = stream.ReadULONG();

        for (int i = 0; i < 3; i++) {
            COrbitData orbit;
            orbit.Type = stream.ReadSLONG();

            orbit.Enabled = stream.ReadULONG() != 0;

            orbit.Radius = stream.ReadFloat();
            orbit.Expand = stream.ReadFloat();

            orbit.CycleFlag = stream.ReadULONG() != 0;

            orbit.CycleTime = stream.ReadFloat();
            orbit.SqueezeScale = stream.ReadFloat();
            orbit.SqueezeAngle = stream.ReadFloat();
            orbit.RotateSpeed = stream.ReadFloat();
            orbit.RotateStart = stream.ReadFloat();
            orbit.RotateSpeedRandom = stream.ReadFloat();
            orbit.RotateStartRandom = stream.ReadFloat();

            OrbitsData.push_back(orbit);
        }
    }
};

struct CPSCAttractor : public CParticleComponent {
    bool AttractorEnabled, AttractorUseParamPosition;
    int32_t AttractorInfluenceFallOff;
    uint32_t AttractorPositionParam, AttractorPositionParamName;
    float AttractorInfluenceRadius, AttractorInfluenceForce;
    std::vector<C3DVector> AttractorUserPointsArray;

    void Parse(CParticleStream& stream) override {
        AttractorEnabled = stream.ReadEBOOL();
        AttractorUseParamPosition = stream.ReadEBOOL();
        AttractorInfluenceFallOff = stream.ReadSLONG();
        AttractorPositionParam = stream.ReadULONG();
        AttractorPositionParamName = stream.ReadULONG();
        AttractorInfluenceRadius = stream.ReadFloat();
        AttractorInfluenceForce = stream.ReadFloat();

        uint32_t count = stream.ReadULONG();
        for (uint32_t i = 0; i < count; i++) {
            AttractorUserPointsArray.push_back(stream.Read3DVector());
        }
    }
};

struct CPSCLight : public CParticleComponent {
    uint32_t LightPositionParam;
    float LightLifeSecs, LightRespawnDelaySecs, LightStartTime, LightTimelineSecs;
    float LightStartRenderWorldRadius, LightEndRenderWorldRadius;
    int32_t LightAttenuationFactor, LightFadeInEndInteger, LightFadeOutBeginInteger;
    float LightWorldRadiusFadeMinimum;
    CRGBColour LightColourFadeMinimum;

    bool LightWorldRadiusFadeEnable, LightUseLifeSecs, LightEnabled, LightRespawns;
    bool LightColourFadeEnable, LightUseStartColour, LightUseMidColour, LightUseEndColour;

    bool LightUseFadeColour, LightUseTimelineSecs, LightHasInitializedTime;

    CRGBColour LightStartColour, LightMidColour, LightEndColour;

    void Parse(CParticleStream& stream) override {
        LightPositionParam = stream.ReadULONG();
        LightLifeSecs = stream.ReadFloat(); LightRespawnDelaySecs = stream.ReadFloat();
        LightStartTime = stream.ReadFloat(); LightTimelineSecs = stream.ReadFloat();
        LightStartRenderWorldRadius = stream.ReadFloat(); LightEndRenderWorldRadius = stream.ReadFloat();
        LightAttenuationFactor = stream.ReadSLONG();
        LightFadeInEndInteger = stream.ReadSLONG(); LightFadeOutBeginInteger = stream.ReadSLONG();
        LightWorldRadiusFadeMinimum = stream.ReadFloat();
        LightColourFadeMinimum = stream.ReadColour();

        LightWorldRadiusFadeEnable = stream.ReadEBOOL();
        LightUseLifeSecs = stream.ReadEBOOL();
        LightEnabled = stream.ReadEBOOL();
        LightRespawns = stream.ReadEBOOL();
        LightColourFadeEnable = stream.ReadEBOOL();
        LightUseStartColour = stream.ReadEBOOL();
        LightUseMidColour = stream.ReadEBOOL();
        LightUseEndColour = stream.ReadEBOOL();

        LightUseFadeColour = stream.ReadEBOOL();
        LightUseTimelineSecs = stream.ReadEBOOL();
        LightHasInitializedTime = stream.ReadEBOOL();

        LightStartColour = stream.ReadColour();
        LightMidColour = stream.ReadColour();
        LightEndColour = stream.ReadColour();
    }
};

struct CPSCRenderMesh : public CParticleComponent {
    int32_t BankIndex, TrailBankIndex;
    CRGBColour TrailStartColour, TrailMidColour, TrailEndColour;
    CRGBColour StartColour, MidColour, EndColour;

    uint32_t BlendMode, TrailBlendMode, BlendOp, TrailBlendOp;
    uint32_t FadeInEndInteger, FadeOutBeginInteger;
    uint32_t TrailLengthInteger, FlickerMinAlphaInteger, FlickerMinSizeInteger;

    C3DVector StartRenderSize;
    bool CentredOnPos, AlphaFadeEnable;

    C3DVector EndRenderSize;
    bool SizeFadeEnable, FlickerEnable;
    bool TrailUseStartColour, TrailUseMidColour, TrailUseEndColour, UseRenderSizeParam;

    float FlickerBias, FlickerSpeed, AlphaFadeMinimum;
    bool UseStartColour;

    float SizeFadeMinimum;
    bool UseMidColour;

    float TrailWidth;
    bool UseEndColour;

    uint32_t RenderSizeParam;

    void Parse(CParticleStream& stream) override {
        BankIndex = stream.ReadSLONG();
        TrailBankIndex = stream.ReadSLONG();
        TrailStartColour = stream.ReadColour();
        TrailMidColour = stream.ReadColour();
        TrailEndColour = stream.ReadColour();
        StartColour = stream.ReadColour();
        MidColour = stream.ReadColour();
        EndColour = stream.ReadColour();

        BlendMode = stream.ReadULONG();
        TrailBlendMode = stream.ReadULONG();
        BlendOp = stream.ReadULONG();
        TrailBlendOp = stream.ReadULONG();
        FadeInEndInteger = stream.ReadULONG();
        FadeOutBeginInteger = stream.ReadULONG();

        TrailLengthInteger = stream.ReadULONG();
        FlickerMinAlphaInteger = stream.ReadULONG();
        FlickerMinSizeInteger = stream.ReadULONG();

        StartRenderSize.X = stream.ReadULONG() / 2047.0f * 20.0f;
        StartRenderSize.Y = stream.ReadULONG() / 2047.0f * 20.0f;
        StartRenderSize.Z = stream.ReadULONG() / 2047.0f * 20.0f;

        CentredOnPos = stream.ReadEBOOL();
        AlphaFadeEnable = stream.ReadEBOOL();

        EndRenderSize.X = stream.ReadULONG() / 2047.0f * 20.0f;
        EndRenderSize.Y = stream.ReadULONG() / 2047.0f * 20.0f;
        EndRenderSize.Z = stream.ReadULONG() / 2047.0f * 20.0f;

        SizeFadeEnable = stream.ReadEBOOL();
        FlickerEnable = stream.ReadEBOOL();
        TrailUseStartColour = stream.ReadEBOOL();
        TrailUseMidColour = stream.ReadEBOOL();
        TrailUseEndColour = stream.ReadEBOOL();
        UseRenderSizeParam = stream.ReadEBOOL();

        FlickerBias = stream.ReadULONG() / 255.0f * 2.0f - 1.0f;
        FlickerSpeed = stream.ReadULONG() / 4095.0f * 30.0f;
        AlphaFadeMinimum = stream.ReadULONG() / 127.0f;

        UseStartColour = stream.ReadEBOOL();

        SizeFadeMinimum = stream.ReadULONG() / 127.0f;
        UseMidColour = stream.ReadEBOOL();

        TrailWidth = stream.ReadULONG() / 1023.0f * 10.0f;
        UseEndColour = stream.ReadEBOOL();

        RenderSizeParam = stream.ReadULONG();
    }
};

inline std::shared_ptr<CParticleComponent> CreateComponent(const std::string& className) {
    if (className == "CPSCRenderSprite") return std::make_shared<CPSCRenderSprite>();
    if (className == "CPSCUpdateNormal") return std::make_shared<CPSCUpdateNormal>();
    if (className == "CPSCEmitterGeneric") return std::make_shared<CPSCEmitterGeneric>();
    if (className == "CPSCSpline") return std::make_shared<CPSCSpline>();
    if (className == "CPSCSingleSprite") return std::make_shared<CPSCSingleSprite>();
    if (className == "CPSCRenderMesh") return std::make_shared<CPSCRenderMesh>();
    if (className == "CPSCLight") return std::make_shared<CPSCLight>();
    if (className == "CPSCAttractor") return std::make_shared<CPSCAttractor>();
    if (className == "CPSCOrbit") return std::make_shared<CPSCOrbit>();
    if (className == "CPSCDecalRenderer") return std::make_shared<CPSCDecalRenderer>();

    return nullptr;
}

struct CParticleSystem {
    std::string Name;
    bool Enabled = true;
    bool ScaleParticles = true;
    C3DVector Scale = { 1.0f, 1.0f, 1.0f };
    std::vector<std::shared_ptr<CParticleComponent>> Components;

    void Parse(CParticleStream& stream) {
        Name = stream.ReadString();
        Enabled = stream.ReadEBOOL();
        ScaleParticles = stream.ReadEBOOL();
        Scale = stream.Read3DVector();

        uint32_t componentCount = stream.ReadULONG();
        for (uint32_t i = 0; i < componentCount; i++) {
            std::string className = stream.ReadString();
            uint32_t instanceID = stream.ReadULONG();
            bool enabled = stream.ReadEBOOL();

            auto component = CreateComponent(className);
            if (component) {
                component->ClassName = className;
                component->InstanceID = instanceID;
                component->Enabled = enabled;

                component->Parse(stream);
                Components.push_back(component);
            }
            else {
                std::cout << "ERROR: Unknown particle component: " << className << "\n";
            }

            uint8_t terminator = stream.ReadEBOOL();
        }
    }
};

struct CParticleEmitter {
    uint32_t Magic = 0x64;
    std::string Name;
    bool Emitter2D, PreWaterEffect, WaterEffect, ZBufferWriteable;
    bool ContinuousEmitter, IsScreenDisplacement, ReadZBuffer;
    float MaxSpawnDistance, MaxDrawDistance;
    float FadeOutStart, FadeInEnd, FadeInStart;
    int32_t Priority;
    bool DieIfOffscreen, OffscreenUpdate, ClipEffectToWeatherMask;
    bool EnableDithering, CalcBoundingSphereOnceOnly;

    std::vector<CParticleSystem> Systems;

    void Parse(CParticleStream& stream) {
        Systems.clear(); // <--- ADD THIS FIX HERE

        Magic = stream.ReadULONG();
        Name = stream.ReadString();
        Emitter2D = stream.ReadEBOOL(); 
        PreWaterEffect = stream.ReadEBOOL(); 
        WaterEffect = stream.ReadEBOOL();
        ZBufferWriteable = stream.ReadEBOOL(); ContinuousEmitter = stream.ReadEBOOL(); IsScreenDisplacement = stream.ReadEBOOL();
        ReadZBuffer = stream.ReadEBOOL();
        MaxSpawnDistance = stream.ReadFloat(); MaxDrawDistance = stream.ReadFloat();
        FadeOutStart = stream.ReadFloat(); FadeInEnd = stream.ReadFloat(); FadeInStart = stream.ReadFloat();
        Priority = stream.ReadSLONG();
        DieIfOffscreen = stream.ReadEBOOL(); OffscreenUpdate = stream.ReadEBOOL(); ClipEffectToWeatherMask = stream.ReadEBOOL();
        EnableDithering = stream.ReadEBOOL(); CalcBoundingSphereOnceOnly = stream.ReadEBOOL();

        uint32_t systemCount = stream.ReadULONG();
        for (uint32_t i = 0; i < systemCount; i++) {
            CParticleSystem sys;
            sys.Parse(stream);
            Systems.push_back(sys);

            uint8_t terminator = stream.ReadEBOOL();
        }
    }
};