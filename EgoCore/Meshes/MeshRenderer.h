#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <vector>
#include <string>
#include "MeshParser.h"
#include "BBMParser.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

inline bool g_DebugDivIndices = true;

struct GPUVertex {
    XMFLOAT3 Pos;
    XMFLOAT3 Norm;
    XMFLOAT2 UV;
    uint32_t Joints;
    XMFLOAT4 Weights;
};

// Ensure this is 16-byte aligned!
struct CBMatrix {
    XMMATRIX WorldViewProj;
    XMMATRIX World;
    XMFLOAT4 Color;
    XMFLOAT4 LightDir;
    XMMATRIX BoneTransforms[256];
    int HasAnimation;
    float SelfIllum;
    int HasBump;    // NEW
    int HasSpec;    // NEW
    XMFLOAT3 CamPos;
    float Pad2;
};

struct RenderBatch {
    uint32_t IndexStart;
    uint32_t IndexCount;
    int32_t MaterialIndex;
};

class MeshRenderer {
public:
    struct RenderMaterial {
        ID3D11ShaderResourceView* Diffuse = nullptr;
        ID3D11ShaderResourceView* Bump = nullptr;
        ID3D11ShaderResourceView* Specular = nullptr;
        float SelfIllumination = 0.0f;
    };

private:
    ID3D11VertexShader* VS = nullptr; ID3D11PixelShader* PS = nullptr; ID3D11PixelShader* PS_Solid = nullptr;
    ID3D11InputLayout* Layout = nullptr;
    ID3D11Buffer* VBuffer = nullptr; ID3D11Buffer* IBuffer = nullptr; ID3D11Buffer* ConstantBuffer = nullptr;
    ID3D11RasterizerState* RastStateSolid = nullptr; ID3D11RasterizerState* RastStateWire = nullptr;
    ID3D11DepthStencilState* DepthState = nullptr;
    ID3D11BlendState* BlendState = nullptr;

    ID3D11SamplerState* Sampler = nullptr;
    ID3D11ShaderResourceView* DefaultWhiteSRV = nullptr;
    ID3D11ShaderResourceView* DefaultNormalSRV = nullptr;
    ID3D11ShaderResourceView* DefaultSpecSRV = nullptr;
    std::vector<RenderMaterial> Materials;

    ID3D11Texture2D* RenderTex = nullptr; ID3D11RenderTargetView* RTV = nullptr;
    ID3D11ShaderResourceView* SRV = nullptr; ID3D11Texture2D* DepthTex = nullptr; ID3D11DepthStencilView* DSV = nullptr;

    std::vector<RenderBatch> Batches;
    float Width = 0, Height = 0;

    ID3D11Buffer* DebugVBuffer = nullptr; ID3D11Buffer* DebugIBuffer = nullptr;
    uint32_t DebugBoxIndexCount = 0;
    uint32_t DebugCircleIndexCount = 0;
    uint32_t DebugCircleStartIndex = 0;

    void CreateDefaultTexture(ID3D11Device* device) {
        if (DefaultWhiteSRV) return;

        // 1. Default White (Diffuse)
        uint32_t white = 0xFFFFFFFF;
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = 1; desc.Height = 1; desc.MipLevels = 1; desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA data = { &white, 4, 0 };
        ID3D11Texture2D* tex = nullptr;
        if (SUCCEEDED(device->CreateTexture2D(&desc, &data, &tex))) {
            device->CreateShaderResourceView(tex, nullptr, &DefaultWhiteSRV);
            tex->Release();
        }

        // 2. Default Normal Map (Flat Normal: R=128, G=128, B=255)
        uint32_t flatNormal = 0xFFFF8080;
        D3D11_SUBRESOURCE_DATA normData = { &flatNormal, 4, 0 };
        if (SUCCEEDED(device->CreateTexture2D(&desc, &normData, &tex))) {
            device->CreateShaderResourceView(tex, nullptr, &DefaultNormalSRV);
            tex->Release();
        }

        // 3. Default Specular Map (Black/No reflection)
        uint32_t black = 0xFF000000;
        D3D11_SUBRESOURCE_DATA specData = { &black, 4, 0 };
        if (SUCCEEDED(device->CreateTexture2D(&desc, &specData, &tex))) {
            device->CreateShaderResourceView(tex, nullptr, &DefaultSpecSRV);
            tex->Release();
        }
    }

