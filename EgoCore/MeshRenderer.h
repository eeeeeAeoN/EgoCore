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

struct GPUVertex { XMFLOAT3 Pos; XMFLOAT3 Norm; XMFLOAT2 UV; };
struct CBMatrix { XMMATRIX WorldViewProj; XMMATRIX World; XMFLOAT4 Color; XMFLOAT4 LightDir; };

struct RenderBatch {
    uint32_t IndexStart;
    uint32_t IndexCount;
    int32_t MaterialIndex;
};

class MeshRenderer {
private:
    ID3D11VertexShader* VS = nullptr; ID3D11PixelShader* PS = nullptr; ID3D11InputLayout* Layout = nullptr;
    ID3D11Buffer* VBuffer = nullptr; ID3D11Buffer* IBuffer = nullptr; ID3D11Buffer* ConstantBuffer = nullptr;
    ID3D11RasterizerState* RastStateSolid = nullptr; ID3D11RasterizerState* RastStateWire = nullptr;
    ID3D11DepthStencilState* DepthState = nullptr;
    ID3D11BlendState* BlendState = nullptr; // [NEW] Blending

    ID3D11SamplerState* Sampler = nullptr;
    ID3D11ShaderResourceView* DefaultWhiteSRV = nullptr;
    std::vector<ID3D11ShaderResourceView*> MaterialTextures;

    ID3D11Texture2D* RenderTex = nullptr; ID3D11RenderTargetView* RTV = nullptr;
    ID3D11ShaderResourceView* SRV = nullptr; ID3D11Texture2D* DepthTex = nullptr; ID3D11DepthStencilView* DSV = nullptr;

    std::vector<RenderBatch> Batches;
    float Width = 0, Height = 0;
    float CamRotX = 0.0f; float CamRotY = 0.0f; float CamDist = 10.0f; XMFLOAT2 CamPan = { 0, 0 };

    void CreateDefaultTexture(ID3D11Device* device) {
        if (DefaultWhiteSRV) return;
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
    }

public:
    ~MeshRenderer() { Release(); }
    void Release() {
        if (VS) VS->Release(); VS = nullptr; if (PS) PS->Release(); PS = nullptr; if (Layout) Layout->Release(); Layout = nullptr;
        if (VBuffer) VBuffer->Release(); VBuffer = nullptr; if (IBuffer) IBuffer->Release(); IBuffer = nullptr;
        if (ConstantBuffer) ConstantBuffer->Release(); ConstantBuffer = nullptr;
        if (RastStateSolid) RastStateSolid->Release(); RastStateSolid = nullptr;
        if (RastStateWire) RastStateWire->Release(); RastStateWire = nullptr;
        if (DepthState) DepthState->Release(); DepthState = nullptr;
        if (Sampler) Sampler->Release(); Sampler = nullptr;
        if (DefaultWhiteSRV) DefaultWhiteSRV->Release(); DefaultWhiteSRV = nullptr;
        if (BlendState) BlendState->Release(); BlendState = nullptr;
        ReleaseResizedResources();
    }
    void ReleaseResizedResources() {
        if (RenderTex) RenderTex->Release(); RenderTex = nullptr; if (RTV) RTV->Release(); RTV = nullptr;
        if (SRV) SRV->Release(); SRV = nullptr; if (DepthTex) DepthTex->Release(); DepthTex = nullptr;
        if (DSV) DSV->Release(); DSV = nullptr;
    }

    bool Initialize(ID3D11Device* device) {
        if (VS) return true;

        // [FIX] Added clip(texColor.a - 0.1f) to shader
        // This effectively performs "Alpha Testing", discarding pixels that are transparent
        const char* shaderSrc = R"(
            cbuffer CBuf : register(b0) { matrix WVP; matrix World; float4 Col; float4 LightDir; };
            struct VS_IN { float3 Pos : POSITION; float3 Norm : NORMAL; float2 UV : TEXCOORD; };
            struct PS_IN { float4 Pos : SV_POSITION; float3 Norm : NORMAL; float2 UV : TEXCOORD; };
            
            PS_IN VS(VS_IN input) { 
                PS_IN output; 
                output.Pos = mul(float4(input.Pos, 1.0f), WVP); 
                output.Norm = mul(input.Norm, (float3x3)World); 
                output.UV = input.UV;
                return output; 
            }
            
            Texture2D tex : register(t0);
            SamplerState sam : register(s0);

            float4 PS(PS_IN input) : SV_Target { 
                float4 texColor = tex.Sample(sam, input.UV);
                
                // ALPHA TEST: Discard invisible pixels so they don't block the background
                clip(texColor.a - 0.1f);

                float3 n = normalize(input.Norm); 
                float3 l = normalize(LightDir.xyz); 
                float diff = max(dot(n, l), 0.2f); 
                
                return float4(texColor.rgb * Col.rgb * diff, texColor.a); 
            }
        )";

