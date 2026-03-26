#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <atomic>
#include <cstring>
#include <fstream>
#include <iostream>
#include "miniaudio.h"

static const int16_t StepTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29800, 32767
};

static const int IndexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static void WriteWavFile(const std::string& path, const std::vector<int16_t>& pcm, int sampleRate, int channels) {
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return;

    int byteRate = sampleRate * channels * 2;
    int blockAlign = channels * 2;
    int dataSize = (int)pcm.size() * 2;
    int chunkSize = 36 + dataSize;

    f.write("RIFF", 4);
    f.write((char*)&chunkSize, 4);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    int subchunk1Size = 16;
    short audioFormat = 1;
    short numChannels = (short)channels;
    int sRate = sampleRate;
    short bAlign = (short)blockAlign;
    short bitsPerSample = 16;

    f.write((char*)&subchunk1Size, 4);
    f.write((char*)&audioFormat, 2);
    f.write((char*)&numChannels, 2);
    f.write((char*)&sRate, 4);
    f.write((char*)&byteRate, 4);
    f.write((char*)&bAlign, 2);
    f.write((char*)&bitsPerSample, 2);

    f.write("data", 4);
    f.write((char*)&dataSize, 4);
    f.write((char*)pcm.data(), dataSize);
    f.close();
}

class XboxAdpcmEncoder {
    struct State {
        int16_t sample = 0;
        uint8_t index = 0;
    };

    static uint8_t EncodeNibble(State& state, int16_t target) {
        int32_t delta = target;
        delta -= state.sample;

        uint8_t nibble = 0;
        if (delta < 0) { delta = -delta; nibble |= 8; }

        uint16_t step = StepTable[state.index];
        int32_t diff = step >> 3;

        for (uint8_t i = 0; i < 3; i++) {
            if (delta > step) {
                delta -= step; diff += step; nibble |= (4 >> i);
            }
            step >>= 1;
        }

        if (nibble & 8) diff = -diff;

        state.sample = (int16_t)(std::clamp)((int32_t)state.sample + diff, -32768, 32767);
        state.index = (uint8_t)(std::clamp)((int)state.index + IndexTable[nibble], 0, 88);

        return nibble;
    }

public:
    static std::vector<uint8_t> Encode(const std::vector<int16_t>& pcm, int channels = 1) {
        std::vector<uint8_t> out;
        if (pcm.empty()) return out;

        const int SamplesPerBlock = 65;
        size_t totalFrames = pcm.size() / channels;
        size_t numBlocks = (totalFrames + SamplesPerBlock - 1) / SamplesPerBlock;

        int stride = (channels == 2) ? 72 : 36;
        out.reserve(numBlocks * stride);

        State stateL, stateR;
        size_t cursor = 0;

        for (size_t b = 0; b < numBlocks; b++) {
            if (channels == 1) {
                int16_t headerSample = (cursor < totalFrames) ? pcm[cursor] : 0;
                stateL.sample = headerSample;
                if (b == 0) stateL.index = 0;

                out.push_back(stateL.sample & 0xFF);
                out.push_back((stateL.sample >> 8) & 0xFF);
                out.push_back(stateL.index);
                out.push_back(0);

                cursor++;

                for (int i = 0; i < 32; i++) {
                    int16_t s1 = (cursor < totalFrames) ? pcm[cursor++] : 0;
                    int16_t s2 = (cursor < totalFrames) ? pcm[cursor++] : 0;
                    uint8_t n1 = EncodeNibble(stateL, s1);
                    uint8_t n2 = EncodeNibble(stateL, s2);
                    out.push_back(n1 | (n2 << 4));
                }
            }
            else {
                int16_t headL = (cursor < totalFrames) ? pcm[cursor * 2] : 0;
                int16_t headR = (cursor < totalFrames) ? pcm[cursor * 2 + 1] : 0;

                stateL.sample = headL;
                stateR.sample = headR;
                if (b == 0) { stateL.index = 0; stateR.index = 0; }

                out.push_back(stateL.sample & 0xFF); out.push_back((stateL.sample >> 8) & 0xFF);
                out.push_back(stateL.index); out.push_back(0);

                out.push_back(stateR.sample & 0xFF); out.push_back((stateR.sample >> 8) & 0xFF);
                out.push_back(stateR.index); out.push_back(0);

                cursor++;

                for (int i = 0; i < 8; i++) {
                    int16_t bufL[8] = { 0 }, bufR[8] = { 0 };
                    for (int k = 0; k < 8; k++) {
                        if (cursor < totalFrames) {
                            bufL[k] = pcm[cursor * 2];
                            bufR[k] = pcm[cursor * 2 + 1];
                            cursor++;
                        }
                    }

                    for (int k = 0; k < 4; k++) {
                        uint8_t n1 = EncodeNibble(stateL, bufL[k * 2]);
                        uint8_t n2 = EncodeNibble(stateL, bufL[k * 2 + 1]);
                        out.push_back(n1 | (n2 << 4));
                    }

                    for (int k = 0; k < 4; k++) {
                        uint8_t n1 = EncodeNibble(stateR, bufR[k * 2]);
                        uint8_t n2 = EncodeNibble(stateR, bufR[k * 2 + 1]);
                        out.push_back(n1 | (n2 << 4));
                    }
                }
            }
        }
        return out;
    }
};