    void CreateDebugBuffers(ID3D11Device* device) {
        if (DebugVBuffer) return;
        std::vector<GPUVertex> verts; std::vector<uint32_t> inds;
        float h = 0.5f;
        XMFLOAT3 boxVerts[] = { {-h,-h,-h}, { h,-h,-h}, { h, h,-h}, {-h, h,-h}, {-h,-h, h}, { h,-h, h}, { h, h, h}, {-h, h, h} };
        for (auto& p : boxVerts) verts.push_back({ p, {0,0,0}, {0,0}, 0, {0,0,0,0} });
        uint32_t boxInds[] = { 0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7 };
        for (auto i : boxInds) inds.push_back(i);
        DebugBoxIndexCount = 24;

        DebugCircleStartIndex = (uint32_t)inds.size();
        uint32_t circleVertStart = (uint32_t)verts.size();
        int segments = 64;
        for (int i = 0; i < segments; i++) {
            float theta = (float)i / segments * XM_2PI;
            verts.push_back({ XMFLOAT3(cosf(theta), sinf(theta), 0.0f), {0,0,0}, {0,0}, 0, {0,0,0,0} });
            inds.push_back(circleVertStart + i); inds.push_back(circleVertStart + ((i + 1) % segments));
        }
        DebugCircleIndexCount = segments * 2;

        D3D11_BUFFER_DESC vDesc = {}; vDesc.ByteWidth = sizeof(GPUVertex) * (UINT)verts.size(); vDesc.Usage = D3D11_USAGE_IMMUTABLE; vDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vData = { verts.data(), 0, 0 }; device->CreateBuffer(&vDesc, &vData, &DebugVBuffer);

        D3D11_BUFFER_DESC iDesc = {}; iDesc.ByteWidth = sizeof(uint32_t) * (UINT)inds.size(); iDesc.Usage = D3D11_USAGE_IMMUTABLE; iDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iData = { inds.data(), 0, 0 }; device->CreateBuffer(&iDesc, &iData, &DebugIBuffer);
    }

public:
    float CamRotX = 0.0f; float CamRotY = 0.0f; float CamDist = 10.0f; XMFLOAT2 CamPan = { 0, 0 };

    ID3D11RenderTargetView* GetRTV() const { return RTV; }
    ID3D11DepthStencilView* GetDSV() const { return DSV; }

    ~MeshRenderer() { Release(); }
    void Release() {
        if (VS) VS->Release(); VS = nullptr;
        if (PS) PS->Release(); PS = nullptr;
        if (PS_Solid) PS_Solid->Release(); PS_Solid = nullptr;
        if (Layout) Layout->Release(); Layout = nullptr;
        if (VBuffer) VBuffer->Release(); VBuffer = nullptr; if (IBuffer) IBuffer->Release(); IBuffer = nullptr;
        if (ConstantBuffer) ConstantBuffer->Release(); ConstantBuffer = nullptr;
        if (RastStateSolid) RastStateSolid->Release(); RastStateSolid = nullptr;
        if (RastStateWire) RastStateWire->Release(); RastStateWire = nullptr;
        if (DepthState) DepthState->Release(); DepthState = nullptr;
        if (Sampler) Sampler->Release(); Sampler = nullptr;
        if (DefaultWhiteSRV) DefaultWhiteSRV->Release(); DefaultWhiteSRV = nullptr;
        if (DefaultNormalSRV) DefaultNormalSRV->Release(); DefaultNormalSRV = nullptr;
        if (DefaultSpecSRV) DefaultSpecSRV->Release(); DefaultSpecSRV = nullptr;
        if (BlendState) BlendState->Release(); BlendState = nullptr;
        if (DebugVBuffer) DebugVBuffer->Release(); DebugVBuffer = nullptr;
        if (DebugIBuffer) DebugIBuffer->Release(); DebugIBuffer = nullptr;
        ReleaseResizedResources();
    }
    void ReleaseResizedResources() {
        if (RenderTex) RenderTex->Release(); RenderTex = nullptr; if (RTV) RTV->Release(); RTV = nullptr;
        if (SRV) SRV->Release(); SRV = nullptr; if (DepthTex) DepthTex->Release(); DepthTex = nullptr;
        if (DSV) DSV->Release(); DSV = nullptr;
    }

