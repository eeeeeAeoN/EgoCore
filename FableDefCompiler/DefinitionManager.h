#pragma once
#include "StringParser.h"
#include "BinaryStreams.h"
#include "DefObjects.h"
#include <string>
#include <vector>
#include <map>

extern void LogToFile(const std::string& msg);

typedef IDefObject* (*DefAllocFunc)();

struct DefClassInfo {
    std::string Name;
    uint32_t NameCRC;
    DefAllocFunc AllocFunc;
};

class CDefinitionManager {
private:
    std::map<uint32_t, DefClassInfo> m_RegisteredClasses;
    std::vector<std::wstring> m_SymbolPathList;
    std::vector<std::wstring> m_CompilePathList;
    std::wstring m_CompiledFileName;

    std::map<std::string, int> m_SymbolMap;
    std::vector<IDefObject*> m_InstantiatedDefs;
    CDefStringTable* m_StringTable;

public:
    CDefinitionManager(const std::wstring& outFileName);
    ~CDefinitionManager();

    void AddDefClass(const std::string& name, DefAllocFunc allocFunc);
    void SetSymbolPaths(const std::vector<std::wstring>& paths);
    void SetCompilePaths(const std::vector<std::wstring>& paths);

    void Compile();

private:
    void CreateSymbolsFromPathList();
    void DoCompilePathList();

    void ParseStringForSymbols(const std::string& script, bool parseDefs);
    void ParseEnumForSymbols(CStringParser& parser);
    void CompileDefinition(CStringParser& parser, const std::string& defClass, const std::string& defName, bool isTemplate);

    std::string ReadFileToString(const std::wstring& path);

    void SaveBinaryDefinitions();
    void CompressBlock(std::vector<uint8_t>& uncompressed, std::vector<uint16_t>& offsets,
        uint32_t firstDef, std::vector<std::pair<uint32_t, uint32_t>>& chunkMap,
        std::vector<uint8_t>& compressedStream);
};