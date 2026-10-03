#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <cstring>
#include "LipSyncParser.h" 

class CSpeechAnalyzer {
private:
    static const int SAMPLE_RATE = 22050;
    static const int FRAME_SIZE = 512;

    struct FrameAnalysis {
        float RMS = 0.0f;
        float ZCR = 0.0f;
        float EnergyLow = 0.0f;
        float EnergyMid = 0.0f;
        float EnergyHigh = 0.0f;
    };

    struct PhonemeSignature {
        uint8_t ID;
        float TargetLow;
        float TargetMid;
        float TargetHigh;
        float TargetZCR;
    };

    static void SimpleRealFFT(const std::vector<float>& timeDomain, std::vector<float>& freqMag) {
        size_t N = timeDomain.size();
        freqMag.resize(N / 2);

        std::vector<float> windowed = timeDomain;
        for (size_t i = 0; i < N; ++i) {
            float multiplier = 0.5f * (1.0f - cos(2.0f * 3.14159265f * i / (N - 1)));
            windowed[i] *= multiplier;
        }

        for (size_t k = 0; k < N / 2; k++) {
            float sumReal = 0;
            float sumImag = 0;
            for (size_t t = 0; t < N; t++) {
                float angle = 2.0f * 3.14159265f * t * k / N;
                sumReal += windowed[t] * cos(angle);
                sumImag -= windowed[t] * sin(angle);
            }
            freqMag[k] = sqrt(sumReal * sumReal + sumImag * sumImag);
        }
    }

    static FrameAnalysis AnalyzeFrame(const std::vector<int16_t>& samples) {
        FrameAnalysis fa;
        if (samples.empty()) return fa;

        float sumSq = 0;
        int crossCount = 0;
        std::vector<float> fSamples(samples.size());

        for (size_t i = 0; i < samples.size(); i++) {
            float val = samples[i] / 32768.0f;
            fSamples[i] = val;
            sumSq += val * val;

            if (i > 0) {
                if ((samples[i] > 0 && samples[i - 1] <= 0) || (samples[i] <= 0 && samples[i - 1] > 0)) {
                    crossCount++;
                }
            }
        }
        fa.RMS = sqrt(sumSq / samples.size());
        fa.ZCR = (float)crossCount / samples.size();

        std::vector<float> spectrum;
        SimpleRealFFT(fSamples, spectrum);

        float eLow = 0, eMid = 0, eHigh = 0;
        for (int i = 0; i < (int)spectrum.size(); i++) {
            float mag = spectrum[i];
            if (i < 14) eLow += mag; 
            else if (i < 58) eMid += mag;
            else eHigh += mag;
        }

        float total = eLow + eMid + eHigh;
        if (total > 0.0001f) {
            fa.EnergyLow = eLow / total;
            fa.EnergyMid = eMid / total;
            fa.EnergyHigh = eHigh / total;
        }

        return fa;
    }

public:
    static bool LoadWav(const std::string& path, std::vector<int16_t>& outPcm, int& outSampleRate) {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return false;

        char chunkID[4]; f.read(chunkID, 4);
        if (strncmp(chunkID, "RIFF", 4) != 0) return false;
        f.seekg(4, std::ios::cur);
        char format[4]; f.read(format, 4);
        if (strncmp(format, "WAVE", 4) != 0) return false;

        uint16_t audioFormat = 0;
        uint16_t numChannels = 0;

        bool foundFmt = false;
        bool foundData = false;

        while (f.good() && !foundData) {
            char subChunkID[4];
            uint32_t subChunkSize = 0;
            f.read(subChunkID, 4);
            f.read((char*)&subChunkSize, 4);
            if (f.gcount() < 4) break;

            if (strncmp(subChunkID, "fmt ", 4) == 0) {
                f.read((char*)&audioFormat, 2);
                f.read((char*)&numChannels, 2);
                f.read((char*)&outSampleRate, 4);
                f.seekg(subChunkSize - 8, std::ios::cur);
                foundFmt = true;
            }
            else if (strncmp(subChunkID, "data", 4) == 0) {
                if (!foundFmt) return false;
                if (audioFormat != 1) return false;

                int samples = subChunkSize / 2;
                std::vector<int16_t> rawData(samples);
                f.read((char*)rawData.data(), subChunkSize);

                if (numChannels == 1) {
                    outPcm = rawData;
                }
                else if (numChannels == 2) {
                    outPcm.clear();
                    outPcm.reserve(samples / 2);
                    for (size_t i = 0; i < rawData.size(); i += 2) {
                        int32_t mixed = (rawData[i] + rawData[i + 1]) / 2;
                        outPcm.push_back((int16_t)mixed);
                    }
                }
                else return false;

                foundData = true;
            }
            else {
                f.seekg(subChunkSize, std::ios::cur);
            }
        }
        return foundData;
    }