    bool Initialize(ID3D11Device* device) {
        if (VS) return true;

        const char* shaderSrc = R"(
            cbuffer CBuf : register(b0) { 
                matrix WVP; matrix World; float4 Col; float4 LightDir; 
                matrix BoneTransforms[256];
                int HasAnimation; float SelfIllum; int HasBump; int HasSpec;
                float3 CamPos; float pad2;
            };
            struct VS_IN { 
                float3 Pos : POSITION; float3 Norm : NORMAL; float2 UV : TEXCOORD; 
                uint4 Joints : BLENDINDICES; float4 Weights : BLENDWEIGHT;
            };
            struct PS_IN { 
                float4 Pos : SV_POSITION; 
                float3 Norm : NORMAL; 
                float2 UV : TEXCOORD; 
                float3 WorldPos : TEXCOORD1; 
            };
            
            PS_IN VS(VS_IN input) { 
                PS_IN output; 
                float4 localPos = float4(input.Pos, 1.0f);
                float3 localNorm = input.Norm; // Make sure this isn't negative anymore!

                float wSum = input.Weights.x + input.Weights.y + input.Weights.z + input.Weights.w;
                if (HasAnimation == 1 && wSum > 0.001f) {
                    matrix boneMat = BoneTransforms[input.Joints.x] * input.Weights.x +
                                     BoneTransforms[input.Joints.y] * input.Weights.y +
                                     BoneTransforms[input.Joints.z] * input.Weights.z +
                                     BoneTransforms[input.Joints.w] * input.Weights.w;
                    
                    localPos = mul(localPos, boneMat);
                    localNorm = mul(localNorm, (float3x3)boneMat);
                }

                output.Pos = mul(localPos, WVP); 
                output.WorldPos = mul(localPos, World).xyz;
                output.Norm = mul(localNorm, (float3x3)World); 
                output.UV = input.UV;
                return output; 
            }
            
            Texture2D texDiffuse : register(t0); 
            Texture2D texBump : register(t1); 
            // texSpec has been removed to save GPU cycles!
            SamplerState sam : register(s0);

            float3x3 ComputeTBN(float3 p, float3 n, float2 uv) {
                float3 dp1 = ddx(p); float3 dp2 = ddy(p);
                float2 duv1 = ddx(uv); float2 duv2 = ddy(uv);
                float3 dp2perp = cross(dp2, n); float3 dp1perp = cross(n, dp1);
                float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
                float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
                float invMax = rsqrt(max(dot(T,T), dot(B,B)));
                return float3x3(T * invMax, B * invMax, n);
            }

            float4 PS(PS_IN input) : SV_Target { 
                float4 texColor = texDiffuse.Sample(sam, input.UV);
                clip(texColor.a - 0.1f);
                
                float3 N = normalize(input.Norm); 
                
                if (HasBump == 1) {
                    float4 bumpSample = texBump.Sample(sam, input.UV);
                    float3 localNorm = normalize(bumpSample.xyz * 2.0f - 1.0f);
                    
                    float3x3 TBN = ComputeTBN(input.WorldPos, N, input.UV);
                    N = normalize(mul(localNorm, TBN));

                    // Flip the final normal because Fable's UV winding inverts the TBN matrix!
                    N = -N; 
                }

                float3 V = normalize(CamPos - input.WorldPos);
                float3 L = V; 
                float diff = max(dot(N, L), 0.25f); 

                float spec = 0.0f;
                if (HasSpec == 1) {
                    // Because L == V, the half-vector H is also just V.
                    // A power of 24.0f and a multiplier of 0.45f gives a clean plastic/metal shine.
                    spec = pow(max(dot(N, V), 0.0), 24.0f) * 0.45f; 
                }

                float3 emissive = texColor.rgb * SelfIllum;
                float3 finalColor = (texColor.rgb * Col.rgb * diff) + float3(spec, spec, spec) + emissive;
                return float4(finalColor, texColor.a * Col.a); 
            }

            float4 PS_Solid(PS_IN input) : SV_Target { return Col; }
        )";

        ID3DBlob* vsBlob = nullptr; ID3DBlob* psBlob = nullptr; ID3DBlob* psSolidBlob = nullptr;
        D3DCompile(shaderSrc, strlen(shaderSrc), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VS);

        D3D11_INPUT_ELEMENT_DESC desc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        device->CreateInputLayout(desc, 5, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &Layout); vsBlob->Release();

