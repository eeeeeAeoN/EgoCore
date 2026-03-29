#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cstring>

// --- HELPER STRUCTURES ---
struct C3DVector { float X, Y, Z; };
struct CRGBColour { uint8_t B, G, R, A; }; // BGRA in memory

// --- BINARY STREAM READER ---
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
        Offset++; // Skip the null terminator
        return str;
    }
};

// --- BASE COMPONENT ---
struct CParticleComponent {
    std::string ClassName;
    uint32_t InstanceID = 0;
    bool Enabled = true;
    bool Visible = true;

    virtual ~CParticleComponent() = default;
    virtual void Parse(CParticleStream& stream) = 0;
};

// --- SPLINE BASE (Inherited by Spline and SingleSprite) ---
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

// --- COMPONENT IMPLEMENTATIONS ---

struct CPSCRenderSprite : public CParticleComponent {
    int32_t SpriteBankIndex, TrailBankIndex;
    CRGBColour StartColour, MidColour, EndColour;

    uint32_t BlendMode, TrailBlendMode, BlendOp, TrailBlendOp;
    uint32_t SpriteAlignment, NoCrossedSprites;
    uint32_t FadeInEndInteger, FadeOutBeginInteger;
    uint32_t FlickerMinAlphaInteger, FlickerMinSizeInteger;

    float StartRenderSize;
    float EndRenderSize;
    float AlphaFadeMinimum;
    float SizeFadeMinimum;
    float FlickerSpeed;
    float FlickerBias;

    uint32_t TrailLengthInteger;
    float TrailWidth;
    float AnimationTimeSecs;

    bool FlagUseStartCol, FlagUseMidCol, FlagUseEndCol;
    bool FlagAlphaFade, FlagSizeFade, FlagFlicker, FlagForceAnimTime;

    void Parse(CParticleStream& stream) override {
        SpriteBankIndex = stream.ReadSLONG();
        TrailBankIndex = stream.ReadSLONG();
        StartColour = stream.ReadColour();
        MidColour = stream.ReadColour();
        EndColour = stream.ReadColour();

        BlendMode = stream.ReadULONG(); TrailBlendMode = stream.ReadULONG();
        BlendOp = stream.ReadULONG(); TrailBlendOp = stream.ReadULONG();
        SpriteAlignment = stream.ReadULONG(); NoCrossedSprites = stream.ReadULONG();
        FadeInEndInteger = stream.ReadULONG(); FadeOutBeginInteger = stream.ReadULONG();
        FlickerMinAlphaInteger = stream.ReadULONG(); FlickerMinSizeInteger = stream.ReadULONG();

        StartRenderSize = stream.ReadULONG() / 100.0f;
        EndRenderSize = stream.ReadULONG() / 100.0f;
        AlphaFadeMinimum = stream.ReadULONG() / 100.0f;
        SizeFadeMinimum = stream.ReadULONG() / 100.0f;
        FlickerSpeed = (float)stream.ReadULONG();
        FlickerBias = stream.ReadULONG() / 100.0f;

        TrailLengthInteger = stream.ReadULONG();
        TrailWidth = stream.ReadULONG() / 100.0f;
        AnimationTimeSecs = stream.ReadULONG() / 10.0f;

        FlagUseStartCol = stream.ReadEBOOL(); FlagUseMidCol = stream.ReadEBOOL();
        FlagUseEndCol = stream.ReadEBOOL(); FlagAlphaFade = stream.ReadEBOOL();
        FlagSizeFade = stream.ReadEBOOL(); FlagFlicker = stream.ReadEBOOL();
        FlagForceAnimTime = stream.ReadEBOOL();
    }
};

struct CPSCEmitterGeneric : public CParticleComponent {
    uint32_t EmitterPosParam, DirectionParamName;
    uint32_t NoParticlesToStart, NoParticlesToStartRand, EmitterType;

    bool Solid, UseEmitterLifeSecs, UseEmitterTimelineSecs;
    bool OrientationXY, OrientationXZ, OrientationYZ;
    bool UseCustomDirection, UseOutwardDirection, UseForwardDirection, UseRandom2DDirection;

    uint32_t AngularPerturbationInteger;
    C3DVector CustomDirection;
    float ParticlesPerSecond, EmitterSize, RadialBias, MinSpeed;

    bool UseRandom3DDirection, UseParamDirection;
    float EmitterTimelineSecs;
    bool OppositeDirection;

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
        UseForwardDirection = stream.ReadEBOOL();
        UseRandom2DDirection = stream.ReadEBOOL();

        AngularPerturbationInteger = stream.ReadULONG();

        // Fixed-Point Linear Quantization Decoding
        CustomDirection.X = stream.ReadULONG() / 255.0f * 2.0f - 1.0f;
        ParticlesPerSecond = stream.ReadULONG() / 16383.0f * 100.0f;
        EmitterSize = stream.ReadULONG() / 1023.0f * 10.0f;
        RadialBias = stream.ReadULONG() / 1023.0f * 10.0f - 5.0f;
        MinSpeed = stream.ReadULONG() / 1023.0f * 10.0f;

        UseRandom3DDirection = stream.ReadEBOOL();
        UseParamDirection = stream.ReadEBOOL();

        CustomDirection.Y = stream.ReadULONG() / 255.0f * 2.0f - 1.0f;
        CustomDirection.Z = stream.ReadULONG() / 255.0f * 2.0f - 1.0f;

