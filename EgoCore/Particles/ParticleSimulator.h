#pragma once
#include "ParticleParser.h"
#include <directxmath.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

using namespace DirectX;

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

    uint32_t RandomSeed = 0;
    bool Active = false;
};

// Simulation state for a single CParticleSystem
struct SystemSimulationState {
    std::vector<SimulatedParticle> Particles;
    float SpawnAccumulator = 0.0f;
    float SystemTime = 0.0f;
    bool BurstSpawned = false;

    // Single sprite persistent particle (for CPSCSingleSprite)
    SimulatedParticle SingleSpriteParticle;
    float SingleSpriteProgress = 0.0f;
    bool HasSingleSprite = false;
};

class ParticleSimulator {
private:
    std::mt19937 Rng;

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

    // Catmull-Rom spline interpolation helper
    static XMFLOAT3 EvaluateCatmullRom(const std::vector<C3DVector>& points, float t) {
        if (points.empty()) return XMFLOAT3(0, 0, 0);
        if (points.size() == 1) return XMFLOAT3(points[0].X, points[0].Y, points[0].Z);

        t = std::clamp(t, 0.0f, 1.0f);
        float scaledT = t * (float)(points.size() - 1);
        int idx = (int)scaledT;
        if (idx >= (int)points.size() - 1) idx = (int)points.size() - 2;
        float u = scaledT - (float)idx;

        int i0 = (std::max)(0, idx - 1);
        int i1 = idx;
        int i2 = idx + 1;
        int i3 = (std::min)((int)points.size() - 1, idx + 2);

        XMVECTOR P0 = XMVectorSet(points[i0].X, points[i0].Y, points[i0].Z, 0.0f);
        XMVECTOR P1 = XMVectorSet(points[i1].X, points[i1].Y, points[i1].Z, 0.0f);
        XMVECTOR P2 = XMVectorSet(points[i2].X, points[i2].Y, points[i2].Z, 0.0f);
        XMVECTOR P3 = XMVectorSet(points[i3].X, points[i3].Y, points[i3].Z, 0.0f);

        XMVECTOR result = XMVectorCatmullRom(P0, P1, P2, P3, u);
        XMFLOAT3 out;
        XMStoreFloat3(&out, result);
        return out;
    }

public:
    std::vector<SystemSimulationState> SystemStates;
    float CurrentSimTime = 0.0f;
    float InterpolationFactor = 0.0f; // Sub-tick interpolation factor [0..1] for renderer
    float TimeAccumulator = 0.0f;     // Accumulator for 30Hz fixed simulation ticks
    int TotalActiveParticles = 0;

    ParticleSimulator() {
        Rng.seed(1337);
    }