        D3DCompile(shaderSrc, strlen(shaderSrc), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, nullptr);
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PS); psBlob->Release();

        D3DCompile(shaderSrc, strlen(shaderSrc), nullptr, nullptr, nullptr, "PS_Solid", "ps_4_0", 0, 0, &psSolidBlob, nullptr);
        device->CreatePixelShader(psSolidBlob->GetBufferPointer(), psSolidBlob->GetBufferSize(), nullptr, &PS_Solid); psSolidBlob->Release();

        D3D11_BUFFER_DESC cbDesc = {}; cbDesc.ByteWidth = sizeof(CBMatrix); cbDesc.Usage = D3D11_USAGE_DEFAULT; cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; device->CreateBuffer(&cbDesc, nullptr, &ConstantBuffer);
        D3D11_RASTERIZER_DESC rsDesc = {}; rsDesc.FillMode = D3D11_FILL_SOLID; rsDesc.CullMode = D3D11_CULL_NONE; rsDesc.DepthClipEnable = true; device->CreateRasterizerState(&rsDesc, &RastStateSolid);
        rsDesc.FillMode = D3D11_FILL_WIREFRAME; device->CreateRasterizerState(&rsDesc, &RastStateWire);
        D3D11_DEPTH_STENCIL_DESC dsDesc = {}; dsDesc.DepthEnable = true; dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; dsDesc.DepthFunc = D3D11_COMPARISON_LESS; device->CreateDepthStencilState(&dsDesc, &DepthState);

        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP; sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; sampDesc.MinLOD = 0; sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        device->CreateSamplerState(&sampDesc, &Sampler);

        D3D11_BLEND_DESC blendDesc = {}; blendDesc.RenderTarget[0].BlendEnable = TRUE; blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA; blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD; blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE; blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO; blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD; blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        device->CreateBlendState(&blendDesc, &BlendState);

        CreateDefaultTexture(device); CreateDebugBuffers(device);
        return true;
    }

    void SetMaterials(const std::vector<RenderMaterial>& materials) { Materials = materials; }

    void UploadMesh(ID3D11Device* device, const C3DMeshContent& mesh, bool resetCamera = true) {
        if (!mesh.IsParsed) return;
        if (VBuffer) VBuffer->Release(); VBuffer = nullptr; if (IBuffer) IBuffer->Release(); IBuffer = nullptr;
        std::vector<GPUVertex> vertices; std::vector<uint32_t> indices; uint32_t indexOffset = 0;
        Batches.clear();

        for (const auto& prim : mesh.Primitives) {
            uint32_t batchStart = (uint32_t)indices.size();

            int reps = (prim.RepeatingMeshReps > 1) ? prim.RepeatingMeshReps : 1;
            uint32_t totalVerts = prim.VertexCount * reps;

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
            }

            int blk = 0; int proc = 0;
            int limit = (prim.AnimatedBlocks.empty() ? 999999 : prim.AnimatedBlocks[0].VertexCount);

            for (int v = 0; v < totalVerts; v++) {
                if (hasBones && proc >= limit) { blk++; proc = 0; if (blk < prim.AnimatedBlocks.size()) limit = prim.AnimatedBlocks[blk].VertexCount; }
                proc++;

                size_t offset = v * prim.VertexStride; if (offset + 12 > prim.VertexBuffer.size()) break;
                GPUVertex gpuV = {};

                if (isPosComp) { uint32_t p = *(uint32_t*)(prim.VertexBuffer.data() + offset); UnpackPOSPACKED3(p, prim.Compression.Scale, prim.Compression.Offset, gpuV.Pos.x, gpuV.Pos.y, gpuV.Pos.z); }
                else { const float* r = (const float*)(prim.VertexBuffer.data() + offset); gpuV.Pos = XMFLOAT3(r[0], r[1], r[2]); }

                if (hasNormals && offset + normOff + 4 <= prim.VertexBuffer.size()) {
                    if (isNormComp) { uint32_t p = *(uint32_t*)(prim.VertexBuffer.data() + offset + normOff); UnpackNORMPACKED3(p, gpuV.Norm.x, gpuV.Norm.y, gpuV.Norm.z); }
                    else { const float* r = (const float*)(prim.VertexBuffer.data() + offset + normOff); gpuV.Norm = XMFLOAT3(r[0], r[1], r[2]); }
                }
                else gpuV.Norm = XMFLOAT3(0, 1, 0);

                if (offset + uvOff + 4 <= prim.VertexBuffer.size()) {
                    if (isNormComp) {
                        if (mesh.MeshType == 4) {
                            uint16_t* u = (uint16_t*)(prim.VertexBuffer.data() + offset + uvOff);
                            gpuV.UV = XMFLOAT2((float)u[0] / 65535.0f, (float)u[1] / 65535.0f);
                        }
                        else {
                            int16_t* u = (int16_t*)(prim.VertexBuffer.data() + offset + uvOff);
                            gpuV.UV = XMFLOAT2(DecompressUV(u[0]), DecompressUV(u[1]));
                        }
                    }
                    else {
                        const float* r = (const float*)(prim.VertexBuffer.data() + offset + uvOff);
                        gpuV.UV = XMFLOAT2(r[0], r[1]);
                    }
                }
                else gpuV.UV = XMFLOAT2(0, 0);

                if (hasBones && offset + iOff + 4 <= prim.VertexBuffer.size()) {
                    uint8_t* wgt = (uint8_t*)(prim.VertexBuffer.data() + offset + wOff);
                    uint8_t* ind = (uint8_t*)(prim.VertexBuffer.data() + offset + iOff);

                    uint8_t j[4] = { 0,0,0,0 };
                    for (int k = 0; k < 4; k++) {
                        uint16_t pID = ind[k] / 3;
                        uint16_t lID = pID;

                        if (blk < prim.AnimatedBlocks.size() && !prim.AnimatedBlocks[blk].Groups.empty()) {
                            if (pID < prim.AnimatedBlocks[blk].Groups.size()) lID = prim.AnimatedBlocks[blk].Groups[pID];
                            else lID = 0;
                        }
                        if (lID >= mesh.BoneCount || lID >= 256) lID = 0;
                        j[k] = (uint8_t)lID;
                    }
                    gpuV.Joints = (j[3] << 24) | (j[2] << 16) | (j[1] << 8) | j[0];

                    float w0 = wgt[0] / 255.0f; float w1 = wgt[1] / 255.0f;
                    float w2 = wgt[2] / 255.0f; float w3 = wgt[3] / 255.0f;
                    float sum = w0 + w1 + w2 + w3;

                    if (sum > 0.01f) {
                        gpuV.Weights = XMFLOAT4(w0 / sum, w1 / sum, w2 / sum, w3 / sum);
                    }
                    else {
                        gpuV.Weights = XMFLOAT4(0, 0, 0, 0);
                    }
                }
                else {
                    gpuV.Joints = 0;
                    gpuV.Weights = XMFLOAT4(0, 0, 0, 0);
                }

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

                // Emit the batch for this specific block!
                uint32_t blockIndexCount = (uint32_t)indices.size() - blockBatchStart;
                if (blockIndexCount > 0) {
                    Batches.push_back({ blockBatchStart, blockIndexCount, matIdx });
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

            RenderBatch batch; batch.IndexStart = batchStart; batch.IndexCount = (uint32_t)indices.size() - batchStart; batch.MaterialIndex = prim.MaterialIndex; Batches.push_back(batch);
        }

        if (vertices.empty()) return;
        D3D11_BUFFER_DESC vDesc = {}; vDesc.ByteWidth = sizeof(GPUVertex) * (UINT)vertices.size(); vDesc.Usage = D3D11_USAGE_DEFAULT; vDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vData = { vertices.data(), 0, 0 }; device->CreateBuffer(&vDesc, &vData, &VBuffer);
        D3D11_BUFFER_DESC iDesc = {}; iDesc.ByteWidth = sizeof(uint32_t) * (UINT)indices.size(); iDesc.Usage = D3D11_USAGE_DEFAULT; iDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iData = { indices.data(), 0, 0 }; device->CreateBuffer(&iDesc, &iData, &IBuffer);

        if (resetCamera) {
            CamDist = (mesh.BoundingSphereRadius > 0) ? mesh.BoundingSphereRadius * 2.0f : 10.0f;
            CamPan = { 0, 0 };
            CamRotX = -XM_PIDIV2;
            CamRotY = XM_PI;
        }
    }

    void UploadBBM(ID3D11Device* device, const CBBMParser& bbm, bool resetCamera = true) {
        if (!bbm.IsParsed || bbm.ParsedVertices.empty()) return;
        if (VBuffer) VBuffer->Release(); VBuffer = nullptr; if (IBuffer) IBuffer->Release(); IBuffer = nullptr;
        Batches.clear();
        std::vector<GPUVertex> vertices; float maxDistSq = 0.0f;
        for (const auto& v : bbm.ParsedVertices) {
            GPUVertex g = {}; g.Pos = XMFLOAT3(v.Position.x, v.Position.y, v.Position.z); g.Norm = XMFLOAT3(v.Normal.x, v.Normal.y, v.Normal.z); g.UV = XMFLOAT2(v.UV.u, v.UV.v);
            g.Joints = 0; g.Weights = XMFLOAT4(1, 0, 0, 0);
            vertices.push_back(g); float d = g.Pos.x * g.Pos.x + g.Pos.y * g.Pos.y + g.Pos.z * g.Pos.z; if (d > maxDistSq) maxDistSq = d;
        }
        std::vector<uint32_t> indices; for (auto idx : bbm.ParsedIndices) indices.push_back(idx);
        D3D11_BUFFER_DESC vDesc = {}; vDesc.ByteWidth = sizeof(GPUVertex) * (UINT)vertices.size(); vDesc.Usage = D3D11_USAGE_DEFAULT; vDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vData = { vertices.data(), 0, 0 }; device->CreateBuffer(&vDesc, &vData, &VBuffer);
        D3D11_BUFFER_DESC iDesc = {}; iDesc.ByteWidth = sizeof(uint32_t) * (UINT)indices.size(); iDesc.Usage = D3D11_USAGE_DEFAULT; iDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iData = { indices.data(), 0, 0 }; device->CreateBuffer(&iDesc, &iData, &IBuffer);
        RenderBatch batch = { 0, (uint32_t)indices.size(), -1 }; Batches.push_back(batch);

        if (resetCamera) {
            float radius = sqrtf(maxDistSq);
            CamDist = (radius > 0) ? radius * 2.5f : 20.0f;
            if (CamDist < 1.0f) CamDist = 10.0f;
            CamPan = { 0, 0 };
            CamRotX = 0.2f;
            CamRotY = XM_PI;
        }
    }

    void Resize(ID3D11Device* device, float w, float h) {
        if (w == Width && h == Height && RenderTex) return;
        ReleaseResizedResources(); Width = w; Height = h; if (w <= 0 || h <= 0) return;
        D3D11_TEXTURE2D_DESC desc = {}; desc.Width = (UINT)w; desc.Height = (UINT)h; desc.MipLevels = 1; desc.ArraySize = 1; desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        device->CreateTexture2D(&desc, nullptr, &RenderTex); device->CreateRenderTargetView(RenderTex, nullptr, &RTV); device->CreateShaderResourceView(RenderTex, nullptr, &SRV);
        D3D11_TEXTURE2D_DESC dDesc = desc; dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        device->CreateTexture2D(&dDesc, nullptr, &DepthTex); device->CreateDepthStencilView(DepthTex, nullptr, &DSV);
    }

    ID3D11ShaderResourceView* Render(ID3D11DeviceContext* ctx, float w, float h, bool showWireframe, bool isPhysics = false, const std::vector<XMMATRIX>* animatedBones = nullptr, bool clearTarget = true, float alpha = 1.0f, ID3D11RenderTargetView* overrideRTV = nullptr, ID3D11DepthStencilView* overrideDSV = nullptr) {
        if (!VS || !VBuffer) return nullptr;
        if (ImGui::IsWindowHovered()) {
            if (ImGui::IsMouseDragging(1)) { CamRotY += ImGui::GetIO().MouseDelta.x * 0.01f; CamRotX += ImGui::GetIO().MouseDelta.y * 0.01f; }
            if (ImGui::IsMouseDragging(2)) { CamPan.x += ImGui::GetIO().MouseDelta.x * (CamDist * 0.002f); CamPan.y -= ImGui::GetIO().MouseDelta.y * (CamDist * 0.002f); }
            CamDist -= ImGui::GetIO().MouseWheel * CamDist * 0.1f; if (CamDist < 0.1f) CamDist = 0.1f;
        }

        ID3D11RenderTargetView* activeRTV = overrideRTV ? overrideRTV : RTV;
        ID3D11DepthStencilView* activeDSV = overrideDSV ? overrideDSV : DSV;

        if (clearTarget) {
            float bgColor[4] = { isPhysics ? 0.15f : 0.1f, 0.12f, isPhysics ? 0.1f : 0.15f, 1.0f };
            ctx->ClearRenderTargetView(activeRTV, bgColor);
            ctx->ClearDepthStencilView(activeDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
        }

        ctx->OMSetRenderTargets(1, &activeRTV, activeDSV);
        D3D11_VIEWPORT vp = { 0, 0, w, h, 0.0f, 1.0f };
        ctx->RSSetViewports(1, &vp);

        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), w / h, 0.1f, 100000.0f);
        XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, -CamDist, 0), XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 1, 0, 0));
        XMMATRIX worldCam = XMMatrixRotationX(CamRotX) * XMMatrixRotationY(CamRotY) * XMMatrixTranslation(CamPan.x, CamPan.y, 0);

        if (isPhysics) {
            worldCam = XMMatrixRotationX(-XM_PIDIV2) * worldCam;
        }

        CBMatrix cb;
        cb.World = XMMatrixTranspose(worldCam);
        cb.WorldViewProj = XMMatrixTranspose(worldCam * view * proj);
        cb.Color = XMFLOAT4(isPhysics ? 0.0f : 0.8f, 0.8f, isPhysics ? 1.0f : 0.8f, alpha);
        cb.LightDir = XMFLOAT4(0.5f, 1.0f, -0.5f, 0.0f);
        cb.CamPos = XMFLOAT3(0.0f, 0.0f, -CamDist);

        cb.HasAnimation = (animatedBones && !animatedBones->empty()) ? 1 : 0;
        if (cb.HasAnimation) {
            for (size_t i = 0; i < animatedBones->size() && i < 256; i++) {
                cb.BoneTransforms[i] = XMMatrixTranspose((*animatedBones)[i]);
            }
        }
        else {
            for (int i = 0; i < 256; i++) cb.BoneTransforms[i] = XMMatrixIdentity();
        }

        UINT stride = sizeof(GPUVertex); UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &VBuffer, &stride, &offset);
        ctx->IASetIndexBuffer(IBuffer, DXGI_FORMAT_R32_UINT, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->IASetInputLayout(Layout);
        ctx->VSSetShader(VS, nullptr, 0);
        ctx->PSSetShader(PS, nullptr, 0);
        ctx->OMSetDepthStencilState(DepthState, 0);
        ctx->RSSetState(RastStateSolid);
        ctx->PSSetSamplers(0, 1, &Sampler);

        float blendFactor[4] = { 0,0,0,0 }; ctx->OMSetBlendState(BlendState, blendFactor, 0xffffffff);

        for (const auto& batch : Batches) {
            ID3D11ShaderResourceView* srvs[3] = { DefaultWhiteSRV, DefaultNormalSRV, DefaultSpecSRV };
            float matIllum = 0.0f;

            // Reset flags per batch
            cb.HasBump = 0;
            cb.HasSpec = 0;

            if (batch.MaterialIndex >= 0 && batch.MaterialIndex < Materials.size()) {
                if (Materials[batch.MaterialIndex].Diffuse) {
                    srvs[0] = Materials[batch.MaterialIndex].Diffuse;
                }
                if (Materials[batch.MaterialIndex].Bump) {
                    srvs[1] = Materials[batch.MaterialIndex].Bump;
                    cb.HasBump = 1; // Flag On!
                }
                if (Materials[batch.MaterialIndex].Specular) {
                    srvs[2] = Materials[batch.MaterialIndex].Specular;
                    cb.HasSpec = 1; // Flag On!
                }
                matIllum = Materials[batch.MaterialIndex].SelfIllumination;
            }

            cb.SelfIllum = matIllum;
            ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);
            ctx->VSSetConstantBuffers(0, 1, &ConstantBuffer);
            ctx->PSSetConstantBuffers(0, 1, &ConstantBuffer);

            ctx->PSSetShaderResources(0, 3, srvs);
            ctx->DrawIndexed(batch.IndexCount, batch.IndexStart, 0);
        }

        if (showWireframe) {
            ctx->RSSetState(RastStateWire);
            ID3D11ShaderResourceView* whiteSRVs[3] = { DefaultWhiteSRV, DefaultNormalSRV, DefaultSpecSRV };
            ctx->PSSetShaderResources(0, 3, whiteSRVs);
            cb.Color = XMFLOAT4(0.0f, 0.0f, 0.0f, alpha);
            cb.SelfIllum = 0.0f;
            ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);
            for (const auto& batch : Batches) ctx->DrawIndexed(batch.IndexCount, batch.IndexStart, 0);
        }
        return SRV;
    }

    void RenderBounds(ID3D11DeviceContext* ctx, float w, float h, const float* bMin, const float* bMax, const float* sCenter, float sRadius) {
        if (!VS || !DebugVBuffer) return;

        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        ctx->PSSetShader(PS_Solid, nullptr, 0);
        ctx->RSSetState(RastStateSolid);

        UINT stride = sizeof(GPUVertex); UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &DebugVBuffer, &stride, &offset);
        ctx->IASetIndexBuffer(DebugIBuffer, DXGI_FORMAT_R32_UINT, 0);

        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), w / h, 0.1f, 100000.0f);
        XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, -CamDist, 0), XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 1, 0, 0));
        XMMATRIX worldCam = XMMatrixRotationX(CamRotX) * XMMatrixRotationY(CamRotY) * XMMatrixTranslation(CamPan.x, CamPan.y, 0);

        CBMatrix cb;
        cb.HasAnimation = 0;

        float bSize[3] = { bMax[0] - bMin[0], bMax[1] - bMin[1], bMax[2] - bMin[2] };
        float bCenter[3] = { bMin[0] + bSize[0] * 0.5f, bMin[1] + bSize[1] * 0.5f, bMin[2] + bSize[2] * 0.5f };

        XMMATRIX boxWorld = XMMatrixScaling(bSize[0], bSize[1], bSize[2]) * XMMatrixTranslation(bCenter[0], bCenter[1], bCenter[2]) * worldCam;
        cb.World = XMMatrixTranspose(boxWorld);
        cb.WorldViewProj = XMMatrixTranspose(boxWorld * view * proj);
        cb.Color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
        ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);
        ctx->DrawIndexed(DebugBoxIndexCount, 0, 0);

        XMMATRIX sphereScaleTrans = XMMatrixScaling(sRadius, sRadius, sRadius) * XMMatrixTranslation(sCenter[0], sCenter[1], sCenter[2]);
        cb.Color = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);

        XMMATRIX ring1 = sphereScaleTrans * worldCam;
        cb.WorldViewProj = XMMatrixTranspose(ring1 * view * proj);
        ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);
        ctx->DrawIndexed(DebugCircleIndexCount, DebugCircleStartIndex, 0);

        XMMATRIX ring2 = XMMatrixRotationX(XM_PIDIV2) * sphereScaleTrans * worldCam;
        cb.WorldViewProj = XMMatrixTranspose(ring2 * view * proj);
        ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);
        ctx->DrawIndexed(DebugCircleIndexCount, DebugCircleStartIndex, 0);

        XMMATRIX ring3 = XMMatrixRotationY(XM_PIDIV2) * sphereScaleTrans * worldCam;
        cb.WorldViewProj = XMMatrixTranspose(ring3 * view * proj);
        ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);
        ctx->DrawIndexed(DebugCircleIndexCount, DebugCircleStartIndex, 0);

        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void RenderVolumes(ID3D11DeviceContext* ctx, float w, float h, const std::vector<CPlane>& planes, bool isPhysics = false) {
        if (!VS || !DebugVBuffer || planes.empty()) return;

        std::vector<XMFLOAT3> vertices;
        for (size_t i = 0; i < planes.size(); i++) {
            for (size_t j = i + 1; j < planes.size(); j++) {
                for (size_t k = j + 1; k < planes.size(); k++) {
                    XMVECTOR n1 = XMVectorSet(planes[i].Normal[0], planes[i].Normal[1], planes[i].Normal[2], 0);
                    XMVECTOR n2 = XMVectorSet(planes[j].Normal[0], planes[j].Normal[1], planes[j].Normal[2], 0);
                    XMVECTOR n3 = XMVectorSet(planes[k].Normal[0], planes[k].Normal[1], planes[k].Normal[2], 0);

                    XMVECTOR n2xn3 = XMVector3Cross(n2, n3);
                    float det = XMVectorGetX(XMVector3Dot(n1, n2xn3));

                    if (std::abs(det) > 0.0001f) {
                        XMVECTOR n3xn1 = XMVector3Cross(n3, n1);
                        XMVECTOR n1xn2 = XMVector3Cross(n1, n2);

                        XMVECTOR p = (n2xn3 * -planes[i].D + n3xn1 * -planes[j].D + n1xn2 * -planes[k].D) / det;

                        bool valid = true;
                        for (size_t m = 0; m < planes.size(); m++) {
                            float dist = XMVectorGetX(p) * planes[m].Normal[0] + XMVectorGetY(p) * planes[m].Normal[1] + XMVectorGetZ(p) * planes[m].Normal[2] + planes[m].D;
                            if (dist > 0.01f) { valid = false; break; }
                        }

                        if (valid) vertices.push_back(XMFLOAT3(XMVectorGetX(p), XMVectorGetY(p), XMVectorGetZ(p)));
                    }
                }
            }
        }

        if (vertices.empty()) return;

        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        ctx->PSSetShader(PS_Solid, nullptr, 0);
        ctx->RSSetState(RastStateSolid);

        std::vector<GPUVertex> gpuVerts;
        std::vector<uint32_t> gpuInds;

        for (const auto& v : vertices) {
            XMFLOAT3 flippedV = { -v.x, -v.y, -v.z };
            gpuVerts.push_back({ flippedV, {0,0,0}, {0,0}, 0, {0,0,0,0} });
        }

        for (uint32_t i = 0; i < vertices.size(); i++) {
            for (uint32_t j = i + 1; j < vertices.size(); j++) {
                int sharedPlanes = 0;
                for (const auto& plane : planes) {
                    float d1 = vertices[i].x * plane.Normal[0] + vertices[i].y * plane.Normal[1] + vertices[i].z * plane.Normal[2] + plane.D;
                    float d2 = vertices[j].x * plane.Normal[0] + vertices[j].y * plane.Normal[1] + vertices[j].z * plane.Normal[2] + plane.D;
                    if (std::abs(d1) < 0.01f && std::abs(d2) < 0.01f) sharedPlanes++;
                }
                if (sharedPlanes >= 2) { gpuInds.push_back(i); gpuInds.push_back(j); }
            }
        }

        if (gpuInds.empty()) return;

        ID3D11Device* device = nullptr;
        ctx->GetDevice(&device);
        if (!device) return;

        ID3D11Buffer* vBuf = nullptr, * iBuf = nullptr;
        D3D11_BUFFER_DESC vDesc = {}; vDesc.ByteWidth = sizeof(GPUVertex) * (UINT)gpuVerts.size(); vDesc.Usage = D3D11_USAGE_IMMUTABLE; vDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vData = { gpuVerts.data(), 0, 0 }; device->CreateBuffer(&vDesc, &vData, &vBuf);

        D3D11_BUFFER_DESC iDesc = {}; iDesc.ByteWidth = sizeof(uint32_t) * (UINT)gpuInds.size(); iDesc.Usage = D3D11_USAGE_IMMUTABLE; iDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iData = { gpuInds.data(), 0, 0 }; device->CreateBuffer(&iDesc, &iData, &iBuf);
        device->Release();

        UINT stride = sizeof(GPUVertex); UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &vBuf, &stride, &offset);
        ctx->IASetIndexBuffer(iBuf, DXGI_FORMAT_R32_UINT, 0);

        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), w / h, 0.1f, 100000.0f);
        XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, -CamDist, 0), XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 1, 0, 0));
        XMMATRIX worldCam = XMMatrixRotationX(CamRotX) * XMMatrixRotationY(CamRotY) * XMMatrixTranslation(CamPan.x, CamPan.y, 0);

        if (isPhysics) {
            worldCam = XMMatrixRotationX(-XM_PIDIV2) * worldCam;
        }

        CBMatrix cb;
        cb.HasAnimation = 0;
        cb.World = XMMatrixTranspose(worldCam);
        cb.WorldViewProj = XMMatrixTranspose(worldCam * view * proj);
        cb.Color = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);

        ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);
        ctx->DrawIndexed((UINT)gpuInds.size(), 0, 0);

        if (vBuf) vBuf->Release();
        if (iBuf) iBuf->Release();
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    bool ProjectToScreen(const XMFLOAT3& worldPos, ImVec2& outPos, float screenW, float screenH, bool isPhysics = false) {
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), screenW / screenH, 0.1f, 100000.0f);
        XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, -CamDist, 0), XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 1, 0, 0));
        XMMATRIX world = XMMatrixRotationX(CamRotX) * XMMatrixRotationY(CamRotY) * XMMatrixTranslation(CamPan.x, CamPan.y, 0);

        if (isPhysics) {
            world = XMMatrixRotationX(-XM_PIDIV2) * world;
        }

        XMMATRIX wvp = world * view * proj;
        XMVECTOR v = XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.0f);
        XMVECTOR vClip = XMVector3TransformCoord(v, wvp);

        float x = XMVectorGetX(vClip);
        float y = XMVectorGetY(vClip);
        float z = XMVectorGetZ(vClip);

        if (z < 0.0f || z > 1.0f) return false;
        outPos.x = (x + 1.0f) * 0.5f * screenW;
        outPos.y = (1.0f - y) * 0.5f * screenH;
        return true;
    }
};