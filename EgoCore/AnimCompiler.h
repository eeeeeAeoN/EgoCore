#pragma once
#include "AnimParser.h"
#include "minilzo.h"
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <functional>
#include <cmath>

struct AnimWriter {
    std::vector<uint8_t> data;
    void Write(const void* src, size_t size) {
        size_t off = data.size();
        data.resize(off + size);
        memcpy(data.data() + off, src, size);
    }
    template<typename T> void Write(T val) { Write(&val, sizeof(T)); }
    void WriteString(const std::string& s) { Write(s.c_str(), s.length() + 1); }
    size_t GetPos() { return data.size(); }
    void WriteAt(size_t pos, const void* src, size_t size) {
        if (pos + size <= data.size()) memcpy(data.data() + pos, src, size);
    }
};

class AnimCompiler {
public:
    static std::vector<uint8_t> Compile(const C3DAnimationInfo& anim) {
        AnimWriter w;

        w.Write(">>>>", 4);
        w.Write("3DAF", 4);
        w.Write<uint32_t>(100);

        std::string comment = "Copyright Big Blue Box Studios Ltd.";
        w.Write(comment.c_str(), comment.length() + 1);

        size_t stringEndAlign = (w.GetPos() + 3) & ~3;
        while (w.GetPos() < stringEndAlign) w.Write<uint8_t>(0);

        std::vector<AnimTrack> safeTracks = anim.Tracks;
        bool hasGlobalIDs = false;
        for (const auto& t : safeTracks) {
            if (t.BoneIndex != 31450 && t.BoneIndex != 0) {
                hasGlobalIDs = true; break;
            }
        }

        if (hasGlobalIDs) {
            for (size_t i = 0; i < safeTracks.size(); i++) {
                int targetParentFableID = safeTracks[i].ParentIndex;
                int newParentIndex = -1;
                for (size_t j = 0; j < safeTracks.size(); j++) {
                    if (safeTracks[j].BoneIndex == targetParentFableID) {
                        newParentIndex = (int)j; break;
                    }
                }
                safeTracks[i].ParentIndex = newParentIndex;
            }
            for (auto& t : safeTracks) t.BoneIndex = 31450;
        }

        // --- TRUE DURATION CALCULATOR ---
        // Dynamically calculate the duration to prevent the engine from stretching
        // short glTF imports over the duration of long original animation files.
        float actualDuration = 0.0f;
        for (const auto& t : safeTracks) {
            if (t.SamplesPerSecond > 0.0f && t.FrameCount > 0) {
                float d = (float)t.FrameCount / t.SamplesPerSecond;
                if (d > actualDuration) actualDuration = d;
            }
        }
        if (actualDuration <= 0.0f) actualDuration = anim.Duration; // Fallback

        WriteChunk(w, "ANRT", [&](AnimWriter& sw) {
            sw.Write<uint8_t>(anim.IsCyclic ? 1 : 0);
            sw.Write<float>(actualDuration); // Write calculated duration

            if (!anim.AnrtHeaderData.empty()) {
                sw.Write(anim.AnrtHeaderData.data(), anim.AnrtHeaderData.size());
            }

            WriteChunk(sw, "XALO", [&](AnimWriter& xaloW) {
                xaloW.Write<uint32_t>(1);
                uint32_t allocSize = 4096;
                auto calcTrackSize = [](const AnimTrack& t) {
                    return 128 + (uint32_t)t.RotationTrack.size() * 16 + (uint32_t)t.PalettedRotations.size() * 1 +
                        (uint32_t)t.PositionTrack.size() * 12 + (uint32_t)t.PalettedPositions.size() * 1;
                    };
                for (const auto& t : safeTracks) allocSize += calcTrackSize(t);
                for (const auto& t : anim.HelperTracks) allocSize += calcTrackSize(t);
                xaloW.Write<uint32_t>(allocSize);
                });

            bool needsHelper = anim.HasHelper || !anim.HelperTracks.empty() || !anim.TimeEvents.empty() ||
                std::abs(anim.MovementVector.x) > 0.001f || std::abs(anim.MovementVector.y) > 0.001f || std::abs(anim.MovementVector.z) > 0.001f;

            if (needsHelper) {
                WriteChunk(sw, "HLPR", [&](AnimWriter& hW) {
                    WriteChunk(hW, "MVEC", [&](AnimWriter& vecW) {
                        vecW.Write<float>(anim.MovementVector.x);
                        vecW.Write<float>(anim.MovementVector.y);
                        vecW.Write<float>(anim.MovementVector.z);
                        });

                    for (const auto& ev : anim.TimeEvents) {
                        WriteChunk(hW, "TMEV", [&](AnimWriter& tW) {
                            tW.WriteString(ev.Name);
                            tW.Write<float>(ev.Time);
                            });
                    }

                    for (const auto& track : anim.HelperTracks) {
                        WriteChunk(hW, "XSEQ", [&](AnimWriter& seqW) {
                            WriteTrackPayload(seqW, track);
                            });
                    }
                    });
            }

            WriteChunk(sw, "AOBJ", [&](AnimWriter& objW) {
                objW.WriteString(anim.ObjectName);
                objW.Write<int32_t>(0);

                if (!anim.BoneMaskBits.empty()) {
                    WriteChunk(objW, "AMSK", [&](AnimWriter& maskW) {
                        maskW.Write<uint32_t>((uint32_t)anim.BoneMaskBits.size() * 32);
                        for (uint32_t bitmask : anim.BoneMaskBits) {
                            for (int bit = 0; bit < 32; bit++) {
                                maskW.Write<uint8_t>(((bitmask >> bit) & 1) != 0 ? 1 : 0);
                            }
                        }
                        });
                }

                for (const auto& track : safeTracks) {
                    WriteChunk(objW, "XSEQ", [&](AnimWriter& seqW) {
                        WriteTrackPayload(seqW, track);
                        });
                }
                });
            });

        size_t footerOff = w.GetPos();
        w.Write<uint32_t>(0); w.Write<uint32_t>(0);
        uint32_t footerSize = (uint32_t)(w.GetPos() - footerOff - 4);
        w.WriteAt(footerOff, &footerSize, 4);

        if (lzo_init() != LZO_E_OK) return std::vector<uint8_t>();
        uint32_t uncompLen = (uint32_t)w.data.size();
        std::vector<uint8_t> compBuf(uncompLen + (uncompLen / 16) + 128);
        lzo_uint compLen = 0; std::vector<uint8_t> wrkmem(LZO1X_1_MEM_COMPRESS);

        if (lzo1x_1_compress(w.data.data(), uncompLen, compBuf.data(), &compLen, wrkmem.data()) != LZO_E_OK) {
            return std::vector<uint8_t>();
        }

        std::vector<uint8_t> finalBlob(4 + compLen);
        memcpy(finalBlob.data(), &uncompLen, 4);
        memcpy(finalBlob.data() + 4, compBuf.data(), compLen);

        return finalBlob;
    }

private:
    struct PosPaletteResult {
        std::vector<uint16_t> Indices;
        std::vector<int16_t> CompressedPalette;
        float Factor = 1.0f;
    };

