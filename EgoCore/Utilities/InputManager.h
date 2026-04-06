#pragma once
#include "imgui.h"
#include <vector>
#include <string>

enum class EAppMode { Banks, Defs, FSE };
inline EAppMode g_CurrentMode = EAppMode::Banks;

enum class EDefViewType { Defs, Headers, Events };
inline EDefViewType g_CurrentDefView = EDefViewType::Defs;

struct ShortcutKey {
    ImGuiKey Key;
    bool Ctrl;
    bool Shift;
    bool Alt;

    bool IsPressed() const {
        ImGuiIO& io = ImGui::GetIO();
        if (Key != ImGuiKey_None && !ImGui::IsKeyPressed(Key, false)) return false;
        if (Key == ImGuiKey_None && !Ctrl && !Shift && !Alt) return false;
        if (Ctrl != io.KeyCtrl) return false;
        if (Shift != io.KeyShift) return false;
        if (Alt != io.KeyAlt) return false;
        return true;
    }

    bool IsDown() const {
        ImGuiIO& io = ImGui::GetIO();
        if (Key != ImGuiKey_None && !ImGui::IsKeyDown(Key)) return false;
        if (Key == ImGuiKey_None && !Ctrl && !Shift && !Alt) return false;
        if (Ctrl != io.KeyCtrl) return false;
        if (Shift != io.KeyShift) return false;
        if (Alt != io.KeyAlt) return false;
        return true;
    }

    std::string ToString() const {
        std::string s = "";
        if (Ctrl) s += "Ctrl + ";
        if (Shift) s += "Shift + ";
        if (Alt) s += "Alt + ";
        if (Key != ImGuiKey_None) s += ImGui::GetKeyName(Key);
        return s;
    }
};

struct AppKeybindings {
    ShortcutKey SwitchBankMode = { ImGuiKey_B, true, false, false };
    ShortcutKey SwitchDefMode = { ImGuiKey_D, true, false, false };
    ShortcutKey SwitchFSEMode = { ImGuiKey_K, true, false, false };
    ShortcutKey SaveEntry = { ImGuiKey_S, true, false, false };
    ShortcutKey Compile = { ImGuiKey_F5, false, false, false };
    ShortcutKey NavigateBack = { ImGuiKey_LeftArrow, true, false, true };
    ShortcutKey NavigateForward = { ImGuiKey_RightArrow, true, false, true };
    ShortcutKey DeleteEntry = { ImGuiKey_Delete, false, false, false };
    ShortcutKey ToggleLeftPanel = { ImGuiKey_T, true, false, false };
    ShortcutKey LookupDefinition = { ImGuiKey_LeftCtrl, true, false, false };
};
inline AppKeybindings g_Keybinds;

struct BankHistoryNode {
    int BankIndex;
    int SubBankIndex;
    int EntryIndex;
};

struct DefHistoryNode {
    EDefViewType View;
    int ContextIndex;
    std::string Category;
    int Index;
};

inline std::vector<BankHistoryNode> g_BankHistory;
inline std::vector<BankHistoryNode> g_BankForwardHistory;

inline std::vector<DefHistoryNode> g_DefHistory;
inline std::vector<DefHistoryNode> g_DefForwardHistory;

inline bool g_IsNavigating = false;

inline void PushBankHistory(int bankIdx, int subBankIdx, int entryIdx) {
    if (g_IsNavigating || bankIdx == -1) return;
    if (!g_BankHistory.empty() && g_BankHistory.back().BankIndex == bankIdx &&
        g_BankHistory.back().SubBankIndex == subBankIdx && g_BankHistory.back().EntryIndex == entryIdx) return;

    g_BankHistory.push_back({ bankIdx, subBankIdx, entryIdx });
    if (g_BankHistory.size() > 50) g_BankHistory.erase(g_BankHistory.begin());
    g_BankForwardHistory.clear();
}

inline void PushDefHistory(EDefViewType view, int contextIdx, const std::string& cat, int idx) {
    if (g_IsNavigating) return;
    if (!g_DefHistory.empty() && g_DefHistory.back().View == view && g_DefHistory.back().ContextIndex == contextIdx &&
        g_DefHistory.back().Category == cat && g_DefHistory.back().Index == idx) return;

    g_DefHistory.push_back({ view, contextIdx, cat, idx });
    if (g_DefHistory.size() > 50) g_DefHistory.erase(g_DefHistory.begin());
    g_DefForwardHistory.clear();
}