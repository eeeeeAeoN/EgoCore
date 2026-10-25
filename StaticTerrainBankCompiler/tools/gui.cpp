#include "gui.hpp"
#include "egostb/stb_compiler.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <shlobj.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <memory>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

// Forward declare ImGui message handler at global namespace
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace egostb {

// Global D3D11 state
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                               createDeviceFlags, featureLevelArray, 2,
                                               D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                                               &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (hr != S_OK) return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
    return true;
}

static void CleanupDeviceD3D() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

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
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Windows native dialog helpers
static std::string OpenFileDialog(HWND hwndOwner, const wchar_t* filter, const wchar_t* title) {
    wchar_t filename[MAX_PATH] = L"";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        int len = WideCharToMultiByte(CP_UTF8, 0, filename, -1, nullptr, 0, nullptr, nullptr);
        if (len > 1) {
            std::string result(len - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, filename, -1, &result[0], len, nullptr, nullptr);
            return result;
        }
    }
    return "";
}

static std::string SaveFileDialog(HWND hwndOwner, const wchar_t* filter, const wchar_t* defaultName, const wchar_t* title) {
    wchar_t filename[MAX_PATH] = L"";
    if (defaultName) wcsncpy_s(filename, defaultName, MAX_PATH - 1);
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameW(&ofn)) {
        int len = WideCharToMultiByte(CP_UTF8, 0, filename, -1, nullptr, 0, nullptr, nullptr);
        if (len > 1) {
            std::string result(len - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, filename, -1, &result[0], len, nullptr, nullptr);
            return result;
        }
    }
    return "";
}

static std::string PickFolderDialog(HWND hwndOwner, const wchar_t* title) {
    std::string result;
    IFileOpenDialog* pFileOpen = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen)))) {
        DWORD dwOptions = 0;
        if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions))) {
            pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }
        if (title) pFileOpen->SetTitle(title);
        if (SUCCEEDED(pFileOpen->Show(hwndOwner))) {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(pFileOpen->GetResult(&pItem))) {
                PWSTR pszFilePath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    int len = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, nullptr, 0, nullptr, nullptr);
                    if (len > 1) {
                        result.resize(len - 1);
                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &result[0], len, nullptr, nullptr);
                    }
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    return result;
}

