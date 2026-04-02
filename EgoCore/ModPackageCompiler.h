#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include "ConfigBackend.h"
#include "BankBackend.h"
#include <unordered_set>
#include <fstream>

namespace fs = std::filesystem;

struct StagedModPackageEntry {
    uint32_t EntryID;
    std::string EntryName;
    int32_t EntryType;
    EBankType BankType;
    std::string TypeName;
    std::string BankName;
    std::string SubBankName;
    std::string SourceFullPath;
};

inline std::string GetEntryTypeName(EBankType bankType, int32_t type, const std::string& bankName) {
    if (bankType == EBankType::Graphics || bankType == EBankType::XboxGraphics) {
        switch (type) {
        case 1: return "Static Mesh";
        case 2: return "Repeated Mesh";
        case 3: return "Physics (BBM)";
        case 4: return "Particle Mesh";
        case 5: return "Animated Mesh";
        case 6: return "Animation";
        case 7: return "Delta Animation";
        case 8: return "Lipsync Animation";
        case 9: return "Partial Animation";
        case 10: return "Relative Animation";
        default: return "Gfx " + std::to_string(type);
        }
    }
    if (bankType == EBankType::Textures || bankType == EBankType::Frontend) {
        switch (type) {
        case 0: return "Graphic Single";
        case 1: return "Graphic Sequence";
        case 2: return "Bumpmap";
        case 3: return "Bumpmap Sequence";
        case 4: return "Volume Texture";
        case 5: return "Flat Sequence";
        default: return "Tex " + std::to_string(type);
        }
    }
    if (bankType == EBankType::Shaders) {
        switch (type) {
        case 0: return "Vertex Shader";
        case 1: return "Pixel Shader";
        default: return "Shader " + std::to_string(type);
        }
    }
    if (bankType == EBankType::Effects) {
        switch (type) {
        case 0: return "Particle Entry";
        default: return "Effect " + std::to_string(type);
        }
    }
    if (bankType == EBankType::Dialogue) {
        switch (type) {
        case 1: return "Lipsync Entry";
        default: return "Dialogue " + std::to_string(type);
        }
    }
    if (bankType == EBankType::Fonts) {
        switch (type) {
        case 0: return "PC Font";
        case 1: return "Xbox Font";
        case 2: return "GlyphData";
        default: return "Font " + std::to_string(type);
        }
    }
    if (bankType == EBankType::Text) {
        switch (type) {
        case 0: return "Text Entry";
        case 1: return "Group Text Entry";
        case 2: return "Narrator List";
        default: return "Text " + std::to_string(type);
        }
    }
    if (bankType == EBankType::Audio) {
        std::string lowerName = bankName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        if (lowerName.find(".lut") != std::string::npos) return "Audio Clip (.lut)";
        if (lowerName.find(".lug") != std::string::npos) return "Scripted Audio (.lug)";

        return "Audio Entry";
    }
    return "Type " + std::to_string(type);
}

class ModPackageCompiler {
public:
    static std::string ExtractLanguageFromPath(const std::string& fullPath) {
        std::string lowerPath = fullPath;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

        size_t langPos = lowerPath.find("\\lang\\");
        if (langPos == std::string::npos) langPos = lowerPath.find("/lang/");

        if (langPos != std::string::npos) {
            size_t start = langPos + 6;
            size_t end = fullPath.find_first_of("\\/", start);
            if (end != std::string::npos) {
                return fullPath.substr(start, end - start);
            }
        }
        return "English";
    }

