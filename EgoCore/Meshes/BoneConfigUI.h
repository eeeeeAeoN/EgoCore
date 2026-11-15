#pragma once
#include "imgui.h"
#include "BoneConfigParser.h"
#include "MeshParser.h"
#include "ConfigBackend.h"
#include "DefBackend.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

inline BoneConfigData g_CurrentBoneConfig;
inline std::vector<std::string> g_BoneConfigFiles;
inline bool g_BoneConfigFilesScanned = false;
inline bool g_ApplyBoneConfig = false;
inline std::string g_SelectedBoneConfig = "";
inline bool g_BoneConfigDirty = false;
inline bool g_BoneMirrorLR = true;
inline std::string g_SelectedBoneName = "";
inline std::string g_BoneConfigStatus = "";

inline fs::path GetBonesDirectory(bool createIfMissing = false) {
    if (g_AppConfig.GameRootPath.empty()) return {};

    std::error_code ec;
    fs::path upper = fs::path(g_AppConfig.GameRootPath) / "Data" / "Bones";
    if (fs::exists(upper, ec) && fs::is_directory(upper, ec)) return upper;

    fs::path lower = fs::path(g_AppConfig.GameRootPath) / "data" / "bones";
    if (fs::exists(lower, ec) && fs::is_directory(lower, ec)) return lower;

    if (createIfMissing) {
        fs::create_directories(upper, ec);
        if (!ec) return upper;
    }
    return {};
}

inline void ScanBoneConfigFiles() {
    g_BoneConfigFiles.clear();
    g_BoneConfigFilesScanned = true;

    fs::path bonesDir = GetBonesDirectory(false);
    if (bonesDir.empty()) return;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(bonesDir, ec)) {
        if (entry.is_regular_file(ec)) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)::tolower(c); });
            if (ext == ".bncfg") {
                g_BoneConfigFiles.push_back(entry.path().filename().string());
            }
        }
    }

    std::sort(g_BoneConfigFiles.begin(), g_BoneConfigFiles.end());
}

inline void LoadBoneConfigFile(const std::string& filename) {
    if (filename.empty() || g_AppConfig.GameRootPath.empty()) {
        g_CurrentBoneConfig = BoneConfigData();
        g_BoneConfigDirty = false;
        return;
    }

    fs::path bonesDir = GetBonesDirectory(false);
    std::error_code ec;
    fs::path filePath = bonesDir.empty() ? fs::path() : (bonesDir / filename);

    if (!filePath.empty() && fs::exists(filePath, ec)) {
        BoneConfigParser::Parse(filePath.string(), g_CurrentBoneConfig);
        g_CurrentBoneConfig.FileName = filename;
        g_CurrentBoneConfig.FullPath = filePath.string();
    }
    else {
        g_CurrentBoneConfig = BoneConfigData();
    }
    g_BoneConfigDirty = false;
}

inline bool SaveCurrentBoneConfigFile(std::string* outError = nullptr) {
    if (!g_CurrentBoneConfig.IsLoaded || g_CurrentBoneConfig.FileName.empty()) {
        if (outError) *outError = "No bone configuration file loaded.";
        return false;
    }

    fs::path savePath = g_CurrentBoneConfig.FullPath;
    if (savePath.empty()) {
        fs::path bonesDir = GetBonesDirectory(true);
        if (bonesDir.empty()) {
            if (outError) *outError = "GameRootPath is not configured.";
            return false;
        }
        savePath = bonesDir / g_CurrentBoneConfig.FileName;
    }

    if (!BoneConfigParser::Save(savePath.string(), g_CurrentBoneConfig)) {
        if (outError) *outError = "Failed to write " + savePath.string();
        return false;
    }

    g_CurrentBoneConfig.FullPath = savePath.string();
    g_BoneConfigDirty = false;
    g_BoneConfigStatus = "Saved " + g_CurrentBoneConfig.FileName;
    ScanBoneConfigFiles();
    return true;
}

inline std::string EnsureBncfgExtension(std::string name) {
    name = BoneConfigParser::Trim(name);
    if (name.empty()) return "";
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)::tolower(c); });
    if (lower.size() < 6 || lower.substr(lower.size() - 6) != ".bncfg") {
        name += ".bncfg";
    }
    return name;
}

