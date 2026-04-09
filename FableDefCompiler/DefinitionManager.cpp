#include "DefinitionManager.h"
#include <fstream>
#include <iostream>
#include <zlib.h>

CDefStringTable* GDefStringTable = nullptr;

CDefinitionManager::CDefinitionManager(const std::wstring& outFileName) : m_CompiledFileName(outFileName) {
    m_StringTable = new CDefStringTable();
    GDefStringTable = m_StringTable;
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

    std::string nullDefName = "NULLDEF_" + name;
    m_StringTable->AddString(nullDefName);
    m_StringTable->AddString(name);

    // Replicate Fable's InstantiateDef for the NULL objects
    IDefObject* nullDef = allocFunc();
    if (nullDef) {
        nullDef->SetInstantiationName(nullDefName);
        m_InstantiatedDefs.push_back(nullDef);
    }
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

    LogToFile("Step 3/4: Saving String Table (names_custom.bin)...");
    std::string nameBinPath = dirPath + "\\names_custom.bin";
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

    while (true) {
        CParsedItem item;
        if (!parser.ReadNextItem(item)) break;
        if (item.Type == EParsedItemType::Symbol && item.StringValue == "}") break;
        if (item.Type != EParsedItemType::Identifier) return;

        std::string memberName = item.StringValue;

        std::string symbol = "";
        if (parser.PeekNextItemType() == EParsedItemType::Symbol) {
             parser.ReadAsSymbol(symbol);
        }

        if (symbol == "=") {
            // handle hex, negative, bitshifts, or identifiers
            CParsedItem valItem;
            if (parser.ReadNextItem(valItem)) {
                 if (valItem.Type == EParsedItemType::Integer) {
                     currentValue = valItem.IntValue;
                 } else if (valItem.Type == EParsedItemType::Identifier) {
                     auto it = m_SymbolMap.find(valItem.StringValue);
                     if (it != m_SymbolMap.end()) currentValue = it->second;
                     
                     // Read away any trailing operations like `<< 8`
                     while (true) {
                         CParsedItem junk;
                         if (!parser.PeekNextItem(junk)) break;
                         if (junk.StringValue == "," || junk.StringValue == "}") break;
                         
                         parser.ReadNextItem(junk);
                         if (junk.StringValue == "<<" || junk.StringValue == ">>") {
                             CParsedItem shift;
                             if (parser.ReadNextItem(shift) && shift.Type == EParsedItemType::Integer) {
                                 if (junk.StringValue == "<<") currentValue <<= shift.IntValue;
                                 else currentValue >>= shift.IntValue;
                             }
                         }
                     }
                 }
            }
            m_SymbolMap[memberName] = currentValue;
            
            if (parser.PeekNextItemType() == EParsedItemType::Symbol) {
                parser.PeekNextItem(valItem);
                if (valItem.StringValue == ",") {
                    parser.ReadAsSymbol(symbol);
                } else if (valItem.StringValue == "}") {
                    // let loop handle }
                }
            }
            currentValue++;
        }
        else if (symbol == "," || symbol == "") {
            m_SymbolMap[memberName] = currentValue;
            currentValue++;
        }
        else if (symbol == "}") {
            m_SymbolMap[memberName] = currentValue;
            break;
        }
        else {
            break;
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
                CompileDefinition(parser, defClass, defName, true);
            }
        }
        parser.SetStringPos(0);
    }

    // Robust Enum and Define Search
    parser.SetStringPos(0);
    while (!parser.IsEOF()) {
        auto enumState = parser.SaveState();
        bool foundEnum = parser.SkipUntilWholeString("enum");
        auto enumFoundState = parser.SaveState();
        parser.RestoreState(enumState);
        
        bool foundDefine = parser.SkipUntilWholeString("#define");
        auto defineFoundState = parser.SaveState();

        if (!foundEnum && !foundDefine) break;

        if (foundEnum && (!foundDefine || enumFoundState.Ptr < defineFoundState.Ptr)) {
            parser.RestoreState(enumFoundState);
            parser.MoveStringPos(4); // past "enum"
            ParseEnumForSymbols(parser);
        } else {
            parser.RestoreState(defineFoundState);
            parser.MoveStringPos(7); // past "#define"
            std::string defName;
            if (parser.ReadAsIdentifierOrNumber(defName)) {
                CParsedItem valItem;
                if (parser.ReadNextItem(valItem)) {
                    if (valItem.Type == EParsedItemType::Integer) {
                        m_SymbolMap[defName] = valItem.IntValue;
                    } else if (valItem.Type == EParsedItemType::Float) {
                        m_SymbolMap[defName] = (int)valItem.FloatValue;
                    }
                }
            }
        }
    }
}

