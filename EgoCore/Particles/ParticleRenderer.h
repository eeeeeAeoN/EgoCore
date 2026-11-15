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

static constexpr int kMaxParticleLights = 8;

struct ParticlePointLightGPU {
    XMFLOAT4 PosAndRadius;  // xyz = WorldPos, w = OuterRadius
    XMFLOAT4 ColorAndAtten; // rgb = Color * Intensity, w = AttenuationFactor
};

struct ParticleCBMatrix {
    XMMATRIX WorldViewProj;
    int32_t IsDXT1;
    int32_t BlendMode;
    int32_t HasBlackBg;
    int32_t SelfIlluminating;
    int32_t NumLights;
    float Padding[3];
    ParticlePointLightGPU Lights[kMaxParticleLights];
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
    XMMATRIX World;
    XMFLOAT4 MeshColor;
    int32_t HasTexture;
    int32_t BlendMode;
    int32_t IsDXT1;
    int32_t NumLights;
    ParticlePointLightGPU Lights[kMaxParticleLights];
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
    bool HasBlackBg = false;
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
    uint32_t GridLineVertexCount = 0;
    uint32_t AxesVertexCount = 0;

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
    ID3D11DepthStencilState* DepthStates[2][2] = {}; // [ReadZBuffer 0..1][ZBufferWriteable 0..1]
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

    void BindBlendState(ID3D11DeviceContext* ctx, uint32_t mode, uint32_t op) {
        float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        if (mode == 5) { // CONST_COLOUR
            blendFactor[0] = blendFactor[1] = blendFactor[2] = blendFactor[3] = 0.5f;
        }
        ctx->OMSetBlendState(GetBlendState(mode, op), blendFactor, 0xFFFFFFFF);
    }

    void BindDepthStateForBlendMode(ID3D11DeviceContext* ctx, int readZ, int emitterWriteZ, uint32_t mode) {
        // Translucent blend modes (ALPHA, ADDITIVE, ADDSMOOTH, CONST_COLOUR) never write depth
        int writeZ = (mode <= 1) ? (emitterWriteZ ? 1 : 0) : 0;
        ID3D11DepthStencilState* ds = DepthStates[readZ ? 1 : 0][writeZ];
        ctx->OMSetDepthStencilState(ds ? ds : DepthStateParticle, 0);
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
                // Straight alpha radial glow (RGB = 255, A = smooth falloff) so both Alpha and Additive shader paths work cleanly
                pixels[y * texSize + x] = ((uint32_t)a << 24) | 0x00FFFFFFu;
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

        GridLineVertexCount = (uint32_t)verts.size();

        // Coordinate axes: X = Red, Y = Green, Z = Blue (Up)
        verts.push_back({ XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f) });
        verts.push_back({ XMFLOAT3(2.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f) });

        verts.push_back({ XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT4(0.2f, 1.0f, 0.2f, 1.0f) });
        verts.push_back({ XMFLOAT3(0.0f, 2.0f, 0.0f), XMFLOAT4(0.2f, 1.0f, 0.2f, 1.0f) });

        verts.push_back({ XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT4(0.2f, 0.4f, 1.0f, 1.0f) });
        verts.push_back({ XMFLOAT3(0.0f, 0.0f, 2.0f), XMFLOAT4(0.2f, 0.4f, 1.0f, 1.0f) });

        GridVertexCount = (uint32_t)verts.size();
        AxesVertexCount = GridVertexCount - GridLineVertexCount;

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

    bool ShowGrid = false;
    bool ShowAxes = false;

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
            struct PointLightData {
                float4 PosAndRadius;
                float4 ColorAndAtten;
            };

            cbuffer CBMatrix : register(b0) {
                matrix WorldViewProj;
                int IsDXT1;
                int BlendMode;
                int HasBlackBg;
                int SelfIlluminating;
                int NumLights;
                float3 Padding;
                PointLightData Lights[8];
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
                float3 WorldPos : TEXCOORD1;
            };

            ParticlePS_IN VS(ParticleVS_IN input) {
                ParticlePS_IN output;
                output.Pos = mul(float4(input.Pos, 1.0f), WorldViewProj);
                output.UV  = input.UV;
                output.Col = input.Col;
                output.WorldPos = input.Pos;
                return output;
            }

            Texture2DArray ParticleTex : register(t0);
            SamplerState Sampler : register(s0);

            float3 EvaluatePointLights(float3 worldPos) {
                float3 lightAccum = float3(0.0f, 0.0f, 0.0f);
                [unroll]
                for (int i = 0; i < 8; ++i) {
                    if (i < NumLights) {
                        float3 lightPos = Lights[i].PosAndRadius.xyz;
                        float outerRadius = max(Lights[i].PosAndRadius.w, 0.001f);
                        float3 lightColor = Lights[i].ColorAndAtten.rgb;
                        float attenFactor = max(Lights[i].ColorAndAtten.w, 1.0f);

                        float dist = length(lightPos - worldPos);
                        if (dist < outerRadius) {
                            float u = saturate(1.0f - dist / outerRadius);
                            float falloff = pow(u, attenFactor);
                            lightAccum += lightColor * falloff;
                        }
                    }
                }
                return lightAccum;
            }

