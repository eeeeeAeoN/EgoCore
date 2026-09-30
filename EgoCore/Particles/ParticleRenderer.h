#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include "imgui.h"
#include "ParticleParser.h"
#include "ParticleSimulator.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "BankBackend.h"
#include "MeshProperties.h"
#include "ConfigBackend.h"
#include <unordered_map>

using namespace DirectX;

// Forward declaration of texture loader from open banks
ID3D11ShaderResourceView* LoadTextureForMesh(int textureID);

struct ParticleGridVertex {
    XMFLOAT3 Pos;
    XMFLOAT4 Col;
};

struct ParticleQuadVertex {
    XMFLOAT3 Pos;
    XMFLOAT3 UV; // x = U, y = V, z = Frame Array Slice
    XMFLOAT4 Col;
};

struct ParticleCBMatrix {
    XMMATRIX WorldViewProj;
    int32_t IsDXT1;
    int32_t BlendMode;
    float Padding[2];
};

struct ParticleMeshBatch {
    uint32_t IndexStart = 0;
    uint32_t IndexCount = 0;
    int32_t MaterialIndex = 0;
    int32_t DiffuseMapID = 0;
};

struct ParticleMeshGPU {
    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
    uint32_t TotalVertexCount = 0;
    uint32_t TotalIndexCount = 0;
    std::vector<ParticleMeshBatch> Batches;
    XMFLOAT3 BoundingCenter = { 0.0f, 0.0f, 0.0f };
    float BoundingRadius = 1.0f;
    bool IsValid = false;

    void Release() {
        if (VertexBuffer) { VertexBuffer->Release(); VertexBuffer = nullptr; }
        if (IndexBuffer) { IndexBuffer->Release(); IndexBuffer = nullptr; }
        Batches.clear();
        IsValid = false;
    }
};

struct ParticleMeshCB {
    XMMATRIX WorldViewProj;
    XMFLOAT4 MeshColor;
    int32_t HasTexture;
    int32_t BlendMode;
    int32_t IsDXT1;
    float PMM_Padding;
};

struct ParticleTextureResource {
    ID3D11ShaderResourceView* SRV = nullptr;
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t FrameWidth = 0;
    uint32_t FrameHeight = 0;
    uint32_t FrameCount = 1;
    bool IsSpriteSheet = false;
    bool IsDXT1 = false;
};

class ParticleRenderer {
private:
    ID3D11Device* Device = nullptr;
    std::unordered_map<int32_t, ParticleTextureResource> ParticleTextureCache;

    // Line shaders (for ground grid, axes, gizmos)
    ID3D11VertexShader* VS_Line = nullptr;
    ID3D11PixelShader* PS_Line = nullptr;
    ID3D11InputLayout* Layout_Line = nullptr;
    ID3D11Buffer* GridVBuffer = nullptr;
    uint32_t GridVertexCount = 0;

    // Particle billboard shaders & geometry
    ID3D11VertexShader* VS_Particle = nullptr;
    ID3D11PixelShader* PS_Particle = nullptr;
    ID3D11InputLayout* Layout_Particle = nullptr;
    ID3D11Buffer* ParticleVBuffer = nullptr;
    ID3D11Buffer* ParticleIBuffer = nullptr;
    uint32_t MaxParticleQuads = 8192; // Up to 8192 quads per draw call
    uint32_t MaxParticleVertices = 8192 * 4;
    uint32_t MaxParticleIndices = 8192 * 6;

    // 3D Particle Mesh Shaders & Resources (CPSCRenderMesh)
    ID3D11VertexShader* VS_ParticleMesh = nullptr;
    ID3D11PixelShader* PS_ParticleMesh = nullptr;
    ID3D11InputLayout* Layout_ParticleMesh = nullptr;
    ID3D11Buffer* ConstantBufferParticleMesh = nullptr;
    ID3D11SamplerState* MeshSampler = nullptr;
    ID3D11ShaderResourceView* DefaultWhiteSRV = nullptr;
    std::unordered_map<int32_t, ParticleMeshGPU> DynamicMeshCache;

    // Shared Constant Buffer
    ID3D11Buffer* ConstantBuffer = nullptr;

    // Pipeline States
    ID3D11RasterizerState* RastState = nullptr;
    ID3D11DepthStencilState* DepthStateGrid = nullptr;
    ID3D11DepthStencilState* DepthStateParticle = nullptr;
    ID3D11BlendState* BlendStateGrid = nullptr;
    ID3D11BlendState* BlendStateAlpha = nullptr;
    ID3D11BlendState* BlendStateAdditive = nullptr;
    ID3D11BlendState* BlendStateAddSmooth = nullptr;
    ID3D11BlendState* BlendStates[6][3] = {}; // [BlendMode 0..5][BlendOp 0..2] matching EEnginePrimitiveBlendMode

    ID3D11BlendState* GetBlendState(uint32_t mode, uint32_t op) {
        if (mode > 5) mode = 2; // Default to ALPHA
        if (op > 2) op = 0;
        return BlendStates[mode][op] ? BlendStates[mode][op] : (BlendStateAlpha ? BlendStateAlpha : nullptr);
    }
    ID3D11SamplerState* ParticleSampler = nullptr;

    // Procedural Fallback Glow Texture
    ID3D11ShaderResourceView* DefaultGlowSRV = nullptr;

    // Offscreen Render Target Resources
    ID3D11Texture2D* RenderTex = nullptr;
    ID3D11RenderTargetView* RTV = nullptr;
    ID3D11ShaderResourceView* SRV = nullptr;
    ID3D11Texture2D* DepthTex = nullptr;
    ID3D11DepthStencilView* DSV = nullptr;

    float Width = 0;
    float Height = 0;
    bool Initialized = false;

    void CreateDefaultWhiteTexture(ID3D11Device* device) {
        if (DefaultWhiteSRV) return;

        uint32_t white = 0xFFFFFFFF;
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA subData = { &white, 4, 0 };
        ID3D11Texture2D* tex = nullptr;
        if (SUCCEEDED(device->CreateTexture2D(&desc, &subData, &tex))) {
            device->CreateShaderResourceView(tex, nullptr, &DefaultWhiteSRV);
            tex->Release();
        }
    }

    void CreateDefaultGlowTexture(ID3D11Device* device) {
        if (DefaultGlowSRV) return;

        const int texSize = 64;
        std::vector<uint32_t> pixels(texSize * texSize);

        for (int y = 0; y < texSize; ++y) {
            for (int x = 0; x < texSize; ++x) {
                float nx = (x + 0.5f - (float)texSize * 0.5f) / ((float)texSize * 0.5f);
                float ny = (y + 0.5f - (float)texSize * 0.5f) / ((float)texSize * 0.5f);
                float dist = std::sqrt(nx * nx + ny * ny);

                float alpha = 0.0f;
                if (dist < 1.0f) {
                    float factor = 1.0f - dist;
                    alpha = factor * factor * (3.0f - 2.0f * factor); // Smoothstep fade
                    alpha = std::pow(alpha, 1.2f);
                }

                uint8_t a = (uint8_t)(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
                // Premultiplied alpha fallback: RGB = Alpha, preventing solid white box outlines in additive blending
                uint32_t val = (uint32_t)a;
                pixels[y * texSize + x] = (val << 24) | (val << 16) | (val << 8) | val;
            }
        }

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = texSize;
        desc.Height = texSize;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA subData = {};
        subData.pSysMem = pixels.data();
        subData.SysMemPitch = texSize * sizeof(uint32_t);

        ID3D11Texture2D* tex = nullptr;
        if (SUCCEEDED(device->CreateTexture2D(&desc, &subData, &tex))) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MostDetailedMip = 0;
            srvDesc.Texture2DArray.MipLevels = 1;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.ArraySize = 1;

            device->CreateShaderResourceView(tex, &srvDesc, &DefaultGlowSRV);
            tex->Release();
        }
    }

    void CreateParticleBuffers(ID3D11Device* device) {
        if (ParticleVBuffer) { ParticleVBuffer->Release(); ParticleVBuffer = nullptr; }
        if (ParticleIBuffer) { ParticleIBuffer->Release(); ParticleIBuffer = nullptr; }

        // Dynamic vertex buffer for billboards
        D3D11_BUFFER_DESC vDesc = {};
        vDesc.ByteWidth = sizeof(ParticleQuadVertex) * MaxParticleVertices;
        vDesc.Usage = D3D11_USAGE_DYNAMIC;
        vDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device->CreateBuffer(&vDesc, nullptr, &ParticleVBuffer);

        // Static index buffer (each quad = 6 indices: 0, 1, 2, 0, 2, 3)
        std::vector<uint32_t> indices(MaxParticleIndices);
        for (uint32_t i = 0; i < MaxParticleQuads; i++) {
            uint32_t vBase = i * 4;
            uint32_t iBase = i * 6;
            indices[iBase + 0] = vBase + 0;
            indices[iBase + 1] = vBase + 1;
            indices[iBase + 2] = vBase + 2;
            indices[iBase + 3] = vBase + 0;
            indices[iBase + 4] = vBase + 2;
            indices[iBase + 5] = vBase + 3;
        }

        D3D11_BUFFER_DESC iDesc = {};
        iDesc.ByteWidth = sizeof(uint32_t) * MaxParticleIndices;
        iDesc.Usage = D3D11_USAGE_IMMUTABLE;
        iDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iData = { indices.data(), 0, 0 };
        device->CreateBuffer(&iDesc, &iData, &ParticleIBuffer);
    }

    void CreateGridBuffers(ID3D11Device* device) {
        if (GridVBuffer) {
            GridVBuffer->Release();
            GridVBuffer = nullptr;
        }

        std::vector<ParticleGridVertex> verts;

        // Ground grid on XY plane (-10 to 10, step 1.0) with Z = 0.0f
        const float extent = 10.0f;
        const float step = 1.0f;
        const XMFLOAT4 cMinor = { 0.22f, 0.22f, 0.25f, 0.6f };
        const XMFLOAT4 cMajor = { 0.35f, 0.35f, 0.40f, 0.85f };

        for (float y = -extent; y <= extent + 0.001f; y += step) {
            bool isMajor = (std::abs(std::round(y / 5.0f) * 5.0f - y) < 0.01f);
            XMFLOAT4 c = isMajor ? cMajor : cMinor;
            verts.push_back({ XMFLOAT3(-extent, y, 0.0f), c });
            verts.push_back({ XMFLOAT3(extent, y, 0.0f), c });
        }
        for (float x = -extent; x <= extent + 0.001f; x += step) {
            bool isMajor = (std::abs(std::round(x / 5.0f) * 5.0f - x) < 0.01f);
            XMFLOAT4 c = isMajor ? cMajor : cMinor;
            verts.push_back({ XMFLOAT3(x, -extent, 0.0f), c });
            verts.push_back({ XMFLOAT3(x, extent, 0.0f), c });
        }

        // Coordinate axes: X = Red, Y = Green, Z = Blue (Up)
        verts.push_back({ XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f) });
        verts.push_back({ XMFLOAT3(2.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f) });

        verts.push_back({ XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT4(0.2f, 1.0f, 0.2f, 1.0f) });
        verts.push_back({ XMFLOAT3(0.0f, 2.0f, 0.0f), XMFLOAT4(0.2f, 1.0f, 0.2f, 1.0f) });

        verts.push_back({ XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT4(0.2f, 0.4f, 1.0f, 1.0f) });
        verts.push_back({ XMFLOAT3(0.0f, 0.0f, 2.0f), XMFLOAT4(0.2f, 0.4f, 1.0f, 1.0f) });

        GridVertexCount = (uint32_t)verts.size();

        D3D11_BUFFER_DESC vDesc = {};
        vDesc.ByteWidth = sizeof(ParticleGridVertex) * (UINT)verts.size();
        vDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vData = { verts.data(), 0, 0 };
        device->CreateBuffer(&vDesc, &vData, &GridVBuffer);
    }

    void ReleaseResizedResources() {
        if (SRV) { SRV->Release(); SRV = nullptr; }
        if (RTV) { RTV->Release(); RTV = nullptr; }
        if (RenderTex) { RenderTex->Release(); RenderTex = nullptr; }
        if (DSV) { DSV->Release(); DSV = nullptr; }
        if (DepthTex) { DepthTex->Release(); DepthTex = nullptr; }
    }