inline void AddStandardBip01Groups(BoneConfigData& data, const C3DMeshContent* mesh = nullptr) {
    auto hasClean = [&](const std::string& bName) -> bool {
        if (!mesh) return true;
        std::string target = BoneConfigParser::CleanName(bName);
        for (const auto& mn : mesh->BoneNames) {
            if (BoneConfigParser::CleanName(mn) == target) return true;
        }
        return false;
    };

    struct GroupTemplate { const char* Name; std::vector<const char*> Candidates; };
    static const GroupTemplate templates[] = {
        { "clavs", { "Bip01 L Clavicle", "Bip01 R Clavicle" } },
        { "upperarms", { "Bip01 L UpperArm", "Bip01 R UpperArm" } },
        { "forearms", { "Bip01 L Forearm", "Bip01 R Forearm" } },
        { "hands", { "Bip01 L Hand", "Bip01 R Hand" } },
        { "thighs", { "Bip01 L Thigh", "Bip01 R Thigh" } },
        { "calves", { "Bip01 L Calf", "Bip01 R Calf" } },
        { "feet", { "Bip01 L Foot", "Bip01 R Foot", "Bip01 L Toe0", "Bip01 R Toe0" } },
        { "spine", { "Bip01 Pelvis", "Bip01 Spine", "Bip01 Spine1", "Bip01 Spine2", "Bip01 Spine3" } },
        { "head", { "Bip01 Head", "Bip01 Neck", "Bip01 Neck1", "Chin", "Chin_tip", "Top_lip_st", "L_Cheek_st", "R_Cheek_st", "eye_deck", "eye_top", "eye_bottom", "L_brow_1", "L_c_bone_1", "R_brow_1", "R_c_bone_1", "EYE_SET_L", "EYE_SET_R", "MOUTH_FOCUS" } },
        { "left fingers", { "Bip01 L Finger0", "Bip01 L Finger01", "Bip01 L Finger02", "Bip01 L Finger1", "Bip01 L Finger11", "Bip01 L Finger12", "Bip01 L Finger2", "Bip01 L Finger21", "Bip01 L Finger22", "Bip01 L Finger3", "Bip01 L Finger31", "Bip01 L Finger32", "Bip01 L Finger4", "Bip01 L Finger41", "Bip01 L Finger42" } },
        { "right fingers", { "Bip01 R Finger0", "Bip01 R Finger01", "Bip01 R Finger02", "Bip01 R Finger1", "Bip01 R Finger11", "Bip01 R Finger12", "Bip01 R Finger2", "Bip01 R Finger21", "Bip01 R Finger22", "Bip01 R Finger3", "Bip01 R Finger31", "Bip01 R Finger32", "Bip01 R Finger4", "Bip01 R Finger41", "Bip01 R Finger42" } }
    };

    for (const auto& tmpl : templates) {
        bool exists = false;
        for (const auto& g : data.Groups) {
            if (BoneConfigParser::CleanName(g.Name) == BoneConfigParser::CleanName(tmpl.Name)) {
                exists = true;
                break;
            }
        }
        if (exists) continue;

        BoneConfigGroup grp;
        grp.Name = tmpl.Name;
        for (const char* cand : tmpl.Candidates) {
            if (hasClean(cand)) {
                grp.Bones.push_back(cand);
            }
        }
        if (!grp.Bones.empty()) {
            data.Groups.push_back(std::move(grp));
        }
    }
}

inline std::string ResolveMeshBoneName(const C3DMeshContent& mesh, const std::string& candidate) {
    std::string target = BoneConfigParser::CleanName(candidate);
    for (const auto& bn : mesh.BoneNames) {
        if (BoneConfigParser::CleanName(bn) == target) return bn;
    }
    return candidate;
}

inline void ApplyBoneScaleWithMirror(const C3DMeshContent& mesh, const std::string& boneName, const XMFLOAT3& scale) {
    if (!g_CurrentBoneConfig.IsLoaded || boneName.empty()) return;

    XMFLOAT3 clamped(
        (std::max)(0.05f, scale.x),
        (std::max)(0.05f, scale.y),
        (std::max)(0.05f, scale.z)
    );
    g_CurrentBoneConfig.SetBoneScale(boneName, clamped);

    if (g_BoneMirrorLR) {
        std::string mirror = BoneConfigParser::GetMirrorBoneName(boneName);
        if (!mirror.empty()) {
            std::string resolvedMirror = ResolveMeshBoneName(mesh, mirror);
            g_CurrentBoneConfig.SetBoneScale(resolvedMirror, clamped);
        }
    }
    g_ApplyBoneConfig = true;
    g_BoneConfigDirty = true;
}

inline std::vector<std::string> GatherCreatureTypeSuggestions() {
    std::set<std::string> types;
    auto it = g_DefWorkspace.CategorizedDefs.find("CREATURE");
    if (it != g_DefWorkspace.CategorizedDefs.end()) {
        for (const auto& def : it->second) {
            if (!def.Name.empty()) types.insert(def.Name);
        }
    }
    if (types.empty()) {
        // Fallback: scan Creature_type values from existing .bncfg files in Data/Bones
        fs::path bonesDir = GetBonesDirectory(false);
        if (!bonesDir.empty()) {
            std::error_code ec;
            for (const auto& entry : fs::directory_iterator(bonesDir, ec)) {
                if (!entry.is_regular_file(ec)) continue;
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)::tolower(c); });
                if (ext != ".bncfg") continue;
                std::ifstream f(entry.path());
                std::string line;
                while (std::getline(f, line)) {
                    std::string t = BoneConfigParser::Trim(line);
                    if (t.rfind("Creature_type:", 0) == 0 || t.rfind("creature_type:", 0) == 0) {
                        size_t colon = t.find(':');
                        size_t semi = t.find(';');
                        size_t endPos = (semi != std::string::npos && semi > colon) ? semi : t.size();
                        std::string val = BoneConfigParser::Trim(t.substr(colon + 1, endPos - colon - 1));
                        if (!val.empty()) types.insert(val);
                        break;
                    }
                }
            }
        }
    }
    return std::vector<std::string>(types.begin(), types.end());
}

struct BoneLiveScaleDragState {
    bool Active = false;
    int TargetGroupIndex = -2; // -2 = none, -1 = global, >=0 = group index
    float Uniform = 1.0f;
    float XYZ[3] = { 1.0f, 1.0f, 1.0f };
    std::vector<std::pair<std::string, XMFLOAT3>> Baseline;

    void Reset() {
        Active = false;
        TargetGroupIndex = -2;
        Uniform = 1.0f;
        XYZ[0] = XYZ[1] = XYZ[2] = 1.0f;
        Baseline.clear();
    }
};