    static std::string GetTargetDirectoryForEntry(const StagedModPackageEntry& entry, const std::string& modRoot) {
        std::string basePath = "";
        std::string bankCategory = "";

        // Identify if this is a LUT file early to handle naming conflicts
        std::string lowerBank = entry.BankName;
        std::transform(lowerBank.begin(), lowerBank.end(), lowerBank.begin(), ::tolower);
        bool isLut = lowerBank.find(".lut") != std::string::npos;

        // 1. Clean the Bank Name (e.g., "graphics.big" -> "Graphics")
        std::string bankNameClean = entry.BankName;
        size_t dotPos = bankNameClean.find_last_of('.');
        if (dotPos != std::string::npos) {
            bankNameClean = bankNameClean.substr(0, dotPos);
        }

        // --- FIX: Append _LUT to the name to avoid conflict with .big files ---
        if (isLut) {
            bankNameClean += "_LUT";
        }

        if (!bankNameClean.empty()) {
            bankNameClean[0] = (char)std::toupper(bankNameClean[0]);
        }

        // 2. Handle Xbox-to-PC SubBank conversions
        std::string finalSubBank = entry.SubBankName;
        if (finalSubBank != "N/A" && !finalSubBank.empty()) {
            if (finalSubBank == "GBANK_MAIN") finalSubBank = "GBANK_MAIN_PC";
            else if (finalSubBank == "GBANK_GUI") finalSubBank = "GBANK_GUI_PC";
            else if (finalSubBank == "GBANK_FRONT_END") finalSubBank = "GBANK_FRONT_END_PC";
            else if (finalSubBank == "PARTICLE_MAIN") finalSubBank = "PARTICLE_MAIN_PC";
        }

        // 3. Determine Base Path and Category 
        if (entry.BankType == EBankType::Graphics || entry.BankType == EBankType::XboxGraphics) {
            if (finalSubBank == "GBANK_MAIN_PC" || finalSubBank == "GBANK_GUI_PC") {
                basePath = "Data\\Graphics\\pc";
                bankCategory = "Textures";
            }
            else {
                basePath = "Data\\Graphics";
                bankCategory = bankNameClean;
            }
        }
        else if (entry.BankType == EBankType::Textures || entry.BankType == EBankType::Frontend) {
            basePath = "Data\\Graphics\\pc";
            bankCategory = "Textures";
        }
        else if (entry.BankType == EBankType::Effects) {
            basePath = "Data\\Misc\\pc";
            bankCategory = bankNameClean;
        }
        else if (entry.BankType == EBankType::Shaders) {
            basePath = "Data\\Shaders\\pc";
            bankCategory = bankNameClean;
        }
        else if (entry.BankType == EBankType::Audio) {
            if (isLut) {
                // Dialogue .lut sounds exported to Data/Lang/[Language]/[BankName]_LUT
                std::string lang = ExtractLanguageFromPath(entry.SourceFullPath);
                basePath = "Data\\Lang\\" + lang;
            }
            else {
                // Sounds .lug files exported to Data/Sound/[BankName]
                basePath = "Data\\Sound";
            }
            bankCategory = bankNameClean;
        }
        else if (entry.BankType == EBankType::Dialogue || entry.BankType == EBankType::Text || entry.BankType == EBankType::Fonts) {
            std::string lang = ExtractLanguageFromPath(entry.SourceFullPath);
            basePath = "Data\\Lang\\" + lang;
            bankCategory = bankNameClean;
        }
        else {
            basePath = "Data\\Unknown";
            bankCategory = bankNameClean;
        }

        // 4. Construct Final Path
        if (finalSubBank != "N/A" && !finalSubBank.empty()) {
            return modRoot + "\\" + basePath + "\\" + bankCategory + "\\" + finalSubBank;
        }

        return modRoot + "\\" + basePath + "\\" + bankCategory;
    }

    static std::string SanitizeForEnum(const std::string& name, bool forceUpper = true) {
        std::string out = name;
        for (char& c : out) {
            if (!std::isalnum(c)) {
                c = '_';
            }
            else if (forceUpper) {
                c = (char)std::toupper(c);
            }
        }
        if (!out.empty() && std::isdigit(out[0])) out = "_" + out;
        if (out.empty()) out = "UNKNOWN_ENTRY";
        return out;
    }