void CDefinitionManager::DoCompilePathList() {
    for (const auto& path : m_CompilePathList) {
        std::string fileName(path.begin(), path.end());
        LogToFile(" -> Compiling File: " + fileName);

        std::string rawContent = ReadFileToString(path);
        if (rawContent.empty()) continue;

        CStringParser parser;

        // PASS 1: Compile Templates First
        parser.Init(rawContent, fileName);
        while (parser.SkipPastWholeString("#definition_template")) {
            std::string defClass, defName;
            if (parser.ReadAsString(defClass) && parser.ReadAsString(defName)) {
                LogToFile("    + " + defClass + " [" + defName + "] (Template)");
                CompileDefinition(parser, defClass, defName, true);
            }
        }

        // PASS 2: Compile Definitions Second
        parser.Init(rawContent, fileName); // Reset parser to the top of the file
        while (parser.SkipPastWholeString("#definition")) {
            std::string defClass, defName;
            if (parser.ReadAsString(defClass) && parser.ReadAsString(defName)) {
                LogToFile("    + " + defClass + " [" + defName + "]");
                CompileDefinition(parser, defClass, defName, false);
            }
        }
    }
}

void CDefinitionManager::CompileDefinition(CStringParser& parser, const std::string& defClass, const std::string& defName, bool isTemplate) {
    if (m_StringTable) {
        m_StringTable->AddString(defClass);
        m_StringTable->AddString(defName);
    }

    uint32_t typeCrc = CDefStringTable::GetCRC(defClass);
    auto classIt = m_RegisteredClasses.find(typeCrc);
    if (classIt == m_RegisteredClasses.end()) {
        LogToFile("      ! Unknown Class: " + defClass);
        std::string junk;
        parser.ReadAsStringUntilWholeString("#end_definition", junk);
        parser.SkipPastWholeString("#end_definition");
        return;
    }

    IDefObject* newDef = nullptr;
    bool bIsNew = false;
    if (isTemplate) {
        auto it = m_Templates.find(defName);
        if (it != m_Templates.end() && it->second->GetClassName() == defClass) {
            newDef = it->second;
        }
    } else {
        for (auto* existingDef : m_InstantiatedDefs) {
            if (existingDef->GetInstantiationName() == defName && existingDef->GetClassName() == defClass) {
                newDef = existingDef;
                break;
            }
        }
    }
    
    if (!newDef) {
        newDef = classIt->second.AllocFunc();
        if (!newDef) return;
        newDef->SetInstantiationName(defName);
        bIsNew = true;
    }

    CParsedItem item;
    if (parser.PeekNextItem(item) && item.StringValue == "specialises") {
        parser.ReadNextItem(item);

        std::string parentName;
        if (parser.ReadAsIdentifierOrNumber(parentName)) {
            if (m_StringTable) {
                m_StringTable->AddString(parentName);
            }

            IDefObject* parentDef = nullptr;
            for (auto* existingDef : m_InstantiatedDefs) {
                if (existingDef->GetInstantiationName() == parentName) {
                    parentDef = existingDef;
                    break;
                }
            }
            if (!parentDef) {
                auto it = m_Templates.find(parentName);
                if (it != m_Templates.end()) {
                    parentDef = it->second;
                }
            }

            if (parentDef) {
                newDef->CopyFrom(parentDef);
                newDef->SetInstantiationName(defName);
            }
            else {
                LogToFile("      ! Warning: Parent '" + parentName + "' not found for '" + defName + "'");
            }
        }
    }

    std::string defText;
    if (!parser.ReadAsStringUntilWholeString("#end_definition", defText)) {
        LogToFile("      ! Error: Missing #end_definition for " + defName);
        delete newDef;
        return;
    }
    parser.SkipPastWholeString("#end_definition");

    CStringParser sandboxParser;
    sandboxParser.Init(defText, "Sandbox_" + defName);

    while (sandboxParser.SkipPastString("<")) {
        uint32_t startPos = sandboxParser.SaveState().Ptr - defText.data() - 1;

        std::string subDefClass;
        sandboxParser.ReadAsIdentifierOrNumber(subDefClass);

        std::string subDefName;
        sandboxParser.ReadAsIdentifierOrNumber(subDefName);

        std::string subDefText;
        sandboxParser.ReadAsStringUntilString(">", subDefText);
        sandboxParser.SkipPastString(">");

        uint32_t endPos = sandboxParser.SaveState().Ptr - defText.data();

        CStringParser subParser;
        subParser.Init(subDefText, "SubDef_" + subDefName);

        LogToFile("      + " + subDefClass + " [" + subDefName + "] (Inline Sub-Definition)");

        CompileDefinition(subParser, subDefClass, subDefName, false);
        for (uint32_t i = startPos; i < endPos; ++i) {
            if (defText[i] != '\n' && defText[i] != '\r') {
                defText[i] = ' ';
            }
        }
    }

    // Use CPersistContext in MODE_LOAD_TEXT to scan the def text for all fields.
    // This mirrors the engine's TransferObjectLoadText / TransferVectorLoadText protocol exactly:
    // each Transfer(name, val) call scans the mutable defText for the keyword and reads the value,
    // then blanks out the consumed region so it isn't matched again.
    CPersistContext loadCtx(&defText, &m_SymbolMap);
    newDef->Transfer(loadCtx);

    if (bIsNew) {
        if (isTemplate) {
            m_Templates[defName] = newDef;
        } else {
            m_InstantiatedDefs.push_back(newDef);
        }
    }
}