inline void CaptureScaleBaseline(BoneLiveScaleDragState& state, int groupIndex, const C3DMeshContent& mesh) {
    state.Baseline.clear();
    state.Active = true;
    state.TargetGroupIndex = groupIndex;

    if (groupIndex < 0) {
        std::set<std::string> seen;
        for (const auto& e : g_CurrentBoneConfig.Entries) {
            std::string clean = BoneConfigParser::CleanName(e.Name);
            if (seen.insert(clean).second) {
                state.Baseline.push_back({ e.Name, e.Scale });
            }
        }
        for (const auto& bn : mesh.BoneNames) {
            if (bn.empty()) continue;
            std::string clean = BoneConfigParser::CleanName(bn);
            if (seen.insert(clean).second) {
                state.Baseline.push_back({ bn, XMFLOAT3(1.0f, 1.0f, 1.0f) });
            }
        }
    }
    else if (groupIndex < (int)g_CurrentBoneConfig.Groups.size()) {
        std::set<std::string> seen;
        for (const auto& rawName : g_CurrentBoneConfig.Groups[groupIndex].Bones) {
            std::string resolved = ResolveMeshBoneName(mesh, rawName);
            std::string clean = BoneConfigParser::CleanName(resolved);
            if (!resolved.empty() && seen.insert(clean).second) {
                state.Baseline.push_back({ resolved, g_CurrentBoneConfig.GetBoneScale(resolved) });
            }
        }
    }
}

inline void ApplyLiveScaleBaseline(const BoneLiveScaleDragState& state, float mx, float my, float mz) {
    for (const auto& kv : state.Baseline) {
        XMFLOAT3 next(
            (std::max)(0.05f, kv.second.x * mx),
            (std::max)(0.05f, kv.second.y * my),
            (std::max)(0.05f, kv.second.z * mz)
        );
        g_CurrentBoneConfig.SetBoneScale(kv.first, next);
    }
    g_ApplyBoneConfig = true;
    g_BoneConfigDirty = true;
}

inline void HandleLiveScaleDragLifecycle(
    BoneLiveScaleDragState& state,
    int groupIndex,
    const C3DMeshContent& mesh,
    bool changed,
    bool uniformChanged
) {
    if (ImGui::IsItemActivated()) {
        CaptureScaleBaseline(state, groupIndex, mesh);
    }
    if (changed) {
        if (!state.Active || state.TargetGroupIndex != groupIndex) {
            CaptureScaleBaseline(state, groupIndex, mesh);
        }
        if (uniformChanged) {
            state.XYZ[0] = state.XYZ[1] = state.XYZ[2] = state.Uniform;
        }
        else {
            state.Uniform = (state.XYZ[0] + state.XYZ[1] + state.XYZ[2]) / 3.0f;
        }
        ApplyLiveScaleBaseline(state, state.XYZ[0], state.XYZ[1], state.XYZ[2]);
    }
    if (ImGui::IsItemDeactivated()) {
        state.Reset();
    }
}

inline void DrawEyeIcon(ImDrawList* dl, const ImVec2& rMin, const ImVec2& rMax, bool active, bool disabled) {
    ImVec2 c((rMin.x + rMax.x) * 0.5f, (rMin.y + rMax.y) * 0.5f);
    ImU32 col = disabled
        ? IM_COL32(120, 120, 120, 140)
        : (active ? IM_COL32(230, 240, 255, 255) : IM_COL32(150, 155, 165, 210));

    float hw = 7.5f;
    float hh = 5.2f;
    dl->AddBezierCubic(ImVec2(c.x - hw, c.y), ImVec2(c.x - hw * 0.45f, c.y - hh), ImVec2(c.x + hw * 0.45f, c.y - hh), ImVec2(c.x + hw, c.y), col, 1.5f);
    dl->AddBezierCubic(ImVec2(c.x - hw, c.y), ImVec2(c.x - hw * 0.45f, c.y + hh), ImVec2(c.x + hw * 0.45f, c.y + hh), ImVec2(c.x + hw, c.y), col, 1.5f);
    dl->AddCircleFilled(c, 2.3f, col);

    if (!active) {
        ImU32 slashCol = disabled ? IM_COL32(120, 120, 120, 140) : IM_COL32(235, 110, 110, 240);
        dl->AddLine(ImVec2(c.x - 6.5f, c.y + 5.5f), ImVec2(c.x + 6.5f, c.y - 5.5f), slashCol, 1.7f);
    }
}