public:
    float CamRotX = 0.35f;
    float CamRotY = 0.0f;
    float CamDist = 12.0f;
    XMFLOAT2 CamPan = { 0.0f, -0.5f };
    XMFLOAT3 CenterOffset = { 0.0f, 0.0f, 0.0f };

    bool ShowGrid = true;
    bool ShowAxes = true;

    ParticleRenderer() = default;
    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator=(const ParticleRenderer&) = delete;

    ~ParticleRenderer() {
        Release();
    }

    void ResetCamera() {
        CamRotX = 0.35f;
        CamRotY = 0.0f;
        CamDist = 12.0f;
        CamPan = { 0.0f, -0.5f };
        CenterOffset = { 0.0f, 0.0f, 0.0f };
    }

    void Initialize(ID3D11Device* device) {
        if (!device) return;
        Device = device;
        if (Initialized) return;

        // 1. Compile Line Shader (for ground grid and axes)
        const char* lineShaderSrc = R"(
            cbuffer CBMatrix : register(b0) {
                matrix WorldViewProj;
            };

            struct VS_IN {
                float3 Pos : POSITION;
                float4 Col : COLOR;
            };

            struct PS_IN {
                float4 Pos : SV_POSITION;
                float4 Col : COLOR;
            };

            PS_IN VS(VS_IN input) {
                PS_IN output;
                output.Pos = mul(float4(input.Pos, 1.0f), WorldViewProj);
                output.Col = input.Col;
                return output;
            }

            float4 PS(PS_IN input) : SV_Target {
                return input.Col;
            }
        )";

        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;

        HRESULT hr = D3DCompile(lineShaderSrc, strlen(lineShaderSrc), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) errorBlob->Release();
            return;
        }
        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VS_Line);

        D3D11_INPUT_ELEMENT_DESC lineDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        device->CreateInputLayout(lineDesc, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &Layout_Line);
        vsBlob->Release();

        hr = D3DCompile(lineShaderSrc, strlen(lineShaderSrc), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) errorBlob->Release();
            return;
        }
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PS_Line);
        psBlob->Release();

        // 2. Compile Particle Billboard Shader
        const char* particleShaderSrc = R"(
            cbuffer CBMatrix : register(b0) {
                matrix WorldViewProj;
                int IsDXT1;
                int BlendMode;
                float2 Padding;
            };

            struct ParticleVS_IN {
                float3 Pos : POSITION;
                float3 UV  : TEXCOORD0;
                float4 Col : COLOR;
            };

            struct ParticlePS_IN {
                float4 Pos : SV_POSITION;
                float3 UV  : TEXCOORD0;
                float4 Col : COLOR;
            };

            ParticlePS_IN VS(ParticleVS_IN input) {
                ParticlePS_IN output;
                output.Pos = mul(float4(input.Pos, 1.0f), WorldViewProj);
                output.UV  = input.UV;
                output.Col = input.Col;
                return output;
            }

            Texture2DArray ParticleTex : register(t0);
            SamplerState Sampler : register(s0);

            // Disassembly 97_PSHADER_SPRITE_GROUP.txt:
            float4 PS(ParticlePS_IN input) : SV_Target {
                float4 texCol = ParticleTex.Sample(Sampler, input.UV);
                // DXT1, or textures with solid black backgrounds in alpha blend mode
                if (IsDXT1 != 0 || (BlendMode <= 2 && texCol.a >= 0.95f)) {
                    float brightness = max(texCol.r, max(texCol.g, texCol.b));
                    if (brightness < 0.05f) {
                        texCol.a *= saturate((brightness - 0.01f) * 25.0f);
                    }
                }
                float4 finalCol;
                finalCol.rgb = 2.0f * texCol.rgb * input.Col.rgb;
                finalCol.a = texCol.a * input.Col.a;

                // Fable Alpha Testing: AlphaRef = 16 (0x10) for SOLID (0), BOOLEAN_ALPHA (1), and ALPHA (2)
                if (BlendMode <= 2) {
                    clip(finalCol.a - (16.0f / 255.0f));
                }
                return finalCol;
            }
        )";

        hr = D3DCompile(particleShaderSrc, strlen(particleShaderSrc), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
        if (SUCCEEDED(hr)) {
            device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VS_Particle);
            D3D11_INPUT_ELEMENT_DESC partDesc[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };
            device->CreateInputLayout(partDesc, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &Layout_Particle);
            vsBlob->Release();
        }

        hr = D3DCompile(particleShaderSrc, strlen(particleShaderSrc), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, &errorBlob);
        if (SUCCEEDED(hr)) {
            device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PS_Particle);
            psBlob->Release();
        }

        // 3. Constant Buffer
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = sizeof(ParticleCBMatrix);
        cbDesc.Usage = D3D11_USAGE_DEFAULT;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        device->CreateBuffer(&cbDesc, nullptr, &ConstantBuffer);

        // 4. Rasterizer State (No culling for double-sided billboards)
        D3D11_RASTERIZER_DESC rsDesc = {};
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_NONE;
        rsDesc.DepthClipEnable = true;
        device->CreateRasterizerState(&rsDesc, &RastState);

        // 5. Depth Stencil States
        // Grid: Depth write enabled
        D3D11_DEPTH_STENCIL_DESC dsGridDesc = {};
        dsGridDesc.DepthEnable = true;
        dsGridDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsGridDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        device->CreateDepthStencilState(&dsGridDesc, &DepthStateGrid);

        // Particle: Depth write disabled (for transparent sorting/blending)
        D3D11_DEPTH_STENCIL_DESC dsPartDesc = {};
        dsPartDesc.DepthEnable = true;
        dsPartDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsPartDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        device->CreateDepthStencilState(&dsPartDesc, &DepthStateParticle);

        // 6. Blend States (6 BlendModes x 3 BlendOps matching EEnginePrimitiveBlendMode: Solid=0, BoolAlpha=1, Alpha=2, Additive=3, AddSmooth=4, ConstColour=5)
        D3D11_BLEND_OP d3dOps[3] = { D3D11_BLEND_OP_ADD, D3D11_BLEND_OP_SUBTRACT, D3D11_BLEND_OP_REV_SUBTRACT };
        for (int m = 0; m < 6; m++) {
            for (int op = 0; op < 3; op++) {
                D3D11_BLEND_DESC bDesc = {};
                bDesc.RenderTarget[0].BlendEnable = (m != 0) ? TRUE : FALSE;
                bDesc.RenderTarget[0].BlendOp = d3dOps[op];
                bDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
                bDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

                if (m == 0) { // SOLID
                    bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
                    bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
                    bDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                    bDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
                }
                else if (m == 1 || m == 2) { // BOOLEAN_ALPHA (1) / ALPHA (2)
                    bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
                    bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
                    bDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                    bDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
                }
                else if (m == 3) { // ADDITIVE
                    bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
                    bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
                    bDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                    bDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
                }
                else if (m == 4) { // ADDSMOOTH
                    bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
                    bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
                    bDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                    bDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_COLOR;
                }
                else if (m == 5) { // CONST_COLOUR
                    bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
                    bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_BLEND_FACTOR;
                    bDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                    bDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
                }

                device->CreateBlendState(&bDesc, &BlendStates[m][op]);
            }
        }
        BlendStateAlpha = BlendStates[2][0];
        BlendStateAdditive = BlendStates[3][0];
        BlendStateAddSmooth = BlendStates[4][0];
        BlendStateGrid = BlendStateAlpha;

        // 7. Sampler State
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        device->CreateSamplerState(&sampDesc, &ParticleSampler);

        // 8. Create Buffers & Fallback Texture
        CreateGridBuffers(device);
        CreateParticleBuffers(device);
        CreateDefaultGlowTexture(device);
        CreateDefaultWhiteTexture(device);

        // 9. Compile 3D Particle Mesh Shaders (CPSCRenderMesh)
        // Disassembly: 98_PSHADER_MESH_GROUP (Normal) & 99_PSHADER_MESH_GROUP_ADDITIVE_ALPHA (Additive/AddSmooth)
        const char* particleMeshShaderSrc = R"(
            cbuffer CBParticleMesh : register(b0) {
                matrix WorldViewProj;
                float4 MeshColor;
                int HasTexture;
                int BlendMode;
                int IsDXT1;
                float PMM_Padding;
            };

            struct ParticleMeshVS_IN {
                float3 Pos : POSITION;
                float3 Norm : NORMAL;
                float2 UV : TEXCOORD0;
                uint4 Joints : BLENDINDICES;
                float4 Weights : BLENDWEIGHT;
            };

            struct ParticleMeshPS_IN {
                float4 Pos : SV_POSITION;
                float3 Norm : NORMAL;
                float2 UV : TEXCOORD0;
            };

            ParticleMeshPS_IN VS(ParticleMeshVS_IN input) {
                ParticleMeshPS_IN output;
                output.Pos = mul(float4(input.Pos, 1.0f), WorldViewProj);
                output.Norm = input.Norm;
                output.UV = input.UV;
                return output;
            }

            Texture2D MeshTex : register(t0);
            SamplerState Sampler : register(s0);

            float4 PS(ParticleMeshPS_IN input) : SV_Target {
                float4 texCol = float4(1.0f, 1.0f, 1.0f, 1.0f);
                if (HasTexture != 0) {
                    texCol = MeshTex.Sample(Sampler, input.UV);
                    if (IsDXT1 != 0 || (BlendMode <= 2 && texCol.a >= 0.95f)) {
                        float brightness = max(texCol.r, max(texCol.g, texCol.b));
                        if (brightness < 0.05f) {
                            texCol.a *= saturate((brightness - 0.01f) * 25.0f);
                        }
                    }
                }
                float4 finalCol;
                finalCol.a = texCol.a * MeshColor.a;
                finalCol.rgb = saturate(2.0f * texCol.rgb * MeshColor.rgb);
                if (BlendMode == 3 || BlendMode == 4) {
                    finalCol.rgb = saturate(finalCol.rgb * finalCol.a);
                } else if (BlendMode <= 2) {
                    // Alpha testing for mesh particles (matching Fable functions8.txt:849: AlphaRef = 16)
                    clip(finalCol.a - (16.0f / 255.0f));
                }
                return finalCol;
            }
        )";

        hr = D3DCompile(particleMeshShaderSrc, strlen(particleMeshShaderSrc), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
        if (SUCCEEDED(hr)) {
            device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VS_ParticleMesh);
            D3D11_INPUT_ELEMENT_DESC partMeshDesc[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };
            device->CreateInputLayout(partMeshDesc, 5, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &Layout_ParticleMesh);
            vsBlob->Release();
        }
        else if (errorBlob) {
            errorBlob->Release();
            errorBlob = nullptr;
        }

        hr = D3DCompile(particleMeshShaderSrc, strlen(particleMeshShaderSrc), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, &errorBlob);
        if (SUCCEEDED(hr)) {
            device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PS_ParticleMesh);
            psBlob->Release();
        }
        else if (errorBlob) {
            errorBlob->Release();
            errorBlob = nullptr;
        }

        D3D11_BUFFER_DESC pmcbDesc = {};
        pmcbDesc.ByteWidth = sizeof(ParticleMeshCB);
        pmcbDesc.Usage = D3D11_USAGE_DEFAULT;
        pmcbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        device->CreateBuffer(&pmcbDesc, nullptr, &ConstantBufferParticleMesh);

        D3D11_SAMPLER_DESC meshSampDesc = {};
        meshSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        meshSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        meshSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        meshSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        meshSampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        meshSampDesc.MinLOD = 0;
        meshSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        device->CreateSamplerState(&meshSampDesc, &MeshSampler);

        Initialized = true;
    }

    void Resize(ID3D11Device* device, float w, float h) {
        if (!device || w <= 0 || h <= 0) return;
        if (w == Width && h == Height && RenderTex) return;

        ReleaseResizedResources();
        Width = w;
        Height = h;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = (UINT)w;
        desc.Height = (UINT)h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        if (SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &RenderTex))) {
            device->CreateRenderTargetView(RenderTex, nullptr, &RTV);
            device->CreateShaderResourceView(RenderTex, nullptr, &SRV);
        }

        D3D11_TEXTURE2D_DESC dDesc = desc;
        dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        if (SUCCEEDED(device->CreateTexture2D(&dDesc, nullptr, &DepthTex))) {
            device->CreateDepthStencilView(DepthTex, nullptr, &DSV);
        }
    }

    void ClearParticleTextureCache() {
        for (auto& pair : ParticleTextureCache) {
            if (pair.second.SRV && pair.second.SRV != DefaultGlowSRV) {
                pair.second.SRV->Release();
            }
        }
        ParticleTextureCache.clear();
    }

    void ClearParticleMeshCache() {
        for (auto& pair : DynamicMeshCache) {
            pair.second.Release();
        }
        DynamicMeshCache.clear();
    }

    void InvalidateParticleMesh(int32_t bankIndex) {
        auto it = DynamicMeshCache.find(bankIndex);
        if (it != DynamicMeshCache.end()) {
            it->second.Release();
            DynamicMeshCache.erase(it);
        }
    }

    ParticleTextureResource GetOrCreateParticleTexture(ID3D11Device* device, int32_t texID) {
        if (texID <= 0) {
            ParticleTextureResource def;
            def.SRV = DefaultGlowSRV;
            def.Width = 64; def.Height = 64; def.FrameWidth = 64; def.FrameHeight = 64;
            def.FrameCount = 1; def.IsSpriteSheet = false; def.IsDXT1 = false;
            return def;
        }

        if (ParticleTextureCache.count(texID)) {
            return ParticleTextureCache[texID];
        }

        if (!device) {
            ParticleTextureResource def;
            def.SRV = DefaultGlowSRV;
            def.Width = 64; def.Height = 64; def.FrameWidth = 64; def.FrameHeight = 64;
            def.FrameCount = 1; def.IsSpriteSheet = false; def.IsDXT1 = false;
            return def;
        }

        // Helper lambda to create ParticleTextureResource from a parsed CTextureParser
        auto CreateResourceFromParser = [&](CTextureParser& parser) -> ParticleTextureResource {
            ParticleTextureResource res;
            if (!parser.IsParsed || parser.DecodedPixels.empty()) return res;

            uint32_t totalW = parser.Header.Width ? parser.Header.Width : parser.Header.FrameWidth;
            uint32_t totalH = parser.Header.Height ? parser.Header.Height : parser.Header.FrameHeight;
            uint32_t frameW = parser.Header.FrameWidth ? parser.Header.FrameWidth : totalW;
            uint32_t frameH = parser.Header.FrameHeight ? parser.Header.FrameHeight : totalH;
            uint32_t frameCount = parser.Header.FrameCount > 0 ? parser.Header.FrameCount : 1;

            if (totalW == 0) totalW = 1;
            if (totalH == 0) totalH = 1;
            if (frameW == 0) frameW = totalW;
            if (frameH == 0) frameH = totalH;

            // Sanity limits
            if (totalW > 8192 || totalH > 8192 || frameCount > 512) return res;

            bool isSpriteSheet = ((parser.Header.Flags & 1) != 0) || 
                                 (frameW > 0 && frameH > 0 && 
                                 (totalW > frameW || totalH > frameH) && 
                                 frameCount > 1);
            bool isDXT1 = (parser.DecodedFormat == ETextureFormat::DXT1 || 
                           parser.DecodedFormat == ETextureFormat::NormalMap_DXT1 ||
                           parser.Header.TransparencyType == 0);

            DXGI_FORMAT dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            uint32_t blockWidth = 1;
            switch (parser.DecodedFormat) {
            case ETextureFormat::DXT1:
            case ETextureFormat::NormalMap_DXT1:
                dxFormat = DXGI_FORMAT_BC1_UNORM;
                blockWidth = 4;
                break;
            case ETextureFormat::DXT3:
                dxFormat = DXGI_FORMAT_BC2_UNORM;
                blockWidth = 4;
                break;
            case ETextureFormat::DXT5:
            case ETextureFormat::NormalMap_DXT5:
                dxFormat = DXGI_FORMAT_BC3_UNORM;
                blockWidth = 4;
                break;
            case ETextureFormat::ARGB8888:
                dxFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
                break;
            default:
                dxFormat = DXGI_FORMAT_BC1_UNORM;
                blockWidth = 4;
                break;
            }

            uint32_t rowPitch = 0;
            uint32_t mip0ByteSize = 0;
            if (blockWidth == 4) {
                uint32_t blocksX = (totalW + 3) / 4;
                uint32_t blocksY = (totalH + 3) / 4;
                uint32_t blockSize = (dxFormat == DXGI_FORMAT_BC1_UNORM) ? 8 : 16;
                rowPitch = blocksX * blockSize;
                mip0ByteSize = blocksX * blocksY * blockSize;
            }
            else {
                rowPitch = totalW * 4;
                mip0ByteSize = totalW * totalH * 4;
            }

            size_t frameStride = parser.CalculateTotalFrameSize();
            if (frameStride < mip0ByteSize) frameStride = mip0ByteSize;
            if (frameStride < parser.TrueFrameStride && parser.TrueFrameStride > 0) {
                frameStride = parser.TrueFrameStride;
            }

            uint32_t arraySize = isSpriteSheet ? 1 : frameCount;
            std::vector<D3D11_SUBRESOURCE_DATA> subData(arraySize);

            for (uint32_t f = 0; f < arraySize; f++) {
                size_t offset = (size_t)f * frameStride;
                if (offset + mip0ByteSize <= parser.DecodedPixels.size()) {
                    subData[f].pSysMem = parser.DecodedPixels.data() + offset;
                }
                else {
                    subData[f].pSysMem = parser.DecodedPixels.data();
                }
                subData[f].SysMemPitch = rowPitch;
                subData[f].SysMemSlicePitch = 0;
            }

            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = totalW;
            desc.Height = totalH;
            desc.MipLevels = 1;
            desc.ArraySize = arraySize;
            desc.Format = dxFormat;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            ID3D11Texture2D* pTex = nullptr;
            ID3D11ShaderResourceView* srv = nullptr;
            HRESULT hr = device->CreateTexture2D(&desc, subData.data(), &pTex);
            if (SUCCEEDED(hr)) {
                D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = dxFormat;
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MostDetailedMip = 0;
                srvDesc.Texture2DArray.MipLevels = 1;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.ArraySize = arraySize;

                hr = device->CreateShaderResourceView(pTex, &srvDesc, &srv);
                pTex->Release();
            }

            if (FAILED(hr)) {
                // CPU software decompress fallback to RGBA32
                std::vector<std::vector<uint8_t>> rgbaFrames(arraySize);
                std::vector<D3D11_SUBRESOURCE_DATA> rgbaSubData(arraySize);
                for (uint32_t f = 0; f < arraySize; f++) {
                    const uint8_t* rawFramePtr = (const uint8_t*)subData[f].pSysMem;
                    rgbaFrames[f] = TextureUtils::DecompressFrameToRGBA(rawFramePtr, totalW, totalH, parser.DecodedFormat);
                    rgbaSubData[f].pSysMem = rgbaFrames[f].data();
                    rgbaSubData[f].SysMemPitch = totalW * 4;
                    rgbaSubData[f].SysMemSlicePitch = 0;
                }
                desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                if (SUCCEEDED(device->CreateTexture2D(&desc, rgbaSubData.data(), &pTex))) {
                    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    srvDesc.Texture2DArray.MostDetailedMip = 0;
                    srvDesc.Texture2DArray.MipLevels = 1;
                    srvDesc.Texture2DArray.FirstArraySlice = 0;
                    srvDesc.Texture2DArray.ArraySize = arraySize;
                    device->CreateShaderResourceView(pTex, &srvDesc, &srv);
                    pTex->Release();
                }
            }

            if (srv) {
                res.SRV = srv;
                res.Width = totalW;
                res.Height = totalH;
                res.FrameWidth = frameW;
                res.FrameHeight = frameH;
                res.FrameCount = frameCount;
                res.IsSpriteSheet = isSpriteSheet;
                res.IsDXT1 = isDXT1;
            }
            return res;
        };

        // Determine if Xbox version: check if active effects subbank is PARTICLE_MAIN (not _PC)
        bool isXbox = false;
        if (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < (int)g_OpenBanks.size()) {
            const auto& activeBank = g_OpenBanks[g_ActiveBankIndex];
            if (activeBank.Type == EBankType::Effects) {
                if (activeBank.ActiveSubBankIndex >= 0 && activeBank.ActiveSubBankIndex < (int)activeBank.SubBanks.size()) {
                    const std::string& sbName = activeBank.SubBanks[activeBank.ActiveSubBankIndex].Name;
                    if (sbName == "PARTICLE_MAIN" || (sbName.find("PARTICLE_MAIN") != std::string::npos && sbName.find("_PC") == std::string::npos)) {
                        isXbox = true;
                    }
                }
            }
        }
        if (!isXbox) {
            bool hasXboxGraphics = false;
            bool hasPcTextures = false;
            for (const auto& b : g_OpenBanks) {
                if (b.Type == EBankType::XboxGraphics) hasXboxGraphics = true;
                if (b.Type == EBankType::Textures) hasPcTextures = true;
            }
            if (hasXboxGraphics && !hasPcTextures) isXbox = true;
        }

        // 1. Xbox Mode: Retrieve from Xbox graphics.big (GBANK subbanks via g_XboxTexCache)
        if (isXbox) {
            for (int bIdx = 0; bIdx < (int)g_OpenBanks.size(); ++bIdx) {
                auto& bank = g_OpenBanks[bIdx];
                if (bank.Type == EBankType::XboxGraphics) {
                    EnsureXboxTextureCache(bIdx);
                    if (g_XboxTexCache[bIdx].count(texID)) {
                        auto& meta = g_XboxTexCache[bIdx][texID];
                        if (meta.Info.size() >= 28 && meta.Size > 0) {
                            try {
                                std::vector<uint8_t> tempData;
                                if (bank.Stream && bank.Stream->is_open()) {
                                    bank.Stream->clear();
                                    bank.Stream->seekg(meta.Offset, std::ios::beg);
                                    tempData.resize(meta.Size + 64);
                                    bank.Stream->read((char*)tempData.data(), meta.Size);
                                }
                                CTextureParser parser;
                                parser.Parse(meta.Info, tempData, meta.Type);
                                ParticleTextureResource res = CreateResourceFromParser(parser);
                                if (res.SRV) {
                                    ParticleTextureCache[texID] = res;
                                    return res;
                                }
                            }
                            catch (...) {}
                        }
                    }
                }
            }
        }
        // 2. PC Mode: Search open Textures and Frontend banks (NEVER search Effects or Graphics)
        else {
            for (auto& bank : g_OpenBanks) {
                if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend) {
                    for (size_t i = 0; i < bank.Entries.size(); ++i) {
                        if (bank.Entries[i].ID == (uint32_t)texID) {
                            // Check staged entries first
                            if (bank.StagedEntries.count((int)i) && bank.StagedEntries[(int)i].Texture) {
                                auto& stagedTex = bank.StagedEntries[(int)i].Texture;
                                if (!stagedTex->RawFrames.empty()) {
                                    uint32_t stagedFrames = (uint32_t)stagedTex->RawFrames.size();
                                    uint32_t sW = stagedTex->Header.Width ? stagedTex->Header.Width : stagedTex->Header.FrameWidth;
                                    uint32_t sH = stagedTex->Header.Height ? stagedTex->Header.Height : stagedTex->Header.FrameHeight;
                                    uint32_t sFW = stagedTex->Header.FrameWidth ? stagedTex->Header.FrameWidth : sW;
                                    uint32_t sFH = stagedTex->Header.FrameHeight ? stagedTex->Header.FrameHeight : sH;
                                    bool sIsSpriteSheet = (sFW > 0 && sFH > 0 && (sW > sFW || sH > sFH) && stagedFrames > 1);

                                    uint32_t arrSize = sIsSpriteSheet ? 1 : stagedFrames;
                                    std::vector<D3D11_SUBRESOURCE_DATA> stagedSubData(arrSize);
                                    for (uint32_t f = 0; f < arrSize; f++) {
                                        stagedSubData[f].pSysMem = stagedTex->RawFrames[f].data();
                                        stagedSubData[f].SysMemPitch = sW * 4;
                                        stagedSubData[f].SysMemSlicePitch = 0;
                                    }

                                    D3D11_TEXTURE2D_DESC sDesc = {};
                                    sDesc.Width = sW;
                                    sDesc.Height = sH;
                                    sDesc.MipLevels = 1;
                                    sDesc.ArraySize = arrSize;
                                    sDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                                    sDesc.SampleDesc.Count = 1;
                                    sDesc.Usage = D3D11_USAGE_DEFAULT;
                                    sDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                                    ID3D11Texture2D* sTex = nullptr;
                                    if (SUCCEEDED(device->CreateTexture2D(&sDesc, stagedSubData.data(), &sTex))) {
                                        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                                        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                                        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                                        srvDesc.Texture2DArray.MostDetailedMip = 0;
                                        srvDesc.Texture2DArray.MipLevels = 1;
                                        srvDesc.Texture2DArray.FirstArraySlice = 0;
                                        srvDesc.Texture2DArray.ArraySize = arrSize;

                                        ID3D11ShaderResourceView* srv = nullptr;
                                        if (SUCCEEDED(device->CreateShaderResourceView(sTex, &srvDesc, &srv))) {
                                            sTex->Release();
                                            ParticleTextureResource res;
                                            res.SRV = srv;
                                            res.Width = sW; res.Height = sH;
                                            res.FrameWidth = sFW; res.FrameHeight = sFH;
                                            res.FrameCount = stagedFrames;
                                            res.IsSpriteSheet = sIsSpriteSheet;
                                            res.IsDXT1 = (stagedTex->TargetFormat == ETextureFormat::DXT1 || stagedTex->TargetFormat == ETextureFormat::NormalMap_DXT1);
                                            ParticleTextureCache[texID] = res;
                                            return res;
                                        }
                                        sTex->Release();
                                    }
                                }
                            }

                            // Read bank raw data
                            if (bank.SubheaderCache.count((int)i) && bank.SubheaderCache[(int)i].size() >= 28) {
                                try {
                                    std::vector<uint8_t> tempData;
                                    if (bank.ModifiedEntryData.count((int)i)) {
                                        tempData = bank.ModifiedEntryData[(int)i];
                                    }
                                    else if (bank.Stream && bank.Stream->is_open()) {
                                        bank.Stream->clear();
                                        bank.Stream->seekg(bank.Entries[i].Offset, std::ios::beg);
                                        tempData.resize(bank.Entries[i].Size + 64);
                                        bank.Stream->read((char*)tempData.data(), bank.Entries[i].Size);
                                    }

                                    CTextureParser parser;
                                    parser.Parse(bank.SubheaderCache[(int)i], tempData, bank.Entries[i].Type);
                                    ParticleTextureResource res = CreateResourceFromParser(parser);
                                    if (res.SRV) {
                                        ParticleTextureCache[texID] = res;
                                        return res;
                                    }
                                }
                                catch (...) {}
                            }
                        }
                    }
                }
            }
        }

        // Fallback to default glow and cache it for this texID
        ParticleTextureResource def;
        def.SRV = DefaultGlowSRV;
        def.Width = 64; def.Height = 64; def.FrameWidth = 64; def.FrameHeight = 64;
        def.FrameCount = 1; def.IsSpriteSheet = false; def.IsDXT1 = false;
        ParticleTextureCache[texID] = def;
        return def;
    }

    ParticleTextureResource GetTextureInfo(int32_t texID) {
        if (texID <= 0) return {};
        if (ParticleTextureCache.count(texID)) return ParticleTextureCache[texID];
        if (Device) return GetOrCreateParticleTexture(Device, texID);
        return {};
    }

    ParticleMeshGPU* GetOrCreateParticleMesh(ID3D11Device* device, int32_t bankIndex) {
        if (bankIndex <= 0 || !device) return nullptr;

        auto it = DynamicMeshCache.find(bankIndex);
        if (it != DynamicMeshCache.end()) {
            if (it->second.IsValid) return &it->second;
        }

        C3DMeshContent mesh;
        bool found = false;

        // Search open banks for matching mesh entry
        for (auto& bank : g_OpenBanks) {
            if (bank.Type == EBankType::Graphics || (bank.Type == EBankType::XboxGraphics && IsGraphicsSubBank(&bank))) {
                for (size_t j = 0; j < bank.Entries.size(); j++) {
                    if (bank.Entries[j].ID == (uint32_t)bankIndex && IsSupportedMesh(bank.Entries[j].Type)) {
                        // Check staged LODs first
                        if (bank.StagedEntries.count((int)j) && !bank.StagedEntries[(int)j].MeshLODs.empty() && bank.StagedEntries[(int)j].MeshLODs[0]) {
                            mesh = *bank.StagedEntries[(int)j].MeshLODs[0];
                            found = true;
                            break;
                        }
                        // Check modified entry data
                        std::vector<uint8_t> rawData;
                        if (bank.ModifiedEntryData.count((int)j)) {
                            rawData = bank.ModifiedEntryData[(int)j];
                        }
                        // Check bank file stream
                        else if (bank.Stream && bank.Stream->is_open()) {
                            bank.Stream->clear();
                            bank.Stream->seekg(bank.Entries[j].Offset, std::ios::beg);
                            rawData.resize(bank.Entries[j].Size);
                            bank.Stream->read((char*)rawData.data(), bank.Entries[j].Size);
                        }

                        if (!rawData.empty()) {
                            if (bank.SubheaderCache.count((int)j)) {
                                mesh.ParseEntryMetadata(bank.SubheaderCache[(int)j]);
                            }
                            if (mesh.Parse(rawData, bank.Entries[j].Type)) {
                                found = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (found) break;
        }

        // Secondary search across any remaining banks
        if (!found) {
            for (auto& bank : g_OpenBanks) {
                for (size_t j = 0; j < bank.Entries.size(); j++) {
                    if (bank.Entries[j].ID == (uint32_t)bankIndex && IsSupportedMesh(bank.Entries[j].Type)) {
                        if (bank.StagedEntries.count((int)j) && !bank.StagedEntries[(int)j].MeshLODs.empty() && bank.StagedEntries[(int)j].MeshLODs[0]) {
                            mesh = *bank.StagedEntries[(int)j].MeshLODs[0];
                            found = true;
                            break;
                        }
                        std::vector<uint8_t> rawData;
                        if (bank.ModifiedEntryData.count((int)j)) {
                            rawData = bank.ModifiedEntryData[(int)j];
                        }
                        else if (bank.Stream && bank.Stream->is_open()) {
                            bank.Stream->clear();
                            bank.Stream->seekg(bank.Entries[j].Offset, std::ios::beg);
                            rawData.resize(bank.Entries[j].Size);
                            bank.Stream->read((char*)rawData.data(), bank.Entries[j].Size);
                        }

                        if (!rawData.empty()) {
                            if (bank.SubheaderCache.count((int)j)) {
                                mesh.ParseEntryMetadata(bank.SubheaderCache[(int)j]);
                            }
                            if (mesh.Parse(rawData, bank.Entries[j].Type)) {
                                found = true;
                                break;
                            }
                        }
                    }
                }
                if (found) break;
            }
        }

        if (!found || !mesh.IsParsed) return nullptr;

        // Build GPU buffers for mesh
        std::vector<GPUVertex> vertices;
        std::vector<uint32_t> indices;
        uint32_t indexOffset = 0;
        std::vector<ParticleMeshBatch> batches;

        for (const auto& prim : mesh.Primitives) {
            int reps = (prim.RepeatingMeshReps > 1) ? prim.RepeatingMeshReps : 1;
            uint32_t totalVerts = prim.VertexCount * reps;
            if (totalVerts == 0 || prim.VertexBuffer.empty()) continue;

            bool hasBones = (prim.AnimatedBlockCount > 0);
            bool isPosComp = (prim.InitFlags & 4) != 0 && (prim.InitFlags & 0x10) == 0;
            bool isNormComp = (prim.InitFlags & 4) != 0;

            if (mesh.MeshType == 4 || (prim.VertexStride == 36 && !hasBones)) {
                isPosComp = false;
                isNormComp = false;
            }

            size_t iOff = isPosComp ? 4 : 12;
            size_t wOff = iOff + 4;
            size_t normOff = iOff + (hasBones ? 8 : 0);
            size_t uvOff = normOff + (isNormComp ? 4 : 12);

            bool hasNormals = true;

            if (reps > 1) {
                isPosComp = false;
                isNormComp = false;
                hasNormals = true;
                normOff = 12;
                uvOff = 24;
            }
            else if (!hasBones) {
                if (prim.VertexStride == 24 && !isPosComp && !isNormComp) {
                    hasNormals = false;
                    uvOff = 16;
                }
                else if (prim.VertexStride == 20 && isPosComp && !isNormComp) {
                    hasNormals = false;
                    uvOff = 12;
                }
                else if (prim.VertexStride == 28) {
                    hasNormals = true;
                    normOff = 12;
                    uvOff = 24;
                }
            }

            for (uint32_t v = 0; v < totalVerts; v++) {
                size_t offset = (size_t)v * prim.VertexStride;
                if (offset + 12 > prim.VertexBuffer.size()) break;
                GPUVertex gpuV = {};

                if (isPosComp) {
                    uint32_t p = *(uint32_t*)(prim.VertexBuffer.data() + offset);
                    UnpackPOSPACKED3(p, prim.Compression.Scale, prim.Compression.Offset, gpuV.Pos.x, gpuV.Pos.y, gpuV.Pos.z);
                }
                else {
                    const float* r = (const float*)(prim.VertexBuffer.data() + offset);
                    gpuV.Pos = XMFLOAT3(r[0], r[1], r[2]);
                }

                if (hasNormals && offset + normOff + 4 <= prim.VertexBuffer.size()) {
                    if (isNormComp) {
                        uint32_t p = *(uint32_t*)(prim.VertexBuffer.data() + offset + normOff);
                        UnpackNORMPACKED3(p, gpuV.Norm.x, gpuV.Norm.y, gpuV.Norm.z);
                    }
                    else if (offset + normOff + 12 <= prim.VertexBuffer.size()) {
                        const float* r = (const float*)(prim.VertexBuffer.data() + offset + normOff);
                        gpuV.Norm = XMFLOAT3(r[0], r[1], r[2]);
                    }
                    else {
                        gpuV.Norm = XMFLOAT3(0.0f, 1.0f, 0.0f);
                    }
                }
                else {
                    gpuV.Norm = XMFLOAT3(0.0f, 1.0f, 0.0f);
                }

                if (offset + uvOff + 4 <= prim.VertexBuffer.size()) {
                    if (isNormComp) {
                        if (mesh.MeshType == 4) {
                            const uint16_t* u = (const uint16_t*)(prim.VertexBuffer.data() + offset + uvOff);
                            gpuV.UV = XMFLOAT2((float)u[0] / 65535.0f, (float)u[1] / 65535.0f);
                        }
                        else {
                            const int16_t* u = (const int16_t*)(prim.VertexBuffer.data() + offset + uvOff);
                            gpuV.UV = XMFLOAT2(DecompressUV(u[0]), DecompressUV(u[1]));
                        }
                    }
                    else if (prim.VertexStride == 28) {
                        const uint16_t* u = (const uint16_t*)(prim.VertexBuffer.data() + offset + uvOff);
                        gpuV.UV = XMFLOAT2((float)u[0] / 65535.0f, (float)u[1] / 65535.0f);
                    }
                    else if (offset + uvOff + 8 <= prim.VertexBuffer.size()) {
                        const float* r = (const float*)(prim.VertexBuffer.data() + offset + uvOff);
                        gpuV.UV = XMFLOAT2(r[0], r[1]);
                    }
                    else {
                        const uint16_t* u = (const uint16_t*)(prim.VertexBuffer.data() + offset + uvOff);
                        gpuV.UV = XMFLOAT2((float)u[0] / 65535.0f, (float)u[1] / 65535.0f);
                    }
                }
                else {
                    gpuV.UV = XMFLOAT2(0.0f, 0.0f);
                }

                gpuV.Joints = 0;
                gpuV.Weights = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
                vertices.push_back(gpuV);
            }

            auto ProcessIndices = [&](uint32_t start, uint32_t count, bool isStrip, int32_t matIdx) {
                uint32_t blockBatchStart = (uint32_t)indices.size();

                if (isStrip) {
                    int parity = 0;
                    for (uint32_t k = 0; k < count; k++) {
                        if (start + k + 2 >= prim.IndexBuffer.size()) break;
                        uint16_t i0 = prim.IndexBuffer[start + k];
                        uint16_t i1 = prim.IndexBuffer[start + k + 1];
                        uint16_t i2 = prim.IndexBuffer[start + k + 2];
                        if (i0 == 0xFFFF || i1 == 0xFFFF || i2 == 0xFFFF) { parity = 0; continue; }
                        if (i0 == i1 || i1 == i2 || i0 == i2) { parity++; continue; }
                        if (parity % 2 != 0) {
                            indices.push_back(i0 + indexOffset);
                            indices.push_back(i2 + indexOffset);
                            indices.push_back(i1 + indexOffset);
                        }
                        else {
                            indices.push_back(i0 + indexOffset);
                            indices.push_back(i1 + indexOffset);
                            indices.push_back(i2 + indexOffset);
                        }
                        parity++;
                    }
                }
                else {
                    for (uint32_t k = 0; k < count * 3; k++) {
                        if (start + k < prim.IndexBuffer.size()) {
                            uint16_t i = prim.IndexBuffer[start + k];
                            if (i != 0xFFFF) indices.push_back(i + indexOffset);
                        }
                    }
                }

                uint32_t blockIndexCount = (uint32_t)indices.size() - blockBatchStart;
                if (blockIndexCount > 0) {
                    int32_t diffMap = 0;
                    if (matIdx >= 0 && matIdx < (int32_t)mesh.Materials.size()) {
                        diffMap = mesh.Materials[matIdx].DiffuseMapID;
                    }
                    batches.push_back({ blockBatchStart, blockIndexCount, matIdx, diffMap });
                }
            };

            if (reps > 1) {
                ProcessIndices(0, (uint32_t)prim.IndexBuffer.size() / 3, false, prim.MaterialIndex);
            }
            else {
                bool processed = false;
                for (const auto& b : prim.StaticBlocks) { ProcessIndices(b.StartIndex, b.PrimitiveCount, b.IsStrip, b.MaterialIndex); processed = true; }
                for (const auto& b : prim.AnimatedBlocks) { ProcessIndices(b.StartIndex, b.PrimitiveCount, b.IsStrip, prim.MaterialIndex); processed = true; }
                if (!processed && !prim.IndexBuffer.empty()) { ProcessIndices(0, (uint32_t)prim.IndexBuffer.size() / 3, false, prim.MaterialIndex); }
            }
            indexOffset += totalVerts;
        }

        if (vertices.empty() || indices.empty()) return nullptr;

        ParticleMeshGPU gpuMesh = {};
        gpuMesh.TotalVertexCount = (uint32_t)vertices.size();
        gpuMesh.TotalIndexCount = (uint32_t)indices.size();
        gpuMesh.Batches = batches;
        if (gpuMesh.Batches.empty()) {
            int32_t diffMap = 0;
            if (!mesh.Materials.empty()) diffMap = mesh.Materials[0].DiffuseMapID;
            gpuMesh.Batches.push_back({ 0, gpuMesh.TotalIndexCount, 0, diffMap });
        }

        // Determine bounding center & radius
        float bRadius = mesh.BoundingSphereRadius;
        XMFLOAT3 bCenter(mesh.BoundingSphereCenter[0], mesh.BoundingSphereCenter[1], mesh.BoundingSphereCenter[2]);
        if (bRadius <= 0.0001f && mesh.EntryMeta.BoundingSphereRadius > 0.0001f) {
            bRadius = mesh.EntryMeta.BoundingSphereRadius;
            bCenter = XMFLOAT3(mesh.EntryMeta.BoundingSphereCenter[0], mesh.EntryMeta.BoundingSphereCenter[1], mesh.EntryMeta.BoundingSphereCenter[2]);
        }
        if (bRadius <= 0.0001f) {
            XMFLOAT3 minP = vertices[0].Pos;
            XMFLOAT3 maxP = vertices[0].Pos;
            for (const auto& v : vertices) {
                minP.x = (std::min)(minP.x, v.Pos.x); minP.y = (std::min)(minP.y, v.Pos.y); minP.z = (std::min)(minP.z, v.Pos.z);
                maxP.x = (std::max)(maxP.x, v.Pos.x); maxP.y = (std::max)(maxP.y, v.Pos.y); maxP.z = (std::max)(maxP.z, v.Pos.z);
            }
            bCenter = XMFLOAT3((minP.x + maxP.x) * 0.5f, (minP.y + maxP.y) * 0.5f, (minP.z + maxP.z) * 0.5f);
            float maxD = 0.0f;
            for (const auto& v : vertices) {
                float dx = v.Pos.x - bCenter.x; float dy = v.Pos.y - bCenter.y; float dz = v.Pos.z - bCenter.z;
                float d = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (d > maxD) maxD = d;
            }
            bRadius = (maxD > 0.0001f) ? maxD : 1.0f;
        }
        gpuMesh.BoundingCenter = bCenter;
        gpuMesh.BoundingRadius = bRadius;

        // Create D3D11 Vertex & Index buffers
        D3D11_BUFFER_DESC vDesc = {};
        vDesc.ByteWidth = sizeof(GPUVertex) * (UINT)vertices.size();
        vDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vData = { vertices.data(), 0, 0 };
        if (FAILED(device->CreateBuffer(&vDesc, &vData, &gpuMesh.VertexBuffer))) {
            return nullptr;
        }

        D3D11_BUFFER_DESC iDesc = {};
        iDesc.ByteWidth = sizeof(uint32_t) * (UINT)indices.size();
        iDesc.Usage = D3D11_USAGE_IMMUTABLE;
        iDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iData = { indices.data(), 0, 0 };
        if (FAILED(device->CreateBuffer(&iDesc, &iData, &gpuMesh.IndexBuffer))) {
            gpuMesh.VertexBuffer->Release();
            gpuMesh.VertexBuffer = nullptr;
            return nullptr;
        }

        gpuMesh.IsValid = true;
        DynamicMeshCache[bankIndex] = gpuMesh;
        return &DynamicMeshCache[bankIndex];
    }

    // 511-Step 3-Point Color Spline (disassembly Functions.txt lines 3425-3548: GFGetInterpolatedColour)
    static XMFLOAT4 EvaluateColorSpline511(const XMFLOAT4& cStart, const XMFLOAT4& cMid, const XMFLOAT4& cEnd,
                                           bool useStart, bool useMid, bool useEnd, float normalised_time) {
        XMFLOAT4 colours[3];
        colours[0] = useStart ? cStart : (useMid ? cMid : (useEnd ? cEnd : XMFLOAT4(1, 1, 1, 1)));
        colours[2] = useEnd ? cEnd : (useMid ? cMid : (useStart ? cStart : XMFLOAT4(1, 1, 1, 1)));
        colours[1] = useMid ? cMid : XMFLOAT4(
            (colours[0].x + colours[2].x) * 0.5f,
            (colours[0].y + colours[2].y) * 0.5f,
            (colours[0].z + colours[2].z) * 0.5f,
            (colours[0].w + colours[2].w) * 0.5f
        );

        int idx = (int)(std::clamp(normalised_time, 0.0f, 1.0f) * 511.0f);
        int seg = idx >> 8; // 0 or 1
        float factor = (float)(idx & 0xFF) / 256.0f;

        const XMFLOAT4& c0 = (seg == 0) ? colours[0] : colours[1];
        const XMFLOAT4& c1 = (seg == 0) ? colours[1] : colours[2];

        return XMFLOAT4(
            c0.x + (c1.x - c0.x) * factor,
            c0.y + (c1.y - c0.y) * factor,
            c0.z + (c1.z - c0.z) * factor,
            c0.w + (c1.w - c0.w) * factor
        );
    }

    // Cosine S-Curve Fade (disassembly Functions.txt lines 3559-3584: GFastCosTable)
    static float EvaluateCosineFade(float t, float fadeInEnd, float fadeOutBegin, float alphaFadeMin) {
        float fade = 1.0f;
        if (fadeInEnd > 0.001f && t < fadeInEnd) {
            float u = t / fadeInEnd;
            float smoothVal = 0.5f - 0.5f * std::cos(XM_PI * u);
            fade *= smoothVal;
        }
        if (fadeOutBegin < 0.999f && t > fadeOutBegin) {
            float u = (1.0f - t) / (1.0f - fadeOutBegin);
            float smoothVal = 0.5f - 0.5f * std::cos(XM_PI * u);
            fade *= smoothVal;
        }
        return fade * (1.0f - alphaFadeMin) + alphaFadeMin;
    }

    ID3D11ShaderResourceView* Render(ID3D11DeviceContext* ctx, float w, float h, bool isHovered,
                                     const CParticleEmitter* emitter = nullptr,
                                     const ParticleSimulator* simulator = nullptr) {
        if (!Initialized || !ctx || !RTV || !DSV || !VS_Line || !GridVBuffer) return nullptr;

        // Viewport camera interactions
        if (isHovered) {
            ImGuiIO& io = ImGui::GetIO();
            if (ImGui::IsMouseDragging(1)) {
                CamRotY += io.MouseDelta.x * 0.01f;
                CamRotX += io.MouseDelta.y * 0.01f;
            }
            if (ImGui::IsMouseDragging(2) || (ImGui::IsMouseDragging(1) && ImGui::IsKeyDown(ImGuiKey_LeftShift))) {
                CamPan.x += io.MouseDelta.x * (CamDist * 0.002f);
                CamPan.y -= io.MouseDelta.y * (CamDist * 0.002f);
            }
            CamDist -= io.MouseWheel * CamDist * 0.1f;
            if (CamDist < 0.5f) CamDist = 0.5f;
            if (CamDist > 500.0f) CamDist = 500.0f;
        }

        // Clear offscreen buffers
        float clearColor[4] = { g_AppConfig.RendererBgColor[0], g_AppConfig.RendererBgColor[1], g_AppConfig.RendererBgColor[2], 1.0f };
        ctx->ClearRenderTargetView(RTV, clearColor);
        ctx->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        ctx->OMSetRenderTargets(1, &RTV, DSV);

        D3D11_VIEWPORT vp = { 0.0f, 0.0f, w, h, 0.0f, 1.0f };
        ctx->RSSetViewports(1, &vp);

        // Compute Matrices
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), (w > 0 && h > 0) ? (w / h) : 1.0f, 0.1f, 2000.0f);
        XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -CamDist, 0.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        XMMATRIX worldCam = XMMatrixTranslation(CenterOffset.x, CenterOffset.y, CenterOffset.z)
                          * XMMatrixRotationX(CamRotX)
                          * XMMatrixRotationY(CamRotY)
                          * XMMatrixTranslation(CamPan.x, CamPan.y, 0.0f);

        // Transform engine Z-Up coordinate space into camera view space matching MeshRenderer.h
        worldCam = XMMatrixRotationX(-XM_PIDIV2) * worldCam;

        XMMATRIX viewMat = worldCam * view;
        XMMATRIX wvp = viewMat * proj;

        ParticleCBMatrix cb = {};
        cb.WorldViewProj = XMMatrixTranspose(wvp);
        cb.IsDXT1 = 0;
        ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);

        // Extract camera Right and Up vectors in world space for billboards
        XMMATRIX invView = XMMatrixInverse(nullptr, viewMat);
        XMVECTOR camRight = XMVector3Normalize(invView.r[0]);
        XMVECTOR camUp    = XMVector3Normalize(invView.r[1]);

        // ----------------------------------------------------
        // PASS 1: Render Ground Grid and Axes
        // ----------------------------------------------------
        if (ShowGrid) {
            UINT stride = sizeof(ParticleGridVertex);
            UINT offset = 0;
            ctx->IASetVertexBuffers(0, 1, &GridVBuffer, &stride, &offset);
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            ctx->IASetInputLayout(Layout_Line);

            ctx->VSSetShader(VS_Line, nullptr, 0);
            ctx->VSSetConstantBuffers(0, 1, &ConstantBuffer);
            ctx->PSSetShader(PS_Line, nullptr, 0);

            ctx->RSSetState(RastState);
            ctx->OMSetDepthStencilState(DepthStateGrid, 0);
            float blendFactor[4] = { 0, 0, 0, 0 };
            ctx->OMSetBlendState(BlendStateGrid, blendFactor, 0xFFFFFFFF);

            ctx->Draw(GridVertexCount, 0);
        }

        // ----------------------------------------------------
        // PASS 2: Render Simulated Particles
        // ----------------------------------------------------
        if (emitter && simulator && VS_Particle && PS_Particle && ParticleVBuffer && ParticleIBuffer) {
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            ctx->IASetInputLayout(Layout_Particle);
            ctx->VSSetShader(VS_Particle, nullptr, 0);
            ctx->VSSetConstantBuffers(0, 1, &ConstantBuffer);
            ctx->PSSetShader(PS_Particle, nullptr, 0);
            ctx->PSSetConstantBuffers(0, 1, &ConstantBuffer);
            ctx->PSSetSamplers(0, 1, &ParticleSampler);

            // Particles use depth test with depth writes disabled
            ctx->OMSetDepthStencilState(DepthStateParticle, 0);

            UINT pStride = sizeof(ParticleQuadVertex);
            UINT pOffset = 0;
            ctx->IASetVertexBuffers(0, 1, &ParticleVBuffer, &pStride, &pOffset);
            ctx->IASetIndexBuffer(ParticleIBuffer, DXGI_FORMAT_R32_UINT, 0);

            std::vector<ParticleQuadVertex> quadVertices;

            for (size_t sysIdx = 0; sysIdx < emitter->Systems.size(); sysIdx++) {
                if (sysIdx >= simulator->SystemStates.size()) break;
                const auto& sys = emitter->Systems[sysIdx];
                const auto& state = simulator->SystemStates[sysIdx];

                if (!sys.Enabled) continue;

                // Find render components
                std::shared_ptr<CPSCRenderSprite> renderSprite;
                std::shared_ptr<CPSCSingleSprite> singleSprite;
                std::shared_ptr<CPSCRenderMesh> renderMesh;
                for (const auto& comp : sys.Components) {
                    if (!comp || !comp->Enabled) continue;
                    if (comp->ClassName == "CPSCRenderSprite") renderSprite = std::dynamic_pointer_cast<CPSCRenderSprite>(comp);
                    else if (comp->ClassName == "CPSCSingleSprite") singleSprite = std::dynamic_pointer_cast<CPSCSingleSprite>(comp);
                    else if (comp->ClassName == "CPSCRenderMesh") renderMesh = std::dynamic_pointer_cast<CPSCRenderMesh>(comp);
                }

                if (!renderSprite && !singleSprite && !renderMesh) continue;

                ID3D11Device* device = Device;
                if (!device) ctx->GetDevice(&device);

                quadVertices.clear();
                float interp = simulator ? simulator->InterpolationFactor : 0.0f;

                auto FlushQuads = [&]() {
                    if (!quadVertices.empty()) {
                        D3D11_MAPPED_SUBRESOURCE mapped;
                        if (SUCCEEDED(ctx->Map(ParticleVBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                            memcpy(mapped.pData, quadVertices.data(), sizeof(ParticleQuadVertex) * quadVertices.size());
                            ctx->Unmap(ParticleVBuffer, 0);

                            uint32_t quadCount = (uint32_t)quadVertices.size() / 4;
                            uint32_t indexCount = quadCount * 6;
                            ctx->DrawIndexed(indexCount, 0, 0);
                        }
                        quadVertices.clear();
                    }
                };

                auto GetFrameUVAndSlice = [&](const ParticleTextureResource& tr, int frameIdx, XMFLOAT4& outUV, float& outSlice) {
                    if (tr.IsSpriteSheet && tr.Width > 0 && tr.Height > 0 && 
                        tr.FrameWidth > 0 && tr.FrameHeight > 0) {
                        uint32_t cols = tr.Width / tr.FrameWidth;
                        if (cols < 1) cols = 1;
                        uint32_t col = (uint32_t)frameIdx % cols;
                        uint32_t row = (uint32_t)frameIdx / cols;

                        float u0 = (float)(col * tr.FrameWidth) / (float)tr.Width;
                        float v0 = (float)(row * tr.FrameHeight) / (float)tr.Height;
                        float u1 = u0 + (float)tr.FrameWidth / (float)tr.Width;
                        float v1 = v0 + (float)tr.FrameHeight / (float)tr.Height;

                        outUV = XMFLOAT4(u0, v0, u1, v1);
                        outSlice = 0.0f;
                    }
                    else {
                        outUV = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
                        outSlice = (float)frameIdx;
                    }
                };

                // Helper to emit a quad given center, half-size X/Y, angle, color, crossed flag, uv bounds, and slice
                auto EmitQuad = [&](const XMFLOAT3& pos, float halfSizeX, float halfSizeY, float angle, const XMFLOAT4& col, bool crossed, const XMFLOAT4& uvBounds, float slice) {
                    if (quadVertices.size() + (crossed ? 8 : 4) > MaxParticleVertices) return;

                    float u0 = uvBounds.x;
                    float v0 = uvBounds.y;
                    float u1 = uvBounds.z;
                    float v1 = uvBounds.w;

                    if (crossed) {
                        float cosA = std::cos(angle);
                        float sinA = std::sin(angle);

                        // Quad 1: along angle in XY plane, height standing upright along Z axis (functions7.txt:1011)
                        XMVECTOR r1 = XMVectorSet(cosA * halfSizeX, sinA * halfSizeX, 0.0f, 0.0f);
                        XMVECTOR u = XMVectorSet(0.0f, 0.0f, halfSizeY, 0.0f);
                        XMVECTOR c = XMVectorSet(pos.x, pos.y, pos.z, 0.0f);

                        XMFLOAT3 p0, p1, p2, p3;
                        XMStoreFloat3(&p0, c - r1 + u);
                        XMStoreFloat3(&p1, c + r1 + u);
                        XMStoreFloat3(&p2, c + r1 - u);
                        XMStoreFloat3(&p3, c - r1 - u);
                        quadVertices.push_back({ p0, XMFLOAT3(u0, v0, slice), col });
                        quadVertices.push_back({ p1, XMFLOAT3(u1, v0, slice), col });
                        quadVertices.push_back({ p2, XMFLOAT3(u1, v1, slice), col });
                        quadVertices.push_back({ p3, XMFLOAT3(u0, v1, slice), col });

                        // Quad 2: perpendicular (+90 deg in XY plane)
                        XMVECTOR r2 = XMVectorSet(-sinA * halfSizeX, cosA * halfSizeX, 0.0f, 0.0f);
                        XMStoreFloat3(&p0, c - r2 + u);
                        XMStoreFloat3(&p1, c + r2 + u);
                        XMStoreFloat3(&p2, c + r2 - u);
                        XMStoreFloat3(&p3, c - r2 - u);
                        quadVertices.push_back({ p0, XMFLOAT3(u0, v0, slice), col });
                        quadVertices.push_back({ p1, XMFLOAT3(u1, v0, slice), col });
                        quadVertices.push_back({ p2, XMFLOAT3(u1, v1, slice), col });
                        quadVertices.push_back({ p3, XMFLOAT3(u0, v1, slice), col });
                    }
                    else {
                        // Camera-facing billboard quad rotated by angle
                        float cosA = std::cos(angle);
                        float sinA = std::sin(angle);

                        XMVECTOR center = XMVectorSet(pos.x, pos.y, pos.z, 0.0f);
                        XMVECTOR r = (camRight * cosA + camUp * sinA) * halfSizeX;
                        XMVECTOR u = (-camRight * sinA + camUp * cosA) * halfSizeY;

                        XMFLOAT3 p0, p1, p2, p3;
                        XMStoreFloat3(&p0, center - r + u); // Top-Left
                        XMStoreFloat3(&p1, center + r + u); // Top-Right
                        XMStoreFloat3(&p2, center + r - u); // Bottom-Right
                        XMStoreFloat3(&p3, center - r - u); // Bottom-Left

                        quadVertices.push_back({ p0, XMFLOAT3(u0, v0, slice), col });
                        quadVertices.push_back({ p1, XMFLOAT3(u1, v0, slice), col });
                        quadVertices.push_back({ p2, XMFLOAT3(u1, v1, slice), col });
                        quadVertices.push_back({ p3, XMFLOAT3(u0, v1, slice), col });
                    }
                };

                // 1. Process simulated particles for CPSCRenderSprite
                if (renderSprite && renderSprite->SpriteBankIndex > 0) {
                    uint32_t rsBMode = renderSprite->BlendMode;
                    uint32_t rsBOp = renderSprite->BlendOp;
                    bool rsCrossed = ((renderSprite->SpriteFlags & 0x40000) != 0);

                    ParticleTextureResource rsTexRes = GetOrCreateParticleTexture(device, renderSprite->SpriteBankIndex);
                    ID3D11ShaderResourceView* rsSrv = rsTexRes.SRV ? rsTexRes.SRV : DefaultGlowSRV;

                    cb.IsDXT1 = rsTexRes.IsDXT1 ? 1 : 0;
                    cb.BlendMode = (int32_t)rsBMode;
                    ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);

                    ctx->PSSetShaderResources(0, 1, &rsSrv);
                    float blendFactor[4] = { 0, 0, 0, 0 };
                    ctx->OMSetBlendState(GetBlendState(rsBMode, rsBOp), blendFactor, 0xFFFFFFFF);

                    float fadeInEnd = (float)renderSprite->FadeInEndInteger / 1000.0f;
                    float fadeOutBegin = (float)renderSprite->FadeOutBeginInteger / 1000.0f;
                    float alphaFadeMin = renderSprite->AlphaFadeMinimum;
                    float sizeFadeMin = renderSprite->SizeFadeMinimum;
                    float maxSysScale = (std::max)({ sys.Scale.X, sys.Scale.Y, sys.Scale.Z });
                    if (maxSysScale <= 0.0001f) maxSysScale = 1.0f;

                    // Collect active particles and sort back-to-front
                    struct ParticleRenderSortItem {
                        const SimulatedParticle* P;
                        XMFLOAT3 RenderPos;
                        float ViewZ;
                    };
                    std::vector<ParticleRenderSortItem> sortedParticles;
                    sortedParticles.reserve(state.Particles.size());

                    for (const auto& p : state.Particles) {
                        if (!p.Active) continue;

                        // Disassembly: if Lifetimer <= 0 and interp * 255.0 < CreationTime, particle isn't born yet!
                        if (p.Lifetimer <= 0 && (interp * 255.0f < (float)p.CreationTime)) continue;

                        // Subframe position interpolation (Functions.txt lines 3621-3641)
                        XMFLOAT3 pos;
                        pos.x = p.PositionBuffer[1].x + (p.PositionBuffer[0].x - p.PositionBuffer[1].x) * interp;
                        pos.y = p.PositionBuffer[1].y + (p.PositionBuffer[0].y - p.PositionBuffer[1].y) * interp;
                        pos.z = p.PositionBuffer[1].z + (p.PositionBuffer[0].z - p.PositionBuffer[1].z) * interp;

                        XMVECTOR posV = XMVector3TransformCoord(XMVectorSet(pos.x, pos.y, pos.z, 1.0f), viewMat);
                        float vz = XMVectorGetZ(posV);
                        if (vz <= 0.1f) continue; // Behind near plane

                        sortedParticles.push_back({ &p, pos, vz });
                    }

                    // Sort back-to-front (furthest first)
                    std::sort(sortedParticles.begin(), sortedParticles.end(), [](const ParticleRenderSortItem& a, const ParticleRenderSortItem& b) {
                        return a.ViewZ > b.ViewZ;
                    });

                    float aspect = (rsTexRes.FrameWidth > 0 && rsTexRes.FrameHeight > 0) ? ((float)rsTexRes.FrameHeight / (float)rsTexRes.FrameWidth) : 1.0f;

                    for (const auto& item : sortedParticles) {
                        const auto& p = *item.P;

                        // Life timer in ticks with sub-frame fractional phase
                        float life_timer = (float)p.Lifetimer - ((float)p.CreationTime / 255.0f) + interp;
                        if (life_timer < 0.0f) life_timer = 0.0f;
                        float maxLifeTicks = (p.MaxLifeTicks > 0) ? (float)p.MaxLifeTicks : (p.MaxLife * 30.0f);
                        if (maxLifeTicks < 1.0f) maxLifeTicks = 1.0f;
                        float t = std::clamp(life_timer / maxLifeTicks, 0.0f, 1.0f);

                        // 511-Step 3-Point Color Spline (Functions.txt lines 3435-3548)
                        XMFLOAT4 cStart = XMFLOAT4(renderSprite->StartColour.R / 255.0f, renderSprite->StartColour.G / 255.0f, renderSprite->StartColour.B / 255.0f, renderSprite->StartColour.A / 255.0f);
                        XMFLOAT4 cMid   = XMFLOAT4(renderSprite->MidColour.R   / 255.0f, renderSprite->MidColour.G   / 255.0f, renderSprite->MidColour.B   / 255.0f, renderSprite->MidColour.A   / 255.0f);
                        XMFLOAT4 cEnd   = XMFLOAT4(renderSprite->EndColour.R   / 255.0f, renderSprite->EndColour.G   / 255.0f, renderSprite->EndColour.B   / 255.0f, renderSprite->EndColour.A   / 255.0f);

                        XMFLOAT4 col = EvaluateColorSpline511(cStart, cMid, cEnd, renderSprite->UseStartColour, renderSprite->UseMidColour, renderSprite->UseEndColour, t);

                        // Cosine S-curve Alpha Fade
                        if (renderSprite->AlphaFadeEnable) {
                            col.w *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, alphaFadeMin);
                        }

                        // Distance Fading
                        if (emitter->MaxDrawDistance > 0.0f && item.ViewZ > emitter->MaxDrawDistance) continue;
                        if (emitter->FadeOutStart > 0.0f && item.ViewZ > emitter->FadeOutStart && emitter->MaxDrawDistance > emitter->FadeOutStart) {
                            float df = 1.0f - (item.ViewZ - emitter->FadeOutStart) / (emitter->MaxDrawDistance - emitter->FadeOutStart);
                            col.w *= std::clamp(df, 0.0f, 1.0f);
                        }

                        // Alpha Test Discard
                        if (col.w <= 0.0001f) continue;

                        // Premultiplied alpha on CPU for Additive (3) and AddSmooth (4) matching Fable functions6.txt:517
                        if (rsBMode == 3 || rsBMode == 4) {
                            col.x *= col.w;
                            col.y *= col.w;
                            col.z *= col.w;
                        }

                        // Size Interpolation
                        float pSize = renderSprite->StartRenderSize;
                        if (renderSprite->SizeFadeEnable) {
                            pSize = renderSprite->StartRenderSize + t * (renderSprite->EndRenderSize - renderSprite->StartRenderSize);
                            if (pSize < sizeFadeMin) pSize = sizeFadeMin;
                        }
                        if (pSize <= 0.001f) pSize = 0.05f;
                        pSize *= maxSysScale;

                        float halfSizeX = pSize * 0.5f;
                        float halfSizeY = halfSizeX * aspect;

                        // Interpolated 2D Angle: p.Angle + (AngleChange * (2.0 / 127.0)) * interp
                        float renderAngle = p.Angle + ((float)p.AngleChange * (2.0f / 127.0f)) * interp;

                        // Animation frame index calculation
                        float animProgress = 0.0f;
                        if (renderSprite->ForceAnimationTime && renderSprite->AnimationTimeSecs > 0.0001f) {
                            animProgress = std::fmod((life_timer / 30.0f) / renderSprite->AnimationTimeSecs, 1.0f);
                            if (animProgress < 0.0f) animProgress += 1.0f;
                        }
                        else {
                            animProgress = t;
                        }

                        int frameCount = (int)rsTexRes.FrameCount;
                        if (frameCount < 1) frameCount = 1;
                        int frameIdx = (int)(animProgress * (float)frameCount);
                        if (frameIdx >= frameCount) frameIdx = frameCount - 1;
                        if (frameIdx < 0) frameIdx = 0;

                        XMFLOAT4 uvBounds;
                        float slice = 0.0f;
                        GetFrameUVAndSlice(rsTexRes, frameIdx, uvBounds, slice);

                        EmitQuad(item.RenderPos, halfSizeX, halfSizeY, renderAngle, col, rsCrossed, uvBounds, slice);
                    }

                    FlushQuads();
                }

                // 2. Process Persistent Single Sprite (CPSCSingleSprite)
                // In Fable disassembly (functions10.txt:1262-1265 & functions7.txt:1437, 1851):
                // ONLY emit a sprite primitive if SpriteBankIndex != 0 AND entry exists in bank!
                // Never fall back to DefaultGlowSRV for SingleSprite.
                if (singleSprite && state.HasSingleSprite && state.SingleSpriteParticle.Active && singleSprite->SpriteBankIndex > 0) {
                    ParticleTextureResource ssTexRes = GetOrCreateParticleTexture(device, singleSprite->SpriteBankIndex);
                    if (ssTexRes.SRV) {
                        uint32_t ssBMode = singleSprite->BlendMode;
                        uint32_t ssBOp = singleSprite->BlendOp;
                        bool ssCrossed = (singleSprite->CrossedSprites != 0);

                        cb.IsDXT1 = ssTexRes.IsDXT1 ? 1 : 0;
                        cb.BlendMode = (int32_t)ssBMode;
                        ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);

                        ctx->PSSetShaderResources(0, 1, &ssTexRes.SRV);
                        float blendFactor[4] = { 0, 0, 0, 0 };
                        ctx->OMSetBlendState(GetBlendState(ssBMode, ssBOp), blendFactor, 0xFFFFFFFF);

                        const auto& sp = state.SingleSpriteParticle;
                        float progress = (singleSprite->ForceAnimationTime && singleSprite->AnimationTimeSecs > 0.0001f)
                            ? std::fmod(state.SystemTime / singleSprite->AnimationTimeSecs, 1.0f)
                            : state.SingleSpriteProgress;
                        if (progress < 0.0f) progress += 1.0f;

                        XMFLOAT4 cStart = XMFLOAT4(singleSprite->StartColour.R / 255.0f, singleSprite->StartColour.G / 255.0f, singleSprite->StartColour.B / 255.0f, singleSprite->StartColour.A / 255.0f);
                        XMFLOAT4 cMid   = XMFLOAT4(singleSprite->MidColour.R   / 255.0f, singleSprite->MidColour.G   / 255.0f, singleSprite->MidColour.B   / 255.0f, singleSprite->MidColour.A   / 255.0f);
                        XMFLOAT4 cEnd   = XMFLOAT4(singleSprite->EndColour.R   / 255.0f, singleSprite->EndColour.G   / 255.0f, singleSprite->EndColour.B   / 255.0f, singleSprite->EndColour.A   / 255.0f);

                        XMFLOAT4 col = EvaluateColorSpline511(cStart, cMid, cEnd, singleSprite->UseStartColour, singleSprite->UseMidColour, singleSprite->UseEndColour, progress);

                        if (singleSprite->AlphaFadeEnable) {
                            float fadeInEnd = (float)singleSprite->FadeInEndInteger / 1000.0f;
                            float fadeOutBegin = (float)singleSprite->FadeOutBeginInteger / 1000.0f;
                            col.w *= EvaluateCosineFade(progress, fadeInEnd, fadeOutBegin, singleSprite->AlphaFadeMinimum);
                        }

                        if (col.w > 0.0001f) {
                            // Premultiplied alpha on CPU for Additive (3) and AddSmooth (4) matching Fable functions6.txt:1795-1803
                            if (ssBMode == 3 || ssBMode == 4) {
                                col.x *= col.w;
                                col.y *= col.w;
                                col.z *= col.w;
                            }

                            float sSize = singleSprite->StartRenderSize;
                            if (singleSprite->SizeFadeEnable) {
                                sSize = singleSprite->StartRenderSize + progress * (singleSprite->EndRenderSize - singleSprite->StartRenderSize);
                                float fadeInEnd = (float)singleSprite->FadeInEndInteger / 1000.0f;
                                float fadeOutBegin = (float)singleSprite->FadeOutBeginInteger / 1000.0f;
                                sSize *= EvaluateCosineFade(progress, fadeInEnd, fadeOutBegin, singleSprite->SizeFadeMinimum);
                            }
                            if (sSize <= 0.0001f) sSize = 0.05f;

                            float maxSysScale = (std::max)({ sys.Scale.X, sys.Scale.Y, sys.Scale.Z });
                            if (maxSysScale <= 0.0001f) maxSysScale = 1.0f;
                            sSize *= maxSysScale;

                            float aspect = (ssTexRes.FrameWidth > 0 && ssTexRes.FrameHeight > 0) ? ((float)ssTexRes.FrameHeight / (float)ssTexRes.FrameWidth) : 1.0f;
                            float halfSizeX = sSize * 0.5f;
                            float halfSizeY = halfSizeX * aspect;

                            float animProgress = (singleSprite->AnimationTimeSecs > 0.0001f)
                                ? std::fmod(state.SystemTime / singleSprite->AnimationTimeSecs, 1.0f)
                                : progress;
                            if (animProgress < 0.0f) animProgress += 1.0f;

                            int frameCount = (int)ssTexRes.FrameCount;
                            if (frameCount < 1) frameCount = 1;
                            int frameIdx = (int)(animProgress * (float)frameCount);
                            if (frameIdx >= frameCount) frameIdx = frameCount - 1;
                            if (frameIdx < 0) frameIdx = 0;

                            XMFLOAT4 uvBounds;
                            float slice = 0.0f;
                            GetFrameUVAndSlice(ssTexRes, frameIdx, uvBounds, slice);

                            XMFLOAT3 spPos;
                            spPos.x = sp.PositionBuffer[1].x + (sp.PositionBuffer[0].x - sp.PositionBuffer[1].x) * interp;
                            spPos.y = sp.PositionBuffer[1].y + (sp.PositionBuffer[0].y - sp.PositionBuffer[1].y) * interp;
                            spPos.z = sp.PositionBuffer[1].z + (sp.PositionBuffer[0].z - sp.PositionBuffer[1].z) * interp;

                            EmitQuad(spPos, halfSizeX, halfSizeY, sp.Angle, col, ssCrossed, uvBounds, slice);
                            FlushQuads();
                        }
                    }
                }

                // 3. Process mesh particles (CPSCRenderMesh)
                if (renderMesh && renderMesh->BankIndex > 0) {
                    ParticleMeshGPU* gpuMesh = GetOrCreateParticleMesh(device, renderMesh->BankIndex);

                    if (gpuMesh && gpuMesh->IsValid && VS_ParticleMesh && PS_ParticleMesh) {
                        // Set up 3D Particle Mesh pipeline state
                        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                        ctx->IASetInputLayout(Layout_ParticleMesh);
                        ctx->VSSetShader(VS_ParticleMesh, nullptr, 0);
                        ctx->VSSetConstantBuffers(0, 1, &ConstantBufferParticleMesh);
                        ctx->PSSetShader(PS_ParticleMesh, nullptr, 0);
                        ctx->PSSetConstantBuffers(0, 1, &ConstantBufferParticleMesh);
                        ctx->PSSetSamplers(0, 1, &MeshSampler);
                        ctx->RSSetState(RastState);
                        ctx->OMSetDepthStencilState(DepthStateParticle, 0);
                        float blendFactor[4] = { 0, 0, 0, 0 };
                        ID3D11BlendState* activeMeshBlend = GetBlendState(renderMesh->BlendMode, renderMesh->BlendOp);
                        ctx->OMSetBlendState(activeMeshBlend, blendFactor, 0xFFFFFFFF);

                        UINT mStride = sizeof(GPUVertex);
                        UINT mOffset = 0;
                        ctx->IASetVertexBuffers(0, 1, &gpuMesh->VertexBuffer, &mStride, &mOffset);
                        ctx->IASetIndexBuffer(gpuMesh->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

                        float fadeInEnd = (float)renderMesh->FadeInEndInteger / 1000.0f;
                        float fadeOutBegin = (float)renderMesh->FadeOutBeginInteger / 1000.0f;
                        float alphaFadeMin = renderMesh->AlphaFadeMinimum;
                        float sizeFadeMin = renderMesh->SizeFadeMinimum;

                        C3DVector startSize = renderMesh->StartRenderSize;
                        C3DVector endSize = renderMesh->EndRenderSize;
                        if (startSize.X <= 0.0001f && startSize.Y <= 0.0001f && startSize.Z <= 0.0001f) {
                            startSize = { 1.0f, 1.0f, 1.0f };
                        }
                        if (endSize.X <= 0.0001f && endSize.Y <= 0.0001f && endSize.Z <= 0.0001f) {
                            endSize = startSize;
                        }

                        C3DVector sysScale = sys.Scale;
                        if (sysScale.X <= 0.0001f) sysScale.X = 1.0f;
                        if (sysScale.Y <= 0.0001f) sysScale.Y = 1.0f;
                        if (sysScale.Z <= 0.0001f) sysScale.Z = 1.0f;

                        XMMATRIX viewProj = viewMat * proj;

                        for (const auto& p : state.Particles) {
                            if (!p.Active) continue;
                            if (p.Lifetimer <= 0 && interp * 255.0f < (float)p.CreationTime) continue;
                            if (p.MaxLifeTicks <= 0 || p.Lifetimer >= p.MaxLifeTicks) continue;

                            float t = std::clamp((float)p.Lifetimer / (float)p.MaxLifeTicks, 0.0f, 1.0f);

                            // 511-Step 3-Point Color Spline (Functions.txt lines 3435-3548)
                            XMFLOAT4 cStart = XMFLOAT4(renderMesh->StartColour.R / 255.0f, renderMesh->StartColour.G / 255.0f, renderMesh->StartColour.B / 255.0f, renderMesh->StartColour.A / 255.0f);
                            XMFLOAT4 cMid   = XMFLOAT4(renderMesh->MidColour.R   / 255.0f, renderMesh->MidColour.G   / 255.0f, renderMesh->MidColour.B   / 255.0f, renderMesh->MidColour.A   / 255.0f);
                            XMFLOAT4 cEnd   = XMFLOAT4(renderMesh->EndColour.R   / 255.0f, renderMesh->EndColour.G   / 255.0f, renderMesh->EndColour.B   / 255.0f, renderMesh->EndColour.A   / 255.0f);

                            XMFLOAT4 col = EvaluateColorSpline511(cStart, cMid, cEnd, renderMesh->UseStartColour, renderMesh->UseMidColour, renderMesh->UseEndColour, t);

                            float cr = col.x;
                            float cg = col.y;
                            float cb = col.z;
                            float ca = col.w;

                            // Cosine S-curve Alpha and Size Fades
                            if (renderMesh->AlphaFadeEnable) {
                                ca *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, alphaFadeMin);
                            }

                            if (ca <= 0.0001f) continue;

                            // Size Interpolation with SizeFade
                            C3DVector curSize;
                            curSize.X = startSize.X + (endSize.X - startSize.X) * t;
                            curSize.Y = startSize.Y + (endSize.Y - startSize.Y) * t;
                            curSize.Z = startSize.Z + (endSize.Z - startSize.Z) * t;

                            if (renderMesh->SizeFadeEnable) {
                                float fadeSize = EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, sizeFadeMin);
                                curSize.X *= fadeSize;
                                curSize.Y *= fadeSize;
                                curSize.Z *= fadeSize;
                            }

                            // In Fable: finalScale = (curSize / BoundingSphere.Radius) * system->Scale (Functions.txt:5744-5781)
                            float meshRadius = (gpuMesh->BoundingRadius > 0.0001f) ? gpuMesh->BoundingRadius : 1.0f;
                            float scaleX = (curSize.X / meshRadius) * sysScale.X;
                            float scaleY = (curSize.Y / meshRadius) * sysScale.Y;
                            float scaleZ = (curSize.Z / meshRadius) * sysScale.Z;

                            // Orientation Slerp (Identity to OrientationChange by interp, then multiply with Orientation)
                            XMVECTOR qOrient = XMLoadFloat4(&p.Orientation);
                            if (XMVectorGetX(XMVector4LengthSq(qOrient)) < 0.0001f) qOrient = XMQuaternionIdentity();
                            else qOrient = XMQuaternionNormalize(qOrient);

                            XMVECTOR qDelta = XMLoadFloat4(&p.OrientationChange);
                            if (XMVectorGetX(XMVector4LengthSq(qDelta)) < 0.0001f) qDelta = XMQuaternionIdentity();

                            XMVECTOR qStep = XMQuaternionSlerp(XMQuaternionIdentity(), qDelta, interp);
                            XMVECTOR qFinal = XMQuaternionNormalize(XMQuaternionMultiply(qOrient, qStep));

                            XMMATRIX rotMat = XMMatrixRotationQuaternion(qFinal);
                            XMVECTOR vRight   = rotMat.r[0];
                            XMVECTOR vForward = rotMat.r[1];
                            XMVECTOR vUp      = rotMat.r[2];

                            // Negate coordinate axes for engine basis convention: E1 = -Right, E2 = -Forward, E3 = Up
                            XMVECTOR E1 = XMVectorScale(vRight, -scaleX);
                            XMVECTOR E2 = XMVectorScale(vForward, -scaleY);
                            XMVECTOR E3 = XMVectorScale(vUp, scaleZ);

                            // Subframe position interpolation
                            XMFLOAT3 interpPos;
                            interpPos.x = p.PositionBuffer[1].x + (p.PositionBuffer[0].x - p.PositionBuffer[1].x) * interp;
                            interpPos.y = p.PositionBuffer[1].y + (p.PositionBuffer[0].y - p.PositionBuffer[1].y) * interp;
                            interpPos.z = p.PositionBuffer[1].z + (p.PositionBuffer[0].z - p.PositionBuffer[1].z) * interp;

                            // Centering: If CentredOnPos, offset by rotated bounding center: centre = E1*C.x + E2*C.y + E3*C.z
                            XMVECTOR vInterpPos = XMLoadFloat3(&interpPos);
                            XMVECTOR centre = XMVectorZero();
                            if (renderMesh->CentredOnPos) {
                                centre = XMVectorAdd(
                                    XMVectorScale(E1, gpuMesh->BoundingCenter.x),
                                    XMVectorAdd(
                                        XMVectorScale(E2, gpuMesh->BoundingCenter.y),
                                        XMVectorScale(E3, gpuMesh->BoundingCenter.z)
                                    )
                                );
                            }
                            XMVECTOR renderPos = XMVectorSubtract(vInterpPos, centre);

                            // Build final instance World Matrix
                            XMMATRIX worldMat;
                            worldMat.r[0] = XMVectorSetW(E1, 0.0f);
                            worldMat.r[1] = XMVectorSetW(E2, 0.0f);
                            worldMat.r[2] = XMVectorSetW(E3, 0.0f);
                            worldMat.r[3] = XMVectorSetW(renderPos, 1.0f);

                            XMMATRIX meshWVP = worldMat * viewProj;

                            for (const auto& batch : gpuMesh->Batches) {
                                ID3D11ShaderResourceView* texSRV = nullptr;
                                if (batch.DiffuseMapID > 0) {
                                    texSRV = LoadTextureForMesh(batch.DiffuseMapID);
                                }
                                if (!texSRV) {
                                    texSRV = DefaultWhiteSRV;
                                }

                                ParticleMeshCB meshCB = {};
                                meshCB.WorldViewProj = XMMatrixTranspose(meshWVP);
                                meshCB.MeshColor = XMFLOAT4(cr, cg, cb, ca);
                                meshCB.HasTexture = (texSRV != nullptr) ? 1 : 0;
                                meshCB.BlendMode = (int32_t)renderMesh->BlendMode;
                                meshCB.IsDXT1 = 1; // Enable black background cutout for mesh textures
                                ctx->UpdateSubresource(ConstantBufferParticleMesh, 0, nullptr, &meshCB, 0, 0);

                                ctx->PSSetShaderResources(0, 1, &texSRV);
                                ctx->DrawIndexed(batch.IndexCount, batch.IndexStart, 0);
                            }
                        }

                        // Restore billboard state
                        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                        ctx->IASetInputLayout(Layout_Particle);
                        ctx->VSSetShader(VS_Particle, nullptr, 0);
                        ctx->VSSetConstantBuffers(0, 1, &ConstantBuffer);
                        ctx->PSSetShader(PS_Particle, nullptr, 0);
                        ctx->PSSetConstantBuffers(0, 1, &ConstantBuffer);
                        ctx->PSSetSamplers(0, 1, &ParticleSampler);
                        UINT pStride = sizeof(ParticleQuadVertex);
                        UINT pOffset = 0;
                        ctx->IASetVertexBuffers(0, 1, &ParticleVBuffer, &pStride, &pOffset);
                        ctx->IASetIndexBuffer(ParticleIBuffer, DXGI_FORMAT_R32_UINT, 0);
                    }
                    else {
                        // Fallback billboard rendering if mesh not found in open banks
                        float fadeInEnd = (float)renderMesh->FadeInEndInteger / 1000.0f;
                        float fadeOutBegin = (float)renderMesh->FadeOutBeginInteger / 1000.0f;
                        float alphaFadeMin = renderMesh->AlphaFadeMinimum;
                        float sizeFadeMin = renderMesh->SizeFadeMinimum;
                        float sysScale = (sys.Scale.X + sys.Scale.Y + sys.Scale.Z) / 3.0f;
                        if (sysScale <= 0.0001f) sysScale = 1.0f;

                        float startSize = (renderMesh->StartRenderSize.X + renderMesh->StartRenderSize.Y + renderMesh->StartRenderSize.Z) / 3.0f;
                        float endSize = (renderMesh->EndRenderSize.X + renderMesh->EndRenderSize.Y + renderMesh->EndRenderSize.Z) / 3.0f;
                        if (startSize <= 0.0001f) startSize = 1.0f;
                        if (endSize <= 0.0001f) endSize = startSize;

                        float blendFactor[4] = { 0, 0, 0, 0 };
                        ctx->OMSetBlendState(GetBlendState(renderMesh->BlendMode, renderMesh->BlendOp), blendFactor, 0xFFFFFFFF);

                        for (const auto& p : state.Particles) {
                            if (!p.Active) continue;
                            if (p.Lifetimer <= 0 && interp * 255.0f < (float)p.CreationTime) continue;
                            if (p.MaxLifeTicks <= 0 || p.Lifetimer >= p.MaxLifeTicks) continue;

                            float t = std::clamp((float)p.Lifetimer / (float)p.MaxLifeTicks, 0.0f, 1.0f);

                            // 511-Step 3-Point Color Spline (Functions.txt lines 3435-3548)
                            XMFLOAT4 cStart = XMFLOAT4(renderMesh->StartColour.R / 255.0f, renderMesh->StartColour.G / 255.0f, renderMesh->StartColour.B / 255.0f, renderMesh->StartColour.A / 255.0f);
                            XMFLOAT4 cMid   = XMFLOAT4(renderMesh->MidColour.R   / 255.0f, renderMesh->MidColour.G   / 255.0f, renderMesh->MidColour.B   / 255.0f, renderMesh->MidColour.A   / 255.0f);
                            XMFLOAT4 cEnd   = XMFLOAT4(renderMesh->EndColour.R   / 255.0f, renderMesh->EndColour.G   / 255.0f, renderMesh->EndColour.B   / 255.0f, renderMesh->EndColour.A   / 255.0f);

                            XMFLOAT4 col = EvaluateColorSpline511(cStart, cMid, cEnd, renderMesh->UseStartColour, renderMesh->UseMidColour, renderMesh->UseEndColour, t);

                            float cr = col.x;
                            float cg = col.y;
                            float cb = col.z;
                            float ca = col.w;

                            if (renderMesh->AlphaFadeEnable) {
                                ca *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, alphaFadeMin);
                            }

                            if (ca <= 0.0001f) continue;

                            if (renderMesh->BlendMode == 3 || renderMesh->BlendMode == 4) {
                                cr *= ca;
                                cg *= ca;
                                cb *= ca;
                            }

                            float pSize = startSize + (endSize - startSize) * t;
                            if (renderMesh->SizeFadeEnable) {
                                pSize *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, sizeFadeMin);
                            }
                            pSize *= sysScale;
                            if (pSize < 0.05f) pSize = 0.05f;

                            XMFLOAT3 spPos;
                            spPos.x = p.PositionBuffer[1].x + (p.PositionBuffer[0].x - p.PositionBuffer[1].x) * interp;
                            spPos.y = p.PositionBuffer[1].y + (p.PositionBuffer[0].y - p.PositionBuffer[1].y) * interp;
                            spPos.z = p.PositionBuffer[1].z + (p.PositionBuffer[0].z - p.PositionBuffer[1].z) * interp;

                            float interpAngle = p.Angle + p.AngleChange * interp;
                            XMFLOAT4 uvBounds(0.0f, 0.0f, 1.0f, 1.0f);
                            EmitQuad(spPos, pSize * 0.5f, pSize * 0.5f, interpAngle, XMFLOAT4(cr, cg, cb, ca), false, uvBounds, 0.0f);
                        }
                        FlushQuads();
                    }
                }
            }
        }

        ID3D11RenderTargetView* nullRTV = nullptr;
        ctx->OMSetRenderTargets(1, &nullRTV, nullptr);

        return SRV;
    }

    void Release() {
        ClearParticleTextureCache();
        ClearParticleMeshCache();
        ReleaseResizedResources();
        if (GridVBuffer) { GridVBuffer->Release(); GridVBuffer = nullptr; }
        if (ParticleVBuffer) { ParticleVBuffer->Release(); ParticleVBuffer = nullptr; }
        if (ParticleIBuffer) { ParticleIBuffer->Release(); ParticleIBuffer = nullptr; }
        if (ConstantBuffer) { ConstantBuffer->Release(); ConstantBuffer = nullptr; }
        if (Layout_Line) { Layout_Line->Release(); Layout_Line = nullptr; }
        if (Layout_Particle) { Layout_Particle->Release(); Layout_Particle = nullptr; }
        if (VS_Line) { VS_Line->Release(); VS_Line = nullptr; }
        if (PS_Line) { PS_Line->Release(); PS_Line = nullptr; }
        if (VS_Particle) { VS_Particle->Release(); VS_Particle = nullptr; }
        if (PS_Particle) { PS_Particle->Release(); PS_Particle = nullptr; }
        if (VS_ParticleMesh) { VS_ParticleMesh->Release(); VS_ParticleMesh = nullptr; }
        if (PS_ParticleMesh) { PS_ParticleMesh->Release(); PS_ParticleMesh = nullptr; }
        if (Layout_ParticleMesh) { Layout_ParticleMesh->Release(); Layout_ParticleMesh = nullptr; }
        if (ConstantBufferParticleMesh) { ConstantBufferParticleMesh->Release(); ConstantBufferParticleMesh = nullptr; }
        if (MeshSampler) { MeshSampler->Release(); MeshSampler = nullptr; }
        if (DefaultWhiteSRV) { DefaultWhiteSRV->Release(); DefaultWhiteSRV = nullptr; }
        if (RastState) { RastState->Release(); RastState = nullptr; }
        if (DepthStateGrid) { DepthStateGrid->Release(); DepthStateGrid = nullptr; }
        if (DepthStateParticle) { DepthStateParticle->Release(); DepthStateParticle = nullptr; }
        if (ParticleSampler) { ParticleSampler->Release(); ParticleSampler = nullptr; }
        for (int m = 0; m < 6; m++) {
            for (int op = 0; op < 3; op++) {
                if (BlendStates[m][op]) {
                    BlendStates[m][op]->Release();
                    BlendStates[m][op] = nullptr;
                }
            }
        }
        BlendStateAlpha = nullptr;
        BlendStateAdditive = nullptr;
        BlendStateAddSmooth = nullptr;
        BlendStateGrid = nullptr;
        if (DefaultGlowSRV) { DefaultGlowSRV->Release(); DefaultGlowSRV = nullptr; }
        Initialized = false;
        Device = nullptr;
    }
};
