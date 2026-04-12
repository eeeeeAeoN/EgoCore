#include "DefinitionManager.h"
#include <fstream>
#include <iostream>
#include <zlib.h>
#include <filesystem>

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

    // --- HARDCODED OVERRIDES FOR BITSHIFT ENUMS ---
    // Bypasses parser complexity for guaranteed vanilla parity
    m_SymbolMap["UI_STATE_CHANGE_NO_UPDATE"] = 0;
    m_SymbolMap["UI_STATE_CHANGE_UPDATE_MOVEMENT"] = 1;
    m_SymbolMap["UI_STATE_CHANGE_UPDATE_COLOUR"] = 2;
    m_SymbolMap["UI_STATE_CHANGE_UPDATE_ZOOM"] = 4;
    m_SymbolMap["UI_STATE_CHANGE_UPDATE_GRAPHIC"] = 8;
    m_SymbolMap["UI_STATE_CHANGE_USE_OWN_COLOUR"] = 16;
    m_SymbolMap["UI_STATE_CHANGE_USE_OWN_POSITION"] = 32;
    m_SymbolMap["UI_STATE_CHANGE_USE_OWN_ZOOM"] = 64;

    m_SymbolMap["TABLE_EXPANSION_HORIZONTAL"] = 1;
    m_SymbolMap["TABLE_EXPANSION_VERTICAL"] = 2;

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

        if (parser.PeekNextItemType() == EParsedItemType::Symbol) {
            CParsedItem peekSym;
            parser.PeekNextItem(peekSym);
            if (peekSym.StringValue == "=") {
                parser.ReadNextItem(peekSym); // Consume '='

                currentValue = 0;
                int currentOperand = 0;
                std::string currentOp = "";

                // Robust Expression Evaluator
                while (true) {
                    CParsedItem token;
                    if (!parser.PeekNextItem(token)) break;
                    if (token.StringValue == "," || token.StringValue == "}") break;

                    parser.ReadNextItem(token);

                    if (token.Type == EParsedItemType::Integer) {
                        currentOperand = token.IntValue;
                    }
                    else if (token.Type == EParsedItemType::Identifier) {
                        auto it = m_SymbolMap.find(token.StringValue);
                        if (it != m_SymbolMap.end()) currentOperand = it->second;
                        else currentOperand = 0;
                    }
                    else if (token.Type == EParsedItemType::Symbol) {
                        if (token.StringValue == "<" || token.StringValue == ">") {
                            CParsedItem token2;
                            if (parser.PeekNextItem(token2) && token2.StringValue == token.StringValue) {
                                parser.ReadNextItem(token2); // Consume second '<' or '>'
                                currentOp = token.StringValue + token2.StringValue;
                                continue;
                            }
                        }
                        else if (token.StringValue == "|" || token.StringValue == "+" || token.StringValue == "-") {
                            currentOp = token.StringValue;
                            continue;
                        }
                    }

                    // Apply operation
                    if (currentOp == "<<") currentValue <<= currentOperand;
                    else if (currentOp == ">>") currentValue >>= currentOperand;
                    else if (currentOp == "|") currentValue |= currentOperand;
                    else if (currentOp == "+") currentValue += currentOperand;
                    else if (currentOp == "-") currentValue -= currentOperand;
                    else currentValue = currentOperand; // First operand

                    currentOp = "";
                }
            }
        }

        m_SymbolMap[memberName] = currentValue;

        // Gracefully consume trailing commas
        if (parser.PeekNextItemType() == EParsedItemType::Symbol) {
            CParsedItem valItem;
            parser.PeekNextItem(valItem);
            if (valItem.StringValue == ",") {
                parser.ReadNextItem(valItem);
            }
        }
        currentValue++;
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
    }
    else {
        for (size_t i = 0; i < m_InstantiatedDefs.size(); ++i) {
            if (m_InstantiatedDefs[i]->GetInstantiationName() == defName && m_InstantiatedDefs[i]->GetClassName() == defClass) {
                newDef = m_InstantiatedDefs[i];

                // RESET the object to prevent cross-file pollution
                // If it re-defines without 'specialises', Vanilla starts fresh.
                IDefObject* fresh = classIt->second.AllocFunc();
                fresh->SetInstantiationName(defName);
                delete newDef;
                m_InstantiatedDefs[i] = fresh;
                newDef = fresh;
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

    // --- NEW: Strip comments from defText to prevent raw text scanner bleeding ---
    bool inQuotes = false;
    for (size_t i = 0; i < defText.size(); ++i) {
        if (defText[i] == '"') inQuotes = !inQuotes;

        // If we hit a comment block outside of a quoted string
        if (!inQuotes && defText[i] == '/' && i + 1 < defText.size()) {

            // Single-line comment //
            if (defText[i + 1] == '/') {
                size_t j = i;
                while (j < defText.size() && defText[j] != '\n') {
                    defText[j] = ' ';
                    j++;
                }
                i = j - 1; // Move iterator to end of this comment block
            }
            // Multi-line block comment /* */
            else if (defText[i + 1] == '*') {
                size_t j = i;
                defText[j++] = ' ';
                defText[j++] = ' ';
                while (j < defText.size()) {
                    if (j + 1 < defText.size() && defText[j] == '*' && defText[j + 1] == '/') {
                        defText[j++] = ' ';
                        defText[j++] = ' ';
                        break;
                    }
                    // Blank everything except newlines to preserve line counting/offsets
                    if (defText[j] != '\n' && defText[j] != '\r') {
                        defText[j] = ' ';
                    }
                    j++;
                }
                i = j - 1; // Move iterator to end of this comment block
            }
        }
    }
    // ---------------------------------------------------------------------------

    CStringParser sandboxParser;
    sandboxParser.Init(defText, "Sandbox_" + defName);

    while (true) {
        CParsedItem item;
        // ReadNextItem safely steps over QuotedStrings, hiding false-positive '<' characters
        if (!sandboxParser.ReadNextItem(item)) break;

        if (item.Type == EParsedItemType::Symbol && item.StringValue == "<") {

            // Ignore bitshifts '<<' or inequalities '<='
            CParsedItem nextItem;
            if (sandboxParser.PeekNextItem(nextItem) && nextItem.Type == EParsedItemType::Symbol &&
                (nextItem.StringValue == "<" || nextItem.StringValue == "=")) {
                continue;
            }

            auto stateBeforeClass = sandboxParser.SaveState();
            uint32_t startPos = stateBeforeClass.Ptr - defText.data() - 1;

            std::string subDefClass;
            if (!sandboxParser.ReadAsIdentifierOrNumber(subDefClass)) continue;

            std::string subDefName;
            if (!sandboxParser.ReadAsIdentifierOrNumber(subDefName)) {
                sandboxParser.RestoreState(stateBeforeClass);
                continue;
            }

            // Scan forward for '>'
            uint32_t contentStart = sandboxParser.SaveState().Ptr - defText.data();
            bool foundEnd = false;

            while (sandboxParser.ReadNextItem(item)) {
                if (item.Type == EParsedItemType::Symbol && item.StringValue == ">") {
                    foundEnd = true;
                    break;
                }
            }

            if (!foundEnd) {
                sandboxParser.RestoreState(stateBeforeClass);
                continue;
            }

            uint32_t endPos = sandboxParser.SaveState().Ptr - defText.data();
            uint32_t contentEnd = endPos - 1;

            std::string subDefText = defText.substr(contentStart, contentEnd - contentStart);

            CStringParser subParser;
            subParser.Init(subDefText, "SubDef_" + subDefName);

            LogToFile("      + " + subDefClass + " [" + subDefName + "] (Inline Sub-Definition)");

            CompileDefinition(subParser, subDefClass, subDefName, false);

            // Blank out the parsed region so the main CPersistContext scanner ignores it
            for (uint32_t i = startPos; i < endPos; ++i) {
                if (defText[i] != '\n' && defText[i] != '\r') {
                    defText[i] = ' ';
                }
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
        // Fable requires templates to be compiled into the binary 
        // so the engine can use them as runtime prefabs!
        m_InstantiatedDefs.push_back(newDef);

        if (isTemplate) {
            m_Templates[defName] = newDef;
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
    bool useSafeBinary = false;
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

void CDefinitionManager::DumpIndividualBinaries(const std::string& outDir) {
    LogToFile("--- Starting Individual Binary Dump ---");

    int dumpedCount = 0;
    for (IDefObject* obj : m_InstantiatedDefs) {
        if (!obj) continue;

        std::string className = obj->GetClassName();
        std::string instName = obj->GetInstantiationName();

        // 1. Create the Class directory (e.g., DumpDir/CONTROL_SCHEME/)
        std::filesystem::path dir = std::filesystem::path(outDir) / className;
        std::filesystem::create_directories(dir);

        // 2. Setup an isolated binary stream for this specific object
        CMemoryDataOutputStream objStream;
        CPersistContext persist(&objStream, CPersistContext::MODE_SAVE_BINARY);
        persist.m_ForceNoTags = false;

        // Add the missing header! (256 matches Vanilla's 00 01 00 00)
        // To this dynamic check:
        uint32_t objectVersion = (className == "UI") ? 257 : 256;
        persist.TransferObjectHeader(objectVersion);

        // 3. Serialize the object into the isolated stream
        obj->Transfer(persist);

        // 4. Write it to disk (e.g., DumpDir/CONTROL_SCHEME/FABLE_PC_CONTROL_SCHEME_GDD.bin)
        std::filesystem::path filePath = dir / (instName + ".bin");
        std::ofstream outFile(filePath, std::ios::binary);

        if (outFile.is_open()) {
            if (objStream.GetLength() > 0) {
                outFile.write(reinterpret_cast<const char*>(objStream.PeekData()), objStream.GetLength());
            }
            outFile.close();
            dumpedCount++;
        }
    }

    LogToFile("Successfully dumped " + std::to_string(dumpedCount) + " individual binaries to: " + outDir);
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