#pragma once
#include "ParticleParser.h"
#include <directxmath.h>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <random>

using namespace DirectX;

// Fable NFractal::CPerlinNoise & LCG PRNG helper (0x1447700, 0x1447e10, 0x1447f50)
struct FablePerlinNoise {
    static constexpr uint32_t TableSize = 512;

    static inline uint32_t NextLCG(uint32_t& seed) {
        seed = 9377u * seed + 9439u;
        seed = (seed >> 13) | (seed << 19);
        return seed;
    }

    static inline float LCGFloat01(uint32_t& seed) {
        return (float)(NextLCG(seed) >> 8) / 16777216.0f;
    }

    static inline float LCGFloatRange(uint32_t& seed, float minVal, float maxVal) {
        return minVal + LCGFloat01(seed) * (maxVal - minVal);
    }

    struct Tables {
        uint8_t IndexTable[TableSize];
        XMFLOAT2 VectorTable[TableSize];

        Tables() {
            uint32_t seed = 0x1337BEEFu;
            for (uint32_t i = 0; i < 256; ++i) {
                IndexTable[i] = (uint8_t)i;
            }
            for (uint32_t i = 255; i > 0; --i) {
                uint32_t j = NextLCG(seed) % (i + 1);
                uint8_t tmp = IndexTable[i];
                IndexTable[i] = IndexTable[j];
                IndexTable[j] = tmp;
            }
            for (uint32_t i = 0; i < 256; ++i) {
                IndexTable[256 + i] = IndexTable[i];
            }

            for (uint32_t i = 0; i < 256; ++i) {
                float angle = LCGFloat01(seed) * XM_2PI;
                VectorTable[i] = XMFLOAT2(std::cos(angle), std::sin(angle));
            }
            for (uint32_t i = 0; i < 256; ++i) {
                VectorTable[256 + i] = VectorTable[i];
            }
        }
    };

    static inline const Tables& GetTables() {
        static const Tables s_Tables;
        return s_Tables;
    }

    static inline const uint8_t* GetIndexTable() { return GetTables().IndexTable; }
    static inline const XMFLOAT2* GetVectorTable() { return GetTables().VectorTable; }

    static inline float GetGaussian2DNoise(float x, float y) {
        const Tables& tbl = GetTables();

        float fxFloor = std::floor(x);
        float fyFloor = std::floor(y);
        int ix = ((int)fxFloor) & 255;
        int iy = ((int)fyFloor) & 255;

        float fx = x - fxFloor;
        float fy = y - fyFloor;

        float sx = fx * fx * (3.0f - 2.0f * fx);
        float sy = fy * fy * (3.0f - 2.0f * fy);

        const XMFLOAT2& g00 = tbl.VectorTable[tbl.IndexTable[ix] + iy];
        const XMFLOAT2& g10 = tbl.VectorTable[tbl.IndexTable[ix + 1] + iy];
        const XMFLOAT2& g01 = tbl.VectorTable[tbl.IndexTable[ix] + iy + 1];
        const XMFLOAT2& g11 = tbl.VectorTable[tbl.IndexTable[ix + 1] + iy + 1];

        float d00 = g00.x * fx          + g00.y * fy;
        float d10 = g10.x * (fx - 1.0f) + g10.y * fy;
        float d01 = g01.x * fx          + g01.y * (fy - 1.0f);
        float d11 = g11.x * (fx - 1.0f) + g11.y * (fy - 1.0f);

        float nx0 = d00 + sx * (d10 - d00);
        float nx1 = d01 + sx * (d11 - d01);
        return nx0 + sy * (nx1 - nx0);
    }
};

// Representation of a live particle in simulation
struct SimulatedParticle {
    XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 PositionBuffer[4] = {}; // 4-step history for sub-frame interpolation
    XMFLOAT3 Velocity = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 Acceleration = { 0.0f, 0.0f, 0.0f };

    // Orientation quaternion and delta quaternion per tick
    XMFLOAT4 Orientation = { 0.0f, 0.0f, 0.0f, 1.0f };
    XMFLOAT4 OrientationChange = { 0.0f, 0.0f, 0.0f, 1.0f };

    float Angle = 0.0f;
    float AngleSpeed = 0.0f;
    int8_t AngleChange = 0;  // Angular delta per tick: scaled by (2.0 / 127.0)
    int16_t Lifetimer = 0;   // Integer tick lifecycle counter
    uint8_t CreationTime = 0;// Sub-tick spawn phase (0..255)
    int16_t MaxLifeTicks = 60;// Total particle lifespan in 30Hz ticks
    float Age = 0.0f;        // in seconds
    float MaxLife = 1.0f;    // in seconds
    float Size = 1.0f;
    XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };

    // System transform attachment
    bool StayWithEmitter = false;

    // Perlin noise flicker offsets
    uint8_t FlickerIndex = 0;
    uint8_t FlickerTimeOffset = 0;

    // Perlin noise RandomisePos state
    float RandPosTimeOffset[3] = { 0.0f, 0.0f, 0.0f };
    uint8_t RandPosIndex[3] = { 0, 0, 0 };
    XMFLOAT3 OldRandPosOffset = { 0.0f, 0.0f, 0.0f };

    uint32_t RandomSeed = 0;
    bool Active = false;
};

// Representation of a live ground decal (CPSCDecalRenderer)
struct SimulatedDecal {
    XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 Normal = { 0.0f, 0.0f, 1.0f };
    XMFLOAT3 Forward = { 1.0f, 0.0f, 0.0f };
    float Age = 0.0f;
    float MaxLife = 1.0f;
    float SizeScale = 1.0f;
    float AlphaScale = 1.0f;
    float FrameOffset = 0.0f;
    uint8_t FlickerIndex = 0;
    uint8_t FlickerTimeOffset = 0;
    bool Active = false;
};

// Simulation state for a single CParticleSystem
struct SystemSimulationState {
    std::vector<SimulatedParticle> Particles;
    float SpawnAccumulator = 0.0f;
    float SystemTime = 0.0f;
    bool BurstSpawned = false;
    float SystemAlpha = 1.0f;

    // Single sprite persistent particle (for CPSCSingleSprite)
    SimulatedParticle SingleSpriteParticle;
    float SingleSpriteProgress = 0.0f;
    bool HasSingleSprite = false;
    std::vector<XMFLOAT3> SingleSpriteTrailPoints;

    // CPSCOrbit state
    XMFLOAT3 OrbitOffset = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 PrevOrbitOffset = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 OrbitForward = { 1.0f, 0.0f, 0.0f };
    XMFLOAT3 OrbitStagePositions[3] = {};
    XMFLOAT3 OrbitStageTangents[3] = {};
    float OrbitAngles[3] = { 0.0f, 0.0f, 0.0f };
    float OrbitSpeedRand[3] = { 0.0f, 0.0f, 0.0f };
    bool OrbitInitialized = false;

    // Resolved emitter origin tracking (for StayWithEmitter)
    XMFLOAT3 EmitterOrigin = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 PrevEmitterOrigin = { 0.0f, 0.0f, 0.0f };

    // CPSCSpline / CPSCSingleSprite spline state
    float SplineParam = 0.0f;
    bool SplineReverse = false;
    std::vector<XMFLOAT3> WorkingSplinePoints;
    std::vector<XMFLOAT3> WorkingSingleSpriteSplinePoints;
    bool SplineInitialized = false;
    XMFLOAT3 SplineOffset = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 PrevSplineOffset = { 0.0f, 0.0f, 0.0f };

    // CPSCLight state
    bool LightActive = false;
    float LightTimer = 0.0f;
    float LightRespawnTimer = 0.0f;
    XMFLOAT3 LightPosition = { 0.0f, 0.0f, 0.0f };

    // CPSCDecalRenderer state
    std::vector<SimulatedDecal> Decals;
};

class ParticleSimulator {
private:
    static constexpr size_t kMaxDecalsPerSystem = 64;
    std::mt19937 Rng;
    uint32_t SimSeed = 1337u;

    float RandFloat(float minVal = 0.0f, float maxVal = 1.0f) {
        std::uniform_real_distribution<float> dist(minVal, maxVal);
        return dist(Rng);
    }

    XMFLOAT3 RandUnitVector3() {
        float theta = RandFloat(0.0f, XM_2PI);
        float z = RandFloat(-1.0f, 1.0f);
        float r = std::sqrt((std::max)(0.0f, 1.0f - z * z));
        return XMFLOAT3(r * std::cos(theta), r * std::sin(theta), z);
    }

    XMFLOAT2 RandUnitVector2() {
        float theta = RandFloat(0.0f, XM_2PI);
        return XMFLOAT2(std::cos(theta), std::sin(theta));
    }

    XMFLOAT3 RandEllipsoidOffset(const C3DVector& limit) {
        XMFLOAT3 u = RandUnitVector3();
        float r = std::cbrt(RandFloat(0.0f, 1.0f));
        return XMFLOAT3(u.x * r * limit.X, u.y * r * limit.Y, u.z * r * limit.Z);
    }

    static XMFLOAT3 ApplyChangeOrientation(const XMFLOAT3& v, const CPSCEmitterGeneric& eg) {
        if (eg.OrientationXZ) {
            return XMFLOAT3(v.x, -v.z, v.y);
        }
        if (eg.OrientationYZ) {
            return XMFLOAT3(-v.z, v.x, v.y);
        }
        return v; // OrientationXY (default)
    }

