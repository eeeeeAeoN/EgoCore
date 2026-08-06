#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <filesystem>
#include <iostream>
#include "BankExplorer.h"
#include "resource.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

// For BETA Launch
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
ImFont* g_EditorFont = nullptr;
ImFont* g_CodeFont = nullptr;

MainMenuAudio g_MenuAudio;
ImTextureID g_MusicOnTexture = 0;
ImTextureID g_MusicOffTexture = 0;
ImTextureID g_SearchTexture = 0;
ImTextureID g_SaveTexture = 0;
ImTextureID g_DeleteTexture = 0;
ImTextureID g_PlayTexture = 0;
ImTextureID g_PauseTexture = 0;
ImTextureID g_StopTexture = 0;
ImTextureID g_LoopTexture = 0;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ID3D11ShaderResourceView* g_BackgroundTexture = nullptr;
int g_BgWidth = 0;
int g_BgHeight = 0;
ImFont* g_TitleFont = nullptr;

ID3D11ShaderResourceView* g_ModManagerBgTexture = nullptr;
int g_ModManagerBgWidth = 0;
int g_ModManagerBgHeight = 0;

ID3D11ShaderResourceView* g_CloudTexture = nullptr;
int g_CloudWidth = 0;
int g_CloudHeight = 0;

bool LoadTextureFromFile(const char* filename, ID3D11Device* d3dDevice, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height, float alphaBoost = 1.0f) {
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) return false;

    // Optionally amplify the alpha channel in-place. Useful for sprites (like soft fog/cloud
    // art) whose peak alpha is low by design - a tint color alone can only make AddImage()
    // draws MORE transparent, never less, since tint multiplies against the source alpha.
    if (alphaBoost != 1.0f) {
        int pixelCount = image_width * image_height;
        for (int i = 0; i < pixelCount; i++) {
            unsigned char* a = &image_data[i * 4 + 3];
            float boosted = (float)(*a) * alphaBoost;
            *a = (unsigned char)(boosted > 255.0f ? 255.0f : boosted);
        }
    }

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    d3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    d3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