static void OpenInExplorer(const std::string& path) {
    if (path.empty()) return;
    if (fs::exists(path)) {
        std::string param = "/select,\"" + path + "\"";
        ShellExecuteA(NULL, "open", "explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
    } else {
        auto parent = fs::path(path).parent_path();
        if (fs::exists(parent)) {
            ShellExecuteA(NULL, "open", parent.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
    }
}

// App State
struct GuiState {
    char fableRoot[512] = "";
    char wldPath[512] = "dean_2003.wld";
    char outputPath[512] = "";
    bool autoOutputPath = true;

    // Quality parameters
    float meshDetail = 2.0f;
    float fgDistance = 74.0f;
    float lodDistanceScale = 1.0f;
    int textureScaleIndex = 0; // 0=1x, 1=2x, 2=4x
    int maxLodBand = 7;
    bool shareLodBands = true;
    bool enableRangeCompression = true;
    bool outputIsm = true;
    bool highQualityDxt = false;
    bool generateMipmaps = false;

    int currentPreset = 0; // 0: Retail Default, 1: High Quality, 2: Ultra Visuals, 3: Fast Draft, 4: Custom

    // Detected worlds
    std::vector<std::string> detectedWorlds;
    int selectedDetectedWorld = -1;

    // Compilation thread state
    std::atomic<bool> isCompiling{ false };
    std::atomic<bool> cancelRequested{ false };
    std::atomic<float> progress{ 0.0f };
    std::mutex dataMutex;
    std::string statusText = "Ready";
    std::vector<std::string> logLines;
    bool autoScrollLog = true;
    char logFilter[128] = "";

    // Result
    bool hasResult = false;
    CompileResult lastResult;
    std::chrono::steady_clock::time_point startTime;
    double elapsedTime = 0.0;

    void updateDefaultOutput() {
        if (!autoOutputPath) return;
        fs::path p(wldPath);
        std::string stem = p.stem().string();
        if (stem.empty()) stem = "dean_2003";

        std::string dir;
        if (!std::string(fableRoot).empty()) {
            dir = (fs::path(fableRoot) / "Data" / "Levels").string();
        } else if (p.has_parent_path()) {
            dir = p.parent_path().string();
        } else {
            dir = (fs::path("Data") / "Levels").string();
        }

        std::string out = (fs::path(dir) / (stem + "_RT.stb")).string();
        strncpy_s(outputPath, out.c_str(), sizeof(outputPath) - 1);
    }

    void scanDetectedWorlds() {
        detectedWorlds.clear();
        selectedDetectedWorld = -1;
        if (strlen(fableRoot) == 0) return;

        fs::path levelsDir = fs::path(fableRoot) / "Data" / "Levels";
        if (fs::exists(levelsDir) && fs::is_directory(levelsDir)) {
            try {
                for (const auto& entry : fs::directory_iterator(levelsDir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".wld") {
                        detectedWorlds.push_back(entry.path().filename().string());
                    }
                }
            } catch (...) {}
        }
        std::sort(detectedWorlds.begin(), detectedWorlds.end());

        // Match current wld
        std::string currentStem = fs::path(wldPath).stem().string();
        for (int i = 0; i < (int)detectedWorlds.size(); ++i) {
            if (fs::path(detectedWorlds[i]).stem().string() == currentStem) {
                selectedDetectedWorld = i;
                break;
            }
        }
    }

    void applyPreset(int preset) {
        currentPreset = preset;
        switch (preset) {
        case 0: // Retail Default
            meshDetail = 2.0f;
            fgDistance = 74.0f;
            lodDistanceScale = 1.0f;
            textureScaleIndex = 0; // 1x
            maxLodBand = 7;
            shareLodBands = true;
            enableRangeCompression = true;
            outputIsm = true;
            highQualityDxt = false;
            generateMipmaps = false;
            break;
        case 1: // High Quality (Modern PC)
            meshDetail = 3.5f;
            fgDistance = 120.0f;
            lodDistanceScale = 1.5f;
            textureScaleIndex = 1; // 2x
            maxLodBand = 7;
            shareLodBands = false;
            enableRangeCompression = true;
            outputIsm = true;
            highQualityDxt = true;
            generateMipmaps = false;
            break;
        case 2: // Ultra Visuals
            meshDetail = 5.0f;
            fgDistance = 180.0f;
            lodDistanceScale = 2.0f;
            textureScaleIndex = 2; // 4x
            maxLodBand = 7;
            shareLodBands = false;
            enableRangeCompression = true;
            outputIsm = true;
            highQualityDxt = true;
            generateMipmaps = true;
            break;
        case 3: // Fast / Draft
            meshDetail = 1.0f;
            fgDistance = 50.0f;
            lodDistanceScale = 1.0f;
            textureScaleIndex = 0; // 1x
            maxLodBand = 6;
            shareLodBands = true;
            enableRangeCompression = true;
            outputIsm = false;
            highQualityDxt = false;
            generateMipmaps = false;
            break;
        default:
            break;
        }
    }

    void addLog(const std::string& line) {
        std::lock_guard<std::mutex> lock(dataMutex);
        logLines.push_back(line);
    }
};

static void SetupModernDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 5.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.WindowPadding = ImVec2(16, 16);
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(10, 8);
    style.ItemInnerSpacing = ImVec2(8, 6);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                  = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.54f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.10f, 0.11f, 0.14f, 0.90f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.10f, 0.11f, 0.14f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.85f, 0.70f, 0.30f, 0.30f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.20f, 0.20f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.26f, 0.32f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.14f, 0.16f, 0.20f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.10f, 0.11f, 0.13f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.25f, 0.26f, 0.32f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.35f, 0.36f, 0.44f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.42f, 0.44f, 0.54f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.75f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.70f, 0.58f, 0.28f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.95f, 0.80f, 0.38f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.35f, 0.30f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.45f, 0.38f, 0.20f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.20f, 0.20f, 0.26f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.32f, 0.28f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.45f, 0.38f, 0.20f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.85f, 0.70f, 0.30f, 0.30f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.85f, 0.70f, 0.30f, 0.50f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.95f, 0.80f, 0.38f, 0.80f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.70f, 0.58f, 0.28f, 0.30f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.85f, 0.70f, 0.30f, 0.70f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.95f, 0.80f, 0.38f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.90f, 0.75f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.75f, 0.35f, 1.00f);
}

static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

int runCompilerGui() {
    HRESULT hrCom = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // Hide dedicated console window if launched directly from Windows Explorer
    HWND consoleWnd = ::GetConsoleWindow();
    DWORD consolePid = 0;
    if (consoleWnd) {
        ::GetWindowThreadProcessId(consoleWnd, &consolePid);
        if (consolePid == ::GetCurrentProcessId()) {
            ::ShowWindow(consoleWnd, SW_HIDE);
        }
    }

    // Register Win32 Window Class
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        L"EgoSTBCompilerWindow", nullptr
    };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName,
        L"EgoSTB - Static Terrain Bank Compiler",
        WS_OVERLAPPEDWINDOW,
        150, 100, 840, 780,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        if (SUCCEEDED(hrCom)) CoUninitialize();
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    SetupModernDarkTheme();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    GuiState state;

    // Load initial Game Root from egocore_config.ini
    std::string iniRoot = StbCompiler::findGameRootFromIni();
    if (!iniRoot.empty()) {
        strncpy_s(state.fableRoot, iniRoot.c_str(), sizeof(state.fableRoot) - 1);
        state.scanDetectedWorlds();
    }
    state.updateDefaultOutput();

    std::unique_ptr<std::thread> compileThread;

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // Update elapsed time during compilation
        if (state.isCompiling.load()) {
            auto now = std::chrono::steady_clock::now();
            state.elapsedTime = std::chrono::duration<double>(now - state.startTime).count();
        }

        // Check if compilation finished on background thread
        if (compileThread && !state.isCompiling.load()) {
            if (compileThread->joinable()) {
                compileThread->join();
            }
            compileThread.reset();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Full window UI
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##MainWindow", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

        // Header Title Banner
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.78f, 1.0f, 1.00f));
        ImGui::SetWindowFontScale(1.15f);
        ImGui::TextUnformatted("EgoSTB: Multi-Map Fable STB Compiler");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("| Fable: The Lost Chapters Terrain & Quadtree Baker");
        ImGui::Separator();

        // Section 1: Files & Paths
        if (ImGui::CollapsingHeader("1. World & Path Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Fable Root directory
            ImGui::Text("Fable Game Root:");
            ImGui::SameLine();
            HelpMarker("Base game installation directory containing Data\\Levels and Data\\graphics\\graphics.big.");
            ImGui::SetNextItemWidth(-100.0f);
            if (ImGui::InputText("##FableRoot", state.fableRoot, sizeof(state.fableRoot))) {
                state.scanDetectedWorlds();
                state.updateDefaultOutput();
            }
            ImGui::SameLine();
            if (ImGui::Button("Browse...##Root", ImVec2(90, 0))) {
                std::string folder = PickFolderDialog(hwnd, L"Select Fable Installation Directory");
                if (!folder.empty()) {
                    strncpy_s(state.fableRoot, folder.c_str(), sizeof(state.fableRoot) - 1);
                    state.scanDetectedWorlds();
                    state.updateDefaultOutput();
                }
            }

            // Quick World selection from detected .wld files
            if (!state.detectedWorlds.empty()) {
                ImGui::Text("Detected Worlds:");
                ImGui::SameLine();
                HelpMarker("Found in Data\\Levels. Select from dropdown or use custom path below.");
                ImGui::SetNextItemWidth(260.0f);
                const char* preview = (state.selectedDetectedWorld >= 0 && state.selectedDetectedWorld < (int)state.detectedWorlds.size())
                    ? state.detectedWorlds[state.selectedDetectedWorld].c_str() : "Choose detected world...";
                if (ImGui::BeginCombo("##DetectedWld", preview)) {
                    for (int i = 0; i < (int)state.detectedWorlds.size(); ++i) {
                        const bool isSelected = (state.selectedDetectedWorld == i);
                        if (ImGui::Selectable(state.detectedWorlds[i].c_str(), isSelected)) {
                            state.selectedDetectedWorld = i;
                            strncpy_s(state.wldPath, state.detectedWorlds[i].c_str(), sizeof(state.wldPath) - 1);
                            state.updateDefaultOutput();
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
            }

            // Target World File (.wld)
            ImGui::Text("World File (.wld):");
            ImGui::SameLine();
            HelpMarker("The .wld world file to compile (e.g. dean_2003.wld or full path).");
            ImGui::SetNextItemWidth(-100.0f);
            if (ImGui::InputText("##WldPath", state.wldPath, sizeof(state.wldPath))) {
                state.updateDefaultOutput();
            }
            ImGui::SameLine();
            if (ImGui::Button("Browse...##Wld", ImVec2(90, 0))) {
                std::string file = OpenFileDialog(hwnd, L"Fable World Files (*.wld)\0*.wld\0All Files (*.*)\0*.*\0", L"Select World File (.wld)");
                if (!file.empty()) {
                    strncpy_s(state.wldPath, file.c_str(), sizeof(state.wldPath) - 1);

                    // If file is inside Data\Levels, auto deduce FableRoot
                    fs::path p(file);
                    fs::path parent = p.parent_path();
                    if (parent.filename() == "Levels" && parent.parent_path().filename() == "Data") {
                        std::string root = parent.parent_path().parent_path().string();
                        strncpy_s(state.fableRoot, root.c_str(), sizeof(state.fableRoot) - 1);
                        state.scanDetectedWorlds();
                    }
                    state.updateDefaultOutput();
                }
            }

            // Output STB file
            ImGui::Text("Output STB Path:");
            ImGui::SameLine();
            HelpMarker("Destination .stb file. By default saved to Data\\Levels\\<WorldName>_RT.stb");
            ImGui::SetNextItemWidth(-100.0f);
            ImGui::InputText("##OutputPath", state.outputPath, sizeof(state.outputPath));
            ImGui::SameLine();
            if (ImGui::Button("Browse...##Out", ImVec2(90, 0))) {
                std::string file = SaveFileDialog(hwnd, L"Static Terrain Bank (*.stb)\0*.stb\0All Files (*.*)\0*.*\0", L"dean_2003_RT.stb", L"Select Output STB Destination");
                if (!file.empty()) {
                    strncpy_s(state.outputPath, file.c_str(), sizeof(state.outputPath) - 1);
                    state.autoOutputPath = false;
                }
            }

            if (ImGui::Checkbox("Auto-sync output STB name with selected world", &state.autoOutputPath)) {
                if (state.autoOutputPath) state.updateDefaultOutput();
            }
        }

        // Section 2: Tuning & Quality Parameters
        if (ImGui::CollapsingHeader("2. Quality & Tuning Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Quality Preset Selector
            ImGui::Text("Quality Preset:");
            ImGui::SameLine();
            HelpMarker("Convenient presets for retail accuracy, enhanced graphics, or faster builds.");
            const char* presets[] = {
                "Retail Default (Original Game Accuracy)",
                "High Quality (Modern PC / Denser Meshes & 2x Textures)",
                "Ultra Visuals (4x Textures, Ultra Meshes, Mipmaps)",
                "Fast / Draft (Quick Compilation Test)",
                "Custom (User Defined)"
            };
            ImGui::SetNextItemWidth(360.0f);
            if (ImGui::Combo("##PresetCombo", &state.currentPreset, presets, IM_ARRAYSIZE(presets))) {
                state.applyPreset(state.currentPreset);
            }

            ImGui::Spacing();

            // Sliders
            ImGui::Columns(2, "ParamColumns", false);
            ImGui::SetColumnWidth(0, 420.0f);

            // Mesh Detail Error Factor
            ImGui::Text("Mesh Detail Error Factor:");
            ImGui::SameLine();
            HelpMarker("LOD geometric simplification error factor (default: 2.0). Higher values produce denser, higher-polygon terrain meshes.");
            if (ImGui::SliderFloat("##MeshDetail", &state.meshDetail, 0.2f, 6.0f, "%.2f")) {
                state.currentPreset = 4; // Custom
            }

            // Foreground Patch Distance
            ImGui::Text("Foreground Patch Distance (meters):");
            ImGui::SameLine();
            HelpMarker("Foreground patch culling threshold (default: 74.0 m). Higher values keep high-detail multi-textured terrain farther into the distance.");
            if (ImGui::SliderFloat("##FgDist", &state.fgDistance, 0.0f, 250.0f, "%.1f m")) {
                state.currentPreset = 4;
            }

            // Background LOD Distance Scale
            ImGui::Text("LOD Distance Scale:");
            ImGui::SameLine();
            HelpMarker("Multiplier for background LOD transition thresholds (default: 1.0). 1.5x or 2.0x pushes LOD transitions further away to eliminate pop-in.");
            if (ImGui::SliderFloat("##LodScale", &state.lodDistanceScale, 0.2f, 3.0f, "%.2fx")) {
                state.currentPreset = 4;
            }

            ImGui::NextColumn();

            // Max LOD Band
            ImGui::Text("Max LOD Simplification Band:");
            ImGui::SameLine();
            HelpMarker("Maximum quadtree simplification band to generate (1 to 7, default: 7).");
            if (ImGui::SliderInt("##MaxLodBand", &state.maxLodBand, 1, 7)) {
                state.currentPreset = 4;
            }

            // Reset Button
            if (ImGui::Button("Reset to Defaults", ImVec2(160, 0))) {
                state.applyPreset(0);
            }

            ImGui::Columns(1);
            ImGui::Spacing();

            // Feature Checkboxes
            ImGui::Columns(2, "CheckboxColumns", false);
            ImGui::SetColumnWidth(0, 390.0f);

            if (ImGui::Checkbox("Generate companion <wld>_RT.ism file", &state.outputIsm)) {
                state.currentPreset = 4;
            }
            ImGui::SameLine(); HelpMarker("Generates companion .ism file containing sea/water meshes and instanced trees/rocks.");

            if (ImGui::Checkbox("Share LOD bands 4-6 (Retail behavior)", &state.shareLodBands)) {
                state.currentPreset = 4;
            }
            ImGui::SameLine(); HelpMarker("When enabled, bands 4-6 share geometry, producing smaller STBs and matching retail game.");

            if (ImGui::Checkbox("Enable range compression for VB/IB streams", &state.enableRangeCompression)) {
                state.currentPreset = 4;
            }
            ImGui::SameLine(); HelpMarker("Compresses vertex and index buffers into retail format.");

            ImGui::NextColumn();

            if (ImGui::Checkbox("High Quality DXT1 compression", &state.highQualityDxt)) {
                state.currentPreset = 4;
            }
            ImGui::SameLine(); HelpMarker("Uses optimal perceptual cluster-fit compression instead of fast bounding-box.");

            if (ImGui::Checkbox("Generate complete mipmap chains", &state.generateMipmaps)) {
                state.currentPreset = 4;
            }
            ImGui::SameLine(); HelpMarker("Generates complete mipmaps for baked patch CTextures to eliminate distant shimmering.");

            ImGui::Columns(1);
        }

        // Section 3: Build & Execution Controls
        if (ImGui::CollapsingHeader("3. Compiler Execution & Progress", ImGuiTreeNodeFlags_DefaultOpen)) {
            const bool compiling = state.isCompiling.load();

            if (!compiling) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.58f, 0.32f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.72f, 0.40f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.48f, 0.26f, 1.0f));
                if (ImGui::Button("Compile STB", ImVec2(180, 34))) {
                    state.hasResult = false;
                    state.isCompiling = true;
                    state.cancelRequested = false;
                    state.progress = 0.0f;
                    state.statusText = "Initializing...";
                    state.startTime = std::chrono::steady_clock::now();
                    state.elapsedTime = 0.0;
                    state.logLines.clear();

                    // Prepare compiler config
                    CompilerConfig config;
                    config.fableRoot = state.fableRoot;
                    config.wldName = state.wldPath;
                    config.outputPath = state.outputPath;
                    config.retailMode = true;
                    config.verbose = true;
                    config.meshDetail = state.meshDetail;
                    config.fgDistanceThreshold = state.fgDistance;
                    config.lodDistanceScale = state.lodDistanceScale;
                    config.textureScale = 1; // Locked to default 1x to avoid crashing the game
                    config.maxLodBand = state.maxLodBand;
                    config.shareLodBands = state.shareLodBands;
                    config.enableRangeCompression = state.enableRangeCompression;
                    config.outputIsm = state.outputIsm;
                    config.highQualityDxt = state.highQualityDxt;
                    config.generateMipmaps = state.generateMipmaps;

                    config.cancelToken = &state.cancelRequested;
                    config.logCallback = [&state](const std::string& msg) {
                        state.addLog(msg);
                    };
                    config.progressCallback = [&state](float p, const std::string& status) {
                        state.progress = p;
                        std::lock_guard<std::mutex> lock(state.dataMutex);
                        state.statusText = status;
                    };

                    compileThread = std::make_unique<std::thread>([&state, config]() {
                        try {
                            auto res = StbCompiler::compileWorld(config);
                            std::lock_guard<std::mutex> lock(state.dataMutex);
                            state.lastResult = res;
                            state.hasResult = true;
                            state.statusText = res.success ? "Completed successfully!" : "Failed!";
                        } catch (const std::exception& e) {
                            std::lock_guard<std::mutex> lock(state.dataMutex);
                            state.lastResult.success = false;
                            state.lastResult.errorMessage = e.what();
                            state.hasResult = true;
                            state.statusText = "Aborted: " + std::string(e.what());
                        }
                        state.isCompiling = false;
                    });
                }
                ImGui::PopStyleColor(3);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.40f, 0.40f, 1.0f));
                ImGui::Button("Compiling...", ImVec2(180, 34));
                ImGui::PopStyleColor();

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.22f, 0.22f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.28f, 0.28f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.16f, 0.16f, 1.0f));
                if (ImGui::Button("Cancel Compilation", ImVec2(160, 34))) {
                    state.cancelRequested = true;
                    state.addLog("[User requested cancellation...]");
                }
                ImGui::PopStyleColor(3);
            }

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Elapsed: %.1fs", state.elapsedTime);

            // Progress bar and stage text
            float curProgress = state.progress.load();
            std::string curStatus;
            {
                std::lock_guard<std::mutex> lock(state.dataMutex);
                curStatus = state.statusText;
            }

            std::ostringstream progOverlay;
            progOverlay << std::fixed << std::setprecision(0) << (curProgress * 100.0f) << "% - " << curStatus;
            ImGui::ProgressBar(curProgress, ImVec2(-1, 22), progOverlay.str().c_str());

            // Result banner
            if (state.hasResult) {
                ImGui::Spacing();
                if (state.lastResult.success) {
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.32f, 0.18f, 0.85f));
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.70f, 0.35f, 1.0f));
                    ImGui::BeginChild("##SuccessBanner", ImVec2(0, 72), true);
                    ImGui::TextColored(ImVec4(0.35f, 1.0f, 0.50f, 1.0f), "COMPILATION SUCCEEDED in %.2fs!", state.lastResult.durationSeconds);
                    ImGui::Text("Output: %s (%.2f MB, %d maps, %d patches)",
                                state.lastResult.outputPath.c_str(),
                                state.lastResult.fileSize / (1024.0 * 1024.0),
                                state.lastResult.mapsCompiled,
                                state.lastResult.totalPatches);
                    if (ImGui::Button("Open in File Explorer")) {
                        OpenInExplorer(state.lastResult.outputPath);
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.38f, 0.14f, 0.14f, 0.85f));
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.80f, 0.25f, 0.25f, 1.0f));
                    ImGui::BeginChild("##FailureBanner", ImVec2(0, 56), true);
                    ImGui::TextColored(ImVec4(1.0f, 0.40f, 0.40f, 1.0f), "COMPILATION FAILED");
                    ImGui::TextWrapped("%s", state.lastResult.errorMessage.c_str());
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2);
                }
            }
        }

        // Section 4: Live Compiler Log Console
        if (ImGui::CollapsingHeader("4. Live Compiler Output Log", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Auto-scroll", &state.autoScrollLog);
            ImGui::SameLine();
            if (ImGui::Button("Clear Log")) {
                std::lock_guard<std::mutex> lock(state.dataMutex);
                state.logLines.clear();
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy to Clipboard")) {
                std::lock_guard<std::mutex> lock(state.dataMutex);
                std::string allLogs;
                for (const auto& l : state.logLines) {
                    allLogs += l + "\r\n";
                }
                ImGui::SetClipboardText(allLogs.c_str());
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputTextWithHint("##LogFilter", "Filter logs...", state.logFilter, sizeof(state.logFilter));

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.10f, 0.12f, 1.00f));
            ImGui::BeginChild("LogRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            {
                std::lock_guard<std::mutex> lock(state.dataMutex);
                std::string filterStr = state.logFilter;
                for (const auto& line : state.logLines) {
                    if (!filterStr.empty() && line.find(filterStr) == std::string::npos) {
                        continue;
                    }
                    if (line.find("ERROR") != std::string::npos || line.find("FAILED") != std::string::npos) {
                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", line.c_str());
                    } else if (line.find("Warning") != std::string::npos) {
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", line.c_str());
                    } else if (line.find("100% - Done") != std::string::npos || line.find("completed successfully") != std::string::npos) {
                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "%s", line.c_str());
                    } else {
                        ImGui::TextUnformatted(line.c_str());
                    }
                }
                if (state.autoScrollLog && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color[4] = { 0.10f, 0.11f, 0.13f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    if (compileThread) {
        state.cancelRequested = true;
        if (compileThread->joinable()) compileThread->join();
        compileThread.reset();
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    if (SUCCEEDED(hrCom)) CoUninitialize();

    return 0;
}

} // namespace egostb
