#pragma once
#include "ShaderParser.h"
#include <vector>
#include <string>
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

typedef HRESULT(WINAPI* pD3DAssemble)(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    LPCSTR pSourceName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    UINT Flags,
    ID3DBlob** ppCode,
    ID3DBlob** ppErrorMsgs
    );

class ShaderCompiler {
public:
    static std::vector<uint8_t> Compile(const std::string& shaderText, const std::vector<uint8_t>& originalRawData, std::string& outError) {
        outError.clear();

        HMODULE hD3DCompiler = LoadLibraryA("d3dcompiler_47.dll");
        if (!hD3DCompiler) hD3DCompiler = LoadLibraryA("d3dcompiler_46.dll");

        if (!hD3DCompiler) {
            outError = "Could not find d3dcompiler_47.dll on this system.";
            return {};
        }

        pD3DAssemble D3DAssembleFunc = (pD3DAssemble)GetProcAddress(hD3DCompiler, "D3DAssemble");
        if (!D3DAssembleFunc) {
            outError = "D3DAssemble function not found in d3dcompiler.dll.";
            FreeLibrary(hD3DCompiler);
            return {};
        }

        ID3DBlob* pCode = nullptr;
        ID3DBlob* pErrors = nullptr;

        HRESULT hr = D3DAssembleFunc(shaderText.c_str(), shaderText.length(), nullptr, nullptr, nullptr, 0, &pCode, &pErrors);

        if (FAILED(hr) || !pCode) {
            if (pErrors) {
                outError = std::string((char*)pErrors->GetBufferPointer(), pErrors->GetBufferSize());
                pErrors->Release();
            }
            else {
                outError = "Unknown D3DAssemble error. HRESULT: " + std::to_string(hr);
            }
            FreeLibrary(hD3DCompiler);
            return {};
        }

        CShaderParser parser;
        parser.Parse(originalRawData);

        if (!parser.IsParsed) {
            outError = "Failed to parse original Fable shader envelope.";
            pCode->Release();
            FreeLibrary(hD3DCompiler);
            return {};
        }

        parser.Data.ByteSize = (uint32_t)pCode->GetBufferSize();
        parser.Data.Bytecode.resize(parser.Data.ByteSize);
        memcpy(parser.Data.Bytecode.data(), pCode->GetBufferPointer(), parser.Data.ByteSize);

        pCode->Release();
        FreeLibrary(hD3DCompiler);

        return parser.Recompile();
    }
};