    static void ClampOffsetToEllipsoid(XMFLOAT3& delta, const C3DVector& limit) {
        float nx = (limit.X > 0.0001f) ? (delta.x / limit.X) : 0.0f;
        float ny = (limit.Y > 0.0001f) ? (delta.y / limit.Y) : 0.0f;
        float nz = (limit.Z > 0.0001f) ? (delta.z / limit.Z) : 0.0f;
        if (limit.X <= 0.0001f) delta.x = 0.0f;
        if (limit.Y <= 0.0001f) delta.y = 0.0f;
        if (limit.Z <= 0.0001f) delta.z = 0.0f;
        float lenSq = nx * nx + ny * ny + nz * nz;
        if (lenSq > 1.0f) {
            float invLen = 1.0f / std::sqrt(lenSq);
            delta.x *= invLen;
            delta.y *= invLen;
            delta.z *= invLen;
        }
    }

    // Cosine smoothstep fade over [0, FadeInEndInteger*0.001] and [FadeOutBeginInteger*0.001, 1]
    static float GetCurrentFadeValue(float systemTime, float systemLifeSecs, uint32_t fadeInEndInt, uint32_t fadeOutBeginInt) {
        if (systemLifeSecs <= 0.0001f) return 1.0f;
        float t = std::clamp(systemTime / systemLifeSecs, 0.0f, 1.0f);
        float fadeInEnd = (float)fadeInEndInt * 0.001f;
        float fadeOutBegin = (float)fadeOutBeginInt * 0.001f;

        float fade = 1.0f;
        if (fadeInEnd > 0.0001f && t < fadeInEnd) {
            float u = std::clamp(t / fadeInEnd, 0.0f, 1.0f);
            fade *= 0.5f - 0.5f * std::cos(XM_PI * u);
        }
        if (fadeOutBegin < 0.9999f && t > fadeOutBegin) {
            float denom = 1.0f - fadeOutBegin;
            float u = (denom > 0.0001f) ? std::clamp((1.0f - t) / denom, 0.0f, 1.0f) : 0.0f;
            fade *= 0.5f - 0.5f * std::cos(XM_PI * u);
        }
        return std::clamp(fade, 0.0f, 1.0f);
    }

    template <typename VecT>
    static XMFLOAT3 EvaluateCatmullRom(const std::vector<VecT>& points, float t) {
        if (points.empty()) return XMFLOAT3(0.0f, 0.0f, 0.0f);
        auto getPos = [](const VecT& pt) -> XMFLOAT3 {
            if constexpr (std::is_same_v<VecT, C3DVector>) {
                return XMFLOAT3(pt.X, pt.Y, pt.Z);
            } else {
                return pt;
            }
        };
        if (points.size() == 1) return getPos(points[0]);

        t = std::clamp(t, 0.0f, 1.0f);
        float scaledT = t * (float)(points.size() - 1);
        int idx = (int)scaledT;
        if (idx >= (int)points.size() - 1) idx = (int)points.size() - 2;
        float u = scaledT - (float)idx;

        int i0 = (std::max)(0, idx - 1);
        int i1 = idx;
        int i2 = idx + 1;
        int i3 = (std::min)((int)points.size() - 1, idx + 2);

        XMFLOAT3 f0 = getPos(points[i0]);
        XMFLOAT3 f1 = getPos(points[i1]);
        XMFLOAT3 f2 = getPos(points[i2]);
        XMFLOAT3 f3 = getPos(points[i3]);

        XMVECTOR P0 = XMVectorSet(f0.x, f0.y, f0.z, 0.0f);
        XMVECTOR P1 = XMVectorSet(f1.x, f1.y, f1.z, 0.0f);
        XMVECTOR P2 = XMVectorSet(f2.x, f2.y, f2.z, 0.0f);
        XMVECTOR P3 = XMVectorSet(f3.x, f3.y, f3.z, 0.0f);

        XMVECTOR result = XMVectorCatmullRom(P0, P1, P2, P3, u);
        XMFLOAT3 out;
        XMStoreFloat3(&out, result);
        return out;
    }

    static float ComputeSplineArcLength(const std::vector<XMFLOAT3>& points) {
        float len = 0.0f;
        for (size_t i = 1; i < points.size(); ++i) {
            float dx = points[i].x - points[i - 1].x;
            float dy = points[i].y - points[i - 1].y;
            float dz = points[i].z - points[i - 1].z;
            len += std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        return len;
    }

    void InitParticleSeedAndPerlin(SimulatedParticle& p) {
        p.RandomSeed = FablePerlinNoise::NextLCG(SimSeed);
        if (p.RandomSeed == 0) p.RandomSeed = 1u;

        uint32_t s = p.RandomSeed;
        p.FlickerIndex = (uint8_t)(FablePerlinNoise::NextLCG(s) & 0xFFu);
        p.FlickerTimeOffset = (uint8_t)(FablePerlinNoise::NextLCG(s) & 0xFFu);

        for (int k = 0; k < 3; ++k) {
            p.RandPosIndex[k] = (uint8_t)(FablePerlinNoise::NextLCG(s) & 0xFFu);
            p.RandPosTimeOffset[k] = (float)(FablePerlinNoise::NextLCG(s) & 0xFFu);
        }
        p.OldRandPosOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);
        p.RandomSeed = s;
    }

    static inline const uint32_t* GetFableCRC32Table() {
        static uint32_t table[256];
        static bool tableComputed = false;
        if (!tableComputed) {
            for (uint32_t i = 0; i < 256; ++i) {
                uint32_t c = i;
                for (int j = 0; j < 8; ++j) {
                    c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                }
                table[i] = c;
            }
            tableComputed = true;
        }
        return table;
    }

public:
    static inline uint32_t ExtendCRC32_Fable(uint32_t baseCrc, const char* suffix) {
        if (!suffix) return baseCrc;
        const uint32_t* table = GetFableCRC32Table();
        uint32_t crc = baseCrc;
        for (const char* p = suffix; *p != '\0'; ++p) {
            crc = table[(uint8_t)crc ^ (uint8_t)(*p)] ^ (crc >> 8);
        }
        return crc;
    }

    static inline uint32_t CalculateCRC32_Fable(const char* str) {
        return ExtendCRC32_Fable(0u, str);
    }

    static inline uint32_t CalculateCRC32_Fable(const std::string& str) {
        const uint32_t* table = GetFableCRC32Table();
        uint32_t crc = 0u;
        for (char c : str) {
            crc = table[(uint8_t)crc ^ (uint8_t)c] ^ (crc >> 8);
        }
        return crc;
    }

private:
    XMFLOAT3 ResolvePositionParam(const CParticleSystem& sys, const SystemSimulationState& state,
                                  uint32_t paramCrc, const XMFLOAT3& sysOffset,
                                  const XMFLOAT3& fallbackPos, bool allowOrbit = true) const {
        if (paramCrc == 0) {
            return fallbackPos;
        }

        static const uint32_t kSystemOffsetCrc = CalculateCRC32_Fable("System Offset");
        if (paramCrc == kSystemOffsetCrc) {
            return sysOffset;
        }

        for (const auto& comp : sys.Components) {
            if (!comp || !comp->Enabled) continue;

            const uint32_t baseCrc = comp->InstanceID;
            const uint32_t centreCrc = ExtendCRC32_Fable(baseCrc, ".Centre");

            if (comp->ClassName == "CPSCOrbit") {
                if (allowOrbit) {
                    if (paramCrc == ExtendCRC32_Fable(baseCrc, ".Primary")) {
                        return state.OrbitStagePositions[0];
                    }
                    if (paramCrc == ExtendCRC32_Fable(baseCrc, ".Secondary")) {
                        return state.OrbitStagePositions[1];
                    }
                    if (paramCrc == ExtendCRC32_Fable(baseCrc, ".Tertiary")) {
                        return state.OrbitStagePositions[2];
                    }
                }
                if (paramCrc == centreCrc || paramCrc == baseCrc) {
                    auto orbitComp = std::dynamic_pointer_cast<CPSCOrbit>(comp);
                    if (orbitComp && orbitComp->CentreParam != 0) {
                        return ResolvePositionParam(sys, state, orbitComp->CentreParam, sysOffset, sysOffset, false);
                    }
                    return sysOffset;
                }
            } else if (comp->ClassName == "CPSCSpline") {
                if (paramCrc == baseCrc ||
                    paramCrc == centreCrc ||
                    paramCrc == ExtendCRC32_Fable(baseCrc, ".Position") ||
                    paramCrc == ExtendCRC32_Fable(baseCrc, ".Spline")) {
                    return XMFLOAT3(
                        state.SplineOffset.x + sysOffset.x,
                        state.SplineOffset.y + sysOffset.y,
                        state.SplineOffset.z + sysOffset.z
                    );
                }
            } else if (comp->ClassName == "CPSCSingleSprite") {
                if (paramCrc == baseCrc || paramCrc == centreCrc) {
                    if (state.HasSingleSprite) {
                        return state.SingleSpriteParticle.Position;
                    }
                    auto ssComp = std::dynamic_pointer_cast<CPSCSingleSprite>(comp);
                    if (ssComp && !ssComp->ControlPoints.empty()) {
                        return XMFLOAT3(
                            ssComp->ControlPoints[0].X + sysOffset.x,
                            ssComp->ControlPoints[0].Y + sysOffset.y,
                            ssComp->ControlPoints[0].Z + sysOffset.z
                        );
                    }
                    return sysOffset;
                }
            } else if (comp->ClassName == "CPSCLight") {
                if (paramCrc == baseCrc || paramCrc == centreCrc) {
                    return state.LightPosition;
                }
            } else if (comp->ClassName == "CPSCAttractor") {
                if (paramCrc == baseCrc || paramCrc == centreCrc) {
                    auto at = std::dynamic_pointer_cast<CPSCAttractor>(comp);
                    if (at && !at->AttractorUserPointsArray.empty()) {
                        return XMFLOAT3(
                            at->AttractorUserPointsArray[0].X + sysOffset.x,
                            at->AttractorUserPointsArray[0].Y + sysOffset.y,
                            at->AttractorUserPointsArray[0].Z + sysOffset.z
                        );
                    }
                    return sysOffset;
                }
            } else if (paramCrc == baseCrc || paramCrc == centreCrc) {
                return sysOffset;
            }
        }

        return fallbackPos;
    }