    struct RotPaletteResult {
        std::vector<uint16_t> Indices;
        std::vector<Vec4> Palette;
    };

    static PosPaletteResult ProcessPositions(const AnimTrack& t) {
        PosPaletteResult res;

        // Strip Root Offsets (Keeps character on the ground)
        if (t.BoneName == "Scene Root" || t.BoneName == "Movement" ||
            t.BoneName == "Movement_dummy" || t.BoneName == "Sub_movement_dummy") {
            return res;
        }

        res.Factor = (t.PositionFactor <= 0.0f) ? 1.0f : t.PositionFactor;

        if (!t.PalettedPositions.empty()) {
            res.Indices = t.PalettedPositions;
            for (const auto& p : t.PositionTrack) {
                res.CompressedPalette.push_back((int16_t)std::clamp(p.x / res.Factor, -32767.0f, 32767.0f));
                res.CompressedPalette.push_back((int16_t)std::clamp(p.y / res.Factor, -32767.0f, 32767.0f));
                res.CompressedPalette.push_back((int16_t)std::clamp(p.z / res.Factor, -32767.0f, 32767.0f));
            }
        }
        else if (!t.PositionTrack.empty()) {
            bool isConstant = true;
            for (size_t i = 1; i < t.PositionTrack.size(); i++) {
                if (std::abs(t.PositionTrack[i].x - t.PositionTrack[0].x) > 0.001f ||
                    std::abs(t.PositionTrack[i].y - t.PositionTrack[0].y) > 0.001f ||
                    std::abs(t.PositionTrack[i].z - t.PositionTrack[0].z) > 0.001f) {
                    isConstant = false; break;
                }
            }

            if (isConstant) {
                auto p = t.PositionTrack[0];
                res.CompressedPalette.push_back((int16_t)std::clamp(p.x / res.Factor, -32767.0f, 32767.0f));
                res.CompressedPalette.push_back((int16_t)std::clamp(p.y / res.Factor, -32767.0f, 32767.0f));
                res.CompressedPalette.push_back((int16_t)std::clamp(p.z / res.Factor, -32767.0f, 32767.0f));
            }
            else {
                for (size_t i = 0; i < t.PositionTrack.size(); i++) {
                    auto p = t.PositionTrack[i];
                    res.CompressedPalette.push_back((int16_t)std::clamp(p.x / res.Factor, -32767.0f, 32767.0f));
                    res.CompressedPalette.push_back((int16_t)std::clamp(p.y / res.Factor, -32767.0f, 32767.0f));
                    res.CompressedPalette.push_back((int16_t)std::clamp(p.z / res.Factor, -32767.0f, 32767.0f));
                    res.Indices.push_back((uint16_t)i);
                }
            }
        }
        return res;
    }