    static CLipSyncData AnalyzeWav(const std::vector<int16_t>& pcmData, int sampleRate) {
        CLipSyncData result;
        result.IsParsed = true;

        if (pcmData.empty()) return result;

        // Resample input audio to 22050 Hz if needed
        std::vector<int16_t> pcm22k;
        if (sampleRate == SAMPLE_RATE || sampleRate <= 0) {
            pcm22k = pcmData;
        }
        else {
            double ratio = (double)SAMPLE_RATE / (double)sampleRate;
            size_t targetSamples = (size_t)(pcmData.size() * ratio);
            pcm22k.resize(targetSamples);
            for (size_t i = 0; i < targetSamples; i++) {
                double srcIdx = i / ratio;
                size_t idx0 = (size_t)srcIdx;
                size_t idx1 = (idx0 + 1 < pcmData.size()) ? idx0 + 1 : idx0;
                double frac = srcIdx - (double)idx0;
                double val = (1.0 - frac) * (double)pcmData[idx0] + frac * (double)pcmData[idx1];
                pcm22k[i] = (int16_t)std::clamp(val, -32768.0, 32767.0);
            }
        }

        // Fable standard spoken phonemes (MM is excluded as it is the default closed-mouth base)
        // ID 0: AH (open jaw / vowel)
        // ID 1: EE (wide / spread lips)
        // ID 2: OH (rounded open lips)
        // ID 3: SZ (fricative / sibilant)
        // ID 4: WW (puckered / rounded narrow)
        std::vector<PhonemeSignature> signatures = {
            { 0, 0.35f, 0.50f, 0.15f, 0.08f }, // AH
            { 1, 0.20f, 0.55f, 0.25f, 0.12f }, // EE
            { 2, 0.60f, 0.32f, 0.08f, 0.05f }, // OH
            { 3, 0.05f, 0.20f, 0.75f, 0.40f }, // SZ
            { 4, 0.80f, 0.16f, 0.04f, 0.03f }  // WW
        };

        std::vector<std::string> symbols = { "AH", "EE", "OH", "SZ", "WW" };
        for (uint8_t i = 0; i < (uint8_t)symbols.size(); i++) {
            result.Dictionary.push_back({ i, symbols[i] });
        }

        size_t totalSamples = pcm22k.size();
        size_t numFrames = totalSamples / FRAME_SIZE;
        result.FPS = (float)SAMPLE_RATE / (float)FRAME_SIZE;
        result.FrameCount = (uint32_t)numFrames;
        result.Duration = (float)totalSamples / (float)SAMPLE_RATE;

        if (numFrames == 0) return result;

        // Pass 1: Raw acoustic analysis and confidence per frame
        std::vector<std::vector<float>> rawWeights(numFrames, std::vector<float>(signatures.size(), 0.0f));
        size_t cursor = 0;

        for (size_t f = 0; f < numFrames; f++) {
            std::vector<int16_t> framePcm(pcm22k.begin() + cursor, pcm22k.begin() + cursor + FRAME_SIZE);
            cursor += FRAME_SIZE;

            FrameAnalysis fa = AnalyzeFrame(framePcm);

            // Speech energy gate: silence/background hum (< 0.018f) leaves weights at 0.0
            if (fa.RMS > 0.018f) {
                // Volume factor: scale mouth opening with speech intensity
                float volumeFactor = std::clamp((fa.RMS - 0.014f) / 0.055f, 0.0f, 1.0f);

                for (size_t s = 0; s < signatures.size(); s++) {
                    const auto& sig = signatures[s];
                    float dLow = fa.EnergyLow - sig.TargetLow;
                    float dMid = fa.EnergyMid - sig.TargetMid;
                    float dHigh = fa.EnergyHigh - sig.TargetHigh;
                    float dZCR = (fa.ZCR - sig.TargetZCR) * 2.0f;

                    float distSq = (dLow * dLow) + (dMid * dMid) + (dHigh * dHigh) + (dZCR * dZCR);
                    float distance = sqrtf(distSq);

                    float confidence = 1.0f - (distance * 1.6f);
                    if (confidence < 0.0f) confidence = 0.0f;

                    rawWeights[f][s] = confidence * volumeFactor;
                }
            }
        }

        // Pass 2: Temporal attack/decay smoothing to avoid erratic jitter between adjacent frames
        std::vector<float> prevWeights(signatures.size(), 0.0f);
        for (size_t f = 0; f < numFrames; f++) {
            CLipSyncFrame lsFrame;
            struct Candidate { uint8_t ID; float Weight; };
            std::vector<Candidate> candidates;

            for (size_t s = 0; s < signatures.size(); s++) {
                float target = rawWeights[f][s];
                // Fast attack when opening, smooth decay when closing
                if (target > prevWeights[s]) {
                    prevWeights[s] += (target - prevWeights[s]) * 0.65f;
                }
                else {
                    prevWeights[s] += (target - prevWeights[s]) * 0.35f;
                }

                if (prevWeights[s] < 0.03f) prevWeights[s] = 0.0f;

                if (prevWeights[s] > 0.12f) {
                    candidates.push_back({ signatures[s].ID, prevWeights[s] });
                }
            }

            if (!candidates.empty()) {
                std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
                    return a.Weight > b.Weight;
                });

                // Pick top dominant phonemes (up to 2)
                float topWeight = candidates[0].Weight;
                CLipSyncFrameKey key1;
                key1.ID = candidates[0].ID;
                key1.WeightFloat = std::clamp(topWeight, 0.15f, 1.0f);
                key1.WeightByte = (uint8_t)(key1.WeightFloat * 255.0f);
                lsFrame.Keys.push_back(key1);

                if (candidates.size() > 1 && candidates[1].Weight >= 0.20f && candidates[1].Weight >= topWeight * 0.35f) {
                    CLipSyncFrameKey key2;
                    key2.ID = candidates[1].ID;
                    float w2 = candidates[1].Weight;
                    if (key1.WeightFloat + w2 > 1.0f) w2 = 1.0f - key1.WeightFloat;
                    if (w2 >= 0.10f) {
                        key2.WeightFloat = w2;
                        key2.WeightByte = (uint8_t)(w2 * 255.0f);
                        lsFrame.Keys.push_back(key2);
                    }
                }
            }

            // Silent frames remain empty (null keys, 0 weight), keeping mouth completely shut in default MM
            result.Frames.push_back(lsFrame);
        }

        return result;
    }
};