    XMFLOAT3 ResolveForwardFromParam(const CParticleSystem& sys, const SystemSimulationState& state, uint32_t paramCrc) const {
        if (paramCrc != 0) {
            for (const auto& comp : sys.Components) {
                if (!comp || !comp->Enabled || comp->ClassName != "CPSCOrbit") continue;
                const uint32_t baseCrc = comp->InstanceID;
                if (paramCrc == ExtendCRC32_Fable(baseCrc, ".Primary")) return state.OrbitStageTangents[0];
                if (paramCrc == ExtendCRC32_Fable(baseCrc, ".Secondary")) return state.OrbitStageTangents[1];
                if (paramCrc == ExtendCRC32_Fable(baseCrc, ".Tertiary")) return state.OrbitStageTangents[2];
            }
        }
        return state.OrbitInitialized ? state.OrbitForward : XMFLOAT3(0.0f, 1.0f, 0.0f);
    }

    void EvaluateOrbitOffset(const CParticleSystem& sys, SystemSimulationState& state,
                             const std::shared_ptr<CPSCOrbit>& orbitComp,
                             const XMFLOAT3& sysOffset, float dt, bool advanceAngles) {
        if (!orbitComp) {
            state.OrbitOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);
            state.OrbitForward = XMFLOAT3(1.0f, 0.0f, 0.0f);
            for (int k = 0; k < 3; ++k) {
                state.OrbitStagePositions[k] = sysOffset;
                state.OrbitStageTangents[k] = XMFLOAT3(1.0f, 0.0f, 0.0f);
            }
            return;
        }

        if (!state.OrbitInitialized) {
            for (int k = 0; k < 3; ++k) {
                if ((size_t)k < orbitComp->OrbitsData.size()) {
                    const auto& orb = orbitComp->OrbitsData[k];
                    state.OrbitAngles[k] = orb.RotateStart + RandFloat(-orb.RotateStartRandom, orb.RotateStartRandom);
                    state.OrbitSpeedRand[k] = RandFloat(-orb.RotateSpeedRandom, orb.RotateSpeedRandom);
                } else {
                    state.OrbitAngles[k] = 0.0f;
                    state.OrbitSpeedRand[k] = 0.0f;
                }
            }
            state.OrbitInitialized = true;
        }

        XMFLOAT3 orbitWorldCentre = sysOffset;
        if (orbitComp->CentreParam != 0) {
            orbitWorldCentre = ResolvePositionParam(sys, state, orbitComp->CentreParam, sysOffset, sysOffset, false);
        }
        XMVECTOR centreVec = XMLoadFloat3(&orbitWorldCentre);

        XMMATRIX worldRot = XMMatrixIdentity();
        XMVECTOR worldPos = XMVectorZero();
        XMVECTOR lastWorldTangent = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
        bool hasActiveOrbit = false;

        for (int k = 0; k < 3; ++k) {
            if ((size_t)k >= orbitComp->OrbitsData.size() || !orbitComp->OrbitsData[k].Enabled) {
                XMStoreFloat3(&state.OrbitStagePositions[k], XMVectorAdd(centreVec, worldPos));
                XMStoreFloat3(&state.OrbitStageTangents[k], lastWorldTangent);
                continue;
            }

            const auto& orb = orbitComp->OrbitsData[k];

            if (advanceAngles) {
                state.OrbitAngles[k] += (orb.RotateSpeed + state.OrbitSpeedRand[k]) * dt;
            }

            float radius = orb.Radius;
            if (orb.CycleFlag) {
                if (orb.CycleTime > 0.0001f) {
                    float phase = (1.0f - std::cos(XM_2PI * state.SystemTime / orb.CycleTime)) * 0.5f;
                    radius += orb.Expand * phase;
                }
            } else {
                if (orb.CycleTime > 0.0001f) {
                    float clampedT = (std::min)(state.SystemTime, orb.CycleTime);
                    float phase = std::sin(XM_2PI * (clampedT * 0.25f / orb.CycleTime));
                    radius += orb.Expand * phase;
                } else if (state.SystemTime > 0.0f) {
                    radius += orb.Expand;
                }
            }

            float ang = state.OrbitAngles[k];
            float cosA = std::cos(ang);
            float sinA = std::sin(ang);

            float lx = cosA * radius;
            float ly = sinA * radius * orb.SqueezeScale;
            float tx = -sinA;
            float ty = cosA * orb.SqueezeScale;

            float cosS = std::cos(orb.SqueezeAngle);
            float sinS = std::sin(orb.SqueezeAngle);

            float px = lx * cosS - ly * sinS;
            float py = lx * sinS + ly * cosS;
            float ptx = tx * cosS - ty * sinS;
            float pty = tx * sinS + ty * cosS;

            int planeType = orb.Type & 3;
            bool rotateWithParent = (k > 0) && ((orb.Type & 4) != 0);

            XMVECTOR localOffset;
            XMVECTOR localTangent;
            XMMATRIX stageRot;
            if (planeType == 1) { // XZ
                localOffset = XMVectorSet(px, 0.0f, py, 0.0f);
                localTangent = XMVectorSet(ptx, 0.0f, pty, 0.0f);
                stageRot = XMMatrixRotationY(-ang);
            } else if (planeType == 2) { // YZ
                localOffset = XMVectorSet(0.0f, px, py, 0.0f);
                localTangent = XMVectorSet(0.0f, ptx, pty, 0.0f);
                stageRot = XMMatrixRotationX(ang);
            } else { // 0 = XY
                localOffset = XMVectorSet(px, py, 0.0f, 0.0f);
                localTangent = XMVectorSet(ptx, pty, 0.0f, 0.0f);
                stageRot = XMMatrixRotationZ(ang);
            }

            XMVECTOR stageWorldStep = rotateWithParent ? XMVector3TransformNormal(localOffset, worldRot) : localOffset;
            XMVECTOR stageWorldTan  = rotateWithParent ? XMVector3TransformNormal(localTangent, worldRot) : localTangent;

            worldPos = XMVectorAdd(worldPos, stageWorldStep);
            worldRot = rotateWithParent ? XMMatrixMultiply(stageRot, worldRot) : stageRot;

            float tLenSq = XMVectorGetX(XMVector3LengthSq(stageWorldTan));
            if (tLenSq > 1e-8f) {
                lastWorldTangent = XMVectorScale(stageWorldTan, 1.0f / std::sqrt(tLenSq));
            }

            XMStoreFloat3(&state.OrbitStagePositions[k], XMVectorAdd(centreVec, worldPos));
            XMStoreFloat3(&state.OrbitStageTangents[k], lastWorldTangent);
            hasActiveOrbit = true;
        }