    void Reset(const CParticleEmitter& emitter) {
        Rng.seed(1337);
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
            state.HasSingleSprite = false;

            // Pre-allocate particle pool (up to 2048 per system)
            state.Particles.assign(2048, SimulatedParticle());

            // Check if system has CPSCSingleSprite
            for (const auto& comp : sys.Components) {
                if (comp && comp->ClassName == "CPSCSingleSprite" && comp->Enabled) {
                    auto ssComp = std::dynamic_pointer_cast<CPSCSingleSprite>(comp);
                    if (ssComp && (ssComp->SpriteBankIndex > 0 || ssComp->TrailBankIndex > 0)) {
                        state.HasSingleSprite = true;
                        state.SingleSpriteProgress = 0.0f;
                        state.SingleSpriteParticle = SimulatedParticle();
                        state.SingleSpriteParticle.Active = true;
                        state.SingleSpriteParticle.MaxLife = 10000.0f; // persistent
                        state.SingleSpriteParticle.MaxLifeTicks = 30000;
                        XMFLOAT3 initPos(0.0f, 0.0f, 0.0f);
                        if (!ssComp->ControlPoints.empty()) {
                            initPos = XMFLOAT3(ssComp->ControlPoints[0].X, ssComp->ControlPoints[0].Y, ssComp->ControlPoints[0].Z);
                        }
                        for (const auto& c : sys.Components) {
                            if (c && c->ClassName == "CPSCUpdateNormal" && c->Enabled) {
                                auto updateComp = std::dynamic_pointer_cast<CPSCUpdateNormal>(c);
                                if (updateComp) {
                                    initPos.x += updateComp->ParticleSystemOffset.X;
                                    initPos.y += updateComp->ParticleSystemOffset.Y;
                                    initPos.z += updateComp->ParticleSystemOffset.Z;
                                }
                                break;
                            }
                        }
                        state.SingleSpriteParticle.Position = initPos;
                        state.SingleSpriteParticle.PositionBuffer[0] = initPos;
                        state.SingleSpriteParticle.PositionBuffer[1] = initPos;
                        state.SingleSpriteParticle.PositionBuffer[2] = initPos;
                        state.SingleSpriteParticle.PositionBuffer[3] = initPos;
                    }
                    break;
                }
            }

            // Perform initial burst spawn if enabled
            if (sys.Enabled) {
                SpawnBurst(emitter, i);
            }
        }
    }

    void SpawnBurst(const CParticleEmitter& emitter, size_t sysIndex) {
        if (sysIndex >= emitter.Systems.size()) return;
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

            if (!sys.Enabled) continue;
            state.SystemTime += dt;

            // Find key components
            std::shared_ptr<CPSCEmitterGeneric> emitterComp;
            std::shared_ptr<CPSCUpdateNormal> updateComp;
            std::shared_ptr<CPSCRenderSprite> renderSpriteComp;
            std::shared_ptr<CPSCSingleSprite> singleSpriteComp;
            std::shared_ptr<CPSCOrbit> orbitComp;
            std::shared_ptr<CPSCAttractor> attractorComp;
            std::shared_ptr<CPSCSpline> splineComp;

            for (const auto& comp : sys.Components) {
                if (!comp || !comp->Enabled) continue;
                if (comp->ClassName == "CPSCEmitterGeneric") emitterComp = std::dynamic_pointer_cast<CPSCEmitterGeneric>(comp);
                else if (comp->ClassName == "CPSCUpdateNormal") updateComp = std::dynamic_pointer_cast<CPSCUpdateNormal>(comp);
                else if (comp->ClassName == "CPSCRenderSprite") renderSpriteComp = std::dynamic_pointer_cast<CPSCRenderSprite>(comp);
                else if (comp->ClassName == "CPSCSingleSprite") singleSpriteComp = std::dynamic_pointer_cast<CPSCSingleSprite>(comp);
                else if (comp->ClassName == "CPSCOrbit") orbitComp = std::dynamic_pointer_cast<CPSCOrbit>(comp);
                else if (comp->ClassName == "CPSCAttractor") attractorComp = std::dynamic_pointer_cast<CPSCAttractor>(comp);
                else if (comp->ClassName == "CPSCSpline") splineComp = std::dynamic_pointer_cast<CPSCSpline>(comp);
            }

            // 1. Continuous Emission (CPSCEmitterGeneric)
            if (emitterComp) {
                bool canEmit = true;
                if (emitterComp->UseEmitterLifeSecs && state.SystemTime > emitterComp->EmitterLifeSecs) {
                    canEmit = false;
                }
                if (state.SystemTime < emitterComp->EmitterStartTime) {
                    canEmit = false;
                }

                if (canEmit && emitterComp->ParticlesPerSecond > 0.0f) {
                    state.SpawnAccumulator += emitterComp->ParticlesPerSecond * dt;
                    int countToSpawn = (int)state.SpawnAccumulator;
                    if (countToSpawn > 256) countToSpawn = 256;
                    if (countToSpawn > 0) {
                        state.SpawnAccumulator -= (float)countToSpawn;
                        SpawnParticles(sys, state, emitterComp, countToSpawn);
                    }
                }
            }

            // 2. Physics & Kinematics Update (CPSCUpdateNormal, CPSCAttractor, CPSCOrbit)
            for (auto& p : state.Particles) {
                if (!p.Active) continue;

                p.Lifetimer++;
                p.Age += dt;
                if (p.Lifetimer >= p.MaxLifeTicks) {
                    p.Active = false;
                    continue;
                }

                // Gravity & Acceleration from CPSCUpdateNormal
                if (updateComp) {
                    // Standard gravity (9.82 m/s^2) scaled by GravityFactor acting along -Z (functions7.txt:1-12)
                    p.Velocity.z -= 9.82f * updateComp->GravityFactor * dt;

                    // Constant particle acceleration
                    p.Velocity.x += updateComp->ParticleAcceleration.X * updateComp->ParticleAccelerationScale * dt;
                    p.Velocity.y += updateComp->ParticleAcceleration.Y * updateComp->ParticleAccelerationScale * dt;
                    p.Velocity.z += updateComp->ParticleAcceleration.Z * updateComp->ParticleAccelerationScale * dt;

                    // Air resistance drag damping
                    if (updateComp->AirResistance > 0.0f) {
                        float drag = (std::max)(0.0f, 1.0f - updateComp->AirResistance * dt);
                        p.Velocity.x *= drag;
                        p.Velocity.y *= drag;
                        p.Velocity.z *= drag;
                    }

                    // Angular rotation (2.0 / 127.0 scale per tick from disassembly)
                    p.Angle += (float)p.AngleChange * (2.0f / 127.0f);
                }

                // CPSCAttractor forces
                if (attractorComp && attractorComp->AttractorInfluenceRadius > 0.0f) {
                    XMFLOAT3 targetPos = { 0.0f, 0.0f, 0.0f };
                    if (!attractorComp->AttractorUserPointsArray.empty()) {
                        targetPos = XMFLOAT3(attractorComp->AttractorUserPointsArray[0].X,
                                             attractorComp->AttractorUserPointsArray[0].Y,
                                             attractorComp->AttractorUserPointsArray[0].Z);
                    }

                    float dx = targetPos.x - p.Position.x;
                    float dy = targetPos.y - p.Position.y;
                    float dz = targetPos.z - p.Position.z;
                    float distSq = dx * dx + dy * dy + dz * dz;
                    float dist = std::sqrt(distSq);

                    if (dist > 0.0001f && dist < attractorComp->AttractorInfluenceRadius) {
                        float dirX = dx / dist;
                        float dirY = dy / dist;
                        float dirZ = dz / dist;

                        float falloff = 1.0f;
                        if (attractorComp->AttractorInfluenceFallOff == 1) { // Linear
                            falloff = 1.0f - (dist / attractorComp->AttractorInfluenceRadius);
                        }
                        else if (attractorComp->AttractorInfluenceFallOff >= 2) { // Inverse square / distance
                            falloff = 1.0f / (1.0f + distSq);
                        }

                        float force = attractorComp->AttractorInfluenceForce * falloff;
                        p.Velocity.x += dirX * force * dt;
                        p.Velocity.y += dirY * force * dt;
                        p.Velocity.z += dirZ * force * dt;
                    }
                }

                // CPSCOrbit forces: tangential velocity in XY plane around Z axis
                if (orbitComp) {
                    for (const auto& orbit : orbitComp->OrbitsData) {
                        if (orbit.Enabled && std::abs(orbit.RotateSpeed) > 0.001f) {
                            float rx = p.Position.x;
                            float ry = p.Position.y;
                            float r = std::sqrt(rx * rx + ry * ry);
                            if (r > 0.01f) {
                                float tangentX = -ry / r;
                                float tangentY = rx / r;
                                float orbitSpeed = orbit.RotateSpeed * r;
                                p.Velocity.x += tangentX * orbitSpeed * dt;
                                p.Velocity.y += tangentY * orbitSpeed * dt;
                            }
                        }
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

                // Integrate position
                p.Position.x += p.Velocity.x * dt;
                p.Position.y += p.Velocity.y * dt;
                p.Position.z += p.Velocity.z * dt;

                // Ground Collision against Z = 0.0 plane (functions7.txt:181-198)
                if (updateComp && updateComp->ParticleCollideWithGround) {
                    if (p.Position.z <= 0.0f) {
                        p.Position.z = 0.0f;
                        if (updateComp->ParticleDieOnCollision) {
                            p.Active = false;
                            continue;
                        }
                        else {
                            // Bounce with velocity damping along Z
                            p.Velocity.z = -p.Velocity.z * (std::clamp)(updateComp->ParticleBounce, 0.0f, 1.0f);
                            p.Velocity.x *= 0.85f; // Ground friction
                            p.Velocity.y *= 0.85f;
                        }
                    }
                }

                p.PositionBuffer[0] = p.Position;
            }

            // 3. Update Persistent Single Sprite (CPSCSingleSprite)
            if (state.HasSingleSprite && singleSpriteComp) {
                auto& sp = state.SingleSpriteParticle;
                sp.Active = true;

                sp.PositionBuffer[3] = sp.PositionBuffer[2];
                sp.PositionBuffer[2] = sp.PositionBuffer[1];
                sp.PositionBuffer[1] = sp.PositionBuffer[0];

                float animTime = (singleSpriteComp->AnimationTimeSecs > 0.01f)
                    ? singleSpriteComp->AnimationTimeSecs
                    : 3.0f;
                state.SingleSpriteProgress += dt / animTime;
                if (state.SingleSpriteProgress > 1.0f) state.SingleSpriteProgress = std::fmod(state.SingleSpriteProgress, 1.0f);

                if (singleSpriteComp->UseSplinePoints && !singleSpriteComp->ControlPoints.empty()) {
                    sp.Position = EvaluateCatmullRom(singleSpriteComp->ControlPoints, state.SingleSpriteProgress);
                }
                else if (!singleSpriteComp->ControlPoints.empty()) {
                    sp.Position = XMFLOAT3(singleSpriteComp->ControlPoints[0].X, singleSpriteComp->ControlPoints[0].Y, singleSpriteComp->ControlPoints[0].Z);
                }
                else {
                    sp.Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
                }

                if (orbitComp) {
                    for (const auto& orbit : orbitComp->OrbitsData) {
                        if (orbit.Enabled) {
                            float orbitAngle = orbit.RotateStart + state.SystemTime * orbit.RotateSpeed;
                            float r = orbit.Radius + orbit.Expand * state.SystemTime;
                            sp.Position.x += std::cos(orbitAngle) * r;
                            sp.Position.y += std::sin(orbitAngle) * r;
                        }
                    }
                }

                if (updateComp) {
                    sp.Position.x += updateComp->ParticleSystemOffset.X;
                    sp.Position.y += updateComp->ParticleSystemOffset.Y;
                    sp.Position.z += updateComp->ParticleSystemOffset.Z;
                }

                sp.PositionBuffer[0] = sp.Position;
                sp.Angle = singleSpriteComp->InitialAngle;
                sp.MaxLifeTicks = 30000;
                sp.Lifetimer = (int)(state.SystemTime * 30.0f);
            }
        }
    }

    void SpawnParticles(const CParticleSystem& sys, SystemSimulationState& state,
                        const std::shared_ptr<CPSCEmitterGeneric>& emitterComp, int count) {
        if (!emitterComp || count <= 0) return;
        if (count > 512) count = 512;

        // Find CPSCUpdateNormal for lifetime & rotation setup
        std::shared_ptr<CPSCUpdateNormal> updateComp;
        for (const auto& c : sys.Components) {
            if (c && c->ClassName == "CPSCUpdateNormal") {
                updateComp = std::dynamic_pointer_cast<CPSCUpdateNormal>(c);
                break;
            }
        }

        float lifeSecs = updateComp ? updateComp->ParticleLifeSecs : 2.0f;
        if (lifeSecs <= 0.01f) lifeSecs = 1.0f;

        for (int i = 0; i < count; i++) {
            // Find inactive particle slot or recycle oldest
            SimulatedParticle* target = nullptr;
            for (auto& p : state.Particles) {
                if (!p.Active) {
                    target = &p;
                    break;
                }
            }
            if (!target) {
                // Find oldest particle to recycle
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
            target->Lifetimer = 0;
            target->CreationTime = (uint8_t)(RandFloat(0.0f, 1.0f) * 255.0f);
            target->MaxLifeTicks = (int16_t)(lifeSecs * 30.0f + 0.5f);
            if (target->MaxLifeTicks < 1) target->MaxLifeTicks = 1;
            target->Age = 0.0f;
            target->MaxLife = lifeSecs;
            target->StayWithEmitter = (updateComp && updateComp->StayWithEmitter);

            // Compute Spawn Position based on emitter shape:
            // 0 = Point, 1 = 2D Disc, 2 = 3D Sphere, 3 = Spline
            XMFLOAT3 spawnPos = { 0.0f, 0.0f, 0.0f };
            XMFLOAT3 outwardDir = { 0.0f, 1.0f, 0.0f };
            float radius = emitterComp->EmitterSize * 0.5f;

            if (emitterComp->EmitterType == 1) { // 2D Disc
                XMFLOAT2 u2 = RandUnitVector2();
                float r = radius;
                if (emitterComp->Solid) {
                    r *= std::sqrt(RandFloat(0.0f, 1.0f));
                }
                XMFLOAT3 localDisc(u2.x * r, u2.y * r, 0.0f);

                // Reorient according to flags: OrientationXY, OrientationXZ, OrientationYZ
                if (emitterComp->OrientationXZ) {
                    spawnPos = XMFLOAT3(localDisc.x, 0.0f, localDisc.y);
                    outwardDir = XMFLOAT3(u2.x, 0.0f, u2.y);
                }
                else if (emitterComp->OrientationYZ) {
                    spawnPos = XMFLOAT3(0.0f, localDisc.x, localDisc.y);
                    outwardDir = XMFLOAT3(0.0f, u2.x, u2.y);
                }
                else { // OrientationXY
                    spawnPos = localDisc;
                    outwardDir = XMFLOAT3(u2.x, u2.y, 0.0f);
                }
            }
            else if (emitterComp->EmitterType == 2) { // 3D Sphere
                XMFLOAT3 u3 = RandUnitVector3();
                float r = radius;
                if (emitterComp->Solid) {
                    r *= std::cbrt(RandFloat(0.0f, 1.0f));
                }
                spawnPos = XMFLOAT3(u3.x * r, u3.y * r, u3.z * r);
                outwardDir = u3;
            }
            else if (emitterComp->EmitterType == 3 && emitterComp->HasSpline && !emitterComp->SplineControlPoints.empty()) { // Spline
                spawnPos = EvaluateCatmullRom(emitterComp->SplineControlPoints, RandFloat(0.0f, 1.0f));
                outwardDir = RandUnitVector3();
            }
            else { // Point emitter
                spawnPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
                outwardDir = RandUnitVector3();
            }

            // Apply non-uniform scaling if configured
            spawnPos.x *= (1.0f + emitterComp->NonUniformScaling.X * 0.1f);
            spawnPos.y *= (1.0f + emitterComp->NonUniformScaling.Y * 0.1f);
            spawnPos.z *= (1.0f + emitterComp->NonUniformScaling.Z * 0.1f);

            // Apply system scale
            spawnPos.x *= sys.Scale.X;
            spawnPos.y *= sys.Scale.Y;
            spawnPos.z *= sys.Scale.Z;

            if (updateComp) {
                spawnPos.x += updateComp->ParticleSystemOffset.X;
                spawnPos.y += updateComp->ParticleSystemOffset.Y;
                spawnPos.z += updateComp->ParticleSystemOffset.Z;
            }

            target->Position = spawnPos;
            target->PositionBuffer[0] = spawnPos;
            target->PositionBuffer[1] = spawnPos;
            target->PositionBuffer[2] = spawnPos;
            target->PositionBuffer[3] = spawnPos;

            // Compute Direction: Outward, Random 3D, Random 2D, or Custom Direction
            XMFLOAT3 dir = outwardDir;
            if (emitterComp->UseRandom3DDirection) {
                dir = RandUnitVector3();
            }
            else if (emitterComp->UseRandom2DDirection) {
                XMFLOAT2 d2 = RandUnitVector2();
                dir = emitterComp->OrientationXZ ? XMFLOAT3(d2.x, 0, d2.y) : XMFLOAT3(d2.x, d2.y, 0);
            }
            else if (emitterComp->UseCustomDirection) {
                dir = XMFLOAT3(emitterComp->CustomDirection.X, emitterComp->CustomDirection.Y, emitterComp->CustomDirection.Z);
            }
            else if (emitterComp->UseOutwardDirection) {
                dir = outwardDir;
            }

            // Normalize direction
            XMVECTOR dirVec = XMVectorSet(dir.x, dir.y, dir.z, 0.0f);
            float lenSq = XMVectorGetX(XMVector3LengthSq(dirVec));
            if (lenSq < 0.0001f) {
                dirVec = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            }
            else {
                dirVec = XMVector3Normalize(dirVec);
            }

            // Apply angular perturbation cone if configured
            if (emitterComp->AngularPerturbationInteger > 0) {
                float maxAngle = (emitterComp->AngularPerturbationInteger / 1023.0f) * XM_PI * 0.5f;
                float perturbAngle = RandFloat(0.0f, maxAngle);
                float perturbRoll = RandFloat(0.0f, XM_2PI);

                XMVECTOR perp = XMVector3Orthogonal(dirVec);
                perp = XMVector3Normalize(perp);
                XMMATRIX rotRoll = XMMatrixRotationAxis(dirVec, perturbRoll);
                perp = XMVector3TransformNormal(perp, rotRoll);
                XMMATRIX rotCone = XMMatrixRotationAxis(perp, perturbAngle);
                dirVec = XMVector3TransformNormal(dirVec, rotCone);
            }

            // Speed range: MinSpeed to MaxSpeed
            float minSpd = emitterComp->MinSpeed;
            float maxSpd = (std::max)(minSpd, emitterComp->MaxSpeed);
            float speed = RandFloat(minSpd, maxSpd);

            XMStoreFloat3(&target->Velocity, dirVec * speed);

            // 2D Rotation & 3D Orientation setup from CPSCUpdateNormal
            if (updateComp) {
                target->Angle = updateComp->UseRandomInitialRotation ? RandFloat(0.0f, XM_2PI) : updateComp->InitialRotationZ;
                target->AngleSpeed = RandFloat(updateComp->RotationMinAngleSpeed, updateComp->RotationMaxAngleSpeed);
                float angleChangeRad = target->AngleSpeed * (1.0f / 30.0f); // 30Hz tick angular delta
                target->AngleChange = (int8_t)std::clamp((int)(angleChangeRad * (127.0f / 2.0f)), -128, 127);

                // Compute 3D rotation axis
                XMFLOAT3 rotAxis = { updateComp->RotationAxis.X, updateComp->RotationAxis.Y, updateComp->RotationAxis.Z };
                float axisLen = std::sqrt(rotAxis.x * rotAxis.x + rotAxis.y * rotAxis.y + rotAxis.z * rotAxis.z);
                if (axisLen < 0.001f) {
                    rotAxis = RandUnitVector3();
                } else {
                    rotAxis.x /= axisLen; rotAxis.y /= axisLen; rotAxis.z /= axisLen;
                }

                XMVECTOR axisVec = XMLoadFloat3(&rotAxis);
                XMVECTOR dq = XMQuaternionRotationAxis(axisVec, angleChangeRad);
                XMStoreFloat4(&target->OrientationChange, dq);

                float initAngle = updateComp->UseRandomInitialRotation ? RandFloat(0.0f, XM_2PI) : updateComp->InitialRotationZ;
                XMVECTOR initQ = XMQuaternionRotationAxis(axisVec, initAngle);
                XMStoreFloat4(&target->Orientation, initQ);
            }
            else {
                target->Angle = 0.0f;
                target->AngleSpeed = 0.0f;
                target->AngleChange = 0;
                target->Orientation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
                target->OrientationChange = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
            }

            target->FlickerIndex = (uint8_t)(RandFloat(0.0f, 255.0f));
            target->FlickerTimeOffset = (uint8_t)(RandFloat(0.0f, 255.0f));
        }
    }
};
