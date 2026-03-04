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

        // 1. Correct Fable 1 Header 
        w.Write(">>>>", 4);
        w.Write("3DAF", 4);
        w.Write<uint32_t>(100);

        std::string comment = "Copyright Big Blue Box Studios Ltd.";
        w.Write(comment.c_str(), comment.length() + 1);

        // Standard string header alignment
        size_t stringEndAlign = (w.GetPos() + 3) & ~3;
        while (w.GetPos() < stringEndAlign) w.Write<uint8_t>(0);

        // 2. Root Chunk (ANRT)
        WriteChunk(w, "ANRT", [&](AnimWriter& sw) {

            sw.Write<uint8_t>(anim.IsCyclic ? 1 : 0);
            sw.Write<float>(anim.Duration);

            // Write back any preserved unknown variables
            if (!anim.AnrtHeaderData.empty()) {
                sw.Write(anim.AnrtHeaderData.data(), anim.AnrtHeaderData.size());
            }

            bool needsHelper = anim.HasHelper || !anim.XaloData.empty() || !anim.HelperTracks.empty() || !anim.TimeEvents.empty() ||
                std::abs(anim.MovementVector.x) > 0.001f ||
                std::abs(anim.MovementVector.y) > 0.001f ||
                std::abs(anim.MovementVector.z) > 0.001f;

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

                    if (!anim.XaloData.empty()) {
                        WriteChunk(hW, "XALO", [&](AnimWriter& xW) {
                            xW.Write(anim.XaloData.data(), anim.XaloData.size());
                            });
                    }
                    });
            }

            WriteChunk(sw, "AOBJ", [&](AnimWriter& objW) {
                objW.WriteString(anim.ObjectName);

                // CRITICAL FIX: Write back ParentIndex/Child/Sibling/Submesh tree pointers!
                if (!anim.AobjHeaderData.empty()) {
                    objW.Write(anim.AobjHeaderData.data(), anim.AobjHeaderData.size());
                }
                else {
                    // Fallback
                    objW.Write<int32_t>(-1); objW.Write<int32_t>(-1);
                    objW.Write<int32_t>(-1); objW.Write<int32_t>(-1);
                }

                for (const auto& track : anim.Tracks) {
                    WriteChunk(objW, "XSEQ", [&](AnimWriter& seqW) {
                        WriteTrackPayload(seqW, track);
                        });
                }

                if (!anim.BoneMaskBits.empty()) {
                    WriteChunk(objW, "AMSK", [&](AnimWriter& maskW) {
                        maskW.Write(anim.BoneMaskBits.data(), anim.BoneMaskBits.size() * 4);
                        });
                }
                });
            });

        size_t footerOff = w.GetPos();
        w.Write<uint32_t>(0);
        w.Write<uint32_t>(0);
        uint32_t footerSize = (uint32_t)(w.GetPos() - footerOff - 4);
        w.WriteAt(footerOff, &footerSize, 4);

        if (lzo_init() != LZO_E_OK) return std::vector<uint8_t>();

        uint32_t uncompLen = (uint32_t)w.data.size();
        std::vector<uint8_t> compBuf(uncompLen + (uncompLen / 16) + 128);
        lzo_uint compLen = 0;
        std::vector<uint8_t> wrkmem(LZO1X_1_MEM_COMPRESS);

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
        std::vector<uint8_t> Indices;
        std::vector<int16_t> CompressedPalette;
        float Factor = 1.0f;
    };

    struct RotPaletteResult {
        std::vector<uint8_t> Indices;
        std::vector<Vec4> Palette;
    };

    static PosPaletteResult ProcessPositions(const AnimTrack& t) {
        PosPaletteResult res;
        res.Indices = t.PalettedPositions;
        res.Factor = t.PositionFactor;
        if (res.Factor == 0.0f) res.Factor = 1.0f;

        for (const auto& p : t.PositionTrack) {
            res.CompressedPalette.push_back((int16_t)std::round(p.x / res.Factor));
            res.CompressedPalette.push_back((int16_t)std::round(p.y / res.Factor));
            res.CompressedPalette.push_back((int16_t)std::round(p.z / res.Factor));
        }
        return res;
    }

    static RotPaletteResult ProcessRotations(const AnimTrack& t) {
        RotPaletteResult res;
        res.Indices = t.PalettedRotations;
        res.Palette = t.RotationTrack;
        return res;
    }

    static void WriteChunk(AnimWriter& w, const char* id, std::function<void(AnimWriter&)> content) {
        w.Write(id, 4);
        size_t sizePos = w.GetPos();
        w.Write<uint32_t>(0); // Placeholder

        size_t start = w.GetPos();
        content(w);
        uint32_t size = (uint32_t)(w.GetPos() - start);

        w.WriteAt(sizePos, &size, 4);

        // Removed chunk 4-byte padding! Let Fable chunks stay tightly packed.
    }

    static void WriteTrackPayload(AnimWriter& w, const AnimTrack& t) {
        w.Write<uint32_t>(t.BoneIndex); // Restored original BoneIndex
        w.Write<int32_t>(t.ParentIndex);
        w.WriteString(t.BoneName);

        w.Write<uint8_t>(t.PreFPSFlag);
        w.Write<float>(t.SamplesPerSecond);

        auto posDat = ProcessPositions(t);
        auto rotDat = ProcessRotations(t);

        w.Write<uint32_t>(t.FrameCount);

        w.Write(t.PostFrameFlags, 4);
        w.Write<float>(posDat.Factor);
        w.Write<float>(t.ScalingFactor);

        w.Write<uint16_t>((uint16_t)rotDat.Palette.size());
        for (const auto& q : rotDat.Palette) w.Write(q);

        w.Write<uint16_t>((uint16_t)rotDat.Indices.size());
        if (!rotDat.Indices.empty()) w.Write(rotDat.Indices.data(), rotDat.Indices.size());

        w.Write<uint16_t>((uint16_t)(posDat.CompressedPalette.size() / 3));
        for (int16_t val : posDat.CompressedPalette) w.Write(val);

        w.Write<uint16_t>((uint16_t)posDat.Indices.size());
        if (!posDat.Indices.empty()) w.Write(posDat.Indices.data(), posDat.Indices.size());
    }
};