#include "DefinitionManager.h"
#include <fstream>
#include <iostream>
#include <zlib.h>

CDefinitionManager::CDefinitionManager(const std::wstring& outFileName) : m_CompiledFileName(outFileName) {
    m_StringTable = new CDefStringTable();
}

CDefinitionManager::~CDefinitionManager() {
    for (auto obj : m_InstantiatedDefs) delete obj;
    delete m_StringTable;
}

std::string CDefinitionManager::ReadFileToString(const std::wstring& path) {
    std::ifstream file(std::string(path.begin(), path.end()), std::ios::binary);
    if (!file.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void CDefinitionManager::AddDefClass(const std::string& name, DefAllocFunc allocFunc) {
    uint32_t crc = CDefStringTable::GetCRC(name);
    m_RegisteredClasses[crc] = { name, crc, allocFunc };
    m_StringTable->AddString("NULLDEF_" + name);
    m_StringTable->AddString(name);
}

void CDefinitionManager::SetSymbolPaths(const std::vector<std::wstring>& paths) { m_SymbolPathList = paths; }
void CDefinitionManager::SetCompilePaths(const std::vector<std::wstring>& paths) { m_CompilePathList = paths; }

void CDefinitionManager::Compile() {
    LogToFile("--- Starting Compilation Sequence ---");

    LogToFile("Step 1/4: Generating Symbol Map...");
    CreateSymbolsFromPathList();

    LogToFile("Step 2/4: Parsing and Instantiating .def files...");
    DoCompilePathList();

    std::string outPath(m_CompiledFileName.begin(), m_CompiledFileName.end());
    size_t lastSlash = outPath.find_last_of("\\/");
    std::string dirPath = (lastSlash != std::string::npos) ? outPath.substr(0, lastSlash) : "";

    LogToFile("Step 3/4: Saving String Table (name.bin)...");
    std::string nameBinPath = dirPath + "\\name.bin";
    m_StringTable->SaveTable(nameBinPath);
    LogToFile("Successfully exported String Table to: " + nameBinPath);

    LogToFile("Step 4/4: Packing Binary Definitions (frontend.bin)...");
    SaveBinaryDefinitions();

    LogToFile("--- All Steps Completed Successfully ---");
}

void CDefinitionManager::CreateSymbolsFromPathList() {
    m_SymbolMap.clear();

    for (const auto& path : m_SymbolPathList) {
        std::string fileName(path.begin(), path.end());
        LogToFile(" -> Reading header for symbols: " + fileName);

        std::string rawContent = ReadFileToString(path);
        if (rawContent.empty()) {
            LogToFile("    ! Warning: File is empty or missing.");
            continue;
        }
        ParseStringForSymbols(rawContent, false);
    }
    LogToFile("Symbol Map populated with " + std::to_string(m_SymbolMap.size()) + " entries.");
}

void CDefinitionManager::ParseEnumForSymbols(CStringParser& parser) {
    std::string enumName = "NULL";
    if (parser.PeekNextItemType() == EParsedItemType::Identifier) {
        parser.ReadAsString(enumName);
    }

    std::string symbol;
    if (!parser.ReadAsSymbol(symbol) || symbol != "{") return;

    int currentValue = 0;
    CParsedItem item;

    while (parser.ReadNextItem(item)) {
        if (item.Type == EParsedItemType::Symbol && item.StringValue == "}") break;
        if (item.Type != EParsedItemType::Identifier) return;

        std::string memberName = item.StringValue;
        if (!parser.ReadAsSymbol(symbol)) break;

        if (symbol == "=") {
            currentValue = parser.ReadAsInteger();
            m_SymbolMap[memberName] = currentValue;
            if (parser.PeekNextItemType() == EParsedItemType::Symbol) {
                parser.PeekNextItem(item);
                if (item.StringValue == ",") parser.ReadNextSymbol(item);
            }
            currentValue++;
        }
        else if (symbol == "," || symbol == "}") {
            m_SymbolMap[memberName] = currentValue;
            currentValue++;
            if (symbol == "}") break;
        }
        else {
            return;
        }
    }
}

void CDefinitionManager::ParseStringForSymbols(const std::string& script, bool parseDefs) {
    CStringParser parser;
    parser.Init(script);

    if (parseDefs) {
        while (parser.SkipPastWholeString("#definition_template")) {
            std::string defClass, defName;
            if (parser.ReadAsString(defClass) && parser.ReadAsString(defName)) {
                m_StringTable->AddString(defClass);
                m_StringTable->AddString("NULLDEF_" + defClass);
                m_StringTable->AddString(defName);
            }
        }
        parser.SetStringPos(0);
        while (parser.SkipPastWholeString("#definition")) {
            std::string defClass, defName;
            if (parser.ReadAsString(defClass) && parser.ReadAsString(defName)) {
                m_StringTable->AddString(defClass);
                m_StringTable->AddString("NULLDEF_" + defClass);
                m_StringTable->AddString(defName);
                CParsedItem peekItem;
                if (parser.PeekNextItem(peekItem) && peekItem.StringValue == "specialises") {
                    parser.ReadNextItem(peekItem);
                    std::string parentName;
                    if (parser.ReadAsString(parentName)) m_StringTable->AddString(parentName);
                }
            }
        }
        parser.SetStringPos(0);
    }

    while (parser.SkipPastWholeString("enum")) {
        ParseEnumForSymbols(parser);
    }
}

void CDefinitionManager::DoCompilePathList() {
    for (const auto& path : m_CompilePathList) {
        std::string fileName(path.begin(), path.end());
        LogToFile(" -> Compiling File: " + fileName);

        std::string rawContent = ReadFileToString(path);
        if (rawContent.empty()) continue;

        CStringParser parser;
        parser.Init(rawContent, fileName);

        while (parser.NextItemExists()) {
            CParsedItem item;
            parser.PeekNextItem(item);

            // Now that # is handled, this single string check will work perfectly
            if (item.StringValue == "#definition_template" || item.StringValue == "#definition") {
                parser.ReadNextItem(item); // Consume directive
                std::string defClass, defName;
                if (parser.ReadAsString(defClass) && parser.ReadAsString(defName)) {
                    LogToFile("    + " + defClass + " [" + defName + "]");
                    CompileDefinition(parser, defClass, defName, false);
                }
            }
            else {
                parser.ReadNextItem(item); // Skip random noise between blocks
            }
        }
    }
}

void CDefinitionManager::CompileDefinition(CStringParser& parser, const std::string& defClass, const std::string& defName, bool isTemplate) {
    uint32_t typeCrc = CDefStringTable::GetCRC(defClass);
    auto classIt = m_RegisteredClasses.find(typeCrc);
    if (classIt == m_RegisteredClasses.end()) {
        LogToFile("      ! Unknown Class: " + defClass);
        parser.SkipUntilWholeString("#end_definition");
        return;
    }

    IDefObject* newDef = classIt->second.AllocFunc();
    if (!newDef) return;
    newDef->SetInstantiationName(defName);

    CParsedItem item;
    while (parser.NextItemExists()) {
        if (parser.PeekNextItem(item) && item.StringValue == "#end_definition") {
            parser.ReadNextItem(item);
            break;
        }

        auto prePos = parser.SaveState();
        try {
            newDef->ParseFromText(parser, m_SymbolMap);
        }
        catch (const std::exception& e) {
            LogToFile("      CRITICAL ERROR in " + defName + ": " + e.what());
        }

        auto postPos = parser.SaveState();
        if (prePos.Ptr == postPos.Ptr) {
            parser.ReadNextItem(item);
            LogToFile("      ? Property Skipped: " + item.StringValue);
        }
    }
    m_InstantiatedDefs.push_back(newDef);
}

void CDefinitionManager::SaveBinaryDefinitions() {
    std::string outPath(m_CompiledFileName.begin(), m_CompiledFileName.end());
    std::ofstream os(outPath, std::ios::binary);
    if (!os.is_open()) {
        LogToFile("ERROR: Failed to open output file for writing: " + outPath);
        return;
    }

    LogToFile(" -> Writing Global Header...");
    uint8_t useSafeBinary = 0;
    uint32_t dependencyCRC = 0xDE4C6EE8;
    uint32_t randomID = 0xE36C34E8;
    uint32_t noDefs = (uint32_t)m_InstantiatedDefs.size();

    os.write((char*)&useSafeBinary, 1);
    os.write((char*)&dependencyCRC, 4);
    os.write((char*)&randomID, 4);
    os.write((char*)&noDefs, 4);

    LogToFile(" -> Writing Definition Roster (" + std::to_string(noDefs) + " entries)...");
    for (auto* def : m_InstantiatedDefs) {
        uint32_t classTablePos = m_StringTable->AddString(def->GetClassName());
        uint32_t instTablePos = m_StringTable->AddString(def->GetInstantiationName());
        uint32_t classIndex = 0;

        os.write((char*)&classTablePos, 4);
        os.write((char*)&instTablePos, 4);
        os.write((char*)&classIndex, 4);
    }

    LogToFile(" -> Serializing and Chunking Data...");
    std::vector<uint8_t> uncompressedBuffer;
    std::vector<uint16_t> defOffsets;
    std::vector<std::pair<uint32_t, uint32_t>> chunkMap;
    std::vector<uint8_t> compressedStream;

    uint32_t firstCompressedDef = 0;

    for (uint32_t i = 0; i < noDefs; ++i) {
        CMemoryDataOutputStream defOs;
        m_InstantiatedDefs[i]->SerializeOut(&defOs);

        uint32_t projectedSize = uncompressedBuffer.size() + (defOffsets.size() * 2) + defOs.GetLength() + 2;

        if (!uncompressedBuffer.empty() && projectedSize > 0x8000) {
            LogToFile("    * Chunk Threshold reached at Def " + std::to_string(i) + ". Compressing block...");
            CompressBlock(uncompressedBuffer, defOffsets, firstCompressedDef, chunkMap, compressedStream);
            firstCompressedDef = i;
        }

        defOffsets.push_back((uint16_t)uncompressedBuffer.size());
        uncompressedBuffer.insert(uncompressedBuffer.end(),
            (uint8_t*)defOs.PeekData(), (uint8_t*)defOs.PeekData() + defOs.GetLength());
    }

    if (!uncompressedBuffer.empty()) {
        LogToFile("    * Compressing final remaining block...");
        CompressBlock(uncompressedBuffer, defOffsets, firstCompressedDef, chunkMap, compressedStream);
    }

    LogToFile(" -> Finalizing Chunk Map (" + std::to_string(chunkMap.size()) + " chunks)...");
    uint32_t chunkMapCount = chunkMap.size();
    os.write((char*)&chunkMapCount, 4);
    for (const auto& chunk : chunkMap) {
        os.write((char*)&chunk.first, 4);
        os.write((char*)&chunk.second, 4);
    }

    LogToFile(" -> Finalizing Compressed Stream...");
    uint32_t compressedSize = compressedStream.size();
    os.write((char*)&compressedSize, 4);
    os.write((char*)compressedStream.data(), compressedStream.size());

    os.close();
    LogToFile("Binary Sequence Complete. frontend.bin written to disk.");
}

void CDefinitionManager::CompressBlock(std::vector<uint8_t>& uncompressed, std::vector<uint16_t>& offsets,
    uint32_t firstDef, std::vector<std::pair<uint32_t, uint32_t>>& chunkMap,
    std::vector<uint8_t>& compressedStream) {

    uint16_t offsetShift = offsets.size() * 2;
    std::vector<uint8_t> blockBuffer;
    blockBuffer.reserve(offsetShift + uncompressed.size());

    for (uint16_t off : offsets) {
        uint16_t shifted = off + offsetShift;
        blockBuffer.push_back(shifted & 0xFF);
        blockBuffer.push_back((shifted >> 8) & 0xFF);
    }

    blockBuffer.insert(blockBuffer.end(), uncompressed.begin(), uncompressed.end());

    uLongf compSize = compressBound(blockBuffer.size());
    std::vector<uint8_t> compBuffer(compSize);
    compress(compBuffer.data(), &compSize, blockBuffer.data(), blockBuffer.size());
    compBuffer.resize(compSize);

    chunkMap.push_back({ firstDef, (uint32_t)compressedStream.size() });
    compressedStream.insert(compressedStream.end(), compBuffer.begin(), compBuffer.end());

    uncompressed.clear();
    offsets.clear();
}