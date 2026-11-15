#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cstring>


struct C3DVector { float X = 0.0f, Y = 0.0f, Z = 0.0f; };
struct CRGBColour { uint8_t B = 255, G = 255, R = 255, A = 255; };

struct COrbitData {
    int32_t Type = 0;
    bool Enabled = false;
    float Radius = 1.0f;
    float Expand = 0.0f;
    bool CycleFlag = false;
    float CycleTime = 1.0f;
    float SqueezeScale = 1.0f;
    float SqueezeAngle = 0.0f;
    float RotateSpeed = 1.0f;
    float RotateStart = 0.0f;
    float RotateSpeedRandom = 0.0f;
    float RotateStartRandom = 0.0f;
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
        ControlPoints.clear();
        RandomOffsets.clear();
        PosParams.clear();

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
    int32_t SpriteBankIndex = 0, TrailBankIndex = 0;
    CRGBColour StartColour, MidColour, EndColour;

    uint32_t BlendMode = 2, TrailBlendMode = 2, BlendOp = 0, TrailBlendOp = 0;
    uint32_t SpriteFlags = 0, FadeInEndInteger = 0, FadeOutBeginInteger = 1000;
    uint32_t TrailLengthInteger = 0, FlickerMinAlphaInteger = 1000, FlickerMinSizeInteger = 1000, NoCrossedSprites = 1;

    float StartRenderSize = 1.0f, AlphaFadeMinimum = 1.0f, EndRenderSize = 1.0f;
    float FlickerBias = 0.0f, AnimationTimeSecs = 1.0f, SizeFadeMinimum = 1.0f;
    float TrailWidth = 1.0f, FlickerSpeed = 1.0f;

    bool UseStartColour = true, UseMidColour = false, UseEndColour = false;
    bool AlphaFadeEnable = false, SizeFadeEnable = false, FlickerEnable = false, ForceAnimationTime = false;

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
    uint32_t EmitterPosParam = 0, DirectionParamName = 0;
    uint32_t NoParticlesToStart = 0, NoParticlesToStartRand = 0, EmitterType = 0;

    bool Solid = false, UseEmitterLifeSecs = false, UseEmitterTimelineSecs = false;
    bool OrientationXY = true, OrientationXZ = false, OrientationYZ = false;
    bool UseCustomDirection = false, UseOutwardDirection = false, UseParamDirection = false, OppositeDirection = false;

    uint32_t AngularPerturbationInteger = 0;

    C3DVector CustomDirection = { 0.0f, 0.0f, 1.0f };
    float ParticlesPerSecond = 10.0f, EmitterSize = 1.0f, RadialBias = 0.0f, MinSpeed = 0.0f;

    bool UseForwardDirection = false, UseRandom2DDirection = false;

    float EmitterTimelineSecs = 1.0f;
    bool UseRandom3DDirection = false;

    float EmitterLifeSecs = 1.0f, EmitterStartTime = 0.0f, MaxSpeed = 1.0f;
    C3DVector NonUniformScaling = { 1.0f, 1.0f, 1.0f };

    bool HasSpline = false;
    float SplineTension = 0.0f;
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