        XMStoreFloat3(&state.OrbitOffset, worldPos);
        if (hasActiveOrbit) {
            XMStoreFloat3(&state.OrbitForward, lastWorldTangent);
        } else {
            state.OrbitForward = XMFLOAT3(1.0f, 0.0f, 0.0f);
        }
    }

    void UpdateSingleSpriteState(const CParticleSystem& sys, SystemSimulationState& state,
                                 const std::shared_ptr<CPSCSingleSprite>& ssComp,
                                 const XMFLOAT3& sysOffset, float dt, bool isReset) {
        if (!ssComp || !ssComp->Enabled || (ssComp->SpriteBankIndex <= 0 && ssComp->TrailBankIndex <= 0)) {
            state.HasSingleSprite = false;
            state.SingleSpriteParticle.Active = false;
            state.SingleSpriteTrailPoints.clear();
            return;
        }

        if (!state.HasSingleSprite || isReset) {
            state.HasSingleSprite = true;
            state.SingleSpriteProgress = 0.0f;
            state.SingleSpriteParticle = SimulatedParticle();
            state.SingleSpriteParticle.Active = true;
            state.SingleSpriteParticle.MaxLife = 10000.0f;
            state.SingleSpriteParticle.MaxLifeTicks = 30000;
            InitParticleSeedAndPerlin(state.SingleSpriteParticle);
        }

        auto& sp = state.SingleSpriteParticle;
        sp.Active = true;

        if (!isReset) {
            sp.PositionBuffer[3] = sp.PositionBuffer[2];
            sp.PositionBuffer[2] = sp.PositionBuffer[1];
            sp.PositionBuffer[1] = sp.PositionBuffer[0];
        }

        UpdateSplineWorkingPoints(*ssComp, state.WorkingSingleSpriteSplinePoints, isReset || !state.SplineInitialized, dt, !isReset);

        if (!isReset) {
            float animTime = (ssComp->AnimationTimeSecs > 0.01f) ? ssComp->AnimationTimeSecs : 3.0f;
            state.SingleSpriteProgress += dt / animTime;
            if (state.SingleSpriteProgress > 1.0f) {
                state.SingleSpriteProgress = std::fmod(state.SingleSpriteProgress, 1.0f);
            }
        }

        // Per Fable CPSCSingleSprite: sprite stays stationary at ControlPoints[0] + sysOffset (or PositionParam if UsePosition)
        XMFLOAT3 basePt = { 0.0f, 0.0f, 0.0f };
        if (!state.WorkingSingleSpriteSplinePoints.empty()) {
            basePt = state.WorkingSingleSpriteSplinePoints[0];
        } else if (!ssComp->ControlPoints.empty()) {
            basePt = XMFLOAT3(ssComp->ControlPoints[0].X, ssComp->ControlPoints[0].Y, ssComp->ControlPoints[0].Z);
        }
        XMFLOAT3 defaultPos = XMFLOAT3(basePt.x + sysOffset.x, basePt.y + sysOffset.y, basePt.z + sysOffset.z);

        sp.Position = ssComp->UsePosition
            ? ResolvePositionParam(sys, state, ssComp->PositionParam, sysOffset, defaultPos)
            : defaultPos;

        if (isReset) {
            sp.PositionBuffer[0] = sp.Position;
            sp.PositionBuffer[1] = sp.Position;
            sp.PositionBuffer[2] = sp.Position;
            sp.PositionBuffer[3] = sp.Position;
        } else {
            sp.PositionBuffer[0] = sp.Position;
        }

        sp.Angle = ssComp->InitialAngle;
        sp.MaxLifeTicks = 30000;
        sp.Lifetimer = isReset ? 0 : (int16_t)(std::min)(30000.0f, state.SystemTime * 30.0f);

        // Populate full spline trail ribbon when trail rendering is enabled
        state.SingleSpriteTrailPoints.clear();
        const auto& splinePts = state.WorkingSingleSpriteSplinePoints;
        if (ssComp->MaxTrailLength > 0 && splinePts.size() >= 2) {
            if (ssComp->UseSplinePoints) {
                state.SingleSpriteTrailPoints.reserve(splinePts.size());
                for (const auto& pt : splinePts) {
                    state.SingleSpriteTrailPoints.push_back(XMFLOAT3(pt.x + sysOffset.x, pt.y + sysOffset.y, pt.z + sysOffset.z));
                }
            } else {
                const int sampleCount = std::clamp((int)(splinePts.size() - 1) * 8, 16, 64);
                state.SingleSpriteTrailPoints.reserve((size_t)sampleCount + 1);
                for (int s = 0; s <= sampleCount; ++s) {
                    float t = (float)s / (float)sampleCount;
                    XMFLOAT3 pt = EvaluateCatmullRom(splinePts, t);
                    state.SingleSpriteTrailPoints.push_back(XMFLOAT3(pt.x + sysOffset.x, pt.y + sysOffset.y, pt.z + sysOffset.z));
                }
            }
        }
    }

    void UpdateSplineWorkingPoints(const CPSCSplineBase& base, std::vector<XMFLOAT3>& workingPoints, bool applyStartupOffsets, float dt, bool advanceTick) {
        if (workingPoints.size() != base.ControlPoints.size()) {
            workingPoints.resize(base.ControlPoints.size());
            for (size_t i = 0; i < base.ControlPoints.size(); ++i) {
                workingPoints[i] = XMFLOAT3(base.ControlPoints[i].X, base.ControlPoints[i].Y, base.ControlPoints[i].Z);
                if (applyStartupOffsets && base.HasRandomOffsetData && base.OffsetOnlyAtStartup && !base.RandomOffsets.empty()) {
                    const auto& ro = base.RandomOffsets[(std::min)(i, base.RandomOffsets.size() - 1)];
                    XMFLOAT3 off = RandEllipsoidOffset(ro.Limit);
                    workingPoints[i].x += off.x;
                    workingPoints[i].y += off.y;
                    workingPoints[i].z += off.z;
                }
            }
        }

        if (advanceTick && base.HasRandomOffsetData && !base.OffsetOnlyAtStartup && !base.RandomOffsets.empty()) {
            for (size_t i = 0; i < workingPoints.size(); ++i) {
                const auto& ro = base.RandomOffsets[(std::min)(i, base.RandomOffsets.size() - 1)];
                const C3DVector& basePt = base.ControlPoints[i];

                if (base.ResetEachFrame) {
                    XMFLOAT3 off = RandEllipsoidOffset(ro.Limit);
                    workingPoints[i] = XMFLOAT3(basePt.X + off.x, basePt.Y + off.y, basePt.Z + off.z);
                } else {
                    XMFLOAT3 dir = RandUnitVector3();
                    workingPoints[i].x += dir.x * ro.Speed.X * dt;
                    workingPoints[i].y += dir.y * ro.Speed.Y * dt;
                    workingPoints[i].z += dir.z * ro.Speed.Z * dt;

                    XMFLOAT3 delta = {
                        workingPoints[i].x - basePt.X,
                        workingPoints[i].y - basePt.Y,
                        workingPoints[i].z - basePt.Z
                    };
                    ClampOffsetToEllipsoid(delta, ro.Limit);
                    workingPoints[i] = XMFLOAT3(basePt.X + delta.x, basePt.Y + delta.y, basePt.Z + delta.z);
                }
            }
        }
    }

    void SpawnDecalAt(SystemSimulationState& state, const std::shared_ptr<CPSCDecalRenderer>& decalComp, float x, float y) {
        if (!decalComp || !decalComp->Enabled) return;

        if (decalComp->PoolingEnable && !state.Decals.empty()) {
            float poolRadius = (decalComp->StencilCubeWidth > 0.01f) ? (decalComp->StencilCubeWidth * 0.5f) : 1.0f;
            float poolRadiusSq = poolRadius * poolRadius;
            for (auto& d : state.Decals) {
                if (!d.Active) continue;
                float dx = d.Position.x - x;
                float dy = d.Position.y - y;
                if (dx * dx + dy * dy <= poolRadiusSq) {
                    if (decalComp->PoolIncreaseSizeEnable) {
                        float maxSize = (decalComp->MaxPoolSize > 1.0f) ? decalComp->MaxPoolSize : 2.0f;
                        d.SizeScale = (std::min)(d.SizeScale + 0.1f, maxSize);
                    }
                    if (decalComp->PoolIncreaseAlphaEnable) {
                        float maxAlpha = (decalComp->MaxPoolAlpha > 1.0f) ? decalComp->MaxPoolAlpha : 2.0f;
                        d.AlphaScale = (std::min)(d.AlphaScale + 0.1f, maxAlpha);
                    }
                    if (decalComp->PoolIncreaseLifeEnable) {
                        float maxLife = (decalComp->MaxPoolLife > d.MaxLife) ? decalComp->MaxPoolLife : (d.MaxLife * 2.0f);
                        d.MaxLife = (std::min)(d.MaxLife + 0.25f, maxLife);
                    }
                    if (decalComp->PoolIncreaseFrameEnable) {
                        d.FrameOffset += decalComp->PoolFrameIncreaseRate;
                    }
                    return;
                }
            }
        }

        if (state.Decals.size() >= kMaxDecalsPerSystem) {
            state.Decals.erase(state.Decals.begin());
        }

        SimulatedDecal decal;
        decal.Position = XMFLOAT3(x, y, 0.0f);
        decal.Normal = XMFLOAT3(0.0f, 0.0f, 1.0f);
        float yaw = FablePerlinNoise::LCGFloatRange(SimSeed, 0.0f, XM_2PI);
        decal.Forward = XMFLOAT3(std::cos(yaw), std::sin(yaw), 0.0f);
        decal.Age = 0.0f;
        decal.MaxLife = (decalComp->DecalLifeSecs > 0.01f) ? decalComp->DecalLifeSecs : 5.0f;
        decal.SizeScale = 1.0f;
        decal.AlphaScale = 1.0f;
        decal.FrameOffset = 0.0f;
        decal.FlickerIndex = (uint8_t)(FablePerlinNoise::NextLCG(SimSeed) & 0xFFu);
        decal.FlickerTimeOffset = (uint8_t)(FablePerlinNoise::NextLCG(SimSeed) & 0xFFu);
        decal.Active = true;
        state.Decals.push_back(decal);
    }