    static void GenerateEnumHeaders(const std::string& modName, const std::vector<StagedModPackageEntry>& entries) {
        std::string baseHeaderPath = g_AppConfig.GameRootPath + "\\Mods\\" + modName + "\\Defs\\RetailHeaders";
        std::string pcHeaderPath = baseHeaderPath + "\\pc";

        std::map<std::string, std::map<std::string, std::vector<StagedModPackageEntry>>> filesToGenerate;

        for (const auto& entry : entries) {
            std::string targetFile = "";
            std::string enumName = "";
            bool isPcPath = false;

            if (entry.BankType == EBankType::Fonts || entry.BankType == EBankType::Shaders) continue;

            // Meshes and Animations
            if (entry.BankType == EBankType::Graphics || entry.BankType == EBankType::XboxGraphics) {
                if (entry.SubBankName == "MBANK_ALLMESHES") {
                    targetFile = "meshdata.h";
                    if (entry.EntryType >= 1 && entry.EntryType <= 5) enumName = "EMeshType2";
                    else if (entry.EntryType >= 6 && entry.EntryType <= 10) enumName = "EAnimType2";
                }
            }
            // Text
            else if (entry.BankType == EBankType::Text) {
                if (entry.EntryType == 0) { targetFile = "text.h"; enumName = "EGameText"; }
                else if (entry.EntryType == 1) { targetFile = "text.h"; enumName = "ETextGroup"; }
            }
            // --- FIXED DIALOGUE ENUM SPLITTING ---
            else if (entry.BankType == EBankType::Dialogue && entry.EntryType == 1) {
                std::string sub = entry.SubBankName;
                if (sub.find("_MAIN_2") != std::string::npos) { targetFile = "dialogue_lipsync2.h"; enumName = "ELipSync3"; }
                else if (sub.find("_MAIN") != std::string::npos) { targetFile = "dialogue_lipsync.h"; enumName = "ELipSync"; }
                else if (sub.find("_SCRIPT_2") != std::string::npos) { targetFile = "script_lipsync2.h"; enumName = "ELipSync4"; }
                else if (sub.find("_SCRIPT") != std::string::npos) { targetFile = "script_lipsync.h"; enumName = "ELipSync2"; }
            }
            // Particles
            else if (entry.BankType == EBankType::Effects) {
                targetFile = "particles.h"; enumName = "EParticleEmitter"; isPcPath = true;
            }
            // Textures
            else if (entry.BankType == EBankType::Textures || entry.BankType == EBankType::Frontend) {
                if (entry.EntryName.size() >= 2 && entry.EntryName.substr(0, 2) == "[\\") continue;
                isPcPath = true;
                std::string sub = entry.SubBankName;
                if (sub == "GBANK_MAIN_PC" || sub == "GBANK_MAIN") { targetFile = "textures.h"; enumName = "EEngineGraphic"; }
                else if (sub == "GBANK_GUI_PC" || sub == "GBANK_GUI") { targetFile = "gui_bank.h"; enumName = "EGuiGraphicBank"; }
                else if (sub == "GBANK_FRONT_END" || sub == "GBANK_FRONT_END_PC") { targetFile = "front_end_bank.h"; enumName = "EFrontEndGraphicBank"; }
            }

            if (!targetFile.empty() && !enumName.empty()) {
                std::string fullPath = (isPcPath ? pcHeaderPath : baseHeaderPath) + "\\" + targetFile;
                filesToGenerate[fullPath][enumName].push_back(entry);
            }
        }

        if (!fs::exists(baseHeaderPath)) fs::create_directories(baseHeaderPath);
        if (!fs::exists(pcHeaderPath)) fs::create_directories(pcHeaderPath);

        for (const auto& [filePath, enumsMap] : filesToGenerate) {
            std::ofstream outFile(filePath);
            if (!outFile.is_open()) continue;

            outFile << "#pragma once\n\n";
            for (const auto& [name, enumEntries] : enumsMap) {
                outFile << "enum " << name << "\n{\n";

                // Check if this is a LipSync enum (ELipSync, ELipSync2, etc.)
                bool isLipSync = (name.find("LipSync") != std::string::npos);

                for (const auto& e : enumEntries) {
                    // Pass !isLipSync as the forceUpper argument
                    outFile << "    " << SanitizeForEnum(e.EntryName, !isLipSync) << " = " << e.EntryID << ",\n";
                }
                outFile << "};\n\n";
            }
            outFile.close();
        }
    }