void CDefinitionManager::SaveBinaryDefinitions() {
    std::string outPath(m_CompiledFileName.begin(), m_CompiledFileName.end());
    std::ofstream os(outPath, std::ios::binary);
    if (!os.is_open()) {
        LogToFile("ERROR: Failed to open output file for writing: " + outPath);
        return;
    }

    LogToFile(" -> Writing Global Header...");
    bool useSafeBinary = true;
    uint32_t dependencyCRC = 0xE86E4CDE;
    uint32_t randomID = m_StringTable ? m_StringTable->m_RandomID : 0xA8E36C34;
    uint32_t noDefs = (uint32_t)m_InstantiatedDefs.size();

    int8_t safeBool = useSafeBinary ? 1 : 0;
    os.write((char*)&safeBool, 1);
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
        LogToFile("    + Serializing " + m_InstantiatedDefs[i]->GetInstantiationName() + " (" + m_InstantiatedDefs[i]->GetClassName() + ")");
        CMemoryDataOutputStream defOs;
        CPersistContext persist(&defOs, CPersistContext::MODE_SAVE_BINARY, useSafeBinary);
        persist.TransferObjectHeader(0);
        m_InstantiatedDefs[i]->Transfer(persist);

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

    // Allocate destination buffer using compressBound just to be safe
    uLongf compBoundSize = compressBound(blockBuffer.size());
    std::vector<uint8_t> compBuffer(compBoundSize);

    // --- LIONHEAD ZLIB REPLICATION ---
    z_stream c_stream;
    c_stream.zalloc = Z_NULL;
    c_stream.zfree = Z_NULL;
    c_stream.opaque = Z_NULL;

    // Initialize with Level 1 (Z_BEST_SPEED)
    deflateInit(&c_stream, 1);

    c_stream.next_in = blockBuffer.data();
    c_stream.next_out = compBuffer.data();

    // The 1-byte-at-a-time loop of madness
    while (c_stream.total_in != blockBuffer.size()) {
        if (c_stream.total_out >= compBuffer.size()) break;
        c_stream.avail_out = 1;
        c_stream.avail_in = 1;
        deflate(&c_stream, Z_NO_FLUSH); // 0
    }

    // Finish flushing the stream 1 byte at a time
    do {
        c_stream.avail_out = 1;
    } while (deflate(&c_stream, Z_FINISH) != Z_STREAM_END); // Z_FINISH is 4

    uint32_t actualCompSize = c_stream.total_out;
    deflateEnd(&c_stream);
    // ---------------------------------

    compBuffer.resize(actualCompSize);

    chunkMap.push_back({ firstDef, (uint32_t)compressedStream.size() });
    compressedStream.insert(compressedStream.end(), compBuffer.begin(), compBuffer.end());

    uncompressed.clear();
    offsets.clear();
}