public:
    std::vector<SystemSimulationState> SystemStates;
    float CurrentSimTime = 0.0f;
    float InterpolationFactor = 0.0f; // Sub-tick interpolation factor [0..1] for renderer
    float TimeAccumulator = 0.0f;     // Accumulator for 30Hz fixed simulation ticks
    int TotalActiveParticles = 0;

    ParticleSimulator() {
        Rng.seed(1337);
        SimSeed = 1337u;
    }

    void Reset(const CParticleEmitter& emitter) {
        Rng.seed(1337);
        SimSeed = 1337u;
        CurrentSimTime = 0.0f;
        InterpolationFactor = 0.0f;
        TimeAccumulator = 0.0f;
        TotalActiveParticles = 0;
        SystemStates.clear();
        SystemStates.resize(emitter.Systems.size());

        for (size_t i = 0; i < emitter.Systems.size(); i++) {
            const auto& sys = emitter.Systems[i];
            auto& state = SystemStates[i];

            state.SystemTime = 0.0f;
            state.SpawnAccumulator = 0.0f;
            state.BurstSpawned = false;
            state.SystemAlpha = 1.0f;
            state.HasSingleSprite = false;
            state.SingleSpriteProgress = 0.0f;
            state.SingleSpriteParticle = SimulatedParticle();
            state.SingleSpriteTrailPoints.clear();

            state.OrbitOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);
            state.PrevOrbitOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);
            state.OrbitForward = XMFLOAT3(1.0f, 0.0f, 0.0f);
            for (int k = 0; k < 3; ++k) {
                state.OrbitStagePositions[k] = XMFLOAT3(0.0f, 0.0f, 0.0f);
                state.OrbitStageTangents[k] = XMFLOAT3(1.0f, 0.0f, 0.0f);
                state.OrbitAngles[k] = 0.0f;
                state.OrbitSpeedRand[k] = 0.0f;
            }
            state.OrbitInitialized = false;

            state.EmitterOrigin = XMFLOAT3(0.0f, 0.0f, 0.0f);
            state.PrevEmitterOrigin = XMFLOAT3(0.0f, 0.0f, 0.0f);

            state.SplineParam = 0.0f;
            state.SplineReverse = false;
            state.WorkingSplinePoints.clear();
            state.WorkingSingleSpriteSplinePoints.clear();
            state.SplineInitialized = false;
            state.SplineOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);
            state.PrevSplineOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);

            state.LightActive = false;
            state.LightTimer = 0.0f;
            state.LightRespawnTimer = 0.0f;
            state.LightPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);

            state.Decals.clear();

            // Pre-allocate particle pool (up to 2048 per system)
            state.Particles.assign(2048, SimulatedParticle());

            std::shared_ptr<CPSCEmitterGeneric> emitterComp;
            std::shared_ptr<CPSCUpdateNormal> updateComp;
            std::shared_ptr<CPSCOrbit> orbitComp;
            std::shared_ptr<CPSCSpline> splineComp;
            std::shared_ptr<CPSCSingleSprite> ssComp;
            std::shared_ptr<CPSCLight> lightComp;

            for (const auto& comp : sys.Components) {
                if (!comp || !comp->Enabled) continue;
                if (comp->ClassName == "CPSCEmitterGeneric") emitterComp = std::dynamic_pointer_cast<CPSCEmitterGeneric>(comp);
                else if (comp->ClassName == "CPSCUpdateNormal") updateComp = std::dynamic_pointer_cast<CPSCUpdateNormal>(comp);
                else if (comp->ClassName == "CPSCOrbit") orbitComp = std::dynamic_pointer_cast<CPSCOrbit>(comp);
                else if (comp->ClassName == "CPSCSpline") splineComp = std::dynamic_pointer_cast<CPSCSpline>(comp);
                else if (comp->ClassName == "CPSCSingleSprite") ssComp = std::dynamic_pointer_cast<CPSCSingleSprite>(comp);
                else if (comp->ClassName == "CPSCLight") lightComp = std::dynamic_pointer_cast<CPSCLight>(comp);
            }

            XMFLOAT3 sysOffset = updateComp
                ? XMFLOAT3(updateComp->ParticleSystemOffset.X, updateComp->ParticleSystemOffset.Y, updateComp->ParticleSystemOffset.Z)
                : XMFLOAT3(0.0f, 0.0f, 0.0f);

            if (splineComp) {
                UpdateSplineWorkingPoints(*splineComp, state.WorkingSplinePoints, true, 0.0f, false);
                if (!state.WorkingSplinePoints.empty()) {
                    state.SplineOffset = EvaluateCatmullRom(state.WorkingSplinePoints, 0.0f);
                    state.PrevSplineOffset = state.SplineOffset;
                }
            }

            EvaluateOrbitOffset(sys, state, orbitComp, sysOffset, 0.0f, false);
            state.PrevOrbitOffset = state.OrbitOffset;

            XMFLOAT3 defaultOrigin = {
                sysOffset.x + state.OrbitOffset.x + state.SplineOffset.x,
                sysOffset.y + state.OrbitOffset.y + state.SplineOffset.y,
                sysOffset.z + state.OrbitOffset.z + state.SplineOffset.z
            };
            state.EmitterOrigin = emitterComp
                ? ResolvePositionParam(sys, state, emitterComp->EmitterPosParam, sysOffset, defaultOrigin)
                : defaultOrigin;
            state.PrevEmitterOrigin = state.EmitterOrigin;

            UpdateSingleSpriteState(sys, state, ssComp, sysOffset, 0.0f, true);
            state.SplineInitialized = true;

            if (lightComp && lightComp->LightEnabled) {
                state.LightPosition = ResolvePositionParam(sys, state, lightComp->LightPositionParam, sysOffset, state.EmitterOrigin);
            } else {
                state.LightPosition = state.EmitterOrigin;
            }

            // Perform initial burst spawn if enabled
            if (sys.Enabled) {
                SpawnBurst(emitter, i);
            }
        }
    }

    void SpawnBurst(const CParticleEmitter& emitter, size_t sysIndex) {
        if (sysIndex >= emitter.Systems.size() || sysIndex >= SystemStates.size()) return;
        const auto& sys = emitter.Systems[sysIndex];
        auto& state = SystemStates[sysIndex];

        for (const auto& comp : sys.Components) {
            if (comp && comp->ClassName == "CPSCEmitterGeneric" && comp->Enabled) {
                auto emitterComp = std::dynamic_pointer_cast<CPSCEmitterGeneric>(comp);
                if (emitterComp) {
                    uint32_t burstCount = emitterComp->NoParticlesToStart;
                    if (emitterComp->NoParticlesToStartRand > 0) {
                        burstCount += (uint32_t)(RandFloat(0.0f, (float)emitterComp->NoParticlesToStartRand + 0.999f));
                    }
                    if (burstCount > 512) burstCount = 512;
                    if (burstCount > 0) {
                        SpawnParticles(sys, state, emitterComp, (int)burstCount);
                    }
                    state.BurstSpawned = true;
                }
            }
        }
    }

    void Update(const CParticleEmitter& emitter, float dt) {
        if (dt <= 0.0f) return;

        // Disassembly: GetTickRate() = 30, GetOneOverTickRate() = 1/30s
        const float tickInterval = 1.0f / 30.0f;
        TimeAccumulator += (std::min)(dt, 0.2f); // Clamp extreme lag spikes

        while (TimeAccumulator >= tickInterval) {
            StepSimulation(emitter, tickInterval);
            TimeAccumulator -= tickInterval;
        }

        InterpolationFactor = std::clamp(TimeAccumulator / tickInterval, 0.0f, 1.0f);
        CurrentSimTime += dt;

        // Count total active particles
        TotalActiveParticles = 0;
        for (const auto& st : SystemStates) {
            for (const auto& p : st.Particles) {
                if (p.Active) TotalActiveParticles++;
            }
            if (st.HasSingleSprite && st.SingleSpriteParticle.Active) {
                TotalActiveParticles++;
            }
        }
    }

    void Seek(const CParticleEmitter& emitter, float targetTime) {
        Reset(emitter);
        if (targetTime <= 0.0f) return;

        // Simulate forward in fixed steps up to targetTime
        const float step = 1.0f / 30.0f;
        float t = 0.0f;
        while (t < targetTime) {
            float dt = (std::min)(step, targetTime - t);
            Update(emitter, dt);
            t += dt;
        }
    }