class XboxAdpcmDecoder {
private:
    static int16_t DecodeSample(int nibble, int16_t pred, int step) {
        int diff = step >> 3;
        if (nibble & 4) diff += step;
        if (nibble & 2) diff += step >> 1;
        if (nibble & 1) diff += step >> 2;

        if (nibble & 8) pred -= diff;
        else pred += diff;

        return (std::clamp)(pred, (int16_t)-32768, (int16_t)32767);
    }

    static void DecodeMonoBlock(const uint8_t* block, std::vector<int16_t>& outPcm) {
        int16_t predictor = (int16_t)(block[0] | (block[1] << 8));
        int stepIndex = (int)block[2];
        stepIndex = (std::clamp)(stepIndex, 0, 88);

        outPcm.push_back(predictor);

        for (int i = 4; i < 36; i++) {
            uint8_t byte = block[i];

            int step = StepTable[stepIndex];
            int nibble = byte & 0x0F;
            predictor = DecodeSample(nibble, predictor, step);
            outPcm.push_back(predictor);
            stepIndex = (std::clamp)(stepIndex + IndexTable[nibble], 0, 88);

            step = StepTable[stepIndex];
            nibble = (byte >> 4) & 0x0F;
            predictor = DecodeSample(nibble, predictor, step);
            outPcm.push_back(predictor);
            stepIndex = (std::clamp)(stepIndex + IndexTable[nibble], 0, 88);
        }
    }

    static void DecodeStereoBlock(const uint8_t* block, std::vector<int16_t>& outPcm) {
        int16_t predL = (int16_t)(block[0] | (block[1] << 8));
        int idxL = (int)block[2];
        idxL = (std::clamp)(idxL, 0, 88);

        int16_t predR = (int16_t)(block[4] | (block[5] << 8));
        int idxR = (int)block[6];
        idxR = (std::clamp)(idxR, 0, 88);

        outPcm.push_back(predL);
        outPcm.push_back(predR);

        const uint8_t* data = block + 8;

        for (int i = 0; i < 8; i++) {
            uint32_t chunkL = *(uint32_t*)(data);
            uint32_t chunkR = *(uint32_t*)(data + 4);
            data += 8;

            for (int k = 0; k < 8; k++) {
                int nibbleL = chunkL & 0xF;
                predL = DecodeSample(nibbleL, predL, StepTable[idxL]);
                idxL = (std::clamp)(idxL + IndexTable[nibbleL], 0, 88);
                outPcm.push_back(predL);
                chunkL >>= 4;

                int nibbleR = chunkR & 0xF;
                predR = DecodeSample(nibbleR, predR, StepTable[idxR]);
                idxR = (std::clamp)(idxR + IndexTable[nibbleR], 0, 88);
                outPcm.push_back(predR);
                chunkR >>= 4;
            }
        }
    }

public:
    static std::vector<int16_t> Decode(const std::vector<uint8_t>& adpcmData, int channels = 1, int blockAlign = 36) {
        std::vector<int16_t> pcm;
        if (adpcmData.empty()) return pcm;

        int stride = (channels == 2) ? 72 : 36;
        int numBlocks = (int)adpcmData.size() / stride;

        pcm.reserve(numBlocks * 65 * channels);

        for (int b = 0; b < numBlocks; b++) {
            const uint8_t* blockData = &adpcmData[b * stride];
            if (channels == 2) DecodeStereoBlock(blockData, pcm);
            else DecodeMonoBlock(blockData, pcm);
        }
        return pcm;
    }
};