        SplineControlPoints.clear();
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
    uint32_t FadeInEndInteger = 0, FadeOutBeginInteger = 1000;
    bool UseParticleLifeSecs = true, UseRandomRotationAxis = false, UseRandomInitialRotation = false, UseAccelerationParam = false;
    bool StayWithEmitter = false, UseSystemLifeSecs = false, SystemAlphaFadeEnable = false, EmissionFadeEnable = false;
    bool ParticleCollideWithGround = false, ParticleCollideWithAnything = false, ParticleDieOnCollision = false;
    bool UseAllAttractors = false, RandomisePosEnable = false, RandomisePosDistVaryEnable = false;
    bool SetOrientationFromDirection = false, SetOrientationFromGame = false, ParticleCreateDecal = false, ParticleCreateDecalEmitter = false;
    std::string DecalEmitterName;
    float SystemLifeSecs = 1.0f, ParticleLifeSecs = 1.0f, WindFactor = 0.0f, GravityFactor = 0.0f, AirResistance = 0.0f, ParticleAccelerationScale = 1.0f;
    float InitialRotationX = 0.0f, InitialRotationY = 0.0f, InitialRotationZ = 0.0f;
    float RotationMinAngleSpeed = 0.0f, RotationMaxAngleSpeed = 0.0f, ParticleBounce = 0.0f;
    float SystemAlphaFadeMinimum = 1.0f, RandomisePosDistVaryScale = 1.0f, EmissionFadeMinimum = 1.0f;
    C3DVector ParticleSystemOffset, ParticleAcceleration, RotationAxis = { 0.0f, 0.0f, 1.0f }, RandomisePosSpeed, RandomisePosScale;
    uint32_t AccelerationParam = 0, OrientFromGameParam = 0;

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
    float DecalLifeSecs = 1.0f;
    int32_t DecalBankIndex = 0;
    CRGBColour StartColour, MidColour, EndColour;

    uint32_t BlendMode = 2, BlendOp = 0, SpriteAlignment = 0, FadeInEndInteger = 0, FadeOutBeginInteger = 1000;
    uint32_t StartRenderSizeInteger = 102, AlphaFadeMinimumInteger = 127, EndRenderSizeInteger = 102;
    uint32_t AnimationTimeSecsInteger = 148, SizeFadeMinimumInteger = 127;

    bool UseStartColour = true, UseMidColour = false, UseEndColour = false;
    bool AlphaFadeEnable = false, SizeFadeEnable = false, ForceAnimationTime = false;

    uint32_t FlickerMinAlphaInteger = 1000, FlickerMinSizeInteger = 1000, FlickerBiasInteger = 128, FlickerSpeedInteger = 137;

    bool FlickerEnable = false, PoolingEnable = false, PoolIncreaseAlphaEnable = false;
    bool PoolIncreaseSizeEnable = false, PoolIncreaseLifeEnable = false;

    float MaxPoolSize = 1.0f, MaxPoolAlpha = 1.0f, MaxPoolLife = 1.0f;
    bool PoolIncreaseFrameEnable = false;
    float PoolFrameIncreaseRate = 0.0f, StencilCubeWidth = 1.0f;

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
    bool SplineBounce = false, ScaleSplineSpeed = false;
    float SplineAnimSpeed = 1.0f;

    void Parse(CParticleStream& stream) override {
        SplineBounce = stream.ReadEBOOL();
        ScaleSplineSpeed = stream.ReadEBOOL();
        SplineAnimSpeed = stream.ReadFloat();
        ParseSplineBase(stream);
    }
};

struct CPSCSingleSprite : public CParticleComponent, public CPSCSplineBase {
    int32_t SpriteBankIndex = 0;
    CRGBColour StartColour, MidColour, EndColour;
    float AnimationTimeSecs = 1.0f, StartRenderSize = 1.0f, EndRenderSize = 1.0f;
    int32_t SpriteAlignment = 0, NoCrossedSprites = 1;
    float InitialAngle = 0.0f;
    int32_t FadeInEndInteger = 0, FadeOutBeginInteger = 1000;
    float AlphaFadeMinimum = 1.0f, SizeFadeMinimum = 1.0f;
    uint32_t PositionParam = 0;
    int32_t BlendMode = 2, BlendOp = 0, TrailBlendMode = 2, TrailBlendOp = 0, TrailBankIndex = 0;
    float TrailWidth = 1.0f;
    int32_t MaxTrailLength = 0, StayWithEmitterIntFactor = 1000;

    bool UseStartColour = true, UseMidColour = false, UseEndColour = false;
    bool SelfIlluminating = false, AlphaFadeEnable = false, RotateAroundCentre = false;
    bool FaceMe2D = false, FaceMe3D = true, CrossedSprites = false;
    bool SizeFadeEnable = false, ForceAnimationTime = false, StayWithEmitter = false;
    bool UsePosition = false, UseSplinePoints = false;

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
    uint32_t CentreParam = 0;
    std::vector<COrbitData> OrbitsData;