private:
    void StepSimulation(const CParticleEmitter& emitter, float dt) {
        for (size_t i = 0; i < emitter.Systems.size(); i++) {
            if (i >= SystemStates.size()) break;
            const auto& sys = emitter.Systems[i];
            auto& state = SystemStates[i];

            if (!sys.Enabled) {
                state.SingleSpriteTrailPoints.clear();
                continue;
            }
            state.SystemTime += dt;

            // Find key components
            std::shared_ptr<CPSCEmitterGeneric> emitterComp;
            std::shared_ptr<CPSCUpdateNormal> updateComp;
            std::shared_ptr<CPSCRenderSprite> renderSpriteComp;
            std::shared_ptr<CPSCSingleSprite> singleSpriteComp;
            std::shared_ptr<CPSCOrbit> orbitComp;
            std::vector<std::shared_ptr<CPSCAttractor>> attractors;
            std::shared_ptr<CPSCSpline> splineComp;
            std::shared_ptr<CPSCLight> lightComp;
            std::shared_ptr<CPSCDecalRenderer> decalComp;

            for (const auto& comp : sys.Components) {
                if (!comp || !comp->Enabled) continue;
                if (comp->ClassName == "CPSCEmitterGeneric") emitterComp = std::dynamic_pointer_cast<CPSCEmitterGeneric>(comp);
                else if (comp->ClassName == "CPSCUpdateNormal") updateComp = std::dynamic_pointer_cast<CPSCUpdateNormal>(comp);
                else if (comp->ClassName == "CPSCRenderSprite") renderSpriteComp = std::dynamic_pointer_cast<CPSCRenderSprite>(comp);
                else if (comp->ClassName == "CPSCSingleSprite") singleSpriteComp = std::dynamic_pointer_cast<CPSCSingleSprite>(comp);
                else if (comp->ClassName == "CPSCOrbit") orbitComp = std::dynamic_pointer_cast<CPSCOrbit>(comp);
                else if (comp->ClassName == "CPSCAttractor") {
                    auto at = std::dynamic_pointer_cast<CPSCAttractor>(comp);
                    if (at && at->AttractorEnabled) attractors.push_back(at);
                }
                else if (comp->ClassName == "CPSCSpline") splineComp = std::dynamic_pointer_cast<CPSCSpline>(comp);
                else if (comp->ClassName == "CPSCLight") lightComp = std::dynamic_pointer_cast<CPSCLight>(comp);
                else if (comp->ClassName == "CPSCDecalRenderer") decalComp = std::dynamic_pointer_cast<CPSCDecalRenderer>(comp);
            }

            XMFLOAT3 sysOffset = updateComp
                ? XMFLOAT3(updateComp->ParticleSystemOffset.X, updateComp->ParticleSystemOffset.Y, updateComp->ParticleSystemOffset.Z)
                : XMFLOAT3(0.0f, 0.0f, 0.0f);

            // 0. Update CPSCSpline & CPSCOrbit emitter-origin offsets (Spline first so Orbit.CentreParam can track Spline)
            state.PrevSplineOffset = state.SplineOffset;
            if (splineComp) {
                UpdateSplineWorkingPoints(*splineComp, state.WorkingSplinePoints, !state.SplineInitialized, dt, true);
                if (!state.WorkingSplinePoints.empty()) {
                    float step = splineComp->SplineAnimSpeed * dt;
                    if (splineComp->ScaleSplineSpeed) {
                        float arcLen = ComputeSplineArcLength(state.WorkingSplinePoints);
                        if (arcLen > 0.001f) step /= arcLen;
                    }
                    if (!state.SplineReverse) {
                        state.SplineParam += step;
                        if (state.SplineParam >= 1.0f) {
                            if (splineComp->SplineBounce) {
                                state.SplineParam = (std::max)(0.0f, 2.0f - state.SplineParam);
                                state.SplineReverse = true;
                            } else {
                                state.SplineParam = std::fmod(state.SplineParam, 1.0f);
                            }
                        }
                    } else {
                        state.SplineParam -= step;
                        if (state.SplineParam <= 0.0f) {
                            state.SplineParam = (std::min)(1.0f, -state.SplineParam);
                            state.SplineReverse = false;
                        }
                    }
                    state.SplineOffset = EvaluateCatmullRom(state.WorkingSplinePoints, state.SplineParam);
                } else {
                    state.SplineOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);
                }
            } else {
                state.SplineOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }

            state.PrevOrbitOffset = state.OrbitOffset;
            EvaluateOrbitOffset(sys, state, orbitComp, sysOffset, dt, true);

            state.PrevEmitterOrigin = state.EmitterOrigin;
            XMFLOAT3 defaultOrigin = {
                sysOffset.x + state.OrbitOffset.x + state.SplineOffset.x,
                sysOffset.y + state.OrbitOffset.y + state.SplineOffset.y,
                sysOffset.z + state.OrbitOffset.z + state.SplineOffset.z
            };
            XMFLOAT3 emitterOrigin = emitterComp
                ? ResolvePositionParam(sys, state, emitterComp->EmitterPosParam, sysOffset, defaultOrigin)
                : defaultOrigin;
            state.EmitterOrigin = emitterOrigin;

            XMFLOAT3 emitterDelta = {
                emitterOrigin.x - state.PrevEmitterOrigin.x,
                emitterOrigin.y - state.PrevEmitterOrigin.y,
                emitterOrigin.z - state.PrevEmitterOrigin.z
            };

            // System-level fades (GetCurrentFadeValue)
            float systemFade = 1.0f;
            if (updateComp && updateComp->UseSystemLifeSecs && updateComp->SystemLifeSecs > 0.0001f) {
                systemFade = GetCurrentFadeValue(state.SystemTime, updateComp->SystemLifeSecs,
                                                 updateComp->FadeInEndInteger, updateComp->FadeOutBeginInteger);
            }
            if (updateComp && updateComp->SystemAlphaFadeEnable) {
                float minAlpha = std::clamp(updateComp->SystemAlphaFadeMinimum, 0.0f, 1.0f);
                state.SystemAlpha = minAlpha + (1.0f - minAlpha) * systemFade;
            } else {
                state.SystemAlpha = 1.0f;
            }

            // 1. Continuous Emission (CPSCEmitterGeneric)
            if (emitterComp) {
                float effectiveTime = state.SystemTime;
                if (emitterComp->UseEmitterTimelineSecs) {
                    float timeline = (std::max)(emitterComp->EmitterTimelineSecs, 0.001f);
                    effectiveTime = std::fmod(state.SystemTime, timeline);
                }

                bool canEmit = true;
                if (effectiveTime < emitterComp->EmitterStartTime) {
                    canEmit = false;
                }
                if (emitterComp->UseEmitterLifeSecs && effectiveTime > (emitterComp->EmitterStartTime + emitterComp->EmitterLifeSecs)) {
                    canEmit = false;
                }
                if (updateComp && updateComp->UseSystemLifeSecs && updateComp->SystemLifeSecs > 0.0001f &&
                    state.SystemTime > updateComp->SystemLifeSecs) {
                    canEmit = false;
                }

                float emissionRate = emitterComp->ParticlesPerSecond;
                if (updateComp && updateComp->EmissionFadeEnable) {
                    float minEm = std::clamp(updateComp->EmissionFadeMinimum, 0.0f, 1.0f);
                    emissionRate *= (minEm + (1.0f - minEm) * systemFade);
                }

                if (canEmit && emissionRate > 0.0f) {
                    state.SpawnAccumulator += emissionRate * dt;
                    int countToSpawn = (int)state.SpawnAccumulator;
                    if (countToSpawn > 256) countToSpawn = 256;
                    if (countToSpawn > 0) {
                        state.SpawnAccumulator -= (float)countToSpawn;
                        SpawnParticles(sys, state, emitterComp, countToSpawn);
                    }
                }
            }

            // 2. Physics & Kinematics Update (CPSCUpdateNormal, CPSCAttractor)
            bool useAllAttractors = (updateComp && updateComp->UseAllAttractors);

            for (auto& p : state.Particles) {
                if (!p.Active) continue;

                p.Lifetimer++;
                p.Age += dt;
                if (p.Lifetimer >= p.MaxLifeTicks) {
                    p.Active = false;
                    continue;
                }

                // Gravity, Acceleration & Exponential Air Resistance from CPSCUpdateNormal
                if (updateComp) {
                    p.Velocity.z -= 9.82f * updateComp->GravityFactor * dt;

                    p.Velocity.x += updateComp->ParticleAcceleration.X * updateComp->ParticleAccelerationScale * dt;
                    p.Velocity.y += updateComp->ParticleAcceleration.Y * updateComp->ParticleAccelerationScale * dt;
                    p.Velocity.z += updateComp->ParticleAcceleration.Z * updateComp->ParticleAccelerationScale * dt;

                    if (updateComp->AirResistance > 0.0f) {
                        float clampedAir = std::clamp(updateComp->AirResistance, 0.0f, 0.9999f);
                        float drag = std::pow(1.0f - clampedAir, dt);
                        p.Velocity.x *= drag;
                        p.Velocity.y *= drag;
                        p.Velocity.z *= drag;
                    }

                    // 2D sprite angular delta (2.0 / 127.0 scale per tick)
                    p.Angle += (float)p.AngleChange * (2.0f / 127.0f);
                }

                // CPSCAttractor forces (0x13590d0)
                if (!attractors.empty()) {
                    const CPSCAttractor* bestAttractor = nullptr;
                    XMFLOAT3 bestPt = { 0.0f, 0.0f, 0.0f };
                    float bestRadius = 1e30f;
                    float bestDist = 0.0f;

                    for (const auto& at : attractors) {
                        float R = at->AttractorInfluenceRadius;
                        if (R <= 0.0001f) continue;

                        size_t ptCount = at->AttractorUserPointsArray.empty() ? 1 : at->AttractorUserPointsArray.size();
                        for (size_t pi = 0; pi < ptCount; ++pi) {
                            XMFLOAT3 targetPos = at->AttractorUserPointsArray.empty()
                                ? XMFLOAT3(0.0f, 0.0f, 0.0f)
                                : XMFLOAT3(at->AttractorUserPointsArray[pi].X, at->AttractorUserPointsArray[pi].Y, at->AttractorUserPointsArray[pi].Z);

                            float dx = targetPos.x - p.Position.x;
                            float dy = targetPos.y - p.Position.y;
                            float dz = targetPos.z - p.Position.z;
                            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

                            if (dist > 0.0001f && dist < R) {
                                if (useAllAttractors) {
                                    float factor = (R - dist) / R;
                                    float expVal = (at->AttractorInfluenceFallOff > 5)
                                        ? (float)(at->AttractorInfluenceFallOff - 5)
                                        : ((float)at->AttractorInfluenceFallOff / 5.0f);
                                    float mag = at->AttractorInfluenceForce * std::pow(factor, expVal);
                                    p.Velocity.x += (dx / dist) * mag * dt;
                                    p.Velocity.y += (dy / dist) * mag * dt;
                                    p.Velocity.z += (dz / dist) * mag * dt;
                                } else {
                                    if (R < bestRadius || (R == bestRadius && dist < bestDist)) {
                                        bestRadius = R;
                                        bestDist = dist;
                                        bestAttractor = at.get();
                                        bestPt = targetPos;
                                    }
                                }
                            }
                        }
                    }

                    if (!useAllAttractors && bestAttractor && bestDist > 0.0001f) {
                        float dx = bestPt.x - p.Position.x;
                        float dy = bestPt.y - p.Position.y;
                        float dz = bestPt.z - p.Position.z;
                        float factor = (bestRadius - bestDist) / bestRadius;
                        float expVal = (bestAttractor->AttractorInfluenceFallOff > 5)
                            ? (float)(bestAttractor->AttractorInfluenceFallOff - 5)
                            : ((float)bestAttractor->AttractorInfluenceFallOff / 5.0f);
                        float mag = bestAttractor->AttractorInfluenceForce * std::pow(factor, expVal);
                        p.Velocity.x += (dx / bestDist) * mag * dt;
                        p.Velocity.y += (dy / bestDist) * mag * dt;
                        p.Velocity.z += (dz / bestDist) * mag * dt;
                    }
                }

                // Update 3D orientation quaternion: Q = Q * dQ
                XMVECTOR qCurrent = XMLoadFloat4(&p.Orientation);
                XMVECTOR qChange = XMLoadFloat4(&p.OrientationChange);
                XMVECTOR qNext = XMQuaternionNormalize(XMQuaternionMultiply(qCurrent, qChange));
                XMStoreFloat4(&p.Orientation, qNext);

                // Set orientation from movement direction if enabled (Functions.txt:2310-2375)
                if (updateComp && updateComp->SetOrientationFromDirection) {
                    float speedSq = p.Velocity.x * p.Velocity.x + p.Velocity.y * p.Velocity.y + p.Velocity.z * p.Velocity.z;
                    if (speedSq > 0.0001f) {
                        float invSpeed = 1.0f / std::sqrt(speedSq);
                        XMFLOAT3 fwd = { p.Velocity.x * invSpeed, p.Velocity.y * invSpeed, p.Velocity.z * invSpeed };
                        XMFLOAT3 upRef = { 0.0f, 0.0f, 1.0f };
                        if (std::abs(fwd.z) > 0.99f) {
                            upRef = (std::abs(fwd.y) >= std::abs(fwd.x)) ? XMFLOAT3(1.0f, 0.0f, 0.0f) : XMFLOAT3(0.0f, 1.0f, 0.0f);
                        }
                        XMVECTOR vFwd = XMLoadFloat3(&fwd);
                        XMVECTOR vUpRef = XMLoadFloat3(&upRef);
                        XMVECTOR vRight = XMVector3Normalize(XMVector3Cross(vUpRef, vFwd));
                        XMVECTOR vUp = XMVector3Cross(vFwd, vRight);
                        XMMATRIX rotMat;
                        rotMat.r[0] = XMVectorSetW(vRight, 0.0f);
                        rotMat.r[1] = XMVectorSetW(vFwd, 0.0f);
                        rotMat.r[2] = XMVectorSetW(vUp, 0.0f);
                        rotMat.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
                        XMVECTOR q = XMQuaternionRotationMatrix(rotMat);
                        XMStoreFloat4(&p.Orientation, q);
                    }
                }

                // Shift position history buffer for sub-frame interpolation
                p.PositionBuffer[3] = p.PositionBuffer[2];
                p.PositionBuffer[2] = p.PositionBuffer[1];
                p.PositionBuffer[1] = p.PositionBuffer[0];

                float prevZ = p.Position.z;

                // Integrate position
                p.Position.x += p.Velocity.x * dt;
                p.Position.y += p.Velocity.y * dt;
                p.Position.z += p.Velocity.z * dt;

                // Follow emitter origin delta if StayWithEmitter
                if (p.StayWithEmitter) {
                    p.Position.x += emitterDelta.x;
                    p.Position.y += emitterDelta.y;
                    p.Position.z += emitterDelta.z;
                }

                // RandomisePos Perlin noise displacement
                if (updateComp && updateComp->RandomisePosEnable) {
                    float tX = (float)p.Lifetimer * updateComp->RandomisePosSpeed.X * dt + p.RandPosTimeOffset[0];
                    float tY = (float)p.Lifetimer * updateComp->RandomisePosSpeed.Y * dt + p.RandPosTimeOffset[1];
                    float tZ = (float)p.Lifetimer * updateComp->RandomisePosSpeed.Z * dt + p.RandPosTimeOffset[2];

                    XMFLOAT3 newOffset;
                    newOffset.x = FablePerlinNoise::GetGaussian2DNoise(tX, (float)p.RandPosIndex[0]) * updateComp->RandomisePosScale.X;
                    newOffset.y = FablePerlinNoise::GetGaussian2DNoise(tY, (float)p.RandPosIndex[1]) * updateComp->RandomisePosScale.Y;
                    newOffset.z = FablePerlinNoise::GetGaussian2DNoise(tZ, (float)p.RandPosIndex[2]) * updateComp->RandomisePosScale.Z;

                    XMFLOAT3 delta = {
                        newOffset.x - p.OldRandPosOffset.x,
                        newOffset.y - p.OldRandPosOffset.y,
                        newOffset.z - p.OldRandPosOffset.z
                    };
                    p.OldRandPosOffset = newOffset;

                    p.Position.x += delta.x;
                    p.Position.y += delta.y;
                    p.Position.z += delta.z;
                }

                // Ground Collision against Z = 0.0 plane (functions7.txt:181-198)
                if (updateComp && updateComp->ParticleCollideWithGround) {
                    if (p.Position.z <= 0.0f) {
                        if (decalComp && updateComp->ParticleCreateDecal && prevZ > 0.0f) {
                            SpawnDecalAt(state, decalComp, p.Position.x, p.Position.y);
                        }
                        p.Position.z = 0.0f;
                        if (updateComp->ParticleDieOnCollision) {
                            p.Active = false;
                            continue;
                        } else {
                            p.Velocity.z = -p.Velocity.z * (std::clamp)(updateComp->ParticleBounce, 0.0f, 1.0f);
                            p.Velocity.x *= 0.85f;
                            p.Velocity.y *= 0.85f;
                        }
                    }
                }

                p.PositionBuffer[0] = p.Position;
            }

            // 3. Update Persistent Single Sprite (CPSCSingleSprite)
            UpdateSingleSpriteState(sys, state, singleSpriteComp, sysOffset, dt, false);
            state.SplineInitialized = true;

            // 4. Update CPSCLight
            if (lightComp && lightComp->LightEnabled) {
                state.LightPosition = ResolvePositionParam(sys, state, lightComp->LightPositionParam, sysOffset, emitterOrigin);
                float effectiveTime = state.SystemTime;
                if (lightComp->LightUseTimelineSecs) {
                    float timeline = (std::max)(lightComp->LightTimelineSecs, 0.001f);
                    effectiveTime = std::fmod(state.SystemTime, timeline);
                }

                if (effectiveTime < lightComp->LightStartTime) {
                    state.LightActive = false;
                } else if (!lightComp->LightUseLifeSecs || lightComp->LightLifeSecs <= 0.0001f) {
                    state.LightActive = true;
                    state.LightTimer += dt;
                } else if (state.LightRespawnTimer > 0.0f) {
                    state.LightActive = false;
                    state.LightRespawnTimer -= dt;
                    if (state.LightRespawnTimer <= 0.0f) {
                        state.LightRespawnTimer = 0.0f;
                        state.LightTimer = 0.0f;
                        state.LightActive = true;
                    }
                } else {
                    if (state.LightTimer < lightComp->LightLifeSecs) {
                        state.LightActive = true;
                        state.LightTimer += dt;
                    } else {
                        state.LightActive = false;
                        if (lightComp->LightRespawns) {
                            if (lightComp->LightRespawnDelaySecs > 0.0f) {
                                state.LightRespawnTimer = lightComp->LightRespawnDelaySecs;
                            } else {
                                state.LightTimer = 0.0f;
                                state.LightActive = true;
                            }
                        }
                    }
                }
            } else {
                state.LightActive = false;
            }

            // 5. Update CPSCDecalRenderer (0x1360420)
            for (auto& d : state.Decals) {
                if (!d.Active) continue;
                d.Age += dt;
                if (d.MaxLife > 0.0f && d.Age >= d.MaxLife) {
                    d.Active = false;
                }
            }
            state.Decals.erase(
                std::remove_if(state.Decals.begin(), state.Decals.end(), [](const SimulatedDecal& d) { return !d.Active; }),
                state.Decals.end());

            if (decalComp && !(updateComp && updateComp->ParticleCreateDecal)) {
                if (state.Decals.empty()) {
                    SpawnDecalAt(state, decalComp, emitterOrigin.x, emitterOrigin.y);
                }
            }
        }
    }

    void SpawnParticles(const CParticleSystem& sys, SystemSimulationState& state,
                        const std::shared_ptr<CPSCEmitterGeneric>& emitterComp, int count) {
        if (!emitterComp || count <= 0) return;
        if (count > 512) count = 512;

        std::shared_ptr<CPSCUpdateNormal> updateComp;
        for (const auto& c : sys.Components) {
            if (c && c->ClassName == "CPSCUpdateNormal" && c->Enabled) {
                updateComp = std::dynamic_pointer_cast<CPSCUpdateNormal>(c);
                break;
            }
        }

        float lifeSecs = (updateComp && (!updateComp->UseParticleLifeSecs || updateComp->ParticleLifeSecs > 0.01f))
            ? ((updateComp->ParticleLifeSecs > 0.01f) ? updateComp->ParticleLifeSecs : 2.0f)
            : 1.0f;
        if (updateComp && !updateComp->UseParticleLifeSecs) {
            lifeSecs = (updateComp->ParticleLifeSecs > 0.01f) ? updateComp->ParticleLifeSecs : 100.0f;
        }

        XMFLOAT3 sysOffset = updateComp
            ? XMFLOAT3(updateComp->ParticleSystemOffset.X, updateComp->ParticleSystemOffset.Y, updateComp->ParticleSystemOffset.Z)
            : XMFLOAT3(0.0f, 0.0f, 0.0f);

        XMFLOAT3 defaultOrigin = {
            sysOffset.x + state.OrbitOffset.x + state.SplineOffset.x,
            sysOffset.y + state.OrbitOffset.y + state.SplineOffset.y,
            sysOffset.z + state.OrbitOffset.z + state.SplineOffset.z
        };
        XMFLOAT3 emitterOrigin = ResolvePositionParam(sys, state, emitterComp->EmitterPosParam, sysOffset, defaultOrigin);

        XMFLOAT3 nuScale = { emitterComp->NonUniformScaling.X, emitterComp->NonUniformScaling.Y, emitterComp->NonUniformScaling.Z };
        if (nuScale.x == 0.0f && nuScale.y == 0.0f && nuScale.z == 0.0f) {
            nuScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        }

        XMFLOAT3 sysScale = { sys.Scale.X, sys.Scale.Y, sys.Scale.Z };
        if (sysScale.x == 0.0f && sysScale.y == 0.0f && sysScale.z == 0.0f) {
            sysScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        }

        for (int i = 0; i < count; i++) {
            SimulatedParticle* target = nullptr;
            for (auto& p : state.Particles) {
                if (!p.Active) {
                    target = &p;
                    break;
                }
            }
            if (!target) {
                float maxAgeRatio = -1.0f;
                for (auto& p : state.Particles) {
                    float ratio = p.Age / p.MaxLife;
                    if (ratio > maxAgeRatio) {
                        maxAgeRatio = ratio;
                        target = &p;
                    }
                }
            }
            if (!target) return;

            target->Active = true;
            target->Acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
            target->Size = 1.0f;
            target->Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            target->Lifetimer = 0;
            target->CreationTime = (uint8_t)(RandFloat(0.0f, 1.0f) * 255.0f);
            target->MaxLifeTicks = (int16_t)(std::min)(30000.0f, lifeSecs * 30.0f + 0.5f);
            if (target->MaxLifeTicks < 1) target->MaxLifeTicks = 1;
            target->Age = 0.0f;
            target->MaxLife = lifeSecs;
            target->StayWithEmitter = (updateComp && updateComp->StayWithEmitter);

            InitParticleSeedAndPerlin(*target);

            // Compute Spawn Position based on emitter shape:
            // 0 = Point, 1 = 2D Disc, 2 = 3D Sphere, 3 = Spline
            XMFLOAT3 spawnPos = { 0.0f, 0.0f, 0.0f };
            XMFLOAT3 outwardDir = { 0.0f, 1.0f, 0.0f };
            float radius = emitterComp->EmitterSize * 0.5f;

            if (emitterComp->EmitterType == 1) { // 2D Disc
                XMFLOAT2 u2 = RandUnitVector2();
                float r = radius;
                if (emitterComp->Solid) {
                    r *= std::pow(RandFloat(0.0f, 1.0f), std::exp2(-emitterComp->RadialBias));
                }
                XMFLOAT3 localDisc(u2.x * r, u2.y * r, 0.0f);
                XMFLOAT3 localDir(u2.x, u2.y, 0.0f);
                spawnPos = ApplyChangeOrientation(localDisc, *emitterComp);
                outwardDir = ApplyChangeOrientation(localDir, *emitterComp);
            } else if (emitterComp->EmitterType == 2) { // 3D Sphere
                XMFLOAT3 u3 = RandUnitVector3();
                float r = radius;
                if (emitterComp->Solid) {
                    r *= std::pow(RandFloat(0.0f, 1.0f), std::exp2(-emitterComp->RadialBias));
                }
                spawnPos = ApplyChangeOrientation(XMFLOAT3(u3.x * r, u3.y * r, u3.z * r), *emitterComp);
                outwardDir = ApplyChangeOrientation(u3, *emitterComp);
            } else if (emitterComp->EmitterType == 3 && emitterComp->HasSpline && !emitterComp->SplineControlPoints.empty()) { // Spline
                spawnPos = ApplyChangeOrientation(EvaluateCatmullRom(emitterComp->SplineControlPoints, RandFloat(0.0f, 1.0f)), *emitterComp);
                outwardDir = ApplyChangeOrientation(RandUnitVector3(), *emitterComp);
            } else { // Point emitter
                spawnPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
                outwardDir = ApplyChangeOrientation(RandUnitVector3(), *emitterComp);
            }

            // Apply NonUniformScaling and system scale
            spawnPos.x *= nuScale.x * sysScale.x;
            spawnPos.y *= nuScale.y * sysScale.y;
            spawnPos.z *= nuScale.z * sysScale.z;

            // Translate by emitter origin (ParticleSystemOffset + OrbitOffset + SplineOffset)
            spawnPos.x += emitterOrigin.x;
            spawnPos.y += emitterOrigin.y;
            spawnPos.z += emitterOrigin.z;

            target->Position = spawnPos;
            target->PositionBuffer[0] = spawnPos;
            target->PositionBuffer[1] = spawnPos;
            target->PositionBuffer[2] = spawnPos;
            target->PositionBuffer[3] = spawnPos;

            // Compute Direction
            XMFLOAT3 dir = outwardDir;
            if (emitterComp->UseRandom3DDirection) {
                dir = RandUnitVector3();
            } else if (emitterComp->UseRandom2DDirection) {
                XMFLOAT2 d2 = RandUnitVector2();
                dir = ApplyChangeOrientation(XMFLOAT3(d2.x, d2.y, 0.0f), *emitterComp);
            } else if (emitterComp->UseCustomDirection) {
                dir = XMFLOAT3(emitterComp->CustomDirection.X, emitterComp->CustomDirection.Y, emitterComp->CustomDirection.Z);
            } else if (emitterComp->UseForwardDirection) {
                dir = ResolveForwardFromParam(sys, state, emitterComp->EmitterPosParam);
            } else if (emitterComp->UseOutwardDirection) {
                dir = outwardDir;
            }

            if (emitterComp->OppositeDirection) {
                dir.x = -dir.x;
                dir.y = -dir.y;
                dir.z = -dir.z;
            }

            XMVECTOR dirVec = XMVectorSet(dir.x, dir.y, dir.z, 0.0f);
            float lenSq = XMVectorGetX(XMVector3LengthSq(dirVec));
            if (lenSq < 0.0001f) {
                dirVec = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            } else {
                dirVec = XMVector3Normalize(dirVec);
            }

            // Apply angular perturbation: k = AngularPerturbationInteger * 0.001f, dir = normalize(dir + p_perp * k)
            if (emitterComp->AngularPerturbationInteger > 0) {
                float k = (float)emitterComp->AngularPerturbationInteger * 0.001f;
                XMFLOAT3 randU = RandUnitVector3();
                XMVECTOR vRand = XMLoadFloat3(&randU);
                XMVECTOR dot = XMVector3Dot(vRand, dirVec);
                XMVECTOR pPerp = XMVectorSubtract(vRand, XMVectorMultiply(dirVec, dot));
                if (XMVectorGetX(XMVector3LengthSq(pPerp)) > 1e-6f) {
                    dirVec = XMVector3Normalize(XMVectorAdd(dirVec, XMVectorScale(pPerp, k)));
                }
            }

            // Speed range: MinSpeed to MaxSpeed
            float minSpd = emitterComp->MinSpeed;
            float maxSpd = (std::max)(minSpd, emitterComp->MaxSpeed);
            float speed = RandFloat(minSpd, maxSpd);

            XMStoreFloat3(&target->Velocity, dirVec * speed);

            // 2D Rotation & 3D Orientation setup from CPSCUpdateNormal
            if (updateComp) {
                target->Angle = updateComp->UseRandomInitialRotation
                    ? RandFloat(0.0f, XM_2PI)
                    : updateComp->InitialRotationX;

                float minRot = (std::min)(updateComp->RotationMinAngleSpeed, updateComp->RotationMaxAngleSpeed);
                float maxRot = (std::max)(updateComp->RotationMinAngleSpeed, updateComp->RotationMaxAngleSpeed);
                target->AngleSpeed = RandFloat(minRot, maxRot);

                float angleChangeRad = target->AngleSpeed * (1.0f / 30.0f);
                target->AngleChange = (int8_t)std::clamp((int)(angleChangeRad * 63.5f), -128, 127);

                // Compute 3D rotation axis
                XMFLOAT3 rotAxis = { updateComp->RotationAxis.X, updateComp->RotationAxis.Y, updateComp->RotationAxis.Z };
                float axisLen = std::sqrt(rotAxis.x * rotAxis.x + rotAxis.y * rotAxis.y + rotAxis.z * rotAxis.z);
                if (updateComp->UseRandomRotationAxis || axisLen < 0.001f) {
                    rotAxis = RandUnitVector3();
                } else {
                    rotAxis.x /= axisLen;
                    rotAxis.y /= axisLen;
                    rotAxis.z /= axisLen;
                }

                XMVECTOR axisVec = XMLoadFloat3(&rotAxis);
                XMVECTOR dq = XMQuaternionRotationAxis(axisVec, angleChangeRad);
                XMStoreFloat4(&target->OrientationChange, dq);

                float rx = updateComp->UseRandomInitialRotation ? RandFloat(0.0f, XM_2PI) : updateComp->InitialRotationX;
                float ry = updateComp->UseRandomInitialRotation ? RandFloat(0.0f, XM_2PI) : updateComp->InitialRotationY;
                float rz = updateComp->UseRandomInitialRotation ? RandFloat(0.0f, XM_2PI) : updateComp->InitialRotationZ;
                XMMATRIX rotMat = XMMatrixRotationX(rx) * XMMatrixRotationY(ry) * XMMatrixRotationZ(rz);
                XMVECTOR initQ = XMQuaternionRotationMatrix(rotMat);
                XMStoreFloat4(&target->Orientation, initQ);
            } else {
                target->Angle = 0.0f;
                target->AngleSpeed = 0.0f;
                target->AngleChange = 0;
                target->Orientation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
                target->OrientationChange = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
            }
        }
    }
};