int main(int, char**) {
    //InitDebugConsole();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"FableTool", nullptr };

    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    wc.hIconSm = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));

    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"EgoCore", WS_OVERLAPPEDWINDOW, 100, 100, 1000, 800, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); ::UnregisterClassW(wc.lpszClassName, wc.hInstance); return 1; }
    ::ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    if (std::filesystem::exists("Assets/Font.ttf")) {
        g_EditorFont = io.Fonts->AddFontFromFileTTF("Assets/Font.ttf", 18.0f);
    }
    else {
        g_EditorFont = io.Fonts->AddFontDefault();
    }

    if (std::filesystem::exists("Assets/TitleFont.ttf")) {
        g_TitleFont = io.Fonts->AddFontFromFileTTF("Assets/TitleFont.ttf", 32.0f);
    }

    if (std::filesystem::exists("Assets/CodeFont.ttf")) {
        g_CodeFont = io.Fonts->AddFontFromFileTTF("Assets/CodeFont.ttf", 18.0f);
    }

    if (std::filesystem::exists("Assets/WorldMap.png")) {
        LoadTextureFromFile("Assets/WorldMap.png", g_pd3dDevice, &g_BackgroundTexture, &g_BgWidth, &g_BgHeight);
    }

    if (std::filesystem::exists("Assets/ModManagerBackground.png")) {
        LoadTextureFromFile("Assets/ModManagerBackground.png", g_pd3dDevice, &g_ModManagerBgTexture, &g_ModManagerBgWidth, &g_ModManagerBgHeight);
    }

    if (std::filesystem::exists("Assets/Cloud.png")) {
        LoadTextureFromFile("Assets/Cloud.png", g_pd3dDevice, &g_CloudTexture, &g_CloudWidth, &g_CloudHeight, 4.0f);
    }

    int iconWidth = 0, iconHeight = 0;
    ID3D11ShaderResourceView* srvMusicOn = nullptr;
    ID3D11ShaderResourceView* srvMusicOff = nullptr;

    if (std::filesystem::exists("Assets/MusicOn.png")) {
        if (LoadTextureFromFile("Assets/MusicOn.png", g_pd3dDevice, &srvMusicOn, &iconWidth, &iconHeight)) {
            g_MusicOnTexture = (ImTextureID)srvMusicOn;
        }
    }

    if (std::filesystem::exists("Assets/MusicOff.png")) {
        if (LoadTextureFromFile("Assets/MusicOff.png", g_pd3dDevice, &srvMusicOff, &iconWidth, &iconHeight)) {
            g_MusicOffTexture = (ImTextureID)srvMusicOff;
        }
    }

    ID3D11ShaderResourceView* srvSearch = nullptr;
    if (std::filesystem::exists("Assets/Search.png")) {
        if (LoadTextureFromFile("Assets/Search.png", g_pd3dDevice, &srvSearch, &iconWidth, &iconHeight)) {
            g_SearchTexture = (ImTextureID)srvSearch;
        }
    }

    ID3D11ShaderResourceView* srvSave = nullptr;
    if (std::filesystem::exists("Assets/Save.png")) {
        if (LoadTextureFromFile("Assets/Save.png", g_pd3dDevice, &srvSave, &iconWidth, &iconHeight)) {
            g_SaveTexture = (ImTextureID)srvSave;
        }
    }

    ID3D11ShaderResourceView* srvDelete = nullptr;
    if (std::filesystem::exists("Assets/Delete.png")) {
        if (LoadTextureFromFile("Assets/Delete.png", g_pd3dDevice, &srvDelete, &iconWidth, &iconHeight)) {
            g_DeleteTexture = (ImTextureID)srvDelete;
        }
    }

    ID3D11ShaderResourceView* srvPlay = nullptr;
    if (std::filesystem::exists("Assets/Play.png")) {
        if (LoadTextureFromFile("Assets/Play.png", g_pd3dDevice, &srvPlay, &iconWidth, &iconHeight)) {
            g_PlayTexture = (ImTextureID)srvPlay;
        }
    }

    ID3D11ShaderResourceView* srvPause = nullptr;
    if (std::filesystem::exists("Assets/Pause.png")) {
        if (LoadTextureFromFile("Assets/Pause.png", g_pd3dDevice, &srvPause, &iconWidth, &iconHeight)) {
            g_PauseTexture = (ImTextureID)srvPause;
        }
    }

    ID3D11ShaderResourceView* srvStop = nullptr;
    if (std::filesystem::exists("Assets/Stop.png")) {
        if (LoadTextureFromFile("Assets/Stop.png", g_pd3dDevice, &srvStop, &iconWidth, &iconHeight)) {
            g_StopTexture = (ImTextureID)srvStop;
        }
    }

    ID3D11ShaderResourceView* srvLoop = nullptr;
    if (std::filesystem::exists("Assets/Loop.png")) {
        if (LoadTextureFromFile("Assets/Loop.png", g_pd3dDevice, &srvLoop, &iconWidth, &iconHeight)) {
            g_LoopTexture = (ImTextureID)srvLoop;
        }
    }

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Main", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoBackground);
        DrawBankExplorer();

        ImGui::End();

        ImGui::Render();
        const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    if (srvMusicOn) { srvMusicOn->Release();  srvMusicOn = nullptr; }
    if (srvMusicOff) { srvMusicOff->Release(); srvMusicOff = nullptr; }
    if (srvSearch) { srvSearch->Release(); srvSearch = nullptr; }
    if (srvSave) { srvSave->Release(); srvSave = nullptr; }
    if (srvDelete) { srvDelete->Release(); srvDelete = nullptr; }
    if (srvPlay) { srvPlay->Release(); srvPlay = nullptr; }
    if (srvPause) { srvPause->Release(); srvPause = nullptr; }
    if (srvStop) { srvStop->Release(); srvStop = nullptr; }
    if (srvLoop) { srvLoop->Release(); srvLoop = nullptr; }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd; ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2; sd.BufferDesc.Width = 0; sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd; sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT createDeviceFlags = 0; D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK) return false;
    CreateRenderTarget(); return true;
}
void CleanupDeviceD3D() { CleanupRenderTarget(); if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; } if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; } if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; } }
void CreateRenderTarget() { ID3D11Texture2D* pBackBuffer; g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)); g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView); pBackBuffer->Release(); }
void CleanupRenderTarget() { if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; } }

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;

    case WM_CLOSE:
    {
        // 1. Always check for unsaved definitions/banks first (this takes priority)
        if ((g_DefWorkspace.IsDirty() || HasUnsavedBankChanges()) && g_AppConfig.ShowUnsavedChangesWarning) {
            g_DefWorkspace.PendingNav = { DefAction::ExitProgram, "", -1 };
            g_DefWorkspace.TriggerUnsavedPopup = true;
            return 0; // don't close yet - the popup will handle it
        }

        // 2. If we are in the Frontend or ModsManager, and there are pending asset changes,
        //    show the asset changes popup instead of closing immediately.
        if ((g_CurrentAppState == EAppState::Frontend || g_CurrentAppState == EAppState::ModsManager) &&
            (g_AppConfig.ModSystemDirty || g_AppConfig.DefSystemDirty || g_AppConfig.TngSystemDirty))
        {
            g_TriggerAssetChangesExitPopup = true;
            return 0; // don't destroy yet - popup will decide
        }

        // 3. Otherwise, close normally.
        ::DestroyWindow(hWnd);
        return 0;
    }
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}