    CPSCOrbit() {
        OrbitsData.resize(3);
    }

    void Parse(CParticleStream& stream) override {
        CentreParam = stream.ReadULONG();
        OrbitsData.clear();

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
    bool AttractorEnabled = true, AttractorUseParamPosition = false;
    int32_t AttractorInfluenceFallOff = 5;
    uint32_t AttractorPositionParam = 0, AttractorPositionParamName = 0;
    float AttractorInfluenceRadius = 1.0f, AttractorInfluenceForce = 1.0f;
    std::vector<C3DVector> AttractorUserPointsArray;

    void Parse(CParticleStream& stream) override {
        AttractorUserPointsArray.clear();
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
    uint32_t LightPositionParam = 0;
    float LightLifeSecs = 1.0f, LightRespawnDelaySecs = 0.0f, LightStartTime = 0.0f, LightTimelineSecs = 1.0f;
    float LightStartRenderWorldRadius = 1.0f, LightEndRenderWorldRadius = 1.0f;
    int32_t LightAttenuationFactor = 1, LightFadeInEndInteger = 0, LightFadeOutBeginInteger = 1000;
    float LightWorldRadiusFadeMinimum = 1.0f;
    CRGBColour LightColourFadeMinimum;

    bool LightWorldRadiusFadeEnable = false, LightUseLifeSecs = false, LightEnabled = true, LightRespawns = false;
    bool LightColourFadeEnable = false, LightUseStartColour = true, LightUseMidColour = false, LightUseEndColour = false;

    bool LightUseFadeColour = false, LightUseTimelineSecs = false, LightHasInitializedTime = false;

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
    int32_t BankIndex = 0, TrailBankIndex = 0;
    CRGBColour TrailStartColour, TrailMidColour, TrailEndColour;
    CRGBColour StartColour, MidColour, EndColour;

    uint32_t BlendMode = 2, TrailBlendMode = 2, BlendOp = 0, TrailBlendOp = 0;
    uint32_t FadeInEndInteger = 0, FadeOutBeginInteger = 1000;
    uint32_t TrailLengthInteger = 0, FlickerMinAlphaInteger = 1000, FlickerMinSizeInteger = 1000;

    C3DVector StartRenderSize = { 1.0f, 1.0f, 1.0f };
    bool CentredOnPos = false, AlphaFadeEnable = false;

    C3DVector EndRenderSize = { 1.0f, 1.0f, 1.0f };
    bool SizeFadeEnable = false, FlickerEnable = false;
    bool TrailUseStartColour = true, TrailUseMidColour = false, TrailUseEndColour = false, UseRenderSizeParam = false;

    float FlickerBias = 0.0f, FlickerSpeed = 1.0f, AlphaFadeMinimum = 1.0f;
    bool UseStartColour = true;

    float SizeFadeMinimum = 1.0f;
    bool UseMidColour = false;

    float TrailWidth = 1.0f;
    bool UseEndColour = false;

    uint32_t RenderSizeParam = 0;

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
        Components.clear();
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
    bool Emitter2D = false, PreWaterEffect = false, WaterEffect = false, ZBufferWriteable = false;
    bool ContinuousEmitter = true, IsScreenDisplacement = false, ReadZBuffer = true;
    float MaxSpawnDistance = 100.0f, MaxDrawDistance = 100.0f;
    float FadeOutStart = 80.0f, FadeInEnd = 0.0f, FadeInStart = 0.0f;
    int32_t Priority = 0;
    bool DieIfOffscreen = false, OffscreenUpdate = false, ClipEffectToWeatherMask = false;
    bool EnableDithering = false, CalcBoundingSphereOnceOnly = false;

    std::vector<CParticleSystem> Systems;

    void Parse(CParticleStream& stream) {
        Systems.clear();

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