    static RotPaletteResult ProcessRotations(const AnimTrack& t) {
        RotPaletteResult res;

        // Strip Root Offsets
        if (t.BoneName == "Scene Root" || t.BoneName == "Movement" ||
            t.BoneName == "Movement_dummy" || t.BoneName == "Sub_movement_dummy") {
            return res;
        }

        if (!t.PalettedRotations.empty()) {
            res.Indices = t.PalettedRotations;
            res.Palette = t.RotationTrack;
        }
        else if (!t.RotationTrack.empty()) {
            bool isConstant = true;
            for (size_t i = 1; i < t.RotationTrack.size(); i++) {
                if (std::abs(t.RotationTrack[i].x - t.RotationTrack[0].x) > 0.0001f ||
                    std::abs(t.RotationTrack[i].y - t.RotationTrack[0].y) > 0.0001f ||
                    std::abs(t.RotationTrack[i].z - t.RotationTrack[0].z) > 0.0001f ||
                    std::abs(t.RotationTrack[i].w - t.RotationTrack[0].w) > 0.0001f) {
                    isConstant = false; break;
                }
            }

            if (isConstant) {
                auto r = t.RotationTrack[0];
                bool isRealIdentity = (std::abs(r.x) < 0.001f && std::abs(r.y) < 0.001f && std::abs(r.z) < 0.001f && std::abs(std::abs(r.w) - 1.0f) < 0.001f);
                bool isGlTFFlipped = (std::abs(std::abs(r.x) - 1.0f) < 0.001f && std::abs(r.y) < 0.001f && std::abs(r.z) < 0.001f && std::abs(r.w) < 0.001f);

                if (!isRealIdentity && !isGlTFFlipped) res.Palette.push_back(r);
            }
            else {
                res.Palette = t.RotationTrack;
                for (size_t i = 0; i < t.RotationTrack.size(); i++) res.Indices.push_back((uint16_t)i);
            }
        }
        return res;
    }

    static void WriteChunk(AnimWriter& w, const char* id, std::function<void(AnimWriter&)> content) {
        w.Write(id, 4);
        size_t sizePos = w.GetPos();
        w.Write<uint32_t>(0);

        size_t start = w.GetPos();
        content(w);
        uint32_t size = (uint32_t)(w.GetPos() - start);

        w.WriteAt(sizePos, &size, 4);
    }

    static void WriteTrackPayload(AnimWriter& w, const AnimTrack& t) {
        w.Write<uint32_t>(t.BoneIndex);
        w.Write<int32_t>(t.ParentIndex);
        w.WriteString(t.BoneName);

        w.Write<uint8_t>(0); // Prevent Additive Rolling
        w.Write<float>(t.SamplesPerSecond);
        w.Write<uint32_t>(t.FrameCount);

        auto posDat = ProcessPositions(t);
        auto rotDat = ProcessRotations(t);

        uint8_t flags[4] = { 0, 0, 0, 0 };
        // FIX: Reverted back to 2 (Linear). 3 causes timing/interpolation distortion in the engine.
        flags[1] = (rotDat.Palette.size() > 1) ? 2 : (rotDat.Palette.size() == 1 ? 1 : 0);
        flags[2] = (posDat.CompressedPalette.size() / 3 > 1) ? 2 : (posDat.CompressedPalette.size() / 3 == 1 ? 1 : 0);
        flags[0] = (flags[1] > 0 || flags[2] > 0) ? 1 : 0;
        flags[3] = 0;

        w.Write(flags, 4);

        w.Write<float>(posDat.Factor);
        w.Write<float>(t.ScalingFactor == 0.0f ? 1.0f : t.ScalingFactor);

        w.Write<uint16_t>((uint16_t)rotDat.Palette.size());
        for (const auto& q : rotDat.Palette) w.Write(q);

        w.Write<uint16_t>((uint16_t)rotDat.Indices.size());
        for (uint16_t idx : rotDat.Indices) {
            w.Write<uint8_t>((uint8_t)std::clamp(idx, (uint16_t)0, (uint16_t)255));
        }

        uint16_t posPalSize = (uint16_t)(posDat.CompressedPalette.size() / 3);
        w.Write<uint16_t>(posPalSize);
        for (int16_t val : posDat.CompressedPalette) w.Write(val);

        w.Write<uint16_t>((uint16_t)posDat.Indices.size());
        for (uint16_t idx : posDat.Indices) {
            w.Write<uint8_t>((uint8_t)std::clamp(idx, (uint16_t)0, (uint16_t)255));
        }
    }
};