            // Disassembly 97_PSHADER_SPRITE_GROUP.txt:
            float4 PS(ParticlePS_IN input) : SV_Target {
                float4 texCol = ParticleTex.Sample(Sampler, input.UV);
                // Smooth luminance-based keying for opaque black-background sprite/trail textures
                if (HasBlackBg != 0 && BlendMode <= 2) {
                    float luma = max(dot(texCol.rgb, float3(0.299f, 0.587f, 0.114f)),
                                     max(texCol.r, max(texCol.g, texCol.b)) * 0.75f);
                    float keyAlpha = smoothstep(0.01f, 0.35f, luma);
                    texCol.a *= keyAlpha;
                    if (keyAlpha > 0.001f) {
                        texCol.rgb = saturate(texCol.rgb / max(keyAlpha, 0.18f));
                    }
                }
                float4 finalCol;
                finalCol.a = texCol.a * input.Col.a;
                finalCol.rgb = saturate(2.0f * texCol.rgb * input.Col.rgb);

                if (NumLights > 0 && SelfIlluminating == 0) {
                    float3 lightAccum = EvaluatePointLights(input.WorldPos);
                    finalCol.rgb = saturate(finalCol.rgb * (float3(0.35f, 0.35f, 0.35f) + lightAccum));
                }

                if (BlendMode == 3 || BlendMode == 4) {
                    finalCol.rgb = saturate(finalCol.rgb * finalCol.a);
                } else if (BlendMode <= 1) {
                    // Fable Alpha Testing: AlphaRef = 16 (0x10) for SOLID (0) and BOOLEAN_ALPHA (1)
                    clip(finalCol.a - (16.0f / 255.0f));
                } else if (BlendMode == 2) {
                    // Smooth ALPHA (2) particles fade cleanly down to near-zero
                    clip(finalCol.a - 0.001f);
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

        // 4 Emitter Depth States indexed by [ReadZBuffer][ZBufferWriteable]
        for (int readZ = 0; readZ < 2; ++readZ) {
            for (int writeZ = 0; writeZ < 2; ++writeZ) {
                D3D11_DEPTH_STENCIL_DESC dsDesc = {};
                dsDesc.DepthEnable = (readZ != 0) ? TRUE : FALSE;
                dsDesc.DepthWriteMask = (writeZ != 0) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
                dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
                device->CreateDepthStencilState(&dsDesc, &DepthStates[readZ][writeZ]);
            }
        }
        DepthStateParticle = DepthStates[1][0];

        // 6. Blend States (6 BlendModes x 3 BlendOps matching EEnginePrimitiveBlendMode: Solid=0, BoolAlpha=1, Alpha=2, Additive=3, AddSmooth=4, ConstColour=5)
        D3D11_BLEND_OP d3dOps[3] = { D3D11_BLEND_OP_ADD, D3D11_BLEND_OP_SUBTRACT, D3D11_BLEND_OP_REV_SUBTRACT };
        for (int m = 0; m < 6; m++) {
            for (int op = 0; op < 3; op++) {
                D3D11_BLEND_DESC bDesc = {};
                bDesc.RenderTarget[0].BlendEnable = TRUE;
                bDesc.RenderTarget[0].BlendOp = d3dOps[op];
                // Keep offscreen RTV alpha pinned to 1.0 so ImGui::Image compositing stays opaque
                bDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
                bDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
                bDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
                bDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

                if (m == 0) { // SOLID (alpha-tested with SRC_ALPHA / INV_SRC_ALPHA parity)
                    bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
                    bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
                }
                else if (m == 1 || m == 2) { // BOOLEAN_ALPHA (1) / ALPHA (2)
                    bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
                    bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
                }
                else if (m == 3) { // ADDITIVE
                    bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
                    bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
                }
                else if (m == 4) { // ADDSMOOTH
                    bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
                    bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
                }
                else if (m == 5) { // CONST_COLOUR
                    bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
                    bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_BLEND_FACTOR;
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
            struct PointLightData {
                float4 PosAndRadius;
                float4 ColorAndAtten;
            };

            cbuffer CBParticleMesh : register(b0) {
                matrix WorldViewProj;
                matrix World;
                float4 MeshColor;
                int HasTexture;
                int BlendMode;
                int IsDXT1;
                int NumLights;
                PointLightData Lights[8];
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
                float3 WorldPos : TEXCOORD1;
            };

            ParticleMeshPS_IN VS(ParticleMeshVS_IN input) {
                ParticleMeshPS_IN output;
                output.Pos = mul(float4(input.Pos, 1.0f), WorldViewProj);
                output.WorldPos = mul(float4(input.Pos, 1.0f), World).xyz;
                output.Norm = mul(input.Norm, (float3x3)World);
                output.UV = input.UV;
                return output;
            }

            Texture2D MeshTex : register(t0);
            SamplerState Sampler : register(s0);

            float4 PS(ParticleMeshPS_IN input) : SV_Target {
                float4 texCol = float4(1.0f, 1.0f, 1.0f, 1.0f);
                if (HasTexture != 0) {
                    // Keep opaque 3D mesh albedo textures untouched by black-background keying
                    texCol = MeshTex.Sample(Sampler, input.UV);
                }
                float4 finalCol;
                finalCol.a = texCol.a * MeshColor.a;
                finalCol.rgb = saturate(2.0f * texCol.rgb * MeshColor.rgb);

                if (NumLights > 0) {
                    float3 N = input.Norm;
                    float nLenSq = dot(N, N);
                    N = (nLenSq > 1e-6f) ? (N * rsqrt(nLenSq)) : float3(0.0f, 0.0f, 1.0f);

                    float3 lightAccum = float3(0.0f, 0.0f, 0.0f);
                    [unroll]
                    for (int i = 0; i < 8; ++i) {
                        if (i < NumLights) {
                            float3 toLight = Lights[i].PosAndRadius.xyz - input.WorldPos;
                            float dist = length(toLight);
                            float outerRadius = max(Lights[i].PosAndRadius.w, 0.001f);
                            if (dist < outerRadius) {
                                float3 L = (dist > 1e-5f) ? (toLight / dist) : float3(0.0f, 0.0f, 1.0f);
                                float ndotl = saturate(0.5f + 0.5f * dot(N, L));
                                float u = saturate(1.0f - dist / outerRadius);
                                float falloff = pow(u, max(Lights[i].ColorAndAtten.w, 1.0f));
                                lightAccum += Lights[i].ColorAndAtten.rgb * (falloff * (0.35f + 0.65f * ndotl));
                            }
                        }
                    }
                    finalCol.rgb = saturate(finalCol.rgb * (float3(0.35f, 0.35f, 0.35f) + lightAccum));
                }

                if (BlendMode == 3 || BlendMode == 4) {
                    finalCol.rgb = saturate(finalCol.rgb * finalCol.a);
                } else if (BlendMode <= 1) {
                    // Alpha testing for SOLID (0) / BOOLEAN_ALPHA (1) mesh particles (AlphaRef = 16)
                    clip(finalCol.a - (16.0f / 255.0f));
                } else if (BlendMode == 2) {
                    // Smooth ALPHA (2) mesh particles fade cleanly down to near-zero
                    clip(finalCol.a - 0.001f);
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

    static bool DetectBlackBgFromRGBA(const uint8_t* rgba, uint32_t width, uint32_t height) {
        if (!rgba || width < 4 || height < 4) return false;

        const size_t totalPixels = (size_t)width * (size_t)height;
        for (size_t i = 0; i < totalPixels; ++i) {
            if (rgba[i * 4 + 3] < 245) {
                return false;
            }
        }

        auto isDarkPixel = [&](uint32_t x, uint32_t y) -> bool {
            const size_t idx = ((size_t)y * (size_t)width + (size_t)x) * 4;
            uint8_t maxRGB = (std::max)({ rgba[idx + 0], rgba[idx + 1], rgba[idx + 2] });
            return maxRGB <= 32;
        };

        if (!isDarkPixel(0, 0) || !isDarkPixel(width - 1, 0) ||
            !isDarkPixel(0, height - 1) || !isDarkPixel(width - 1, height - 1)) {
            return false;
        }

        uint32_t darkBorderCount = 0;
        uint32_t totalBorderCount = 0;
        for (uint32_t x = 0; x < width; ++x) {
            if (isDarkPixel(x, 0)) ++darkBorderCount;
            if (isDarkPixel(x, height - 1)) ++darkBorderCount;
            totalBorderCount += 2;
        }
        for (uint32_t y = 1; y + 1 < height; ++y) {
            if (isDarkPixel(0, y)) ++darkBorderCount;
            if (isDarkPixel(width - 1, y)) ++darkBorderCount;
            totalBorderCount += 2;
        }

        return (totalBorderCount > 0) && ((float)darkBorderCount / (float)totalBorderCount >= 0.5f);
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

            bool hasBlackBg = false;
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
                if (!rgbaFrames.empty()) {
                    hasBlackBg = DetectBlackBgFromRGBA(rgbaFrames[0].data(), totalW, totalH);
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
            else if (srv && parser.DecodedPixels.size() >= mip0ByteSize && parser.DecodedFormat != ETextureFormat::Unknown) {
                std::vector<uint8_t> frame0RGBA = TextureUtils::DecompressFrameToRGBA(
                    (const uint8_t*)subData[0].pSysMem, totalW, totalH, parser.DecodedFormat);
                hasBlackBg = DetectBlackBgFromRGBA(frame0RGBA.data(), totalW, totalH);
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
                res.HasBlackBg = hasBlackBg;
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
                                            if (stagedTex->RawFrames[0].size() >= (size_t)sW * (size_t)sH * 4) {
                                                res.HasBlackBg = DetectBlackBgFromRGBA(stagedTex->RawFrames[0].data(), sW, sH);
                                            }
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

    struct SpriteQuadParams {
        bool FaceMe3D = true;
        bool RotateAroundCentre = true;
        bool CrossedSprites = false;
        int NoCrossedSprites = 1;
        int SpriteAlignment = 0;
        float FullArc = 0.5f;
    };

    static XMFLOAT4 ColourToFloat4(const CRGBColour& c) {
        return XMFLOAT4(c.R / 255.0f, c.G / 255.0f, c.B / 255.0f, c.A / 255.0f);
    }

    static float ComputeScalarSystemScale(const CParticleSystem& sys) {
        if (!sys.ScaleParticles) return 1.0f;
        float s = (std::max)({ sys.Scale.X, sys.Scale.Y, sys.Scale.Z });
        return (s > 0.0001f) ? s : 1.0f;
    }

    static C3DVector ComputeVectorSystemScale(const CParticleSystem& sys) {
        if (!sys.ScaleParticles) return { 1.0f, 1.0f, 1.0f };
        return {
            (sys.Scale.X > 0.0001f) ? sys.Scale.X : 1.0f,
            (sys.Scale.Y > 0.0001f) ? sys.Scale.Y : 1.0f,
            (sys.Scale.Z > 0.0001f) ? sys.Scale.Z : 1.0f
        };
    }

    static float EvaluateEmitterDistanceFade(const CParticleEmitter* emitter, float dist) {
        if (!emitter) return 1.0f;
        if (emitter->MaxDrawDistance > 0.0f && dist > emitter->MaxDrawDistance) return 0.0f;

        float fade = 1.0f;
        if (emitter->FadeOutStart > 0.0f && emitter->MaxDrawDistance > emitter->FadeOutStart && dist > emitter->FadeOutStart) {
            fade *= std::clamp(1.0f - (dist - emitter->FadeOutStart) / (emitter->MaxDrawDistance - emitter->FadeOutStart), 0.0f, 1.0f);
        }
        if (emitter->FadeInEnd > 0.0f) {
            if (dist <= emitter->FadeInStart) return 0.0f;
            if (dist < emitter->FadeInEnd && emitter->FadeInEnd > emitter->FadeInStart) {
                fade *= std::clamp((dist - emitter->FadeInStart) / (emitter->FadeInEnd - emitter->FadeInStart), 0.0f, 1.0f);
            }
        }
        return fade;
    }

    static float EvaluatePerlinFlicker(float timeSecs, uint8_t flickerTimeOffset, uint8_t flickerIndex,
                                       float speed, float bias, uint32_t minAlphaInt, uint32_t minSizeInt,
                                       float& outSizeMul) {
        float noise = FablePerlinNoise::GetGaussian2DNoise(timeSecs * speed + (float)flickerTimeOffset, (float)flickerIndex);
        float c = std::clamp(noise * (std::fabsf(bias) + 0.5f) + (bias + 0.5f), 0.0f, 1.0f);
        float minAlpha = std::clamp((float)minAlphaInt * 0.001f, 0.0f, 1.0f);
        float minSize = std::clamp((float)minSizeInt * 0.001f, 0.0f, 1.0f);
        outSizeMul = minSize + (1.0f - minSize) * c;
        return minAlpha + (1.0f - minAlpha) * c;
    }

    static XMFLOAT3 SamplePositionBuffer(const SimulatedParticle& p, float idx) {
        float clamped = std::clamp(idx, 0.0f, 3.0f);
        int i0 = (int)std::floor(clamped);
        int i1 = (std::min)(i0 + 1, 3);
        float frac = clamped - (float)i0;
        const XMFLOAT3& a = p.PositionBuffer[i0];
        const XMFLOAT3& b = p.PositionBuffer[i1];
        return XMFLOAT3(
            a.x + (b.x - a.x) * frac,
            a.y + (b.y - a.y) * frac,
            a.z + (b.z - a.z) * frac
        );
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

            if (!io.WantTextInput) {
                float dt = std::clamp(io.DeltaTime, 0.0f, 0.1f);
                float rotSpeed = 2.5f * dt;
                float panSpeed = CamDist * 1.5f * dt;

                if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))  CamRotY -= rotSpeed;
                if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) CamRotY += rotSpeed;
                if (ImGui::IsKeyDown(ImGuiKey_UpArrow))    CamRotX -= rotSpeed;
                if (ImGui::IsKeyDown(ImGuiKey_DownArrow))  CamRotX += rotSpeed;

                if (ImGui::IsKeyDown(ImGuiKey_W)) CamPan.y -= panSpeed;
                if (ImGui::IsKeyDown(ImGuiKey_S)) CamPan.y += panSpeed;
                if (ImGui::IsKeyDown(ImGuiKey_A)) CamPan.x += panSpeed;
                if (ImGui::IsKeyDown(ImGuiKey_D)) CamPan.x -= panSpeed;
            }

            CamRotX = std::clamp(CamRotX, -1.5f, 1.5f);
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

        // Extract camera Right, Up, Forward, and Position vectors in world space for billboards
        XMMATRIX invView = XMMatrixInverse(nullptr, viewMat);
        XMVECTOR camRight = XMVector3Normalize(invView.r[0]);
        XMVECTOR camUp    = XMVector3Normalize(invView.r[1]);
        XMVECTOR camFwd   = XMVector3Normalize(invView.r[2]);
        XMVECTOR camPos   = invView.r[3];

        // ----------------------------------------------------
        // PASS 1: Render Ground Grid and Axes
        // ----------------------------------------------------
        if (ShowGrid || ShowAxes) {
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

            if (ShowGrid && GridLineVertexCount > 0) {
                ctx->Draw(GridLineVertexCount, 0);
            }
            if (ShowAxes && AxesVertexCount > 0) {
                ctx->Draw(AxesVertexCount, GridLineVertexCount);
            }
        }

        // ----------------------------------------------------
        // PASS 2: Render Simulated Particles
        // ----------------------------------------------------
        if (emitter && simulator && VS_Particle && PS_Particle && ParticleVBuffer && ParticleIBuffer) {
            auto BindBillboardPipeline = [&]() {
                ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                ctx->IASetInputLayout(Layout_Particle);
                ctx->VSSetShader(VS_Particle, nullptr, 0);
                ctx->VSSetConstantBuffers(0, 1, &ConstantBuffer);
                ctx->PSSetShader(PS_Particle, nullptr, 0);
                ctx->PSSetConstantBuffers(0, 1, &ConstantBuffer);
                ctx->PSSetSamplers(0, 1, &ParticleSampler);
                ctx->RSSetState(RastState);
                UINT pStride = sizeof(ParticleQuadVertex);
                UINT pOffset = 0;
                ctx->IASetVertexBuffers(0, 1, &ParticleVBuffer, &pStride, &pOffset);
                ctx->IASetIndexBuffer(ParticleIBuffer, DXGI_FORMAT_R32_UINT, 0);
            };

            BindBillboardPipeline();

            // Bind depth state matching emitter->ReadZBuffer / emitter->ZBufferWriteable
            int readZ = emitter->ReadZBuffer ? 1 : 0;
            int writeZ = emitter->ZBufferWriteable ? 1 : 0;
            ID3D11DepthStencilState* emitterDepthState = DepthStates[readZ][writeZ] ? DepthStates[readZ][writeZ] : DepthStateParticle;
            ctx->OMSetDepthStencilState(emitterDepthState, 0);

            std::vector<ParticleQuadVertex> quadVertices;
            float interp = simulator->InterpolationFactor;

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

            auto EmitQuad = [&](const XMFLOAT3& pos, float halfSizeX, float halfSizeY, float angleRad,
                                const XMFLOAT4& col, const SpriteQuadParams& params, const XMFLOAT4& uvBounds, float slice) {
                float u0 = uvBounds.x;
                float v0 = uvBounds.y;
                float u1 = uvBounds.z;
                float v1 = uvBounds.w;
                XMVECTOR c = XMVectorSet(pos.x, pos.y, pos.z, 0.0f);

                float ox = 0.0f, oy = 0.0f;
                if (params.SpriteAlignment != 0) {
                    if (params.SpriteAlignment & 1) oy += 1.0f;
                    if (params.SpriteAlignment & 2) oy -= 1.0f;
                    if (params.SpriteAlignment & 4) ox += 1.0f;
                    if (params.SpriteAlignment & 8) ox -= 1.0f;
                }

                if (params.CrossedSprites) {
                    int count = std::clamp(params.NoCrossedSprites, 1, 31);
                    if (quadVertices.size() + (size_t)count * 4 > MaxParticleVertices) {
                        FlushQuads();
                    }

                    XMVECTOR u = XMVectorSet(0.0f, 0.0f, halfSizeY, 0.0f);
                    for (int i = 0; i < count; ++i) {
                        float a = (((float)i + 0.5f) * params.FullArc / (float)count) * XM_2PI + angleRad;
                        XMVECTOR r = XMVectorSet(std::cos(a) * halfSizeX, std::sin(a) * halfSizeX, 0.0f, 0.0f);

                        XMVECTOR quadCenter = c;
                        if (params.SpriteAlignment != 0) {
                            if (params.RotateAroundCentre) {
                                XMVECTOR rUnrot = XMVectorSet(std::cos(angleRad) * halfSizeX, std::sin(angleRad) * halfSizeX, 0.0f, 0.0f);
                                quadCenter = c + rUnrot * ox + u * oy;
                            }
                            else {
                                quadCenter = c + r * ox + u * oy;
                            }
                        }

                        XMFLOAT3 p0, p1, p2, p3;
                        XMStoreFloat3(&p0, quadCenter - r + u);
                        XMStoreFloat3(&p1, quadCenter + r + u);
                        XMStoreFloat3(&p2, quadCenter + r - u);
                        XMStoreFloat3(&p3, quadCenter - r - u);

                        quadVertices.push_back({ p0, XMFLOAT3(u0, v0, slice), col });
                        quadVertices.push_back({ p1, XMFLOAT3(u1, v0, slice), col });
                        quadVertices.push_back({ p2, XMFLOAT3(u1, v1, slice), col });
                        quadVertices.push_back({ p3, XMFLOAT3(u0, v1, slice), col });
                    }
                }
                else {
                    if (quadVertices.size() + 4 > MaxParticleVertices) {
                        FlushQuads();
                    }

                    XMVECTOR baseRight, baseUp;
                    if (params.FaceMe3D) {
                        // Cylindrical upright billboard facing camera position around world Z-up
                        float camX = XMVectorGetX(camPos);
                        float camY = XMVectorGetY(camPos);
                        XMVECTOR cylRight = XMVectorSet(camY - pos.y, pos.x - camX, 0.0f, 0.0f);
                        if (XMVectorGetX(XMVector3LengthSq(cylRight)) < 1e-6f) {
                            XMVECTOR flatRight = XMVectorSet(XMVectorGetX(camRight), XMVectorGetY(camRight), 0.0f, 0.0f);
                            cylRight = (XMVectorGetX(XMVector3LengthSq(flatRight)) < 1e-6f)
                                ? XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f)
                                : XMVector3Normalize(flatRight);
                        }
                        else {
                            cylRight = XMVector3Normalize(cylRight);
                        }
                        baseRight = cylRight;
                        baseUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
                    }
                    else {
                        // Default billboard: full camera view plane
                        baseRight = camRight;
                        baseUp = camUp;
                    }

                    float cosA = std::cos(angleRad);
                    float sinA = std::sin(angleRad);
                    XMVECTOR r = (baseRight * cosA + baseUp * sinA) * halfSizeX;
                    XMVECTOR u = (-baseRight * sinA + baseUp * cosA) * halfSizeY;

                    XMVECTOR quadCenter = c;
                    if (params.SpriteAlignment != 0) {
                        if (params.RotateAroundCentre) {
                            quadCenter = c + baseRight * (ox * halfSizeX) + baseUp * (oy * halfSizeY);
                        }
                        else {
                            quadCenter = c + r * ox + u * oy;
                        }
                    }

                    XMFLOAT3 p0, p1, p2, p3;
                    XMStoreFloat3(&p0, quadCenter - r + u);
                    XMStoreFloat3(&p1, quadCenter + r + u);
                    XMStoreFloat3(&p2, quadCenter + r - u);
                    XMStoreFloat3(&p3, quadCenter - r - u);

                    quadVertices.push_back({ p0, XMFLOAT3(u0, v0, slice), col });
                    quadVertices.push_back({ p1, XMFLOAT3(u1, v0, slice), col });
                    quadVertices.push_back({ p2, XMFLOAT3(u1, v1, slice), col });
                    quadVertices.push_back({ p3, XMFLOAT3(u0, v1, slice), col });
                }
            };

            auto EmitTrailQuad = [&](const SimulatedParticle& p, float delay, float halfWidth,
                                     const XMFLOAT4& col, const XMFLOAT4& uvBounds, float slice) {
                if (delay <= 0.0001f || halfWidth <= 0.00001f) return;

                XMFLOAT3 pos1 = SamplePositionBuffer(p, 1.0f - interp);
                XMFLOAT3 pos2;
                if (delay > 2.0f) {
                    XMFLOAT3 pSample = SamplePositionBuffer(p, 3.0f - interp);
                    float stretch = delay * 0.5f;
                    pos2 = XMFLOAT3(
                        pos1.x + (pSample.x - pos1.x) * stretch,
                        pos1.y + (pSample.y - pos1.y) * stretch,
                        pos1.z + (pSample.z - pos1.z) * stretch
                    );
                }
                else {
                    pos2 = SamplePositionBuffer(p, delay + 1.0f - interp);
                }

                XMVECTOR v1 = XMLoadFloat3(&pos1);
                XMVECTOR v2 = XMLoadFloat3(&pos2);
                XMVECTOR seg = XMVectorSubtract(v2, v1);
                float segLenSq = XMVectorGetX(XMVector3LengthSq(seg));
                if (segLenSq < 1e-8f) return;

                XMVECTOR dir = XMVectorScale(seg, 1.0f / std::sqrt(segLenSq));
                XMVECTOR mid = XMVectorScale(XMVectorAdd(v1, v2), 0.5f);
                XMVECTOR toCam = XMVectorSubtract(camPos, mid);
                if (XMVectorGetX(XMVector3LengthSq(toCam)) < 1e-8f) {
                    toCam = XMVectorNegate(camFwd);
                }
                else {
                    toCam = XMVector3Normalize(toCam);
                }

                XMVECTOR side = XMVector3Cross(dir, toCam);
                float sideLenSq = XMVectorGetX(XMVector3LengthSq(side));
                if (sideLenSq < 1e-8f) {
                    side = camRight;
                }
                else {
                    side = XMVectorScale(side, 1.0f / std::sqrt(sideLenSq));
                }
                side = XMVectorScale(side, halfWidth);

                if (quadVertices.size() + 4 > MaxParticleVertices) {
                    FlushQuads();
                }

                XMFLOAT3 p0, p1, p2, p3;
                XMStoreFloat3(&p0, XMVectorSubtract(v1, side));
                XMStoreFloat3(&p1, XMVectorAdd(v1, side));
                XMStoreFloat3(&p2, XMVectorAdd(v2, side));
                XMStoreFloat3(&p3, XMVectorSubtract(v2, side));

                float u0 = uvBounds.x, v0 = uvBounds.y, u1 = uvBounds.z, v1uv = uvBounds.w;
                quadVertices.push_back({ p0, XMFLOAT3(u0, v0, slice), col });
                quadVertices.push_back({ p1, XMFLOAT3(u1, v0, slice), col });
                quadVertices.push_back({ p2, XMFLOAT3(u1, v1uv, slice), col });
                quadVertices.push_back({ p3, XMFLOAT3(u0, v1uv, slice), col });
            };

            auto EmitTrailRibbon = [&](const std::vector<XMFLOAT3>& points, float halfWidth,
                                       const XMFLOAT4& col, const XMFLOAT4& uvBounds, float slice) {
                if (points.size() < 2 || halfWidth <= 0.00001f) return;

                const size_t n = points.size();
                std::vector<XMVECTOR> sides(n);
                XMVECTOR lastSide = camRight * halfWidth;

                for (size_t i = 0; i < n; ++i) {
                    XMVECTOR pCurr = XMLoadFloat3(&points[i]);
                    XMVECTOR dir;
                    if (i == 0) {
                        dir = XMVectorSubtract(XMLoadFloat3(&points[1]), pCurr);
                    }
                    else if (i + 1 == n) {
                        dir = XMVectorSubtract(pCurr, XMLoadFloat3(&points[i - 1]));
                    }
                    else {
                        dir = XMVectorSubtract(XMLoadFloat3(&points[i + 1]), XMLoadFloat3(&points[i - 1]));
                        if (XMVectorGetX(XMVector3LengthSq(dir)) < 1e-8f) {
                            dir = XMVectorSubtract(XMLoadFloat3(&points[i + 1]), pCurr);
                        }
                    }

                    float dirLenSq = XMVectorGetX(XMVector3LengthSq(dir));
                    if (dirLenSq < 1e-8f) {
                        sides[i] = lastSide;
                        continue;
                    }
                    dir = XMVectorScale(dir, 1.0f / std::sqrt(dirLenSq));

                    XMVECTOR toCam = XMVectorSubtract(camPos, pCurr);
                    if (XMVectorGetX(XMVector3LengthSq(toCam)) < 1e-8f) {
                        toCam = XMVectorNegate(camFwd);
                    }
                    else {
                        toCam = XMVector3Normalize(toCam);
                    }

                    XMVECTOR side = XMVector3Cross(dir, toCam);
                    float sideLenSq = XMVectorGetX(XMVector3LengthSq(side));
                    if (sideLenSq < 1e-8f) {
                        side = lastSide;
                    }
                    else {
                        side = XMVectorScale(side, halfWidth / std::sqrt(sideLenSq));
                        lastSide = side;
                    }
                    sides[i] = side;
                }

                float u0 = uvBounds.x, v0 = uvBounds.y, u1 = uvBounds.z, v1uv = uvBounds.w;
                for (size_t i = 0; i + 1 < n; ++i) {
                    XMVECTOR v1 = XMLoadFloat3(&points[i]);
                    XMVECTOR v2 = XMLoadFloat3(&points[i + 1]);
                    if (XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(v2, v1))) < 1e-8f) continue;

                    if (quadVertices.size() + 4 > MaxParticleVertices) {
                        FlushQuads();
                    }

                    float t0 = (float)i / (float)(n - 1);
                    float t1 = (float)(i + 1) / (float)(n - 1);
                    float segV0 = v0 + (v1uv - v0) * t0;
                    float segV1 = v0 + (v1uv - v0) * t1;

                    XMFLOAT3 p0, p1, p2, p3;
                    XMStoreFloat3(&p0, XMVectorSubtract(v1, sides[i]));
                    XMStoreFloat3(&p1, XMVectorAdd(v1, sides[i]));
                    XMStoreFloat3(&p2, XMVectorAdd(v2, sides[i + 1]));
                    XMStoreFloat3(&p3, XMVectorSubtract(v2, sides[i + 1]));

                    quadVertices.push_back({ p0, XMFLOAT3(u0, segV0, slice), col });
                    quadVertices.push_back({ p1, XMFLOAT3(u1, segV0, slice), col });
                    quadVertices.push_back({ p2, XMFLOAT3(u1, segV1, slice), col });
                    quadVertices.push_back({ p3, XMFLOAT3(u0, segV1, slice), col });
                }
            };

            ID3D11Device* device = Device;
            if (!device) ctx->GetDevice(&device);

            // Collect active CPSCLight components across all systems first for dynamic lighting
            struct ActiveParticleLight {
                size_t SysIdx;
                XMFLOAT3 Position;
                XMFLOAT4 Color;
                float OuterRadius;
                float InnerRadius;
                float AttenuationFactor;
                float ViewZ;
            };
            std::vector<ActiveParticleLight> activeLights;
            for (size_t sysIdx = 0; sysIdx < emitter->Systems.size(); sysIdx++) {
                if (sysIdx >= simulator->SystemStates.size()) break;
                const auto& sys = emitter->Systems[sysIdx];
                const auto& state = simulator->SystemStates[sysIdx];
                if (!sys.Enabled || !state.LightActive) continue;

                float scalarSysScale = ComputeScalarSystemScale(sys);
                float sysAlpha = std::clamp(state.SystemAlpha, 0.0f, 1.0f);

                for (const auto& comp : sys.Components) {
                    if (!comp || !comp->Enabled || !comp->Visible || comp->ClassName != "CPSCLight") continue;
                    auto lightComp = std::dynamic_pointer_cast<CPSCLight>(comp);
                    if (!lightComp || !lightComp->LightEnabled) continue;

                    XMVECTOR posV = XMVector3TransformCoord(XMVectorSet(state.LightPosition.x, state.LightPosition.y, state.LightPosition.z, 1.0f), viewMat);
                    float vz = XMVectorGetZ(posV);
                    float camDist = XMVectorGetX(XMVector3Length(posV));

                    float t = 0.0f;
                    if (lightComp->LightUseLifeSecs && lightComp->LightLifeSecs > 0.0001f) {
                        t = std::clamp(state.LightTimer / lightComp->LightLifeSecs, 0.0f, 1.0f);
                    }
                    else if (lightComp->LightUseTimelineSecs && lightComp->LightTimelineSecs > 0.0001f) {
                        t = std::clamp(std::fmod(state.LightTimer, lightComp->LightTimelineSecs) / lightComp->LightTimelineSecs, 0.0f, 1.0f);
                    }

                    XMFLOAT4 lcStart = ColourToFloat4(lightComp->LightStartColour);
                    XMFLOAT4 lcMid   = ColourToFloat4(lightComp->LightMidColour);
                    XMFLOAT4 lcEnd   = ColourToFloat4(lightComp->LightEndColour);

                    XMFLOAT4 col = EvaluateColorSpline511(lcStart, lcMid, lcEnd, lightComp->LightUseStartColour, lightComp->LightUseMidColour, lightComp->LightUseEndColour, t);
                    float fadeInEnd = (float)lightComp->LightFadeInEndInteger * 0.001f;
                    float fadeOutBegin = (float)lightComp->LightFadeOutBeginInteger * 0.001f;

                    if (lightComp->LightColourFadeEnable) {
                        if (lightComp->LightUseFadeColour) {
                            float fadeT = EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, 0.0f);
                            XMFLOAT4 minCol = ColourToFloat4(lightComp->LightColourFadeMinimum);
                            col.x = minCol.x + (col.x - minCol.x) * fadeT;
                            col.y = minCol.y + (col.y - minCol.y) * fadeT;
                            col.z = minCol.z + (col.z - minCol.z) * fadeT;
                            col.w = minCol.w + (col.w - minCol.w) * fadeT;
                        }
                        else {
                            float minAlpha = lightComp->LightColourFadeMinimum.A / 255.0f;
                            col.w *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, minAlpha);
                        }
                    }

                    col.w *= sysAlpha * EvaluateEmitterDistanceFade(emitter, camDist);

                    float outerRadius = lightComp->LightStartRenderWorldRadius + t * (lightComp->LightEndRenderWorldRadius - lightComp->LightStartRenderWorldRadius);
                    if (lightComp->LightWorldRadiusFadeEnable) {
                        outerRadius *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, lightComp->LightWorldRadiusFadeMinimum);
                    }
                    outerRadius *= scalarSysScale;

                    if (col.w > 0.0001f && outerRadius > 0.0001f) {
                        float attenFactor = (float)(std::max)(1, lightComp->LightAttenuationFactor);
                        float innerRadius = (lightComp->LightAttenuationFactor > 1)
                            ? (outerRadius / attenFactor)
                            : (outerRadius * 0.35f);
                        activeLights.push_back({ sysIdx, state.LightPosition, col, outerRadius, innerRadius, attenFactor, vz });
                    }
                }
            }

            cb.NumLights = (int32_t)(std::min)(activeLights.size(), (size_t)kMaxParticleLights);
            for (int li = 0; li < cb.NumLights; ++li) {
                const auto& al = activeLights[li];
                cb.Lights[li].PosAndRadius = XMFLOAT4(al.Position.x, al.Position.y, al.Position.z, al.OuterRadius);
                cb.Lights[li].ColorAndAtten = XMFLOAT4(
                    al.Color.x * al.Color.w,
                    al.Color.y * al.Color.w,
                    al.Color.z * al.Color.w,
                    al.AttenuationFactor
                );
            }

            for (size_t sysIdx = 0; sysIdx < emitter->Systems.size(); sysIdx++) {
                if (sysIdx >= simulator->SystemStates.size()) break;
                const auto& sys = emitter->Systems[sysIdx];
                const auto& state = simulator->SystemStates[sysIdx];

                if (!sys.Enabled) continue;

                // Find visible & enabled render components
                std::shared_ptr<CPSCRenderSprite> renderSprite;
                std::shared_ptr<CPSCSingleSprite> singleSprite;
                std::shared_ptr<CPSCRenderMesh> renderMesh;
                std::shared_ptr<CPSCDecalRenderer> decalRenderer;
                std::shared_ptr<CPSCLight> lightComp;
                for (const auto& comp : sys.Components) {
                    if (!comp || !comp->Enabled || !comp->Visible) continue;
                    if (comp->ClassName == "CPSCRenderSprite") renderSprite = std::dynamic_pointer_cast<CPSCRenderSprite>(comp);
                    else if (comp->ClassName == "CPSCSingleSprite") singleSprite = std::dynamic_pointer_cast<CPSCSingleSprite>(comp);
                    else if (comp->ClassName == "CPSCRenderMesh") renderMesh = std::dynamic_pointer_cast<CPSCRenderMesh>(comp);
                    else if (comp->ClassName == "CPSCDecalRenderer") decalRenderer = std::dynamic_pointer_cast<CPSCDecalRenderer>(comp);
                    else if (comp->ClassName == "CPSCLight") lightComp = std::dynamic_pointer_cast<CPSCLight>(comp);
                }

                if (!renderSprite && !singleSprite && !renderMesh && !decalRenderer && !lightComp) continue;

                quadVertices.clear();
                float scalarSysScale = ComputeScalarSystemScale(sys);
                C3DVector vecSysScale = ComputeVectorSystemScale(sys);
                float sysAlpha = std::clamp(state.SystemAlpha, 0.0f, 1.0f);

                // ----------------------------------------------------
                // 1. Process simulated particles for CPSCRenderSprite
                // ----------------------------------------------------
                if (renderSprite && (renderSprite->SpriteBankIndex > 0 || renderSprite->TrailBankIndex > 0)) {
                    float fadeInEnd = (float)renderSprite->FadeInEndInteger * 0.001f;
                    float fadeOutBegin = (float)renderSprite->FadeOutBeginInteger * 0.001f;
                    float alphaFadeMin = renderSprite->AlphaFadeMinimum;
                    float sizeFadeMin = renderSprite->SizeFadeMinimum;

                    struct ParticleRenderSortItem {
                        const SimulatedParticle* P;
                        XMFLOAT3 RenderPos;
                        float ViewZ;
                        float CamDist;
                        float LifeTimer;
                        float T;
                    };
                    std::vector<ParticleRenderSortItem> sortedParticles;
                    sortedParticles.reserve(state.Particles.size());

                    for (const auto& p : state.Particles) {
                        if (!p.Active) continue;
                        float life_timer = (float)p.Lifetimer - ((float)p.CreationTime / 255.0f) + interp;
                        if (life_timer < 0.0f) continue;
                        if (p.MaxLifeTicks <= 0 || life_timer > (float)p.MaxLifeTicks) continue;

                        XMFLOAT3 pos = SamplePositionBuffer(p, 1.0f - interp);
                        XMVECTOR posV = XMVector3TransformCoord(XMVectorSet(pos.x, pos.y, pos.z, 1.0f), viewMat);
                        float vz = XMVectorGetZ(posV);
                        if (vz <= 0.1f) continue;

                        float camDist = XMVectorGetX(XMVector3Length(posV));
                        float t = std::clamp(life_timer / (float)p.MaxLifeTicks, 0.0f, 1.0f - 0.0001f);
                        sortedParticles.push_back({ &p, pos, vz, camDist, life_timer, t });
                    }

                    std::sort(sortedParticles.begin(), sortedParticles.end(), [](const ParticleRenderSortItem& a, const ParticleRenderSortItem& b) {
                        return a.ViewZ > b.ViewZ;
                    });

                    XMFLOAT4 cStart = ColourToFloat4(renderSprite->StartColour);
                    XMFLOAT4 cMid   = ColourToFloat4(renderSprite->MidColour);
                    XMFLOAT4 cEnd   = ColourToFloat4(renderSprite->EndColour);

                    if (renderSprite->SpriteBankIndex > 0 && !sortedParticles.empty()) {
                        uint32_t rsBMode = renderSprite->BlendMode;
                        uint32_t rsBOp = renderSprite->BlendOp;
                        uint32_t flags = renderSprite->SpriteFlags;

                        SpriteQuadParams rsParams;
                        rsParams.FaceMe3D = (flags & 0x001u) != 0;
                        rsParams.FullArc = ((flags & 0x006u) != 0) ? 1.0f : 0.5f;
                        rsParams.RotateAroundCentre = (flags & 0x020u) != 0;
                        rsParams.CrossedSprites = ((flags & 0x040u) != 0) || ((flags & 0x40000u) != 0);
                        rsParams.NoCrossedSprites = (int)renderSprite->NoCrossedSprites;
                        rsParams.SpriteAlignment = (int)((flags & 0x1Eu) >> 1);

                        ParticleTextureResource rsTexRes = GetOrCreateParticleTexture(device, renderSprite->SpriteBankIndex);
                        ID3D11ShaderResourceView* rsSrv = rsTexRes.SRV ? rsTexRes.SRV : DefaultGlowSRV;

                        cb.IsDXT1 = rsTexRes.IsDXT1 ? 1 : 0;
                        cb.BlendMode = (int32_t)rsBMode;
                        cb.HasBlackBg = rsTexRes.HasBlackBg ? 1 : 0;
                        cb.SelfIlluminating = ((flags & 0x080u) == 0) ? 1 : 0;
                        ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);

                        ctx->PSSetShaderResources(0, 1, &rsSrv);
                        BindBlendState(ctx, rsBMode, rsBOp);
                        BindDepthStateForBlendMode(ctx, readZ, writeZ, rsBMode);

                        float aspect = (rsTexRes.FrameWidth > 0 && rsTexRes.FrameHeight > 0) ? ((float)rsTexRes.FrameHeight / (float)rsTexRes.FrameWidth) : 1.0f;
                        int frameCount = (std::max)(1, (int)rsTexRes.FrameCount);

                        for (const auto& item : sortedParticles) {
                            const auto& p = *item.P;
                            float t = item.T;

                            XMFLOAT4 col = EvaluateColorSpline511(cStart, cMid, cEnd, renderSprite->UseStartColour, renderSprite->UseMidColour, renderSprite->UseEndColour, t);
                            if (renderSprite->AlphaFadeEnable) {
                                col.w *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, alphaFadeMin);
                            }

                            float pSize = renderSprite->StartRenderSize + t * (renderSprite->EndRenderSize - renderSprite->StartRenderSize);
                            if (renderSprite->SizeFadeEnable) {
                                pSize *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, sizeFadeMin);
                            }

                            if (renderSprite->FlickerEnable) {
                                float fSize = 1.0f;
                                col.w *= EvaluatePerlinFlicker(item.LifeTimer / 30.0f, p.FlickerTimeOffset, p.FlickerIndex,
                                                               renderSprite->FlickerSpeed, renderSprite->FlickerBias,
                                                               renderSprite->FlickerMinAlphaInteger, renderSprite->FlickerMinSizeInteger, fSize);
                                pSize *= fSize;
                            }

                            col.w *= sysAlpha * EvaluateEmitterDistanceFade(emitter, item.CamDist);
                            pSize *= scalarSysScale;

                            if (col.w <= 0.0001f || pSize <= 0.00001f) continue;

                            float halfSizeX = pSize * 0.5f;
                            float halfSizeY = halfSizeX * aspect;
                            float renderAngle = p.Angle + ((float)p.AngleChange * (2.0f / 127.0f)) * interp;

                            float animProgress = t;
                            if (renderSprite->ForceAnimationTime && renderSprite->AnimationTimeSecs > 0.0001f) {
                                animProgress = std::fmod((item.LifeTimer / 30.0f) / renderSprite->AnimationTimeSecs, 1.0f);
                                if (animProgress < 0.0f) animProgress += 1.0f;
                            }

                            int frameIdx = std::clamp((int)(animProgress * (float)frameCount), 0, frameCount - 1);
                            XMFLOAT4 uvBounds;
                            float slice = 0.0f;
                            GetFrameUVAndSlice(rsTexRes, frameIdx, uvBounds, slice);

                            EmitQuad(item.RenderPos, halfSizeX, halfSizeY, renderAngle, col, rsParams, uvBounds, slice);
                        }

                        FlushQuads();
                    }

                    // Ribbon trails for CPSCRenderSprite
                    if (renderSprite->TrailBankIndex > 0 && renderSprite->TrailLengthInteger > 0 && !sortedParticles.empty()) {
                        ParticleTextureResource trTexRes = GetOrCreateParticleTexture(device, renderSprite->TrailBankIndex);
                        ID3D11ShaderResourceView* trSrv = trTexRes.SRV ? trTexRes.SRV : DefaultGlowSRV;

                        cb.IsDXT1 = trTexRes.IsDXT1 ? 1 : 0;
                        cb.BlendMode = (int32_t)renderSprite->TrailBlendMode;
                        cb.HasBlackBg = trTexRes.HasBlackBg ? 1 : 0;
                        cb.SelfIlluminating = ((renderSprite->SpriteFlags & 0x080u) == 0) ? 1 : 0;
                        ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);

                        ctx->PSSetShaderResources(0, 1, &trSrv);
                        BindBlendState(ctx, renderSprite->TrailBlendMode, renderSprite->TrailBlendOp);
                        BindDepthStateForBlendMode(ctx, readZ, writeZ, renderSprite->TrailBlendMode);

                        float delay = (float)renderSprite->TrailLengthInteger * 0.001f;
                        float halfWidth = renderSprite->TrailWidth * 0.5f * scalarSysScale;
                        int trFrameCount = (std::max)(1, (int)trTexRes.FrameCount);

                        for (const auto& item : sortedParticles) {
                            const auto& p = *item.P;
                            float t = item.T;

                            XMFLOAT4 col = EvaluateColorSpline511(cStart, cMid, cEnd, renderSprite->UseStartColour, renderSprite->UseMidColour, renderSprite->UseEndColour, t);
                            if (renderSprite->AlphaFadeEnable) {
                                col.w *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, alphaFadeMin);
                            }
                            if (renderSprite->FlickerEnable) {
                                float unusedSize = 1.0f;
                                col.w *= EvaluatePerlinFlicker(item.LifeTimer / 30.0f, p.FlickerTimeOffset, p.FlickerIndex,
                                                               renderSprite->FlickerSpeed, renderSprite->FlickerBias,
                                                               renderSprite->FlickerMinAlphaInteger, renderSprite->FlickerMinSizeInteger, unusedSize);
                            }
                            col.w *= sysAlpha * EvaluateEmitterDistanceFade(emitter, item.CamDist);
                            if (col.w <= 0.0001f) continue;

                            float animProgress = t;
                            if (renderSprite->ForceAnimationTime && renderSprite->AnimationTimeSecs > 0.0001f) {
                                animProgress = std::fmod((item.LifeTimer / 30.0f) / renderSprite->AnimationTimeSecs, 1.0f);
                                if (animProgress < 0.0f) animProgress += 1.0f;
                            }
                            int frameIdx = std::clamp((int)(animProgress * (float)trFrameCount), 0, trFrameCount - 1);
                            XMFLOAT4 uvBounds;
                            float slice = 0.0f;
                            GetFrameUVAndSlice(trTexRes, frameIdx, uvBounds, slice);

                            EmitTrailQuad(p, delay, halfWidth, col, uvBounds, slice);
                        }

                        FlushQuads();
                    }
                }

                // ----------------------------------------------------
                // 2. Process Persistent Single Sprite (CPSCSingleSprite)
                // ----------------------------------------------------
                // In Fable disassembly (functions10.txt:1262-1265 & functions7.txt:1437, 1851):
                // ONLY emit a sprite primitive if SpriteBankIndex != 0 AND entry exists in bank!
                // Never fall back to DefaultGlowSRV for SingleSprite.
                if (singleSprite && state.HasSingleSprite && state.SingleSpriteParticle.Active) {
                    const auto& sp = state.SingleSpriteParticle;
                    XMFLOAT3 spPos = SamplePositionBuffer(sp, 1.0f - interp);
                    XMVECTOR posV = XMVector3TransformCoord(XMVectorSet(spPos.x, spPos.y, spPos.z, 1.0f), viewMat);
                    float vz = XMVectorGetZ(posV);
                    float camDist = XMVectorGetX(XMVector3Length(posV));

                    float simStep = interp * (1.0f / 30.0f);
                    float animTime = (singleSprite->AnimationTimeSecs > 0.01f) ? singleSprite->AnimationTimeSecs : 3.0f;
                    float progress = std::fmod(state.SingleSpriteProgress + simStep / animTime, 1.0f);
                    if (progress < 0.0f) progress += 1.0f;

                    float fadeInEnd = (float)singleSprite->FadeInEndInteger * 0.001f;
                    float fadeOutBegin = (float)singleSprite->FadeOutBeginInteger * 0.001f;

                    XMFLOAT4 cStart = ColourToFloat4(singleSprite->StartColour);
                    XMFLOAT4 cMid   = ColourToFloat4(singleSprite->MidColour);
                    XMFLOAT4 cEnd   = ColourToFloat4(singleSprite->EndColour);

                    XMFLOAT4 col = EvaluateColorSpline511(cStart, cMid, cEnd, singleSprite->UseStartColour, singleSprite->UseMidColour, singleSprite->UseEndColour, progress);
                    if (singleSprite->AlphaFadeEnable) {
                        col.w *= EvaluateCosineFade(progress, fadeInEnd, fadeOutBegin, singleSprite->AlphaFadeMinimum);
                    }
                    col.w *= sysAlpha * EvaluateEmitterDistanceFade(emitter, camDist);

                    if (col.w > 0.0001f) {
                        float animProgress = progress;
                        if (singleSprite->ForceAnimationTime && singleSprite->AnimationTimeSecs > 0.0001f) {
                            animProgress = std::fmod((state.SystemTime + simStep) / singleSprite->AnimationTimeSecs, 1.0f);
                            if (animProgress < 0.0f) animProgress += 1.0f;
                        }

                        if (vz > 0.1f && singleSprite->SpriteBankIndex > 0) {
                            ParticleTextureResource ssTexRes = GetOrCreateParticleTexture(device, singleSprite->SpriteBankIndex);
                            if (ssTexRes.SRV) {
                                uint32_t ssBMode = (uint32_t)singleSprite->BlendMode;
                                uint32_t ssBOp = (uint32_t)singleSprite->BlendOp;

                                cb.IsDXT1 = ssTexRes.IsDXT1 ? 1 : 0;
                                cb.BlendMode = (int32_t)ssBMode;
                                cb.HasBlackBg = ssTexRes.HasBlackBg ? 1 : 0;
                                cb.SelfIlluminating = singleSprite->SelfIlluminating ? 1 : 0;
                                ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);

                                ctx->PSSetShaderResources(0, 1, &ssTexRes.SRV);
                                BindBlendState(ctx, ssBMode, ssBOp);
                                BindDepthStateForBlendMode(ctx, readZ, writeZ, ssBMode);

                                float sSize = singleSprite->StartRenderSize + progress * (singleSprite->EndRenderSize - singleSprite->StartRenderSize);
                                if (singleSprite->SizeFadeEnable) {
                                    sSize *= EvaluateCosineFade(progress, fadeInEnd, fadeOutBegin, singleSprite->SizeFadeMinimum);
                                }
                                sSize *= scalarSysScale;

                                if (sSize > 0.00001f) {
                                    SpriteQuadParams ssParams;
                                    ssParams.FaceMe3D = singleSprite->FaceMe3D && !singleSprite->FaceMe2D;
                                    ssParams.FullArc = (singleSprite->SpriteAlignment != 0) ? 1.0f : 0.5f;
                                    ssParams.RotateAroundCentre = singleSprite->RotateAroundCentre;
                                    ssParams.CrossedSprites = singleSprite->CrossedSprites;
                                    ssParams.NoCrossedSprites = singleSprite->NoCrossedSprites;
                                    ssParams.SpriteAlignment = singleSprite->SpriteAlignment;

                                    float aspect = (ssTexRes.FrameWidth > 0 && ssTexRes.FrameHeight > 0) ? ((float)ssTexRes.FrameHeight / (float)ssTexRes.FrameWidth) : 1.0f;
                                    float halfSizeX = sSize * 0.5f;
                                    float halfSizeY = halfSizeX * aspect;

                                    int frameCount = (std::max)(1, (int)ssTexRes.FrameCount);
                                    int frameIdx = std::clamp((int)(animProgress * (float)frameCount), 0, frameCount - 1);
                                    XMFLOAT4 uvBounds;
                                    float slice = 0.0f;
                                    GetFrameUVAndSlice(ssTexRes, frameIdx, uvBounds, slice);

                                    float renderAngle = sp.Angle + ((float)sp.AngleChange * (2.0f / 127.0f)) * interp;
                                    EmitQuad(spPos, halfSizeX, halfSizeY, renderAngle, col, ssParams, uvBounds, slice);
                                    FlushQuads();
                                }
                            }
                        }

                        // Connected spline ribbon trail (or safe fallback) for CPSCSingleSprite
                        if (singleSprite->TrailBankIndex > 0 && singleSprite->MaxTrailLength > 0) {
                            bool hasSplineTrail = (state.SingleSpriteTrailPoints.size() >= 2);
                            if (hasSplineTrail || vz > 0.1f) {
                                ParticleTextureResource trTexRes = GetOrCreateParticleTexture(device, singleSprite->TrailBankIndex);
                                ID3D11ShaderResourceView* trSrv = trTexRes.SRV ? trTexRes.SRV : DefaultGlowSRV;

                                uint32_t trBMode = (uint32_t)singleSprite->TrailBlendMode;
                                uint32_t trBOp = (uint32_t)singleSprite->TrailBlendOp;

                                cb.IsDXT1 = trTexRes.IsDXT1 ? 1 : 0;
                                cb.BlendMode = (int32_t)trBMode;
                                cb.HasBlackBg = trTexRes.HasBlackBg ? 1 : 0;
                                cb.SelfIlluminating = singleSprite->SelfIlluminating ? 1 : 0;
                                ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);

                                ctx->PSSetShaderResources(0, 1, &trSrv);
                                BindBlendState(ctx, trBMode, trBOp);
                                BindDepthStateForBlendMode(ctx, readZ, writeZ, trBMode);

                                float delay = (float)singleSprite->MaxTrailLength * 0.001f;
                                float halfWidth = singleSprite->TrailWidth * 0.5f * scalarSysScale;
                                int trFrameCount = (std::max)(1, (int)trTexRes.FrameCount);
                                int frameIdx = std::clamp((int)(animProgress * (float)trFrameCount), 0, trFrameCount - 1);
                                XMFLOAT4 uvBounds;
                                float slice = 0.0f;
                                GetFrameUVAndSlice(trTexRes, frameIdx, uvBounds, slice);

                                if (hasSplineTrail) {
                                    EmitTrailRibbon(state.SingleSpriteTrailPoints, halfWidth, col, uvBounds, slice);
                                }
                                else {
                                    EmitTrailQuad(sp, delay, halfWidth, col, uvBounds, slice);
                                }
                                FlushQuads();
                            }
                        }
                    }
                }

                // ----------------------------------------------------
                // 3. Process mesh particles & trails (CPSCRenderMesh)
                // ----------------------------------------------------
                if (renderMesh) {
                    float fadeInEnd = (float)renderMesh->FadeInEndInteger * 0.001f;
                    float fadeOutBegin = (float)renderMesh->FadeOutBeginInteger * 0.001f;
                    float alphaFadeMin = renderMesh->AlphaFadeMinimum;
                    float sizeFadeMin = renderMesh->SizeFadeMinimum;

                    XMFLOAT4 cStart = ColourToFloat4(renderMesh->StartColour);
                    XMFLOAT4 cMid   = ColourToFloat4(renderMesh->MidColour);
                    XMFLOAT4 cEnd   = ColourToFloat4(renderMesh->EndColour);

                    if (renderMesh->BankIndex > 0) {
                        ParticleMeshGPU* gpuMesh = GetOrCreateParticleMesh(device, renderMesh->BankIndex);

                        if (gpuMesh && gpuMesh->IsValid && VS_ParticleMesh && PS_ParticleMesh) {
                            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                            ctx->IASetInputLayout(Layout_ParticleMesh);
                            ctx->VSSetShader(VS_ParticleMesh, nullptr, 0);
                            ctx->VSSetConstantBuffers(0, 1, &ConstantBufferParticleMesh);
                            ctx->PSSetShader(PS_ParticleMesh, nullptr, 0);
                            ctx->PSSetConstantBuffers(0, 1, &ConstantBufferParticleMesh);
                            ctx->PSSetSamplers(0, 1, &MeshSampler);
                            ctx->RSSetState(RastState);
                            BindBlendState(ctx, renderMesh->BlendMode, renderMesh->BlendOp);
                            BindDepthStateForBlendMode(ctx, readZ, writeZ, renderMesh->BlendMode);

                            UINT mStride = sizeof(GPUVertex);
                            UINT mOffset = 0;
                            ctx->IASetVertexBuffers(0, 1, &gpuMesh->VertexBuffer, &mStride, &mOffset);
                            ctx->IASetIndexBuffer(gpuMesh->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

                            C3DVector startSize = renderMesh->StartRenderSize;
                            C3DVector endSize = renderMesh->EndRenderSize;
                            XMMATRIX viewProj = viewMat * proj;

                            for (const auto& p : state.Particles) {
                                if (!p.Active) continue;
                                float life_timer = (float)p.Lifetimer - ((float)p.CreationTime / 255.0f) + interp;
                                if (life_timer < 0.0f) continue;
                                if (p.MaxLifeTicks <= 0 || life_timer > (float)p.MaxLifeTicks) continue;

                                XMFLOAT3 interpPos = SamplePositionBuffer(p, 1.0f - interp);
                                XMVECTOR posV = XMVector3TransformCoord(XMVectorSet(interpPos.x, interpPos.y, interpPos.z, 1.0f), viewMat);
                                float vz = XMVectorGetZ(posV);
                                if (vz <= 0.1f) continue;
                                float camDist = XMVectorGetX(XMVector3Length(posV));

                                float t = std::clamp(life_timer / (float)p.MaxLifeTicks, 0.0f, 1.0f - 0.0001f);
                                XMFLOAT4 col = EvaluateColorSpline511(cStart, cMid, cEnd, renderMesh->UseStartColour, renderMesh->UseMidColour, renderMesh->UseEndColour, t);

                                float cr = col.x, cg = col.y, cbVal = col.z, ca = col.w;
                                if (renderMesh->AlphaFadeEnable) {
                                    ca *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, alphaFadeMin);
                                }

                                float flickerSizeMul = 1.0f;
                                if (renderMesh->FlickerEnable) {
                                    ca *= EvaluatePerlinFlicker(life_timer / 30.0f, p.FlickerTimeOffset, p.FlickerIndex,
                                                                renderMesh->FlickerSpeed, renderMesh->FlickerBias,
                                                                renderMesh->FlickerMinAlphaInteger, renderMesh->FlickerMinSizeInteger, flickerSizeMul);
                                }

                                ca *= sysAlpha * EvaluateEmitterDistanceFade(emitter, camDist);
                                if (ca <= 0.0001f) continue;

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

                                curSize.X *= flickerSizeMul;
                                curSize.Y *= flickerSizeMul;
                                curSize.Z *= flickerSizeMul;

                                if (curSize.X <= 0.00001f && curSize.Y <= 0.00001f && curSize.Z <= 0.00001f) continue;

                                float meshRadius = (gpuMesh->BoundingRadius > 0.0001f) ? gpuMesh->BoundingRadius : 1.0f;
                                float scaleX = (curSize.X / meshRadius) * vecSysScale.X;
                                float scaleY = (curSize.Y / meshRadius) * vecSysScale.Y;
                                float scaleZ = (curSize.Z / meshRadius) * vecSysScale.Z;

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

                                XMVECTOR E1 = XMVectorScale(vRight, -scaleX);
                                XMVECTOR E2 = XMVectorScale(vForward, -scaleY);
                                XMVECTOR E3 = XMVectorScale(vUp, scaleZ);

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
                                    bool hasTex = (texSRV != nullptr);
                                    if (!texSRV) {
                                        texSRV = DefaultWhiteSRV;
                                    }

                                    ParticleMeshCB meshCB = {};
                                    meshCB.WorldViewProj = XMMatrixTranspose(meshWVP);
                                    meshCB.World = XMMatrixTranspose(worldMat);
                                    meshCB.MeshColor = XMFLOAT4(cr, cg, cbVal, ca);
                                    meshCB.HasTexture = hasTex ? 1 : 0;
                                    meshCB.BlendMode = (int32_t)renderMesh->BlendMode;
                                    meshCB.IsDXT1 = 0;
                                    meshCB.NumLights = cb.NumLights;
                                    memcpy(meshCB.Lights, cb.Lights, sizeof(cb.Lights));
                                    ctx->UpdateSubresource(ConstantBufferParticleMesh, 0, nullptr, &meshCB, 0, 0);

                                    ctx->PSSetShaderResources(0, 1, &texSRV);
                                    ctx->DrawIndexed(batch.IndexCount, batch.IndexStart, 0);
                                }
                            }

                            BindBillboardPipeline();
                        }
                        else {
                            // Fallback billboard rendering if mesh not found in open banks
                            float startSize = (renderMesh->StartRenderSize.X + renderMesh->StartRenderSize.Y + renderMesh->StartRenderSize.Z) / 3.0f;
                            float endSize   = (renderMesh->EndRenderSize.X   + renderMesh->EndRenderSize.Y   + renderMesh->EndRenderSize.Z)   / 3.0f;

                            cb.IsDXT1 = 0;
                            cb.BlendMode = (int32_t)renderMesh->BlendMode;
                            cb.HasBlackBg = 0;
                            cb.SelfIlluminating = 0;
                            ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);
                            ctx->PSSetShaderResources(0, 1, &DefaultGlowSRV);
                            BindBlendState(ctx, renderMesh->BlendMode, renderMesh->BlendOp);
                            BindDepthStateForBlendMode(ctx, readZ, writeZ, renderMesh->BlendMode);

                            SpriteQuadParams fbParams;
                            fbParams.FaceMe3D = false;
                            fbParams.RotateAroundCentre = true;
                            fbParams.CrossedSprites = false;
                            fbParams.SpriteAlignment = 0;

                            for (const auto& p : state.Particles) {
                                if (!p.Active) continue;
                                float life_timer = (float)p.Lifetimer - ((float)p.CreationTime / 255.0f) + interp;
                                if (life_timer < 0.0f) continue;
                                if (p.MaxLifeTicks <= 0 || life_timer > (float)p.MaxLifeTicks) continue;

                                XMFLOAT3 spPos = SamplePositionBuffer(p, 1.0f - interp);
                                XMVECTOR posV = XMVector3TransformCoord(XMVectorSet(spPos.x, spPos.y, spPos.z, 1.0f), viewMat);
                                float vz = XMVectorGetZ(posV);
                                if (vz <= 0.1f) continue;
                                float camDist = XMVectorGetX(XMVector3Length(posV));

                                float t = std::clamp(life_timer / (float)p.MaxLifeTicks, 0.0f, 1.0f - 0.0001f);
                                XMFLOAT4 col = EvaluateColorSpline511(cStart, cMid, cEnd, renderMesh->UseStartColour, renderMesh->UseMidColour, renderMesh->UseEndColour, t);

                                if (renderMesh->AlphaFadeEnable) {
                                    col.w *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, alphaFadeMin);
                                }

                                float pSize = startSize + (endSize - startSize) * t;
                                if (renderMesh->SizeFadeEnable) {
                                    pSize *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, sizeFadeMin);
                                }

                                if (renderMesh->FlickerEnable) {
                                    float fSize = 1.0f;
                                    col.w *= EvaluatePerlinFlicker(life_timer / 30.0f, p.FlickerTimeOffset, p.FlickerIndex,
                                                                   renderMesh->FlickerSpeed, renderMesh->FlickerBias,
                                                                   renderMesh->FlickerMinAlphaInteger, renderMesh->FlickerMinSizeInteger, fSize);
                                    pSize *= fSize;
                                }

                                col.w *= sysAlpha * EvaluateEmitterDistanceFade(emitter, camDist);
                                pSize *= scalarSysScale;
                                if (col.w <= 0.0001f || pSize <= 0.00001f) continue;

                                float interpAngle = p.Angle + ((float)p.AngleChange * (2.0f / 127.0f)) * interp;
                                XMFLOAT4 uvBounds(0.0f, 0.0f, 1.0f, 1.0f);
                                EmitQuad(spPos, pSize * 0.5f, pSize * 0.5f, interpAngle, col, fbParams, uvBounds, 0.0f);
                            }
                            FlushQuads();
                        }
                    }

                    // Ribbon trails for CPSCRenderMesh
                    if (renderMesh->TrailBankIndex > 0 && renderMesh->TrailLengthInteger > 0) {
                        BindBillboardPipeline();
                        ParticleTextureResource trTexRes = GetOrCreateParticleTexture(device, renderMesh->TrailBankIndex);
                        ID3D11ShaderResourceView* trSrv = trTexRes.SRV ? trTexRes.SRV : DefaultGlowSRV;

                        cb.IsDXT1 = trTexRes.IsDXT1 ? 1 : 0;
                        cb.BlendMode = (int32_t)renderMesh->TrailBlendMode;
                        cb.HasBlackBg = trTexRes.HasBlackBg ? 1 : 0;
                        cb.SelfIlluminating = 0;
                        ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);

                        ctx->PSSetShaderResources(0, 1, &trSrv);
                        BindBlendState(ctx, renderMesh->TrailBlendMode, renderMesh->TrailBlendOp);
                        BindDepthStateForBlendMode(ctx, readZ, writeZ, renderMesh->TrailBlendMode);

                        XMFLOAT4 tcStart = ColourToFloat4(renderMesh->TrailStartColour);
                        XMFLOAT4 tcMid   = ColourToFloat4(renderMesh->TrailMidColour);
                        XMFLOAT4 tcEnd   = ColourToFloat4(renderMesh->TrailEndColour);

                        float delay = (float)renderMesh->TrailLengthInteger * 0.001f;
                        float halfWidth = renderMesh->TrailWidth * 0.5f * scalarSysScale;
                        int trFrameCount = (std::max)(1, (int)trTexRes.FrameCount);

                        for (const auto& p : state.Particles) {
                            if (!p.Active) continue;
                            float life_timer = (float)p.Lifetimer - ((float)p.CreationTime / 255.0f) + interp;
                            if (life_timer < 0.0f) continue;
                            if (p.MaxLifeTicks <= 0 || life_timer > (float)p.MaxLifeTicks) continue;

                            XMFLOAT3 pos = SamplePositionBuffer(p, 1.0f - interp);
                            XMVECTOR posV = XMVector3TransformCoord(XMVectorSet(pos.x, pos.y, pos.z, 1.0f), viewMat);
                            float vz = XMVectorGetZ(posV);
                            if (vz <= 0.1f) continue;
                            float camDist = XMVectorGetX(XMVector3Length(posV));

                            float t = std::clamp(life_timer / (float)p.MaxLifeTicks, 0.0f, 1.0f - 0.0001f);
                            XMFLOAT4 col = EvaluateColorSpline511(tcStart, tcMid, tcEnd, renderMesh->TrailUseStartColour, renderMesh->TrailUseMidColour, renderMesh->TrailUseEndColour, t);
                            if (renderMesh->AlphaFadeEnable) {
                                col.w *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, alphaFadeMin);
                            }
                            if (renderMesh->FlickerEnable) {
                                float unusedSize = 1.0f;
                                col.w *= EvaluatePerlinFlicker(life_timer / 30.0f, p.FlickerTimeOffset, p.FlickerIndex,
                                                               renderMesh->FlickerSpeed, renderMesh->FlickerBias,
                                                               renderMesh->FlickerMinAlphaInteger, renderMesh->FlickerMinSizeInteger, unusedSize);
                            }
                            col.w *= sysAlpha * EvaluateEmitterDistanceFade(emitter, camDist);
                            if (col.w <= 0.0001f) continue;

                            int frameIdx = std::clamp((int)(t * (float)trFrameCount), 0, trFrameCount - 1);
                            XMFLOAT4 uvBounds;
                            float slice = 0.0f;
                            GetFrameUVAndSlice(trTexRes, frameIdx, uvBounds, slice);

                            EmitTrailQuad(p, delay, halfWidth, col, uvBounds, slice);
                        }
                        FlushQuads();
                    }
                }

                // ----------------------------------------------------
                // 4. Process Ground Decals (CPSCDecalRenderer)
                // ----------------------------------------------------
                if (decalRenderer && !state.Decals.empty()) {
                    ParticleTextureResource decTexRes = GetOrCreateParticleTexture(device, decalRenderer->DecalBankIndex);
                    ID3D11ShaderResourceView* decSrv = decTexRes.SRV ? decTexRes.SRV : DefaultGlowSRV;

                    cb.IsDXT1 = decTexRes.IsDXT1 ? 1 : 0;
                    cb.BlendMode = (int32_t)decalRenderer->BlendMode;
                    cb.HasBlackBg = decTexRes.HasBlackBg ? 1 : 0;
                    cb.SelfIlluminating = 0;
                    ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);

                    ctx->PSSetShaderResources(0, 1, &decSrv);
                    BindBlendState(ctx, decalRenderer->BlendMode, decalRenderer->BlendOp);
                    BindDepthStateForBlendMode(ctx, readZ, writeZ, decalRenderer->BlendMode);

                    float startSize = (float)decalRenderer->StartRenderSizeInteger / 2047.0f * 20.0f;
                    float endSize = (float)decalRenderer->EndRenderSizeInteger / 2047.0f * 20.0f;
                    float fadeInEnd = (float)decalRenderer->FadeInEndInteger * 0.001f;
                    float fadeOutBegin = (float)decalRenderer->FadeOutBeginInteger * 0.001f;
                    float alphaFadeMin = (float)decalRenderer->AlphaFadeMinimumInteger / 127.0f;
                    float sizeFadeMin = (float)decalRenderer->SizeFadeMinimumInteger / 127.0f;
                    float animTimeSecs = (float)decalRenderer->AnimationTimeSecsInteger / 16383.0f * 99.9f + 0.1f;
                    float flickerBias = (float)decalRenderer->FlickerBiasInteger / 255.0f * 2.0f - 1.0f;
                    float flickerSpeed = (float)decalRenderer->FlickerSpeedInteger / 4095.0f * 30.0f;

                    XMFLOAT4 dcStart = ColourToFloat4(decalRenderer->StartColour);
                    XMFLOAT4 dcMid   = ColourToFloat4(decalRenderer->MidColour);
                    XMFLOAT4 dcEnd   = ColourToFloat4(decalRenderer->EndColour);

                    float aspect = (decTexRes.FrameWidth > 0 && decTexRes.FrameHeight > 0) ? ((float)decTexRes.FrameHeight / (float)decTexRes.FrameWidth) : 1.0f;
                    int frameCount = (std::max)(1, (int)decTexRes.FrameCount);

                    for (const auto& d : state.Decals) {
                        if (!d.Active) continue;
                        float maxLife = (d.MaxLife > 0.0001f) ? d.MaxLife : ((decalRenderer->DecalLifeSecs > 0.01f) ? decalRenderer->DecalLifeSecs : 1.0f);
                        float t = std::clamp(d.Age / maxLife, 0.0f, 1.0f - 0.0001f);

                        XMVECTOR nrm = XMVector3Normalize(XMLoadFloat3(&d.Normal));
                        if (XMVectorGetX(XMVector3LengthSq(nrm)) < 1e-6f) nrm = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
                        XMVECTOR center = XMLoadFloat3(&d.Position) + nrm * 0.015f;

                        XMFLOAT3 centerF;
                        XMStoreFloat3(&centerF, center);
                        XMVECTOR posV = XMVector3TransformCoord(XMVectorSet(centerF.x, centerF.y, centerF.z, 1.0f), viewMat);
                        float vz = XMVectorGetZ(posV);
                        if (vz <= 0.1f) continue;
                        float camDist = XMVectorGetX(XMVector3Length(posV));

                        XMFLOAT4 col = EvaluateColorSpline511(dcStart, dcMid, dcEnd, decalRenderer->UseStartColour, decalRenderer->UseMidColour, decalRenderer->UseEndColour, t);
                        if (decalRenderer->AlphaFadeEnable) {
                            col.w *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, alphaFadeMin);
                        }
                        col.w *= d.AlphaScale;

                        float dSize = (startSize + t * (endSize - startSize)) * d.SizeScale;
                        if (decalRenderer->SizeFadeEnable) {
                            dSize *= EvaluateCosineFade(t, fadeInEnd, fadeOutBegin, sizeFadeMin);
                        }

                        if (decalRenderer->FlickerEnable) {
                            float fSize = 1.0f;
                            col.w *= EvaluatePerlinFlicker(d.Age, d.FlickerTimeOffset, d.FlickerIndex,
                                                           flickerSpeed, flickerBias,
                                                           decalRenderer->FlickerMinAlphaInteger, decalRenderer->FlickerMinSizeInteger, fSize);
                            dSize *= fSize;
                        }

                        col.w *= sysAlpha * EvaluateEmitterDistanceFade(emitter, camDist);
                        dSize *= scalarSysScale;
                        if (col.w <= 0.0001f || dSize <= 0.00001f) continue;

                        float animProgress = t;
                        if (decalRenderer->ForceAnimationTime && animTimeSecs > 0.0001f) {
                            animProgress = std::fmod(d.Age / animTimeSecs, 1.0f);
                            if (animProgress < 0.0f) animProgress += 1.0f;
                        }
                        int frameIdx = (int)(animProgress * (float)frameCount + d.FrameOffset);
                        frameIdx = ((frameIdx % frameCount) + frameCount) % frameCount;

                        XMFLOAT4 uvBounds;
                        float slice = 0.0f;
                        GetFrameUVAndSlice(decTexRes, frameIdx, uvBounds, slice);

                        XMVECTOR fwd = XMLoadFloat3(&d.Forward);
                        XMVECTOR right = XMVector3Cross(nrm, fwd);
                        if (XMVectorGetX(XMVector3LengthSq(right)) < 1e-6f) {
                            right = XMVector3Cross(nrm, XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
                            if (XMVectorGetX(XMVector3LengthSq(right)) < 1e-6f) {
                                right = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
                            }
                        }
                        right = XMVector3Normalize(right);
                        fwd = XMVector3Normalize(XMVector3Cross(right, nrm));

                        float halfW = dSize * 0.5f;
                        float halfH = halfW * aspect;
                        XMVECTOR rVec = right * halfW;
                        XMVECTOR fVec = fwd * halfH;

                        if (quadVertices.size() + 4 > MaxParticleVertices) {
                            FlushQuads();
                        }

                        XMFLOAT3 p0, p1, p2, p3;
                        XMStoreFloat3(&p0, center - rVec + fVec);
                        XMStoreFloat3(&p1, center + rVec + fVec);
                        XMStoreFloat3(&p2, center + rVec - fVec);
                        XMStoreFloat3(&p3, center - rVec - fVec);

                        quadVertices.push_back({ p0, XMFLOAT3(uvBounds.x, uvBounds.y, slice), col });
                        quadVertices.push_back({ p1, XMFLOAT3(uvBounds.z, uvBounds.y, slice), col });
                        quadVertices.push_back({ p2, XMFLOAT3(uvBounds.z, uvBounds.w, slice), col });
                        quadVertices.push_back({ p3, XMFLOAT3(uvBounds.x, uvBounds.w, slice), col });
                    }
                    FlushQuads();
                }

                // ----------------------------------------------------
                // 5. Process Dynamic Light Visualization (CPSCLight)
                // ----------------------------------------------------
                if (lightComp && lightComp->LightEnabled && state.LightActive) {
                    bool boundLightState = false;
                    for (const auto& al : activeLights) {
                        if (al.SysIdx != sysIdx) continue;

                        if (!boundLightState) {
                            cb.IsDXT1 = 0;
                            cb.BlendMode = 3; // ADDITIVE
                            cb.HasBlackBg = 0;
                            cb.SelfIlluminating = 1;
                            ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);
                            ctx->PSSetShaderResources(0, 1, &DefaultGlowSRV);
                            BindBlendState(ctx, 3, 0);
                            BindDepthStateForBlendMode(ctx, 1, 0, 3);
                            boundLightState = true;
                        }

                        // 1) 3-layer volumetric radial glow billboard at light position
                        if (al.ViewZ > 0.1f) {
                            SpriteQuadParams glowParams;
                            glowParams.FaceMe3D = false;
                            glowParams.RotateAroundCentre = true;
                            glowParams.CrossedSprites = false;
                            glowParams.SpriteAlignment = 0;

                            float outerHalf = std::clamp(al.OuterRadius * 0.55f, 0.35f, 3.5f);
                            XMFLOAT4 outerCol(al.Color.x, al.Color.y, al.Color.z, std::clamp(al.Color.w * 0.28f, 0.0f, 1.0f));
                            EmitQuad(al.Position, outerHalf, outerHalf, 0.0f, outerCol, glowParams, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), 0.0f);

                            float midHalf = std::clamp(al.InnerRadius * 0.65f, 0.22f, 2.0f);
                            XMFLOAT4 midCol(al.Color.x, al.Color.y, al.Color.z, std::clamp(al.Color.w * 0.55f, 0.0f, 1.0f));
                            EmitQuad(al.Position, midHalf, midHalf, 0.0f, midCol, glowParams, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), 0.0f);

                            float coreHalf = std::clamp(al.InnerRadius * 0.28f, 0.10f, 0.85f);
                            XMFLOAT4 coreCol(
                                (std::min)(1.0f, al.Color.x * 0.7f + 0.3f),
                                (std::min)(1.0f, al.Color.y * 0.7f + 0.3f),
                                (std::min)(1.0f, al.Color.z * 0.7f + 0.3f),
                                std::clamp(al.Color.w * 0.90f, 0.0f, 1.0f));
                            EmitQuad(al.Position, coreHalf, coreHalf, 0.0f, coreCol, glowParams, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), 0.0f);
                        }

                        // 2) Soft additive ground illumination pool quad on z = 0.01f
                        float hGround = std::fabs(al.Position.z - 0.01f);
                        float maxGroundRange = al.OuterRadius * 1.5f;
                        if (hGround < maxGroundRange && maxGroundRange > 0.05f) {
                            float hRatio = std::clamp(hGround / maxGroundRange, 0.0f, 1.0f);
                            float attenPow = std::clamp(al.AttenuationFactor * 0.5f, 1.0f, 3.0f);
                            float groundFalloff = std::pow(1.0f - hRatio, attenPow);
                            float groundRadius = std::sqrt((std::max)(0.05f, al.OuterRadius * al.OuterRadius - hGround * hGround * 0.4f));
                            groundRadius = std::clamp(groundRadius, 0.25f, (std::max)(0.25f, al.OuterRadius));

                            XMFLOAT4 groundCol(al.Color.x, al.Color.y, al.Color.z, std::clamp(al.Color.w * 0.45f * groundFalloff, 0.0f, 1.0f));
                            if (groundCol.w > 0.001f) {
                                if (quadVertices.size() + 4 > MaxParticleVertices) {
                                    FlushQuads();
                                }
                                float gx = al.Position.x;
                                float gy = al.Position.y;
                                float gz = 0.01f;
                                quadVertices.push_back({ XMFLOAT3(gx - groundRadius, gy + groundRadius, gz), XMFLOAT3(0.0f, 0.0f, 0.0f), groundCol });
                                quadVertices.push_back({ XMFLOAT3(gx + groundRadius, gy + groundRadius, gz), XMFLOAT3(1.0f, 0.0f, 0.0f), groundCol });
                                quadVertices.push_back({ XMFLOAT3(gx + groundRadius, gy - groundRadius, gz), XMFLOAT3(1.0f, 1.0f, 0.0f), groundCol });
                                quadVertices.push_back({ XMFLOAT3(gx - groundRadius, gy - groundRadius, gz), XMFLOAT3(0.0f, 1.0f, 0.0f), groundCol });
                            }
                        }
                    }
                    if (boundLightState) {
                        FlushQuads();
                    }
                }
            }

            // Restore default depth and blend state for subsequent UI/grid passes
            ctx->OMSetDepthStencilState(DepthStateGrid, 0);
            float defBlend[4] = { 0, 0, 0, 0 };
            ctx->OMSetBlendState(BlendStateGrid, defBlend, 0xFFFFFFFF);
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
        for (int rz = 0; rz < 2; ++rz) {
            for (int wz = 0; wz < 2; ++wz) {
                if (DepthStates[rz][wz]) {
                    DepthStates[rz][wz]->Release();
                    DepthStates[rz][wz] = nullptr;
                }
            }
        }
        DepthStateParticle = nullptr;
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
