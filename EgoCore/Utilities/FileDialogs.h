#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <shlobj.h>
#include <shobjidl.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

static inline std::string OpenFileDialog(const char* filter = "Big Bank Files\0*.big\0All Files\0*.*\0") {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn) == TRUE) return std::string(ofn.lpstrFile);
    return "";
}

static inline std::string OpenFolderDialog(const char* title = "Select Game Data Folder") {
    std::string resultPath;

    // Running on a dedicated thread with COINIT_APARTMENTTHREADED ensures a pristine STA
    // COM environment regardless of the caller thread's COM state, completely avoiding deadlocks.
    std::thread dialogThread([&]() {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        bool coInitialized = SUCCEEDED(hr);

        IFileOpenDialog* pFolderDialog = nullptr;
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFolderDialog));
        if (SUCCEEDED(hr) && pFolderDialog) {
            DWORD dwOptions = 0;
            if (SUCCEEDED(pFolderDialog->GetOptions(&dwOptions))) {
                pFolderDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
            }
            if (title && strlen(title) > 0) {
                int wlen = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
                if (wlen > 0) {
                    std::vector<wchar_t> wtitle(wlen);
                    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle.data(), wlen);
                    pFolderDialog->SetTitle(wtitle.data());
                }
            }

            if (SUCCEEDED(pFolderDialog->Show(NULL))) {
                IShellItem* pItem = nullptr;
                if (SUCCEEDED(pFolderDialog->GetResult(&pItem)) && pItem) {
                    PWSTR pszPath = nullptr;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)) && pszPath) {
                        int u8len = WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, NULL, 0, NULL, NULL);
                        if (u8len > 0) {
                            resultPath.resize(u8len - 1);
                            WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, &resultPath[0], u8len, NULL, NULL);
                        }
                        CoTaskMemFree(pszPath);
                    }
                    pItem->Release();
                }
            }
            pFolderDialog->Release();
        }
        else {
            // Fallback to SHBrowseForFolder in clean STA apartment
            char szDir[MAX_PATH] = { 0 };
            BROWSEINFOA bi = { 0 };
            bi.lpszTitle = title;
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
            if (pidl != 0) {
                SHGetPathFromIDListA(pidl, szDir);
                CoTaskMemFree(pidl);
                resultPath = szDir;
            }
        }

        if (coInitialized) {
            CoUninitialize();
        }
    });

    if (dialogThread.joinable()) {
        dialogThread.join();
    }

    return resultPath;
}

static inline std::string SaveFileDialog(const char* filter = "All Files\0*.*\0") {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (GetSaveFileNameA(&ofn) == TRUE) return std::string(ofn.lpstrFile);
    return "";
}