#pragma once
#include <d3d11.h>
#include "TextureParser.h"
#include "imgui.h"

struct GPUTexture {
    ID3D11ShaderResourceView* SRV = nullptr;
    uint32_t Width = 0, Height = 0;

    void Release() {
        if (SRV) { SRV->Release(); SRV = nullptr; }
    }

    bool Upload(ID3D11Device* device, const CTextureParser& parser) {
        Release();
        if (!parser.IsParsed || parser.DecodedPixels.empty()) return false;

        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
        // Map Fable formats to DXGI formats
        switch (parser.DecodedFormat) {
        case ETextureFormat::DXT1: format = DXGI_FORMAT_BC1_UNORM; break;
        case ETextureFormat::DXT3: format = DXGI_FORMAT_BC2_UNORM; break;
        case ETextureFormat::DXT5: format = DXGI_FORMAT_BC3_UNORM; break;
        case ETextureFormat::ARGB8888: format = DXGI_FORMAT_B8G8R8A8_UNORM; break;
        default: return false;
        }

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = parser.Header.Width;
        desc.Height = parser.Header.Height;
        desc.MipLevels = 1; // Start with base level for the viewport
        desc.ArraySize = 1;
        desc.Format = format;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA subData = {};
        subData.pSysMem = parser.DecodedPixels.data();
        // Calculate pitch based on format
        if (parser.DecodedFormat == ETextureFormat::DXT1)
            subData.SysMemPitch = ((desc.Width + 3) / 4) * 8;
        else if (parser.DecodedFormat == ETextureFormat::DXT3 || parser.DecodedFormat == ETextureFormat::DXT5)
            subData.SysMemPitch = ((desc.Width + 3) / 4) * 16;
        else
            subData.SysMemPitch = desc.Width * 4;

        ID3D11Texture2D* tex2D = nullptr;
        if (FAILED(device->CreateTexture2D(&desc, &subData, &tex2D))) return false;
        device->CreateShaderResourceView(tex2D, nullptr, &SRV);
        tex2D->Release();

        Width = desc.Width; Height = desc.Height;
        return true;
    }
};

static GPUTexture g_ViewportTexture;