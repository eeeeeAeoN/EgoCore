#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

enum class EShaderType {
    Unknown,
    VertexShader_1_1,
    VertexShader_2_0,
    VertexShader_3_0,
    PixelShader_1_1,
    PixelShader_1_4,
    PixelShader_2_0,
    PixelShader_3_0
};

struct CShaderData {
    uint32_t ByteSize = 0;
    std::vector<uint8_t> Bytecode;
    EShaderType Type = EShaderType::Unknown;

    uint32_t ConstantCount = 0;
    std::vector<std::string> ConstantNames;

    std::string VSConstantLayout;
};

class CShaderParser {
public:
    bool IsParsed = false;
    CShaderData Data;
    std::string DecompiledText;

    void Parse(const std::vector<uint8_t>& rawData) {
        IsParsed = false;
        Data = CShaderData();
        DecompiledText.clear();

        if (rawData.size() < 4) return;

        size_t offset = 0;
        size_t maxOffset = rawData.size();

        Data.ByteSize = *(uint32_t*)(&rawData[offset]);
        offset += 4;

        if (Data.ByteSize > maxOffset - offset) return;

        if (Data.ByteSize > 0) {
            Data.Bytecode.resize(Data.ByteSize);
            memcpy(Data.Bytecode.data(), &rawData[offset], Data.ByteSize);

            if (Data.ByteSize >= 4) {
                uint32_t token = *(uint32_t*)(Data.Bytecode.data());
                if (token == 0xFFFE0101) Data.Type = EShaderType::VertexShader_1_1;
                else if (token == 0xFFFE0200) Data.Type = EShaderType::VertexShader_2_0;
                else if (token == 0xFFFE0300) Data.Type = EShaderType::VertexShader_3_0;
                else if (token == 0xFFFF0101) Data.Type = EShaderType::PixelShader_1_1;
                else if (token == 0xFFFF0104) Data.Type = EShaderType::PixelShader_1_4;
                else if (token == 0xFFFF0200) Data.Type = EShaderType::PixelShader_2_0;
                else if (token == 0xFFFF0300) Data.Type = EShaderType::PixelShader_3_0;
            }
        }
        offset += Data.ByteSize;

        if (offset + 4 > maxOffset) {
            IsParsed = true;
            DecompileBytecode();
            return;
        }

        Data.ConstantCount = *(uint32_t*)(&rawData[offset]);
        offset += 4;

        for (uint32_t i = 0; i < Data.ConstantCount; ++i) {
            size_t strStart = offset;
            while (offset < maxOffset && rawData[offset] != '\0') offset++;
            if (offset >= maxOffset) break;

            Data.ConstantNames.push_back(std::string((char*)&rawData[strStart], offset - strStart));
            offset++;
        }

        if (offset < maxOffset) {
            size_t strStart = offset;
            while (offset < maxOffset && rawData[offset] != '\0') offset++;
            if (offset <= maxOffset) {
                Data.VSConstantLayout = std::string((char*)&rawData[strStart], offset - strStart);
            }
        }

        IsParsed = true;
        DecompileBytecode();
    }

    void DecompileBytecode() {
        if (Data.Bytecode.empty()) {
            DecompiledText = "// No bytecode to decompile.";
            return;
        }

        ID3DBlob* pDisassembly = nullptr;
        HRESULT hr = D3DDisassemble(Data.Bytecode.data(), Data.ByteSize, 0, nullptr, &pDisassembly);

        if (SUCCEEDED(hr) && pDisassembly) {
            DecompiledText = std::string((const char*)pDisassembly->GetBufferPointer(), pDisassembly->GetBufferSize() - 1);
            pDisassembly->Release();
        }
        else {
            DecompiledText = "// Failed to disassemble bytecode. HRESULT: " + std::to_string(hr);
        }
    }

    std::vector<uint8_t> Recompile() const {
        std::vector<uint8_t> out;

        uint32_t size = (uint32_t)Data.Bytecode.size();
        out.insert(out.end(), (uint8_t*)&size, (uint8_t*)&size + 4);
        out.insert(out.end(), Data.Bytecode.begin(), Data.Bytecode.end());

        uint32_t cCount = (uint32_t)Data.ConstantNames.size();
        out.insert(out.end(), (uint8_t*)&cCount, (uint8_t*)&cCount + 4);

        for (const auto& name : Data.ConstantNames) {
            out.insert(out.end(), name.begin(), name.end());
            out.push_back('\0');
        }

        if (!Data.VSConstantLayout.empty()) {
            out.insert(out.end(), Data.VSConstantLayout.begin(), Data.VSConstantLayout.end());
        }
        out.push_back('\0');

        return out;
    }
};