    static void BuildPackageStructure(const std::string& modName, const std::vector<StagedModPackageEntry>& entries) {
        if (modName.empty() || entries.empty()) return;

        // --- 1. COMPILE ALL PENDING BANKS TO DISK FIRST ---
        // We trigger the actual Save routine for any bank that has staged or modified data
        for (auto& bank : g_OpenBanks) {
            if (!bank.StagedEntries.empty() || !bank.ModifiedEntryData.empty()) {
                if (bank.Type == EBankType::Audio) {
                    SaveAudioBank(&bank);
                }
                else {
                    SaveBigBank(&bank);
                }
            }
        }

        std::string modRoot = g_AppConfig.GameRootPath + "\\Mods\\" + modName;

        if (!fs::exists(modRoot)) {
            fs::create_directories(modRoot);
        }

        // --- 2. EXTRACT DATA & METADATA ---
        for (const auto& entry : entries) {
            std::string targetDir = GetTargetDirectoryForEntry(entry, modRoot);

            if (!fs::exists(targetDir)) {
                fs::create_directories(targetDir);
            }

            // Locate the live bank in RAM
            LoadedBank* targetBank = nullptr;
            for (auto& b : g_OpenBanks) {
                if (b.FileName == entry.BankName) {
                    targetBank = &b;
                    break;
                }
            }

            if (!targetBank) continue;

            // Locate the specific entry index
            int entryIndex = -1;
            for (int i = 0; i < (int)targetBank->Entries.size(); ++i) {
                if (targetBank->Entries[i].ID == entry.EntryID) {
                    entryIndex = i;
                    break;
                }
            }

            if (entryIndex == -1) continue;

            const auto& bankEntry = targetBank->Entries[entryIndex];

            // 3. Extract the DATA bytes and drop them to a .resource file
            std::string resourcePath = targetDir + "\\" + entry.EntryName + ".resource";
            std::ofstream resFile(resourcePath, std::ios::binary);

            if (resFile.is_open()) {
                if (targetBank->ModifiedEntryData.count(entryIndex)) {
                    const auto& data = targetBank->ModifiedEntryData[entryIndex];
                    resFile.write((const char*)data.data(), data.size());
                }
                else if (bankEntry.Size > 0) {

                    // --- NEW: Audio Bank Stream Fallback ---
                    // Since .lut and .lug banks don't keep targetBank->Stream open,
                    // we dynamically open a direct filestream to the source on the user's disk.
                    std::ifstream diskStream;
                    std::istream* activeStream = nullptr;

                    if (targetBank->Stream && targetBank->Stream->is_open()) {
                        activeStream = targetBank->Stream.get();
                    }
                    else {
                        diskStream.open(targetBank->FullPath, std::ios::binary);
                        if (diskStream.is_open()) activeStream = &diskStream;
                    }

                    if (activeStream) {
                        activeStream->clear();
                        activeStream->seekg(bankEntry.Offset, std::ios::beg);

                        std::vector<char> buffer(bankEntry.Size);
                        activeStream->read(buffer.data(), bankEntry.Size);
                        resFile.write(buffer.data(), bankEntry.Size);
                    }

                    if (diskStream.is_open()) diskStream.close();
                }
                resFile.close();
            }

            // 4. Extract the METADATA bytes and drop them to a .header file
            if (bankEntry.InfoSize > 0 && targetBank->SubheaderCache.count(entryIndex)) {
                std::string headerPath = targetDir + "\\" + entry.EntryName + ".header";
                std::ofstream hdrFile(headerPath, std::ios::binary);

                if (hdrFile.is_open()) {
                    const auto& metaData = targetBank->SubheaderCache[entryIndex];
                    hdrFile.write((const char*)metaData.data(), metaData.size());
                    hdrFile.close();
                }
            }
        }

        // --- 5. GENERATE ENUMS ---
        // (Make sure to leave your GenerateEnumHeaders function in this file!)
        GenerateEnumHeaders(modName, entries);
    }
};