        EmitterTimelineSecs = stream.ReadULONG() / 32767.0f * 30.0f;
        OppositeDirection = stream.ReadEBOOL();

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
    uint32_t ParamA, ParamB;
    bool Flags[18];
    std::string DecalEmitterName;
    float SystemLifeSecs, ParticleLifeSecs, WindFactor, GravityFactor, AirResistance, ParticleAccelerationScale;
    float InitialRotationX, InitialRotationY, InitialRotationZ;
    float RotationMinAngleSpeed, RotationMaxAngleSpeed, ParticleBounce;
    float SystemAlphaFadeMinimum, RandomisePosDistVaryScale, EmissionFadeMinimum;
    C3DVector ParticleSystemOffset, ParticleAcceleration, RotationAxis, RandomisePosSpeed, RandomisePosScale;
    uint32_t AccelerationParam, OrientFromGameParam;

    void Parse(CParticleStream& stream) override {
        ParamA = stream.ReadULONG(); ParamB = stream.ReadULONG();
        for (int i = 0; i < 18; i++) Flags[i] = stream.ReadEBOOL();

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

    uint32_t U1[10];
    bool B1[6];
    uint32_t U2[4];
    bool B2[5];

    float MaxPoolSize, MaxPoolAlpha, MaxPoolLife;
    bool Flag53;
    float PoolFrameIncreaseRate, StencilCubeWidth;

    void Parse(CParticleStream& stream) override {
        DecalLifeSecs = stream.ReadFloat();
        DecalBankIndex = stream.ReadSLONG();
        StartColour = stream.ReadColour();
        MidColour = stream.ReadColour();
        EndColour = stream.ReadColour();

        for (int i = 0; i < 10; i++) U1[i] = stream.ReadULONG();
        for (int i = 0; i < 6; i++) B1[i] = stream.ReadEBOOL();
        for (int i = 0; i < 4; i++) U2[i] = stream.ReadULONG();
        for (int i = 0; i < 5; i++) B2[i] = stream.ReadEBOOL();

        MaxPoolSize = stream.ReadFloat();
        MaxPoolAlpha = stream.ReadFloat();
        MaxPoolLife = stream.ReadFloat();
        Flag53 = stream.ReadEBOOL();
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

    bool Flags[12];
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

        for (int i = 0; i < 12; i++) Flags[i] = stream.ReadEBOOL();

        UsePosition = stream.ReadEBOOL(); UseSplinePoints = stream.ReadEBOOL();
        ParseSplineBase(stream);
    }
};

struct CPSCOrbit : public CParticleComponent {
    uint32_t CentreParam;
    std::vector<float> OrbitsData;

    void Parse(CParticleStream& stream) override {
        CentreParam = stream.ReadULONG();
        for (int i = 0; i < 36; i++) {
            OrbitsData.push_back(stream.ReadFloat());
        }
    }
};

struct CPSCAttractor : public CParticleComponent {
    bool FlagA, FlagB;
    int32_t AttractorInfluenceFallOff;
    uint32_t AttractorPositionParam, AttractorPositionParamName;
    float AttractorInfluenceRadius, AttractorInfluenceForce;
    std::vector<C3DVector> AttractorUserPointsArray;

    void Parse(CParticleStream& stream) override {
        FlagA = stream.ReadEBOOL(); FlagB = stream.ReadEBOOL();
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
    bool Flags72[8];
    bool Flags73[3];
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

        for (int i = 0; i < 8; i++) Flags72[i] = stream.ReadEBOOL();
        for (int i = 0; i < 3; i++) Flags73[i] = stream.ReadEBOOL();

        LightStartColour = stream.ReadColour();
        LightMidColour = stream.ReadColour();
        LightEndColour = stream.ReadColour();
    }
};

struct CPSCRenderMesh : public CParticleComponent {
    int32_t BankIndex, TrailBankIndex;
    CRGBColour TrailStartColour, TrailMidColour, TrailEndColour;
    CRGBColour StartColour, MidColour, EndColour;

    uint32_t U1[12];
    bool B1, B2;
    uint32_t U2[3];
    bool B3[6];
    uint32_t U3[3];
    bool B4;
    uint32_t U4; bool B5; uint32_t U5; bool B6;
    uint32_t RenderSizeParam;

    void Parse(CParticleStream& stream) override {
        BankIndex = stream.ReadSLONG(); TrailBankIndex = stream.ReadSLONG();
        TrailStartColour = stream.ReadColour(); TrailMidColour = stream.ReadColour(); TrailEndColour = stream.ReadColour();
        StartColour = stream.ReadColour(); MidColour = stream.ReadColour(); EndColour = stream.ReadColour();

        for (int i = 0; i < 12; i++) U1[i] = stream.ReadULONG();
        B1 = stream.ReadEBOOL(); B2 = stream.ReadEBOOL();
        for (int i = 0; i < 3; i++) U2[i] = stream.ReadULONG();
        for (int i = 0; i < 6; i++) B3[i] = stream.ReadEBOOL();
        for (int i = 0; i < 3; i++) U3[i] = stream.ReadULONG();
        B4 = stream.ReadEBOOL();
        U4 = stream.ReadULONG(); B5 = stream.ReadEBOOL();
        U5 = stream.ReadULONG(); B6 = stream.ReadEBOOL();
        RenderSizeParam = stream.ReadULONG();
    }
};

// --- COMPONENT FACTORY ---
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

// --- SYSTEM & EMITTER HIERARCHY ---

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

            // Magic terminator byte 0x7B ('{')
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
        Magic = stream.ReadULONG();
        Name = stream.ReadString();
        Emitter2D = stream.ReadEBOOL(); PreWaterEffect = stream.ReadEBOOL(); WaterEffect = stream.ReadEBOOL();
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

            // System Magic terminator byte 0x26 ('&')
            uint8_t terminator = stream.ReadEBOOL();
        }
    }
};