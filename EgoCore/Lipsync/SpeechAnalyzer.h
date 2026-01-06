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
    static const int FRAME_SIZE = 512; // ~43.06 FPS

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

    // --- SIGNAL PROCESSING ---
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
            if (i < 14) eLow += mag;       // 0-600Hz
            else if (i < 58) eMid += mag;  // 600-2500Hz
            else eHigh += mag;             // 2500Hz+
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
    // --- HELPER: Load WAV to Mono PCM ---
    static bool LoadWav(const std::string& path, std::vector<int16_t>& outPcm, int& outSampleRate) {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return false;

        char chunkID[4]; f.read(chunkID, 4);
        if (strncmp(chunkID, "RIFF", 4) != 0) return false;
        f.seekg(4, std::ios::cur); // ChunkSize
        char format[4]; f.read(format, 4);
        if (strncmp(format, "WAVE", 4) != 0) return false;

        uint16_t audioFormat = 0;
        uint16_t numChannels = 0;

        // Scan chunks until we find 'fmt ' and 'data'
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
                f.seekg(subChunkSize - 8, std::ios::cur); // Skip rest of fmt
                foundFmt = true;
            }
            else if (strncmp(subChunkID, "data", 4) == 0) {
                if (!foundFmt) return false; // Malformed
                if (audioFormat != 1) return false; // Only PCM supported

                int samples = subChunkSize / 2; // 16-bit
                std::vector<int16_t> rawData(samples);
                f.read((char*)rawData.data(), subChunkSize);

                if (numChannels == 1) {
                    outPcm = rawData;
                }
                else if (numChannels == 2) {
                    // Mix Stereo to Mono
                    outPcm.clear();
                    outPcm.reserve(samples / 2);
                    for (size_t i = 0; i < rawData.size(); i += 2) {
                        int32_t mixed = (rawData[i] + rawData[i + 1]) / 2;
                        outPcm.push_back((int16_t)mixed);
                    }
                }
                else return false; // Unsupported channels

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

        // 1. Setup Signatures
        // These are heuristic baselines for Fable's basic phoneme set
        std::vector<PhonemeSignature> signatures;
        signatures.push_back({ 0, 0.40f, 0.40f, 0.20f, 0.10f }); // AA
        signatures.push_back({ 1, 0.20f, 0.50f, 0.30f, 0.10f }); // EE
        signatures.push_back({ 2, 0.80f, 0.15f, 0.05f, 0.02f }); // MM
        signatures.push_back({ 3, 0.60f, 0.30f, 0.10f, 0.05f }); // OH
        signatures.push_back({ 4, 0.10f, 0.20f, 0.70f, 0.45f }); // SZ

        // IMPORTANT: Add to dictionary in Sorted ID order (0..4)
        std::vector<std::string> symbols = { "AA", "EE", "MM", "OH", "SZ" };
        for (uint8_t i = 0; i < symbols.size(); i++) {
            result.Dictionary.push_back({ i, symbols[i] });
        }

        // 2. Setup Timing
        size_t totalSamples = pcmData.size();
        size_t numFrames = totalSamples / FRAME_SIZE;
        result.FPS = (float)SAMPLE_RATE / FRAME_SIZE;
        result.FrameCount = (uint32_t)numFrames;
        result.Duration = (float)totalSamples / SAMPLE_RATE;

        // 3. Process
        size_t cursor = 0;
        for (size_t f = 0; f < numFrames; f++) {
            std::vector<int16_t> framePcm;
            if (cursor + FRAME_SIZE <= totalSamples) {
                framePcm.assign(pcmData.begin() + cursor, pcmData.begin() + cursor + FRAME_SIZE);
                cursor += FRAME_SIZE;
            }
            else break;

            FrameAnalysis fa = AnalyzeFrame(framePcm);
            CLipSyncFrame lsFrame;

            // Silence Gate
            if (fa.RMS > 0.015f) {
                struct Score { uint8_t ID; float Val; };
                std::vector<Score> scores;

                for (const auto& sig : signatures) {
                    float dLow = fa.EnergyLow - sig.TargetLow;
                    float dMid = fa.EnergyMid - sig.TargetMid;
                    float dHigh = fa.EnergyHigh - sig.TargetHigh;
                    float dZCR = (fa.ZCR - sig.TargetZCR) * 2.5f;

                    float distSq = (dLow * dLow) + (dMid * dMid) + (dHigh * dHigh) + (dZCR * dZCR);
                    float distance = sqrt(distSq);

                    float confidence = 1.0f - (distance * 1.5f);
                    if (confidence < 0.0f) confidence = 0.0f;
                    if (confidence > 1.0f) confidence = 1.0f;

                    float volumeFactor = (std::min)(fa.RMS * 5.0f, 1.0f);
                    float finalWeight = confidence * volumeFactor;

                    if (finalWeight > 0.01f) {
                        scores.push_back({ sig.ID, finalWeight });
                    }
                }

                // Sort Descending by Score
                std::sort(scores.begin(), scores.end(), [](const Score& a, const Score& b) {
                    return a.Val > b.Val;
                    });

                // Take Top 3
                int count = (std::min)((int)scores.size(), 3);

                for (int i = 0; i < count; ++i) {
                    if (scores[i].Val > 0.15f) {
                        CLipSyncFrameKey key;
                        key.ID = scores[i].ID;
                        key.WeightFloat = scores[i].Val;
                        key.WeightByte = (uint8_t)(scores[i].Val * 255.0f);
                        lsFrame.Keys.push_back(key);
                    }
                }
            }
            result.Frames.push_back(lsFrame);
        }

        return result;
    }
};