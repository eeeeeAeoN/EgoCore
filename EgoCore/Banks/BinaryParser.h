#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <regex>
#include <filesystem>
#include <set>
#include "DefBackend.h" // Access to g_DefWorkspace

struct BinaryEntry {
    uint32_t CRC;
    int32_t ID;
};

struct BinaryData {
    std::string FileName;
    std::vector<BinaryEntry> Entries;
    bool IsParsed = false;
};

class BinaryParser {
public:
    BinaryData Data;
    std::string StatusMessage;

    // --- STANDARD CRC32 (Reference) ---
    static uint32_t CalculateCRC32_Standard(const std::string& str) {
        static uint32_t table[256];
        static bool tableComputed = false;
        if (!tableComputed) {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t c = i;
                for (int j = 0; j < 8; j++) c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
                table[i] = c;
            }
            tableComputed = true;
        }
        uint32_t crc = 0xFFFFFFFF;
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (char c : lower) crc = table[(uint8_t)c ^ (crc & 0xFF)] ^ (crc >> 8);
        return ~crc;
    }

    // --- FABLE CUSTOM CRC32 ---
    // Init 0, No Final XOR, Case Sensitive processing
    static uint32_t CalculateCRC32_Fable(const std::string& str) {
        static uint32_t table[256];
        static bool tableComputed = false;
        if (!tableComputed) {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t c = i;
                for (int j = 0; j < 8; j++) c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
                table[i] = c;
            }
            tableComputed = true;
        }

        uint32_t crc = 0;
        for (char c : str) {
            crc = table[(uint8_t)crc ^ (uint8_t)c] ^ (crc >> 8);
        }
        return crc;
    }

    static uint32_t CalculateCRC32(const std::string& str) {
        // Wrapper for the checker UI
        return CalculateCRC32_Fable(str);
    }

    void Parse(const std::string& path) {
        Data = BinaryData();
        Data.FileName = std::filesystem::path(path).filename().string();
        StatusMessage = "Loading...";

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            StatusMessage = "Failed to open file.";
            return;
        }

        int32_t count = 0;
        file.read((char*)&count, 4);

        if (count < 0 || count > 2000000) {
            StatusMessage = "Invalid header (Count: " + std::to_string(count) + ")";
            return;
        }

        Data.Entries.reserve(count);
        for (int i = 0; i < count; i++) {
            BinaryEntry e;
            file.read((char*)&e.CRC, 4);
            file.read((char*)&e.ID, 4);
            if (file.gcount() != 4) break;
            Data.Entries.push_back(e);
        }

        Data.IsParsed = true;
        StatusMessage = "Loaded " + std::to_string(Data.Entries.size()) + " entries.";
    }

    // --- COMPILER FUNCTION ---
    static void CompileSoundBinaries(const std::string& outputFolder, std::string& outStatus) {
        if (!g_DefWorkspace.IsLoaded) {
            outStatus = "Error: Definitions not loaded! Please set game path first.";
            return;
        }

        outStatus = "Scanning SOUND_SETUP definitions...\n";

        std::set<std::string> targetHeaders;

        // Defaults
        targetHeaders.insert("dialoguesnds.h");
        targetHeaders.insert("dialoguesnds2.h");
        targetHeaders.insert("scriptdialoguesnds.h");
        targetHeaders.insert("scriptdialoguesnds2.h");

        // Parse SOUND_SETUP definitions to find referenced headers
        if (g_DefWorkspace.CategorizedDefs.count("SOUND_SETUP")) {
            const auto& setups = g_DefWorkspace.CategorizedDefs["SOUND_SETUP"];

            // FIXED: Added 'Rgx' delimiter to avoid conflict with )" in the regex pattern
            std::regex bankRegex(R"Rgx(CSoundBankEntry\s*\(\s*[^,]+,\s*"[^"]+",\s*"([^"]+)")Rgx");

            for (const auto& def : setups) {
                std::ifstream defFile(def.SourceFile, std::ios::binary);
                if (defFile.is_open()) {
                    defFile.seekg(def.StartOffset);
                    std::string content(def.EndOffset - def.StartOffset, '\0');
                    defFile.read(&content[0], content.size());

                    auto begin = std::sregex_iterator(content.begin(), content.end(), bankRegex);
                    auto end = std::sregex_iterator();

                    for (std::sregex_iterator i = begin; i != end; ++i) {
                        std::smatch match = *i;
                        std::string headerName = match[1].str();
                        if (targetHeaders.find(headerName) == targetHeaders.end()) {
                            targetHeaders.insert(headerName);
                            outStatus += "> Found new dependency: " + headerName + "\n";
                        }
                    }
                }
            }
        }

        int successCount = 0;
        std::regex entryRegex(R"(^\s*(\w+)\s*=\s*(\d+)\s*,?)");

        for (const auto& target : targetHeaders) {
            auto it = std::find_if(g_DefWorkspace.AllEnums.begin(), g_DefWorkspace.AllEnums.end(),
                [&](const EnumEntry& e) {
                    return std::filesystem::path(e.FilePath).filename().string() == target;
                });

            if (it == g_DefWorkspace.AllEnums.end()) {
                outStatus += "Skipped (Not Loaded): " + target + "\n";
                continue;
            }

            std::vector<BinaryEntry> binEntries;
            std::stringstream ss(it->FullContent);
            std::string line;

            while (std::getline(ss, line)) {
                std::smatch match;
                if (std::regex_search(line, match, entryRegex)) {
                    std::string name = match[1].str();
                    int id = std::stoi(match[2].str());

                    BinaryEntry be;
                    be.ID = id;
                    be.CRC = CalculateCRC32_Fable(name);
                    binEntries.push_back(be);
                }
            }

            if (binEntries.empty()) {
                outStatus += "Skipped (Empty): " + target + "\n";
                continue;
            }

            std::filesystem::path outPath = std::filesystem::path(outputFolder) / target;
            outPath.replace_extension(".bin");

            std::ofstream outFile(outPath, std::ios::binary);
            if (outFile.is_open()) {
                int32_t count = (int32_t)binEntries.size();
                outFile.write((char*)&count, 4);

                std::sort(binEntries.begin(), binEntries.end(), [](const BinaryEntry& a, const BinaryEntry& b) {
                    return a.CRC < b.CRC;
                    });

                for (const auto& e : binEntries) {
                    outFile.write((char*)&e.CRC, 4);
                    outFile.write((char*)&e.ID, 4);
                }
                outFile.close();
                successCount++;
                outStatus += "Compiled: " + outPath.filename().string() + " (" + std::to_string(count) + " entries)\n";
            }
            else {
                outStatus += "Error Writing: " + outPath.string() + "\n";
            }
        }

        if (successCount == 0) outStatus += "Warning: No binaries were compiled.\n";
        else outStatus += "Done. " + std::to_string(successCount) + " files compiled.\n";
    }
};