inline void DrawBoneConfigTab(
    const C3DMeshContent& mesh,
    int& selectedBoneIdx,
    std::function<void(int)> onSelectMeshBone
) {
    if (!g_BoneConfigFilesScanned) {
        ScanBoneConfigFiles();
    }

    static bool s_TriggerNewPopup = false;
    static bool s_ShowNewPopup = false;
    static char s_NewFileNameBuf[128] = "custom_bones.bncfg";
    static char s_NewCreatureTypeBuf[128] = "CREATURE_HERO";
    static int s_NewTemplateMode = 1; // 0 = empty, 1 = mesh bones, 2 = copy existing
    static int s_NewCopySourceIdx = 0;
    static bool s_NewAddStdGroups = true;

    static bool s_TriggerDeletePopup = false;
    static bool s_ShowDeletePopup = false;
    static std::string s_PendingDeleteFile = "";
    static bool s_CloseComboAfterAction = false;

    static bool s_TriggerCreaturePopup = false;
    static bool s_ShowCreaturePopup = false;
    static char s_CreatureSearchBuf[128] = "";
    static std::vector<std::string> s_CreatureSuggestions;

    static char s_BoneListFilterBuf[128] = "";
    static char s_NewGroupNameBuf[64] = "";
    static BoneLiveScaleDragState s_GlobalScaleDrag;
    static BoneLiveScaleDragState s_GroupScaleDrag;

    auto OpenNewDialog = [&]() {
        strncpy_s(s_NewFileNameBuf, "custom_bones.bncfg", sizeof(s_NewFileNameBuf) - 1);
        if (g_CurrentBoneConfig.IsLoaded && !g_CurrentBoneConfig.CreatureType.empty()) {
            strncpy_s(s_NewCreatureTypeBuf, g_CurrentBoneConfig.CreatureType.c_str(), sizeof(s_NewCreatureTypeBuf) - 1);
        }
        else {
            strncpy_s(s_NewCreatureTypeBuf, "CREATURE_HERO", sizeof(s_NewCreatureTypeBuf) - 1);
        }
        s_NewTemplateMode = 1;
        s_TriggerNewPopup = true;
    };

    auto OpenDuplicateDialogFor = [&](int fileIdx) {
        if (fileIdx < 0 || fileIdx >= (int)g_BoneConfigFiles.size()) return;
        std::string base = g_BoneConfigFiles[fileIdx];
        size_t dot = base.rfind('.');
        if (dot != std::string::npos) base = base.substr(0, dot);
        base += "_copy.bncfg";
        strncpy_s(s_NewFileNameBuf, base.c_str(), sizeof(s_NewFileNameBuf) - 1);

        BoneConfigData srcCfg;
        fs::path srcPath = GetBonesDirectory(false) / g_BoneConfigFiles[fileIdx];
        if (BoneConfigParser::Parse(srcPath.string(), srcCfg) && !srcCfg.CreatureType.empty()) {
            strncpy_s(s_NewCreatureTypeBuf, srcCfg.CreatureType.c_str(), sizeof(s_NewCreatureTypeBuf) - 1);
        }
        else {
            strncpy_s(s_NewCreatureTypeBuf, "CREATURE_HERO", sizeof(s_NewCreatureTypeBuf) - 1);
        }

        s_NewTemplateMode = 2;
        s_NewCopySourceIdx = fileIdx;
        s_TriggerNewPopup = true;
    };

    // Top Preset Row: [ Dropdown ] [ + ] [ Eye ]
    bool hasRoot = !g_AppConfig.GameRootPath.empty();
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float plusW = 24.0f;
    float eyeW = 28.0f;
    float comboW = ImGui::GetContentRegionAvail().x - plusW - eyeW - spacing * 2.0f;
    ImGui::SetNextItemWidth((std::max)(80.0f, comboW));

    std::string previewStr = g_SelectedBoneConfig.empty()
        ? "No Bone Preset"
        : (g_SelectedBoneConfig + (g_BoneConfigDirty ? " *" : ""));

    if (ImGui::BeginCombo("##BoneConfigSelect", previewStr.c_str())) {
        bool isNoneSelected = g_SelectedBoneConfig.empty();
        if (ImGui::Selectable("No Bone Preset", isNoneSelected)) {
            g_SelectedBoneConfig = "";
            g_ApplyBoneConfig = false;
            g_CurrentBoneConfig = BoneConfigData();
            g_BoneConfigDirty = false;
            g_BoneConfigStatus.clear();
        }
        if (isNoneSelected) ImGui::SetItemDefaultFocus();

        float rowDelW = 20.0f;
        for (int idx = 0; idx < (int)g_BoneConfigFiles.size(); ++idx) {
            const std::string& file = g_BoneConfigFiles[idx];
            bool isSelected = (g_SelectedBoneConfig == file);

            ImGui::PushID(idx);
            float selW = (std::max)(80.0f, ImGui::GetContentRegionAvail().x - rowDelW - spacing);
            if (ImGui::Selectable(file.c_str(), isSelected, ImGuiSelectableFlags_None, ImVec2(selW, 0))) {
                g_SelectedBoneConfig = file;
                g_ApplyBoneConfig = true;
                LoadBoneConfigFile(g_SelectedBoneConfig);
                g_BoneConfigStatus.clear();
            }
            if (isSelected) ImGui::SetItemDefaultFocus();

            if (ImGui::BeginPopupContextItem("##BncfgCtx")) {
                if (ImGui::MenuItem("Duplicate")) {
                    OpenDuplicateDialogFor(idx);
                    s_CloseComboAfterAction = true;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Delete")) {
                    s_PendingDeleteFile = file;
                    s_TriggerDeletePopup = true;
                    s_CloseComboAfterAction = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.12f, 0.12f, 0.70f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.20f, 0.20f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.14f, 0.14f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.88f, 0.88f, 1.0f));
            if (ImGui::SmallButton("X##DelRow")) {
                s_PendingDeleteFile = file;
                s_TriggerDeletePopup = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(4);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete %s", file.c_str());

            ImGui::PopID();
        }

        if (g_BoneConfigFiles.empty()) {
            ImGui::TextDisabled("No .bncfg files found in Data/Bones");
        }

        if (s_CloseComboAfterAction) {
            s_CloseComboAfterAction = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndCombo();
    }
    if (g_BoneConfigDirty && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Unsaved changes - click Save Entry or press Ctrl+S");
    }

    ImGui::SameLine();
    if (!hasRoot) ImGui::BeginDisabled();
    if (ImGui::Button("+##AddBncfg", ImVec2(plusW, 0))) {
        OpenNewDialog();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Create new .bncfg file");
    if (!hasRoot) ImGui::EndDisabled();

    ImGui::SameLine();
    bool hasLoaded = g_CurrentBoneConfig.IsLoaded;
    if (!hasLoaded) ImGui::BeginDisabled();
    if (hasLoaded && g_ApplyBoneConfig) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.42f, 0.68f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.52f, 0.80f, 0.95f));
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.20f, 0.22f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.34f, 0.90f));
    }
    if (ImGui::Button("##EyePreviewBncfg", ImVec2(eyeW, 0))) {
        g_ApplyBoneConfig = !g_ApplyBoneConfig;
    }
    DrawEyeIcon(ImGui::GetWindowDrawList(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), g_ApplyBoneConfig, !hasLoaded);
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(g_ApplyBoneConfig ? "Preview ON (Click to disable bone scaling)" : "Preview OFF (Click to enable bone scaling)");
    }
    if (!hasLoaded) ImGui::EndDisabled();

    if (!g_BoneConfigStatus.empty()) {
        ImGui::TextColored(ImVec4(0.55f, 0.9f, 0.55f, 1.0f), "%s", g_BoneConfigStatus.c_str());
    }

    // Popups: New / Duplicate
    if (s_TriggerNewPopup) {
        ImGui::OpenPopup("New Bone Configuration");
        s_ShowNewPopup = true;
        s_TriggerNewPopup = false;
    }
    if (ImGui::BeginPopupModal("New Bone Configuration", &s_ShowNewPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Filename", s_NewFileNameBuf, sizeof(s_NewFileNameBuf));
        ImGui::InputText("Creature_type", s_NewCreatureTypeBuf, sizeof(s_NewCreatureTypeBuf));
        ImGui::SameLine();
        if (ImGui::Button("Pick...##PickCreatureNew")) {
            s_CreatureSuggestions = GatherCreatureTypeSuggestions();
            s_CreatureSearchBuf[0] = '\0';
            s_TriggerCreaturePopup = true;
        }

        ImGui::Separator();
        ImGui::Text("Initial Bone Data:");
        ImGui::RadioButton("Current Mesh Skeleton (1.0, 1.0, 1.0)", &s_NewTemplateMode, 1);
        ImGui::RadioButton("Empty File", &s_NewTemplateMode, 0);
        if (!g_BoneConfigFiles.empty()) {
            ImGui::RadioButton("Copy From Existing Preset", &s_NewTemplateMode, 2);
            if (s_NewTemplateMode == 2) {
                if (s_NewCopySourceIdx < 0 || s_NewCopySourceIdx >= (int)g_BoneConfigFiles.size()) s_NewCopySourceIdx = 0;
                ImGui::SetNextItemWidth(260);
                if (ImGui::BeginCombo("##CopySrc", g_BoneConfigFiles[s_NewCopySourceIdx].c_str())) {
                    for (int i = 0; i < (int)g_BoneConfigFiles.size(); ++i) {
                        bool sel = (s_NewCopySourceIdx == i);
                        if (ImGui::Selectable(g_BoneConfigFiles[i].c_str(), sel)) s_NewCopySourceIdx = i;
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
        }
        if (s_NewTemplateMode == 1) {
            ImGui::Checkbox("Include Standard Bip01 Groups", &s_NewAddStdGroups);
        }

        ImGui::Separator();
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            std::string fname = EnsureBncfgExtension(s_NewFileNameBuf);
            if (!fname.empty()) {
                fs::path bonesDir = GetBonesDirectory(true);
                if (!bonesDir.empty()) {
                    BoneConfigData newCfg;
                    if (s_NewTemplateMode == 2 && !g_BoneConfigFiles.empty()) {
                        fs::path srcPath = GetBonesDirectory(false) / g_BoneConfigFiles[s_NewCopySourceIdx];
                        BoneConfigParser::Parse(srcPath.string(), newCfg);
                    }
                    else if (s_NewTemplateMode == 1) {
                        for (const auto& bn : mesh.BoneNames) {
                            if (!bn.empty()) newCfg.SetBoneScale(bn, XMFLOAT3(1.0f, 1.0f, 1.0f));
                        }
                        if (s_NewAddStdGroups) {
                            AddStandardBip01Groups(newCfg, &mesh);
                        }
                    }

                    std::string ct = BoneConfigParser::Trim(s_NewCreatureTypeBuf);
                    if (!ct.empty()) newCfg.CreatureType = ct;
                    if (newCfg.CreatureType.empty()) newCfg.CreatureType = "CREATURE_HERO";

                    newCfg.FileName = fname;
                    newCfg.FullPath = (bonesDir / fname).string();
                    newCfg.IsLoaded = true;
                    newCfg.RebuildScaleMap();

                    if (BoneConfigParser::Save(newCfg.FullPath, newCfg)) {
                        g_CurrentBoneConfig = std::move(newCfg);
                        g_SelectedBoneConfig = fname;
                        g_ApplyBoneConfig = true;
                        g_BoneConfigDirty = false;
                        g_BoneConfigStatus = "Created " + fname;
                        ScanBoneConfigFiles();
                    }
                }
            }
            s_ShowNewPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            s_ShowNewPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Popup: Delete Confirmation
    if (s_TriggerDeletePopup) {
        ImGui::OpenPopup("Delete Bone Config?");
        s_ShowDeletePopup = true;
        s_TriggerDeletePopup = false;
    }
    if (ImGui::BeginPopupModal("Delete Bone Config?", &s_ShowDeletePopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::string targetFile = s_PendingDeleteFile.empty() ? g_CurrentBoneConfig.FileName : s_PendingDeleteFile;
        ImGui::Text("Permanently delete '%s' from Data/Bones?", targetFile.c_str());
        ImGui::Separator();
        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            std::error_code ec;
            fs::path bonesDir = GetBonesDirectory(false);
            fs::path delPath = bonesDir.empty() ? fs::path(g_CurrentBoneConfig.FullPath) : (bonesDir / targetFile);
            if (!delPath.empty() && fs::exists(delPath, ec)) {
                fs::remove(delPath, ec);
            }
            if (BoneConfigParser::CleanName(targetFile) == BoneConfigParser::CleanName(g_CurrentBoneConfig.FileName)) {
                g_CurrentBoneConfig = BoneConfigData();
                g_SelectedBoneConfig = "";
                g_ApplyBoneConfig = false;
                g_BoneConfigDirty = false;
            }
            g_BoneConfigStatus = "Deleted " + targetFile;
            s_PendingDeleteFile.clear();
            ScanBoneConfigFiles();
            s_ShowDeletePopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            s_PendingDeleteFile.clear();
            s_ShowDeletePopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (!g_CurrentBoneConfig.IsLoaded) {
        return;
    }

    // Creature_type editor + picker + Mirror toggle
    ImGui::Separator();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Creature Type");
    ImGui::SameLine(105);
    char ctBuf[128];
    strncpy_s(ctBuf, g_CurrentBoneConfig.CreatureType.c_str(), sizeof(ctBuf) - 1);
    float pickBtnW = 30.0f;
    ImGui::SetNextItemWidth((std::max)(80.0f, ImGui::GetContentRegionAvail().x - pickBtnW - spacing));
    if (ImGui::InputText("##CreatureTypeEdit", ctBuf, sizeof(ctBuf))) {
        g_CurrentBoneConfig.CreatureType = ctBuf;
        g_BoneConfigDirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("...##PickCreatureEdit", ImVec2(pickBtnW, 0))) {
        s_CreatureSuggestions = GatherCreatureTypeSuggestions();
        s_CreatureSearchBuf[0] = '\0';
        s_TriggerCreaturePopup = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Choose Creature_type from definitions");

    // Creature Type Picker Modal
    if (s_TriggerCreaturePopup) {
        ImGui::OpenPopup("Select Creature Type");
        s_ShowCreaturePopup = true;
        s_TriggerCreaturePopup = false;
    }
    if (ImGui::BeginPopupModal("Select Creature Type", &s_ShowCreaturePopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputTextWithHint("##CreatureSearch", "Filter creature types...", s_CreatureSearchBuf, sizeof(s_CreatureSearchBuf));
        std::string filter = s_CreatureSearchBuf;
        std::transform(filter.begin(), filter.end(), filter.begin(), [](unsigned char c) { return (char)::tolower(c); });

        ImGui::BeginChild("CreatureTypeList", ImVec2(360, 280), true);
        if (s_CreatureSuggestions.empty()) {
            ImGui::TextDisabled("No CREATURE definitions loaded yet. Open Definitions tab to scan Data/Defs, or type manually.");
        }
        for (const auto& ct : s_CreatureSuggestions) {
            std::string lower = ct;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)::tolower(c); });
            if (filter.empty() || lower.find(filter) != std::string::npos) {
                if (ImGui::Selectable(ct.c_str(), g_CurrentBoneConfig.CreatureType == ct)) {
                    if (s_ShowNewPopup) {
                        strncpy_s(s_NewCreatureTypeBuf, ct.c_str(), sizeof(s_NewCreatureTypeBuf) - 1);
                    }
                    else {
                        g_CurrentBoneConfig.CreatureType = ct;
                        g_BoneConfigDirty = true;
                    }
                    s_ShowCreaturePopup = false;
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            s_ShowCreaturePopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Checkbox("Mirror", &g_BoneMirrorLR);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Mirror Left / Right bone scale edits");
    ImGui::SameLine();
    if (ImGui::Button("Reset##ResetAllBones")) {
        for (auto& e : g_CurrentBoneConfig.Entries) {
            e.Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        }
        g_CurrentBoneConfig.RebuildScaleMap();
        g_BoneConfigDirty = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset all bone scales to (1.0, 1.0, 1.0)");

    // Keep g_SelectedBoneName synced with viewport selection
    if (selectedBoneIdx >= 0 && selectedBoneIdx < (int)mesh.BoneNames.size()) {
        g_SelectedBoneName = mesh.BoneNames[selectedBoneIdx];
    }

    // Global Scale Multiplier (auto-apply while dragging, springs back to 1.0 on release)
    if (ImGui::CollapsingHeader("Global Scale", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Uniform");
        ImGui::SameLine(80);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        float globU = (s_GlobalScaleDrag.Active && s_GlobalScaleDrag.TargetGroupIndex == -1) ? s_GlobalScaleDrag.Uniform : 1.0f;
        bool uChanged = ImGui::DragFloat("##GlobUniform", &globU, 0.01f, 0.1f, 5.0f, "%.3fx");
        if (uChanged) s_GlobalScaleDrag.Uniform = globU;
        HandleLiveScaleDragLifecycle(s_GlobalScaleDrag, -1, mesh, uChanged, true);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("XYZ");
        ImGui::SameLine(80);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        float globXYZ[3] = {
            (s_GlobalScaleDrag.Active && s_GlobalScaleDrag.TargetGroupIndex == -1) ? s_GlobalScaleDrag.XYZ[0] : 1.0f,
            (s_GlobalScaleDrag.Active && s_GlobalScaleDrag.TargetGroupIndex == -1) ? s_GlobalScaleDrag.XYZ[1] : 1.0f,
            (s_GlobalScaleDrag.Active && s_GlobalScaleDrag.TargetGroupIndex == -1) ? s_GlobalScaleDrag.XYZ[2] : 1.0f
        };
        bool xyzChanged = ImGui::DragFloat3("##GlobXYZ", globXYZ, 0.01f, 0.1f, 5.0f, "%.2fx");
        if (xyzChanged) {
            s_GlobalScaleDrag.XYZ[0] = globXYZ[0];
            s_GlobalScaleDrag.XYZ[1] = globXYZ[1];
            s_GlobalScaleDrag.XYZ[2] = globXYZ[2];
        }
        HandleLiveScaleDragLifecycle(s_GlobalScaleDrag, -1, mesh, xyzChanged, false);
    }

    // Bone Groups (#Start_group_settings)
    if (ImGui::CollapsingHeader(("Bone Groups (" + std::to_string(g_CurrentBoneConfig.Groups.size()) + ")###GrpNode").c_str())) {
        float addBtnW = 24.0f;
        float stdBtnW = 72.0f;
        ImGui::SetNextItemWidth((std::max)(80.0f, ImGui::GetContentRegionAvail().x - addBtnW - stdBtnW - spacing * 2.0f));
        ImGui::InputTextWithHint("##NewGrpName", "New group...", s_NewGroupNameBuf, sizeof(s_NewGroupNameBuf));
        ImGui::SameLine();
        if (ImGui::Button("+##AddGrp", ImVec2(addBtnW, 0))) {
            std::string gn = BoneConfigParser::Trim(s_NewGroupNameBuf);
            if (!gn.empty()) {
                g_CurrentBoneConfig.Groups.push_back({ gn, {} });
                s_NewGroupNameBuf[0] = '\0';
                g_BoneConfigDirty = true;
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Create bone group");
        ImGui::SameLine();
        if (ImGui::Button("Std Bip01", ImVec2(stdBtnW, 0))) {
            AddStandardBip01Groups(g_CurrentBoneConfig, &mesh);
            g_BoneConfigDirty = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add standard Fable Bip01 groups for this skeleton");

        for (int gi = 0; gi < (int)g_CurrentBoneConfig.Groups.size(); ++gi) {
            auto& grp = g_CurrentBoneConfig.Groups[gi];
            ImGui::PushID(("Grp_" + std::to_string(gi)).c_str());

            float rightControlsW = 104.0f; // 78px drag + spacing + 20px X
            ImGui::SetNextItemAllowOverlap();
            bool openGrp = ImGui::TreeNodeEx((grp.Name + " (" + std::to_string(grp.Bones.size()) + ")###GrpItem").c_str());

            ImGui::SameLine((std::max)(115.0f, ImGui::GetWindowContentRegionMax().x - rightControlsW));
            ImGui::SetNextItemWidth(76.0f);
            float grpU = (s_GroupScaleDrag.Active && s_GroupScaleDrag.TargetGroupIndex == gi) ? s_GroupScaleDrag.Uniform : 1.0f;
            bool guChanged = ImGui::DragFloat("##GrpU", &grpU, 0.01f, 0.1f, 5.0f, "%.2fx");
            if (guChanged) s_GroupScaleDrag.Uniform = grpU;
            HandleLiveScaleDragLifecycle(s_GroupScaleDrag, gi, mesh, guChanged, true);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Drag to uniformly scale all bones in '%s'", grp.Name.c_str());

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.12f, 0.12f, 0.70f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.20f, 0.20f, 1.0f));
            if (ImGui::SmallButton("X##DelGrp")) {
                if (s_GroupScaleDrag.TargetGroupIndex == gi) s_GroupScaleDrag.Reset();
                g_CurrentBoneConfig.Groups.erase(g_CurrentBoneConfig.Groups.begin() + gi);
                g_BoneConfigDirty = true;
                if (openGrp) ImGui::TreePop();
                ImGui::PopStyleColor(2);
                ImGui::PopID();
                --gi;
                continue;
            }
            ImGui::PopStyleColor(2);

            if (openGrp) {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("XYZ");
                ImGui::SameLine();
                float grpXYZ[3] = {
                    (s_GroupScaleDrag.Active && s_GroupScaleDrag.TargetGroupIndex == gi) ? s_GroupScaleDrag.XYZ[0] : 1.0f,
                    (s_GroupScaleDrag.Active && s_GroupScaleDrag.TargetGroupIndex == gi) ? s_GroupScaleDrag.XYZ[1] : 1.0f,
                    (s_GroupScaleDrag.Active && s_GroupScaleDrag.TargetGroupIndex == gi) ? s_GroupScaleDrag.XYZ[2] : 1.0f
                };
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                bool gxyzChanged = ImGui::DragFloat3("##GrpXYZ", grpXYZ, 0.01f, 0.1f, 5.0f, "%.2fx");
                if (gxyzChanged) {
                    s_GroupScaleDrag.XYZ[0] = grpXYZ[0];
                    s_GroupScaleDrag.XYZ[1] = grpXYZ[1];
                    s_GroupScaleDrag.XYZ[2] = grpXYZ[2];
                }
                HandleLiveScaleDragLifecycle(s_GroupScaleDrag, gi, mesh, gxyzChanged, false);

                auto groupHasBone = [&](const std::string& candidate) -> bool {
                    std::string target = BoneConfigParser::CleanName(candidate);
                    for (const auto& b : grp.Bones) {
                        if (BoneConfigParser::CleanName(b) == target) return true;
                    }
                    return false;
                };

                bool canAddSelected = !g_SelectedBoneName.empty() && !groupHasBone(g_SelectedBoneName);
                float selBtnW = canAddSelected ? 48.0f : 0.0f;
                ImGui::SetNextItemWidth((std::max)(80.0f, ImGui::GetContentRegionAvail().x - (canAddSelected ? (selBtnW + spacing) : 0.0f)));
                if (ImGui::BeginCombo("##AddBoneToGrp", "Add bone...")) {
                    for (int mi = 0; mi < mesh.BoneCount && mi < (int)mesh.BoneNames.size(); ++mi) {
                        const std::string& mName = mesh.BoneNames[mi];
                        if (mName.empty() || groupHasBone(mName)) continue;
                        if (ImGui::Selectable(mName.c_str())) {
                            grp.Bones.push_back(mName);
                            g_BoneConfigDirty = true;
                        }
                    }
                    ImGui::EndCombo();
                }
                if (canAddSelected) {
                    ImGui::SameLine();
                    if (ImGui::Button("+Sel##AddSelToGrp", ImVec2(selBtnW, 0))) {
                        grp.Bones.push_back(g_SelectedBoneName);
                        g_BoneConfigDirty = true;
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add %s", g_SelectedBoneName.c_str());
                }

                for (int bi = 0; bi < (int)grp.Bones.size(); ++bi) {
                    ImGui::PushID(bi);
                    const std::string& memberName = grp.Bones[bi];
                    bool isMemberSel = !g_SelectedBoneName.empty() &&
                        BoneConfigParser::CleanName(g_SelectedBoneName) == BoneConfigParser::CleanName(memberName);
                    float bRowW = (std::max)(60.0f, ImGui::GetContentRegionAvail().x - 24.0f);
                    if (ImGui::Selectable(memberName.c_str(), isMemberSel, ImGuiSelectableFlags_None, ImVec2(bRowW, 0))) {
                        std::string cleanMem = BoneConfigParser::CleanName(memberName);
                        for (int mi = 0; mi < (int)mesh.BoneNames.size(); ++mi) {
                            if (BoneConfigParser::CleanName(mesh.BoneNames[mi]) == cleanMem) {
                                g_SelectedBoneName = mesh.BoneNames[mi];
                                selectedBoneIdx = mi;
                                if (onSelectMeshBone) onSelectMeshBone(mi);
                                break;
                            }
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("x##RemMem")) {
                        grp.Bones.erase(grp.Bones.begin() + bi);
                        g_BoneConfigDirty = true;
                        --bi;
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    // Mesh Skeleton Bones List
    ImGui::Separator();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##BoneFilter", "Filter mesh bones...", s_BoneListFilterBuf, sizeof(s_BoneListFilterBuf));
    std::string boneFilter = s_BoneListFilterBuf;
    std::transform(boneFilter.begin(), boneFilter.end(), boneFilter.begin(), [](unsigned char c) { return (char)::tolower(c); });

    float listHeight = (std::max)(140.0f, ImGui::GetContentRegionAvail().y);
    ImGui::BeginChild("MeshBonesScaleList", ImVec2(0, listHeight), true);
    for (int i = 0; i < mesh.BoneCount; ++i) {
        std::string bName = (i < (int)mesh.BoneNames.size()) ? mesh.BoneNames[i] : ("Bone_" + std::to_string(i));
        std::string lower = bName;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)::tolower(c); });
        if (!boneFilter.empty() && lower.find(boneFilter) == std::string::npos && std::to_string(i).find(boneFilter) == std::string::npos) {
            continue;
        }

        ImGui::PushID(i);
        bool isSel = (selectedBoneIdx == i) || (!g_SelectedBoneName.empty() && BoneConfigParser::CleanName(g_SelectedBoneName) == BoneConfigParser::CleanName(bName));
        XMFLOAT3 sc = g_CurrentBoneConfig.GetBoneScale(bName);
        bool isModified = (fabsf(sc.x - 1.0f) > 0.001f || fabsf(sc.y - 1.0f) > 0.001f || fabsf(sc.z - 1.0f) > 0.001f);

        if (isModified) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.4f, 1.0f));
        if (ImGui::Selectable((std::to_string(i) + ": " + bName).c_str(), isSel, ImGuiSelectableFlags_AllowOverlap, ImVec2(145, 0))) {
            g_SelectedBoneName = bName;
            selectedBoneIdx = i;
            if (onSelectMeshBone) onSelectMeshBone(i);
        }
        if (isModified) ImGui::PopStyleColor();

        ImGui::SameLine();
        float vals[3] = { sc.x, sc.y, sc.z };
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat3("##BScale", vals, 0.01f, 0.05f, 10.0f, "%.2f")) {
            g_SelectedBoneName = bName;
            selectedBoneIdx = i;
            if (onSelectMeshBone) onSelectMeshBone(i);
            ApplyBoneScaleWithMirror(mesh, bName, XMFLOAT3(vals[0], vals[1], vals[2]));
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
}