        ID3DBlob* vsBlob = nullptr; ID3DBlob* psBlob = nullptr;
        D3DCompile(shaderSrc, strlen(shaderSrc), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VS);
        D3D11_INPUT_ELEMENT_DESC desc[] = { { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 } };
        device->CreateInputLayout(desc, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &Layout); vsBlob->Release();
        D3DCompile(shaderSrc, strlen(shaderSrc), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, nullptr);
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PS); psBlob->Release();
        D3D11_BUFFER_DESC cbDesc = {}; cbDesc.ByteWidth = sizeof(CBMatrix); cbDesc.Usage = D3D11_USAGE_DEFAULT; cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; device->CreateBuffer(&cbDesc, nullptr, &ConstantBuffer);
        D3D11_RASTERIZER_DESC rsDesc = {}; rsDesc.FillMode = D3D11_FILL_SOLID; rsDesc.CullMode = D3D11_CULL_NONE; rsDesc.DepthClipEnable = true; device->CreateRasterizerState(&rsDesc, &RastStateSolid);
        rsDesc.FillMode = D3D11_FILL_WIREFRAME; device->CreateRasterizerState(&rsDesc, &RastStateWire);
        D3D11_DEPTH_STENCIL_DESC dsDesc = {}; dsDesc.DepthEnable = true; dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; dsDesc.DepthFunc = D3D11_COMPARISON_LESS; device->CreateDepthStencilState(&dsDesc, &DepthState);

        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0; sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        device->CreateSamplerState(&sampDesc, &Sampler);

        // Blend State (Still useful for partial transparency on edges)
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        device->CreateBlendState(&blendDesc, &BlendState);

        CreateDefaultTexture(device);
        return true;
    }

    void SetMaterialTextures(const std::vector<ID3D11ShaderResourceView*>& textures) {
        MaterialTextures = textures;
    }

    void UploadMesh(ID3D11Device* device, const C3DMeshContent& mesh) {
        if (!mesh.IsParsed) return;
        if (VBuffer) VBuffer->Release(); VBuffer = nullptr; if (IBuffer) IBuffer->Release(); IBuffer = nullptr;
        std::vector<GPUVertex> vertices; std::vector<uint32_t> indices; uint32_t indexOffset = 0;
        Batches.clear();

        for (const auto& prim : mesh.Primitives) {
            uint32_t batchStart = (uint32_t)indices.size();

            for (int v = 0; v < prim.VertexCount; v++) {
                size_t offset = v * prim.VertexStride; if (offset + 12 > prim.VertexBuffer.size()) break;
                GPUVertex gpuV;
                if (prim.IsCompressed) { uint32_t p = *(uint32_t*)(prim.VertexBuffer.data() + offset); UnpackPOSPACKED3(p, prim.Compression.Scale, prim.Compression.Offset, gpuV.Pos.x, gpuV.Pos.y, gpuV.Pos.z); }
                else { const float* r = (const float*)(prim.VertexBuffer.data() + offset); gpuV.Pos = XMFLOAT3(r[0], r[1], r[2]); }

                size_t normOff = prim.IsCompressed ? (prim.AnimatedBlockCount > 0 ? 12 : 4) : (prim.AnimatedBlockCount > 0 ? 20 : 12);
                if (offset + normOff + 4 <= prim.VertexBuffer.size()) {
                    if (prim.IsCompressed) { uint32_t p = *(uint32_t*)(prim.VertexBuffer.data() + offset + normOff); UnpackNORMPACKED3(p, gpuV.Norm.x, gpuV.Norm.y, gpuV.Norm.z); }
                    else { const float* r = (const float*)(prim.VertexBuffer.data() + offset + normOff); gpuV.Norm = XMFLOAT3(r[0], r[1], r[2]); }
                }
                else gpuV.Norm = XMFLOAT3(0, 1, 0);

                size_t uvOff = prim.IsCompressed ? (prim.AnimatedBlockCount > 0 ? 16 : 8) : (prim.AnimatedBlockCount > 0 ? 32 : 24);
                if (offset + uvOff + 4 <= prim.VertexBuffer.size()) {
                    if (prim.IsCompressed) { int16_t* u = (int16_t*)(prim.VertexBuffer.data() + offset + uvOff); gpuV.UV = XMFLOAT2(DecompressUV(u[0]), DecompressUV(u[1])); }
                    else { const float* r = (const float*)(prim.VertexBuffer.data() + offset + uvOff); gpuV.UV = XMFLOAT2(r[0], r[1]); }
                }
                else gpuV.UV = XMFLOAT2(0, 0);
                vertices.push_back(gpuV);
            }

            auto ProcessIndices = [&](uint32_t start, uint32_t count, bool isStrip) {
                if (isStrip) {
                    for (uint32_t k = 0; k < count; k++) {
                        uint16_t idx[3];
                        if (k % 2 == 0) { idx[0] = prim.IndexBuffer[start + k]; idx[1] = prim.IndexBuffer[start + k + 1]; idx[2] = prim.IndexBuffer[start + k + 2]; }
                        else { idx[0] = prim.IndexBuffer[start + k]; idx[1] = prim.IndexBuffer[start + k + 2]; idx[2] = prim.IndexBuffer[start + k + 1]; }
                        if (idx[0] == idx[1] || idx[1] == idx[2] || idx[0] == idx[2]) continue;
                        indices.push_back(idx[0] + indexOffset); indices.push_back(idx[1] + indexOffset); indices.push_back(idx[2] + indexOffset);
                    }
                }
                else { for (uint32_t k = 0; k < count * 3; k++) if (start + k < prim.IndexBuffer.size()) indices.push_back(prim.IndexBuffer[start + k] + indexOffset); }
                };
            bool processed = false;
            for (const auto& b : prim.StaticBlocks) { ProcessIndices(b.StartIndex, b.PrimitiveCount, b.IsStrip); processed = true; }
            for (const auto& b : prim.AnimatedBlocks) { ProcessIndices(b.StartIndex, b.PrimitiveCount, b.IsStrip); processed = true; }
            if (!processed && !prim.IndexBuffer.empty()) { ProcessIndices(0, (uint32_t)prim.IndexBuffer.size() / 3, false); }
            indexOffset += prim.VertexCount;

            RenderBatch batch;
            batch.IndexStart = batchStart;
            batch.IndexCount = (uint32_t)indices.size() - batchStart;
            batch.MaterialIndex = prim.MaterialIndex;
            Batches.push_back(batch);
        }

        if (vertices.empty()) return;
        D3D11_BUFFER_DESC vDesc = {}; vDesc.ByteWidth = sizeof(GPUVertex) * (UINT)vertices.size(); vDesc.Usage = D3D11_USAGE_DEFAULT; vDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vData = { vertices.data(), 0, 0 }; device->CreateBuffer(&vDesc, &vData, &VBuffer);
        D3D11_BUFFER_DESC iDesc = {}; iDesc.ByteWidth = sizeof(uint32_t) * (UINT)indices.size(); iDesc.Usage = D3D11_USAGE_DEFAULT; iDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iData = { indices.data(), 0, 0 }; device->CreateBuffer(&iDesc, &iData, &IBuffer);

        CamDist = (mesh.BoundingSphereRadius > 0) ? mesh.BoundingSphereRadius * 2.0f : 10.0f;
        CamPan = { 0, 0 }; CamRotX = -XM_PIDIV2; CamRotY = XM_PI;
    }

    void UploadBBM(ID3D11Device* device, const CBBMParser& bbm) {
        if (!bbm.IsParsed || bbm.ParsedVertices.empty()) return;
        if (VBuffer) VBuffer->Release(); VBuffer = nullptr; if (IBuffer) IBuffer->Release(); IBuffer = nullptr;
        Batches.clear();

        std::vector<GPUVertex> vertices; float maxDistSq = 0.0f;
        for (const auto& v : bbm.ParsedVertices) {
            GPUVertex g; g.Pos = XMFLOAT3(v.Position.x, v.Position.y, v.Position.z); g.Norm = XMFLOAT3(v.Normal.x, v.Normal.y, v.Normal.z); g.UV = XMFLOAT2(v.UV.u, v.UV.v);
            vertices.push_back(g); float d = g.Pos.x * g.Pos.x + g.Pos.y * g.Pos.y + g.Pos.z * g.Pos.z; if (d > maxDistSq) maxDistSq = d;
        }
        std::vector<uint32_t> indices; for (auto idx : bbm.ParsedIndices) indices.push_back(idx);

        D3D11_BUFFER_DESC vDesc = {}; vDesc.ByteWidth = sizeof(GPUVertex) * (UINT)vertices.size(); vDesc.Usage = D3D11_USAGE_DEFAULT; vDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vData = { vertices.data(), 0, 0 }; device->CreateBuffer(&vDesc, &vData, &VBuffer);
        D3D11_BUFFER_DESC iDesc = {}; iDesc.ByteWidth = sizeof(uint32_t) * (UINT)indices.size(); iDesc.Usage = D3D11_USAGE_DEFAULT; iDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iData = { indices.data(), 0, 0 }; device->CreateBuffer(&iDesc, &iData, &IBuffer);

        RenderBatch batch = { 0, (uint32_t)indices.size(), -1 };
        Batches.push_back(batch);

        float radius = sqrtf(maxDistSq); CamDist = (radius > 0) ? radius * 2.5f : 20.0f; if (CamDist < 1.0f) CamDist = 10.0f;
        CamPan = { 0, 0 }; CamRotX = -XM_PIDIV2; CamRotY = XM_PI;
    }

    void Resize(ID3D11Device* device, float w, float h) {
        if (w == Width && h == Height && RenderTex) return;
        ReleaseResizedResources(); Width = w; Height = h; if (w <= 0 || h <= 0) return;
        D3D11_TEXTURE2D_DESC desc = {}; desc.Width = (UINT)w; desc.Height = (UINT)h; desc.MipLevels = 1; desc.ArraySize = 1; desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        device->CreateTexture2D(&desc, nullptr, &RenderTex); device->CreateRenderTargetView(RenderTex, nullptr, &RTV); device->CreateShaderResourceView(RenderTex, nullptr, &SRV);
        D3D11_TEXTURE2D_DESC dDesc = desc; dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        device->CreateTexture2D(&dDesc, nullptr, &DepthTex); device->CreateDepthStencilView(DepthTex, nullptr, &DSV);
    }

    ID3D11ShaderResourceView* Render(ID3D11DeviceContext* ctx, float w, float h, bool showWireframe) {
        if (!VS || !VBuffer) return nullptr;
        if (ImGui::IsWindowHovered()) {
            if (ImGui::IsMouseDragging(1)) { CamRotY += ImGui::GetIO().MouseDelta.x * 0.01f; CamRotX += ImGui::GetIO().MouseDelta.y * 0.01f; }
            if (ImGui::IsMouseDragging(2)) { CamPan.x += ImGui::GetIO().MouseDelta.x * (CamDist * 0.002f); CamPan.y -= ImGui::GetIO().MouseDelta.y * (CamDist * 0.002f); }
            CamDist -= ImGui::GetIO().MouseWheel * CamDist * 0.1f; if (CamDist < 0.1f) CamDist = 0.1f;
        }
        float bgColor[] = { 0.1f, 0.12f, 0.15f, 1.0f }; ctx->ClearRenderTargetView(RTV, bgColor); ctx->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH, 1.0f, 0); ctx->OMSetRenderTargets(1, &RTV, DSV);
        D3D11_VIEWPORT vp = { 0, 0, w, h, 0.0f, 1.0f }; ctx->RSSetViewports(1, &vp);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), w / h, 0.1f, 100000.0f);
        XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, -CamDist, 0), XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 1, 0, 0));
        XMMATRIX world = XMMatrixRotationX(CamRotX) * XMMatrixRotationY(CamRotY); world = world * XMMatrixTranslation(CamPan.x, CamPan.y, 0);
        CBMatrix cb; cb.World = XMMatrixTranspose(world); cb.WorldViewProj = XMMatrixTranspose(world * view * proj);
        cb.Color = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f); cb.LightDir = XMFLOAT4(0.5f, 1.0f, -0.5f, 0.0f);
        ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);
        UINT stride = sizeof(GPUVertex); UINT offset = 0; ctx->IASetVertexBuffers(0, 1, &VBuffer, &stride, &offset);
        ctx->IASetIndexBuffer(IBuffer, DXGI_FORMAT_R32_UINT, 0); ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->IASetInputLayout(Layout); ctx->VSSetShader(VS, nullptr, 0); ctx->VSSetConstantBuffers(0, 1, &ConstantBuffer);
        ctx->PSSetShader(PS, nullptr, 0); ctx->PSSetConstantBuffers(0, 1, &ConstantBuffer);
        ctx->OMSetDepthStencilState(DepthState, 0); ctx->RSSetState(RastStateSolid);
        ctx->PSSetSamplers(0, 1, &Sampler);

        float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->OMSetBlendState(BlendState, blendFactor, 0xffffffff);

        for (const auto& batch : Batches) {
            ID3D11ShaderResourceView* tex = DefaultWhiteSRV;
            if (batch.MaterialIndex >= 0 && batch.MaterialIndex < MaterialTextures.size()) {
                if (MaterialTextures[batch.MaterialIndex]) tex = MaterialTextures[batch.MaterialIndex];
            }
            ctx->PSSetShaderResources(0, 1, &tex);
            ctx->DrawIndexed(batch.IndexCount, batch.IndexStart, 0);
        }

        if (showWireframe) {
            ctx->RSSetState(RastStateWire);
            ID3D11ShaderResourceView* white = DefaultWhiteSRV;
            ctx->PSSetShaderResources(0, 1, &white);
            cb.Color = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
            ctx->UpdateSubresource(ConstantBuffer, 0, nullptr, &cb, 0, 0);
            for (const auto& batch : Batches) ctx->DrawIndexed(batch.IndexCount, batch.IndexStart, 0);
        }
        return SRV;
    }
};