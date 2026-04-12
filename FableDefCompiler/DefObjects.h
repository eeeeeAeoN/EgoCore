#pragma once
#include "StringParser.h"
#include "BinaryStreams.h"
#include <map>
#include <string>

// --- BASE CLASSES ---
struct CSubComponent {
    virtual ~CSubComponent() = default;
    virtual void Transfer(CPersistContext& persist) = 0;
};

class IDefObject {
public:
    virtual ~IDefObject() = default;
    virtual void Transfer(CPersistContext& persist) = 0;
    virtual std::string GetClassName() const = 0;
    virtual std::string GetInstantiationName() const = 0;
    virtual void SetInstantiationName(const std::string& name) = 0;
    virtual void CopyFrom(const IDefObject* parentObject) = 0;
};

class CDefObject : public IDefObject {
public:
    std::string m_InstantiationName;
    std::string GetInstantiationName() const override { return m_InstantiationName; }
    void SetInstantiationName(const std::string& name) override { m_InstantiationName = name; }
};

// --- SUBCLASSES & STRUCTURES ---
struct CDefString {
    std::string Value;
    void Transfer(CPersistContext& persist) { 
        // Logic handled by CPersistContext::Transfer(const char*, CDefString&, ...) overload
    }
};

inline void CPersistContext::Transfer(const char* name, CDefString& val, const CDefString& defaultVal) {
    if (Mode == MODE_LOAD_TEXT) {
        size_t after = FindWholeWord(name);
        if (after == std::string::npos) return;
        size_t fieldStart = after - std::strlen(name);
        size_t pos = after;
        val.Value = ReadString(pos);
        if (GDefStringTable) GDefStringTable->AddString(val.Value);
        size_t sc = FindSemicolon(pos);
        BlankRegion(fieldStart, sc != std::string::npos ? sc : pos);
        return;
    }
    if (Mode == MODE_SAVE_BINARY) {
        WriteTag(name);
        uint32_t offset = GDefStringTable ? GDefStringTable->AddString(val.Value) : (uint32_t)-1;
        int32_t toWrite = offset == (uint32_t)-1 ? -1 : (int32_t)offset;
        PSaveStream->WriteSLONG(toWrite);
    }
}

inline void CPersistContext::Transfer(const char* name, C2DVector& val, const C2DVector& defaultVal) {
    if (Mode == MODE_SAVE_BINARY) {
        WriteTag(name);
        PSaveStream->WriteFloat(val.x);
        PSaveStream->WriteFloat(val.y);
    }
}

// (Factory declarations moved below IDefObject)

struct CActionInputControl;
template<> void CPersistContext::TransferVector<CActionInputControl>(const char* name, std::vector<CActionInputControl>& vec);

// CActionInputControl — parsed via inline parser, exactly matching
// CPersistTraits<CActionInputControl>::TransferIn (ida_funcs.c line 1071-1197)
// Text format: CActionInputControl(GameAction, ControllerType, Key[, C2DCoordF(x,y)])
// EControllerType: CONTROLLER_XBOX_PAD=0, CONTROLLER_KEYBOARD=1, CONTROLLER_MOUSE=2
struct CActionInputControl : CSubComponent {
    int32_t GameAction = 0;
    int32_t ControllerType = 0;
    int32_t KeyboardKey = 0;
    int32_t XboxButton = 0;
    int32_t MouseButton = 0;
    C2DVector ControlDirection = { 0.0f, 0.0f };

    // Parse from def text inline. Called by CPersistContext::TransferVector<CActionInputControl>
    // parser is the CPersistContext at position just after the outer '.Add('
    // (text layout: VectorName.Add(CActionInputControl(arg1, arg2, arg3[, C2DCoordF(x,y)]));)
    void ParseInline(CPersistContext& ctx, size_t& pos) {
        std::string& text = ctx.GetDefText();

        auto SkipWS = [&]() { while (pos < text.length() && isspace(text[pos])) pos++; };
        auto SkipChar = [&](char c) {
            SkipWS();
            if (pos < text.length() && text[pos] == c) { pos++; return true; }
            return false;
            };

        SkipWS();
        ctx.SkipToken_pub(pos, "CActionInputControl");
        if (!SkipChar('(')) return;

        // Arg 1: GameAction
        SkipWS();
        GameAction = ctx.ReadIntExpr_pub(pos);

        if (!SkipChar(',')) return;

        // Arg 2: ControllerType 
        SkipWS();
        size_t savedPos = pos;
        std::string cToken;
        while (pos < text.length() && (isalnum(text[pos]) || text[pos] == '_')) cToken += text[pos++];

        if (cToken == "CONTROLLER_XBOX_PAD") ControllerType = 1;
        else if (cToken == "CONTROLLER_KEYBOARD") ControllerType = 2;
        else if (cToken == "CONTROLLER_MOUSE") ControllerType = 3;
        else if (cToken == "CONTROLLER_NONE") ControllerType = 0;
        else {
            pos = savedPos;
            ControllerType = ctx.ReadIntExpr_pub(pos);
        }

        if (!SkipChar(',')) return;

        // Arg 3: Button/Key (Bypass Symbol Map with explicit arrays)
        SkipWS();
        size_t savedPos3 = pos;
        std::string btnToken;
        while (pos < text.length() && (isalnum(text[pos]) || text[pos] == '_')) btnToken += text[pos++];

        int32_t thirdArg = 0;
        if (ControllerType == 2) {
            const char* kb_keys[] = {
                "KB_NULL", "KB_ESC", "KB_1", "KB_2", "KB_3", "KB_4", "KB_5", "KB_6", "KB_7", "KB_8", "KB_9", "KB_0",
                "KB_MINUS", "KB_EQUALS", "KB_BACKSPACE", "KB_TAB", "KB_Q", "KB_W", "KB_E", "KB_R", "KB_T", "KB_Y",
                "KB_U", "KB_I", "KB_O", "KB_P", "KB_LBRACKET", "KB_RBRACKET", "KB_RETURN", "KB_LCONTROL", "KB_A",
                "KB_S", "KB_D", "KB_F", "KB_G", "KB_H", "KB_J", "KB_K", "KB_L", "KB_SEMICOLON", "KB_APOSTROPHE",
                "KB_HASH", "KB_LSHIFT", "KB_BACKSLASH", "KB_Z", "KB_X", "KB_C", "KB_V", "KB_B", "KB_N", "KB_M",
                "KB_COMMA", "KB_FULLSTOP", "KB_SLASH", "KB_RSHIFT", "KB_PMULTIPLY", "KB_LALT", "KB_SPACE", "KB_CAPSLOCK",
                "KB_F1", "KB_F2", "KB_F3", "KB_F4", "KB_F5", "KB_F6", "KB_F7", "KB_F8", "KB_F9", "KB_F10", "KB_NUMLOCK",
                "KB_SCROLLLOCK", "KB_P7", "KB_P8", "KB_P9", "KB_PMINUS", "KB_P4", "KB_P5", "KB_P6", "KB_PPLUS", "KB_P1",
                "KB_P2", "KB_P3", "KB_P0", "KB_PFULLSTOP", "KB_F11", "KB_F12", "KB_F13", "KB_F14", "KB_F15", "KB_KANA",
                "KB_CONVERT", "KB_NOCONVERT", "KB_YEN", "KB_PEQUALS", "KB_CIRCUMFLEX", "KB_AT", "KB_COLON", "KB_UNDERLINE",
                "KB_KANJI", "KB_STOP", "KB_AX", "KB_UNLABELED", "KB_PENTER", "KB_RCONTROL", "KB_PCOMMA", "KB_PDIVIDE",
                "KB_SYSRQ", "KB_RALT", "KB_HOME", "KB_UP", "KB_PAGEUP", "KB_LEFT", "KB_RIGHT", "KB_END", "KB_DOWN",
                "KB_PAGEDOWN", "KB_INSERT", "KB_DELETE", "KB_LWIN", "KB_RWIN", "KB_APPS"
            };
            for (int i = 0; i < 119; ++i) {
                if (btnToken == kb_keys[i]) { thirdArg = i; break; }
            }
            if (thirdArg == 0 && btnToken != "KB_NULL") { pos = savedPos3; thirdArg = ctx.ReadIntExpr_pub(pos); }
        }
        else if (ControllerType == 3) {
            const char* ms_keys[] = {
                "MOUSE_BUTTON_NULL_CONTROL", "MOUSE_BUTTON_LEFT_CONTROL", "MOUSE_BUTTON_RIGHT_CONTROL",
                "MOUSE_BUTTON_MIDDLE_CONTROL", "MOUSE_MOVEMENT", "MOUSE_WHEEL_MOVEMENT", "MOUSE_WHEEL_MOVEMENT_UP",
                "MOUSE_WHEEL_MOVEMENT_DOWN", "MOUSE_BUTTON_4_CONTROL", "MOUSE_BUTTON_5_CONTROL", "MOUSE_BUTTON_6_CONTROL",
                "MOUSE_BUTTON_7_CONTROL", "MOUSE_BUTTON_8_CONTROL"
            };
            for (int i = 0; i < 13; ++i) {
                if (btnToken == ms_keys[i]) { thirdArg = i; break; }
            }
            if (thirdArg == 0 && btnToken != "MOUSE_BUTTON_NULL_CONTROL") { pos = savedPos3; thirdArg = ctx.ReadIntExpr_pub(pos); }
        }
        else if (ControllerType == 1) {
            const char* xb_keys[] = {
                "XBOX_PAD_UNDEFINED_BUTTON", "XBOX_PAD_X_BUTTON", "XBOX_PAD_Y_BUTTON", "XBOX_PAD_BLACK_BUTTON",
                "XBOX_PAD_A_BUTTON", "XBOX_PAD_B_BUTTON", "XBOX_PAD_WHITE_BUTTON", "XBOX_PAD_LEFT_TRIGGER",
                "XBOX_PAD_RIGHT_TRIGGER", "XBOX_PAD_LEFT_STICK_BUTTON", "XBOX_PAD_RIGHT_STICK_BUTTON",
                "XBOX_PAD_START_BUTTON", "XBOX_PAD_BACK_BUTTON", "XBOX_PAD_DPAD_UP_BUTTON", "XBOX_PAD_DPAD_DOWN_BUTTON",
                "XBOX_PAD_DPAD_LEFT_BUTTON", "XBOX_PAD_DPAD_RIGHT_BUTTON", "XBOX_PAD_LEFT_ANALOGUE_STICK",
                "XBOX_PAD_RIGHT_ANALOGUE_STICK"
            };
            for (int i = 0; i < 19; ++i) {
                if (btnToken == xb_keys[i]) { thirdArg = i; break; }
            }
            if (thirdArg == 0 && btnToken != "XBOX_PAD_UNDEFINED_BUTTON") { pos = savedPos3; thirdArg = ctx.ReadIntExpr_pub(pos); }
        }
        else {
            pos = savedPos3;
            thirdArg = ctx.ReadIntExpr_pub(pos);
        }

        KeyboardKey = 0;
        XboxButton = 0;
        MouseButton = 0;

        switch (ControllerType) {
        case 1: XboxButton = thirdArg; break;
        case 2: KeyboardKey = thirdArg; break;
        case 3: MouseButton = thirdArg; break;
        default: break;
        }

        // Arg 4: Optional C2DCoordF
        if (SkipChar(',')) {
            SkipWS();
            if (ctx.SkipToken_pub(pos, "C2DCoordF")) {
                if (SkipChar('(')) {
                    SkipWS();
                    ControlDirection.x = ctx.ReadFloat_pub(pos);
                    if (SkipChar(',')) {
                        SkipWS();
                        ControlDirection.y = ctx.ReadFloat_pub(pos);
                    }
                    SkipChar(')');
                }
            }
        }
        SkipChar(')');
    }

    void Transfer(CPersistContext& persist) override {
        if (persist.Mode == CPersistContext::MODE_SAVE_BINARY) {
           // Fixed 28-byte chunk for CActionInputControl in binary
           persist.PSaveStream->WriteSLONG(GameAction);
           persist.PSaveStream->WriteSLONG(ControllerType);
           persist.PSaveStream->WriteSLONG(KeyboardKey);
           persist.PSaveStream->WriteSLONG(XboxButton);
           persist.PSaveStream->WriteSLONG(MouseButton);
           persist.PSaveStream->Write(&ControlDirection.x, 4);
           persist.PSaveStream->Write(&ControlDirection.y, 4);
        } else {
           persist.Transfer("GameAction", GameAction, (int32_t)0);
           persist.Transfer("ControllerType", ControllerType, (int32_t)0);
           persist.Transfer("KeyboardKey", KeyboardKey, (int32_t)0);
           persist.Transfer("XboxButton", XboxButton, (int32_t)30);
           persist.Transfer("MouseButton", MouseButton, (int32_t)0);
           persist.Transfer("ControlDirectionX", ControlDirection.x, 0.0f);
           persist.Transfer("ControlDirectionY", ControlDirection.y, 0.0f);
        }
    }
};

struct CRectF { float TLX = 0.0f, TLY = 0.0f, BRX = 0.0f, BRY = 0.0f; };
struct CRect { int32_t TLX = 0, TLY = 0, BRX = 0, BRY = 0; };
struct CColor { float R = 1.0f, G = 1.0f, B = 1.0f, A = 1.0f; };

// --- CUIStateDef ---
struct CUIStateDef : CSubComponent {
    uint32_t GraphicIndex = 0;
    C2DVector Position = {0.0f, 0.0f};
    C2DVector Zoom = {1.0f, 1.0f};
    CColor Colour = {1.0f, 1.0f, 1.0f, 1.0f};
    float UpdateTime = -1.0f;
    int32_t StateChangeType = 0;
    bool LinearChange = false;
    uint32_t StateChangeFlag = 7;
    std::vector<uint32_t> ChildrenNotAffected;

    void Transfer(CPersistContext& persist) override {
        persist.Transfer("GraphicIndex", GraphicIndex, (uint32_t)0);
        persist.Transfer("PositionX", Position.x, 0.0f);
        persist.Transfer("PositionY", Position.y, 0.0f);
        persist.Transfer("ZoomX", Zoom.x, 1.0f);
        persist.Transfer("ZoomY", Zoom.y, 1.0f);
        persist.Transfer("ColourR", Colour.R, 1.0f);
        persist.Transfer("ColourG", Colour.G, 1.0f);
        persist.Transfer("ColourB", Colour.B, 1.0f);
        persist.Transfer("ColourA", Colour.A, 1.0f);
        persist.Transfer("UpdateTime", UpdateTime, -1.0f);
        persist.Transfer("StateChangeType", StateChangeType, (int32_t)0);
        persist.Transfer("LinearChange", LinearChange, false);
        persist.Transfer("StateChangeFlag", StateChangeFlag, (uint32_t)7);
        persist.TransferVector("ChildrenNotAffected", ChildrenNotAffected);
    }
};

class CFrontEndDef : public CDefObject {
public:
    std::vector<std::string> vAttractModeMovie;
    int32_t ErrorMessageBackgroundGraphic = 0;
    int32_t ButtonABigGraphic = 0;
    int32_t ButtonBBigGraphic = 0;

    std::string GetClassName() const override { return "FRONT_END"; }
    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* p = dynamic_cast<const std::remove_pointer<decltype(this)>::type*>(parentObject)) {
            std::string myName = m_InstantiationName;
            *this = *p;                   
            m_InstantiationName = myName;
        }
    }

    void Transfer(CPersistContext& persist) override {
        persist.TransferVector("vAttractModeMovie", vAttractModeMovie);
        persist.Transfer("ErrorMessageBackgroundGraphic", ErrorMessageBackgroundGraphic, (int32_t)0);
        persist.Transfer("ButtonABigGraphic", ButtonABigGraphic, (int32_t)0);
        persist.Transfer("ButtonBBigGraphic", ButtonBBigGraphic, (int32_t)0);
    }
};

// --- CControlSchemeDef ---
class CControlSchemeDef : public CDefObject {
public:
    std::vector<CActionInputControl> Controls;
    bool ToggleZTarget = false;
    bool ToggleSpells = false;
    bool ToggleSneak = false;
    bool ToggleExpressionMenu = false;
    bool ToggleExpressionShift = false;
    bool FlourishNeedsAttackButtonHeld = false;

    std::string GetClassName() const override { return "CONTROL_SCHEME"; }
    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* p = dynamic_cast<const std::remove_pointer<decltype(this)>::type*>(parentObject)) {
            std::string myName = m_InstantiationName;
            *this = *p;
            m_InstantiationName = myName;
        }
    }

    void Transfer(CPersistContext& persist) override {
        persist.TransferVector("Controls", Controls);
        persist.Transfer("ToggleZTarget", ToggleZTarget, false);
        persist.Transfer("ToggleSpells", ToggleSpells, false);
        persist.Transfer("ToggleSneak", ToggleSneak, false);
        persist.Transfer("ToggleExpressionMenu", ToggleExpressionMenu, false);
        persist.Transfer("ToggleExpressionShift", ToggleExpressionShift, false);
        persist.Transfer("FlourishNeedsAttackButtonHeld", FlourishNeedsAttackButtonHeld, false);
    }
};

extern CDefStringTable* GDefStringTable;

class CEngineDef : public CDefObject {
public:
    float LODErrorTolerance = 0.0f, CharacterLODErrorTolerance = 0.0f, LODErrorFactor = 0.0f, SeaHeight = 0.0f;
    int32_t LocalDetailBooleanAlphaDefaultAlphaRef = 0, DefaultPrimitiveAlphaRef = 0;
    float GamePrimitiveDefaultFadeStart = 0.0f, GamePrimitiveDefaultFadeRangeRatio = 0.0f;
    float LocalDetailDefaultFadeStart = 0.0f, LocalDetailDefaultFadeRangeRatio = 0.0f;
    int32_t TestStaticMesh = 0, TestAnimatedMesh = 0, TestAnim = 0, TestGraphic = 0;
    float FOV_2D = 0.0f;
    int32_t InvalidTextureStandin = 0, InvalidThemeStandin = 0;

    std::string GetClassName() const override { return "ENGINE"; }
    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* p = dynamic_cast<const std::remove_pointer<decltype(this)>::type*>(parentObject)) {
            std::string myName = m_InstantiationName;
            *this = *p;
            m_InstantiationName = myName;
        }
    }


    void Transfer(CPersistContext& persist) override {
        persist.Transfer("LODErrorTolerance", LODErrorTolerance, 0.0f);
        persist.Transfer("CharacterLODErrorTolerance", CharacterLODErrorTolerance, 0.0f);
        persist.Transfer("LODErrorFactor", LODErrorFactor, 0.0f);
        persist.Transfer("SeaHeight", SeaHeight, 0.0f);
        persist.Transfer("LocalDetailBooleanAlphaDefaultAlphaRef", LocalDetailBooleanAlphaDefaultAlphaRef, (int32_t)0);
        persist.Transfer("DefaultPrimitiveAlphaRef", DefaultPrimitiveAlphaRef, (int32_t)0);
        persist.Transfer("GamePrimitiveDefaultFadeStart", GamePrimitiveDefaultFadeStart, 0.0f);
        persist.Transfer("GamePrimitiveDefaultFadeRangeRatio", GamePrimitiveDefaultFadeRangeRatio, 0.0f);
        persist.Transfer("LocalDetailDefaultFadeStart", LocalDetailDefaultFadeStart, 0.0f);
        persist.Transfer("LocalDetailDefaultFadeRangeRatio", LocalDetailDefaultFadeRangeRatio, 0.0f);
        persist.Transfer("TestStaticMesh", TestStaticMesh, (int32_t)0);
        persist.Transfer("TestAnimatedMesh", TestAnimatedMesh, (int32_t)0);
        persist.Transfer("TestAnim", TestAnim, (int32_t)0);
        persist.Transfer("TestGraphic", TestGraphic, (int32_t)0);
        persist.Transfer("FOV_2D", FOV_2D, 0.0f);
        persist.Transfer("InvalidTextureStandin", InvalidTextureStandin, (int32_t)0);
        persist.Transfer("InvalidThemeStandin", InvalidThemeStandin, (int32_t)0);
    }
};

class CConfigOptionsDefaultsDef : public CDefObject {
public:
    uint32_t Antialiasing = 0;
    int32_t ResolutionWidth = 1024, MinResolutionWidth = 640, ResolutionHeight = 768, MinResolutionHeight = 480, BitDepth = 16;
    float TextureDetail = 1.0f, MaxTextureDetail = 3.0f, ShadowDetail = 1.0f, MaxShadowDetail = 3.0f;
    float MeshDetail = 1.0f, MaxMeshDetail = 3.0f, EffectsDetail = 1.0f, MaxEffectsDetail = 3.0f;

    std::string GetClassName() const override { return "CONFIG_OPTIONS_DEFAULTS_DEF"; }
    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* p = dynamic_cast<const std::remove_pointer<decltype(this)>::type*>(parentObject)) {
            std::string myName = m_InstantiationName;
            *this = *p;
            m_InstantiationName = myName;
        }
    }


    void Transfer(CPersistContext& persist) override {
        persist.Transfer("Antialiasing", Antialiasing, 0u);
        persist.Transfer("ResolutionWidth", ResolutionWidth, (int32_t)1024);   // <--- FIXED
        persist.Transfer("ResolutionHeight", ResolutionHeight, (int32_t)768);  // <--- FIXED
        persist.Transfer("BitDepth", BitDepth, (int32_t)16);
        persist.Transfer("TextureDetail", TextureDetail, 1.0f);
        persist.Transfer("MaxTextureDetail", MaxTextureDetail, 3.0f);
        persist.Transfer("ShadowDetail", ShadowDetail, 1.0f);
        persist.Transfer("MaxShadowDetail", MaxShadowDetail, 3.0f);
        persist.Transfer("MeshDetail", MeshDetail, 1.0f);
        persist.Transfer("MaxMeshDetail", MaxMeshDetail, 3.0f);
        persist.Transfer("EffectsDetail", EffectsDetail, 1.0f);
        persist.Transfer("MaxEffectsDetail", MaxEffectsDetail, 3.0f);
        persist.Transfer("MinResolutionWidth", MinResolutionWidth, (int32_t)1024);
        persist.Transfer("MinResolutionHeight", MinResolutionHeight, (int32_t)768);
    }
};

class CEngineVideoOptionsDef : public IDefObject {
public:
    std::string m_InstantiationName;
    int32_t HiresTextureMemory = 0;
    float LODErrorTolerance = 0.0f, CharacterLODErrorTolerance = 0.0f, DrawDistanceMultiplier = 0.0f;
    float DrawDistanceMinimum = 0.0f, DrawDistanceMaximum = 0.0f, RepeatedMeshDrawDistanceFactor = 0.0f;
    float MinimumZSpriteAsMeshDistance = 0.0f, MaximumZSpriteAsMeshDistance = 0.0f, ZSpriteDrawDistanceMultiplier = 0.0f;
    int32_t ShadowBufferSize = 1024; float ShadowDistanceScale = 1.0f;
    bool Enable2DDisplacement = false, Enable3DDisplacement = false, EnableGlow = false, EnableRadialBlur = false;
    bool EnableWaterReflection = false, EnableWeatherEffects = false, EnableColourFilter = false;
    float WeatherDensity = 1.0f; bool EnableRepeatedMeshes = true;

    std::string GetClassName() const override { return "ENGINE_VIDEO_OPTIONS"; }
    std::string GetInstantiationName() const override { return m_InstantiationName; }
    void SetInstantiationName(const std::string& name) override { m_InstantiationName = name; }
    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* p = dynamic_cast<const std::remove_pointer<decltype(this)>::type*>(parentObject)) {
            std::string myName = m_InstantiationName;
            *this = *p;
            m_InstantiationName = myName;
        }
    }


    void Transfer(CPersistContext& persist) override {
        persist.Transfer("HiresTextureMemory", HiresTextureMemory, (int32_t)0);
        persist.Transfer("LODErrorTolerance", LODErrorTolerance, 0.0f);
        persist.Transfer("CharacterLODErrorTolerance", CharacterLODErrorTolerance, 0.0f);
        persist.Transfer("DrawDistanceMultiplier", DrawDistanceMultiplier, 0.0f);
        persist.Transfer("DrawDistanceMinimum", DrawDistanceMinimum, 0.0f);
        persist.Transfer("DrawDistanceMaximum", DrawDistanceMaximum, 0.0f);
        persist.Transfer("RepeatedMeshDrawDistanceFactor", RepeatedMeshDrawDistanceFactor, 0.0f);
        persist.Transfer("MinimumZSpriteAsMeshDistance", MinimumZSpriteAsMeshDistance, 0.0f);
        persist.Transfer("MaximumZSpriteAsMeshDistance", MaximumZSpriteAsMeshDistance, 0.0f);
        persist.Transfer("ZSpriteDrawDistanceMultiplier", ZSpriteDrawDistanceMultiplier, 0.0f);
        persist.Transfer("ShadowBufferSize", ShadowBufferSize, (int32_t)1024);
        persist.Transfer("ShadowDistanceScale", ShadowDistanceScale, 1.0f);
        persist.Transfer("Enable2DDisplacement", Enable2DDisplacement, false);
        persist.Transfer("Enable3DDisplacement", Enable3DDisplacement, false);
        persist.Transfer("EnableGlow", EnableGlow, false);
        persist.Transfer("EnableRadialBlur", EnableRadialBlur, false);
        persist.Transfer("EnableWaterReflection", EnableWaterReflection, false);
        persist.Transfer("EnableWeatherEffects", EnableWeatherEffects, false);
        persist.Transfer("EnableColourFilter", EnableColourFilter, false);
        persist.Transfer("WeatherDensity", WeatherDensity, 1.0f);
        persist.Transfer("EnableRepeatedMeshes", EnableRepeatedMeshes, true);
    }
};

class CUIDef : public CDefObject {
public:
    int32_t Type = 4;
    std::vector<int32_t> Children;
    uint32_t MeshIndex = 0;
    std::wstring TextValue;
    CDefString Font;
    float Height = 0.0f, Width = 0.0f;
    int32_t ExpansionType = 1;
    std::map<int32_t, int32_t> Sprites;
    std::vector<uint32_t> HorizontalSeparations;
    std::vector<uint32_t> VerticalSeparations;
    std::vector<CUIStateDef> States;
    bool TextLineBreak = true;
    bool ScaleText = true;
    bool Independant = false;
    int32_t MeshType = 5;
    std::vector<uint32_t> NonScrollingChildren;
    float TextWindowTLX = 0.0f, TextWindowTLY = 0.0f, TextWindowBRX = 0.0f, TextWindowBRY = 0.0f;
    int32_t Layer = 0;
    float Angle = 0.0f;
    bool PositionIsCenter = false;
    float ScrollingSpeed = 1.0f;
    bool Wrapping = true;
    bool Inverted = false;
    float PositionOffsetX = 0.0f, PositionOffsetY = 0.0f;
    uint32_t AlphaOffset = 0;
    C3DVector Up = {0.0f, 0.0f, 1.0f}, Forward = {0.0f, 1.0f, 0.0f}, RotationAxis = {0.0f, 1.0f, 0.0f};
    float RotationSpeed = 0.0f;
    uint32_t AnimationIndex = 0;
    int32_t DownArrow = 0, UpArrow = 0, UpLimit = 0, DownLimit = 0;
    bool Scrolling = true;
    bool ComputeOffsetsOnActivate = false;
    float MinX = 0.0f, MinY = 0.0f, MaxX = 0.0f, MaxY = 0.0f, StepX = 0.0f, StepY = 0.0f;
    float DimensionsX = 0.0f, DimensionsY = 0.0f;
    int32_t SliderLeft = 0, SliderRight = 0;
    int32_t Action = 0, ActionOnBack = 0, ActionOnSelected = 0, ActionOnUnselected = 0;
    int32_t ActionOnDestruction = 0, ActionOnLeftClicked = 0, ActionOnLeftUnclicked = 0, ActionOnLeftHeld = 0;
    int32_t ActionOnRightClicked = 0, ActionOnDropped = 0, ActionOnDroppedNowhere = 0, PreAction = 0;
    int32_t ActionOnDraggedUp = 0, ActionOnDraggedDown = 0, ActionOnLeftClickedAbove = 0, ActionOnLeftClickedUnder = 0;
    float InputDelay = 0.2f;
    bool DrawFromViewport = false;
    uint32_t TextBankIndex = 0;
    int32_t ActionText = 0, KeyText = 0, Redefiner = 0, UndefinedWarning = 0;
    std::map<std::string, uint32_t> ActionMap;
    std::map<uint32_t, uint32_t> ActionMapAliases;
    std::vector<uint32_t> ActionOrder;
    bool EditBoxParentIsButton = false, PasswordBox = false;
    int32_t EditBoxCharLimit = 0;
    bool EditBoxUsesIME = false;
    std::wstring MovieFilename;
    bool DisallowSpaceAsFirstChar = false;
    bool LayerIndependant = false;
    std::vector<uint32_t> SwappingStates;
    std::vector<float> SwappingTimes;
    bool BastardChild = false;
    int32_t Alignement = 0;
    bool RandomSwap = false;
    bool UseRelativeZoom = false;
    bool UseRelativePosition = false;
    int32_t HoveredState = 3, LeftClickedState = 3, RightClickedState = 3;
    std::vector<uint32_t> ShapeChildren;
    int32_t ViewAreaTLX = 0, ViewAreaTLY = 0, ViewAreaBRX = 640, ViewAreaBRY = 480;
    bool UseViewArea = false;
    bool PartOfListTree = true;
    bool PCStyle = false;
    int32_t Sprite2DFlag = 2;

    std::string GetClassName() const override { return "UI"; }
    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* p = dynamic_cast<const std::remove_pointer<decltype(this)>::type*>(parentObject)) {
            std::string myName = m_InstantiationName;
            *this = *p;
            m_InstantiationName = myName;
        }
    }

    void Transfer(CPersistContext& persist) override {
        persist.TransferEnum("Type", Type, 4);
        persist.TransferVector("Children", Children);
        persist.Transfer("MeshIndex", MeshIndex, (uint32_t)0);
        persist.Transfer("TextValue", TextValue, std::wstring(L""));
        persist.Transfer("Font", Font, Font);
        persist.Transfer("Height", Height, 0.0f);
        persist.Transfer("Width", Width, 0.0f);
        persist.TransferEnum("ExpansionType", ExpansionType, 1);
        persist.TransferMap("Sprites", Sprites, (int32_t)0);
        persist.TransferVector("HorizontalSeparations", HorizontalSeparations);
        persist.TransferVector("VerticalSeparations", VerticalSeparations);
        persist.TransferVectorOfSubComponents("States", States);
        persist.Transfer("TextLineBreak", TextLineBreak, true);
        persist.Transfer("ScaleText", ScaleText, true);
        persist.Transfer("Independant", Independant, false);
        persist.TransferEnum("MeshType", MeshType, 5);
        persist.TransferVector("NonScrollingChildren", NonScrollingChildren);
        persist.Transfer("TextWindowTLX", TextWindowTLX, 0.0f);
        persist.Transfer("TextWindowTLY", TextWindowTLY, 0.0f);
        persist.Transfer("TextWindowBRX", TextWindowBRX, 0.0f);
        persist.Transfer("TextWindowBRY", TextWindowBRY, 0.0f);
        persist.Transfer("Layer", Layer, (int32_t)0);
        persist.Transfer("Angle", Angle, 0.0f);
        persist.Transfer("PositionIsCenter", PositionIsCenter, false);
        persist.Transfer("ScrollingSpeed", ScrollingSpeed, 1.0f);
        persist.Transfer("Wrapping", Wrapping, true);
        persist.Transfer("Inverted", Inverted, false);
        persist.Transfer("PositionOffsetX", PositionOffsetX, 0.0f);
        persist.Transfer("PositionOffsetY", PositionOffsetY, 0.0f);
        persist.Transfer("AlphaOffset", AlphaOffset, (uint32_t)0);
        persist.Transfer("UpX", Up.x, 0.0f);
        persist.Transfer("UpY", Up.y, 0.0f);
        persist.Transfer("UpZ", Up.z, 1.0f);
        persist.Transfer("ForwardX", Forward.x, 0.0f);
        persist.Transfer("ForwardY", Forward.y, 1.0f);
        persist.Transfer("ForwardZ", Forward.z, 0.0f);
        persist.Transfer("RotationAxisX", RotationAxis.x, 0.0f);
        persist.Transfer("RotationAxisY", RotationAxis.y, 1.0f);
        persist.Transfer("RotationAxisZ", RotationAxis.z, 0.0f);
        persist.Transfer("RotationSpeed", RotationSpeed, 0.0f);
        persist.Transfer("AnimationIndex", AnimationIndex, (uint32_t)0);
        persist.Transfer("DownArrow", DownArrow, (int32_t)0);
        persist.Transfer("UpArrow", UpArrow, (int32_t)0);
        persist.Transfer("UpLimit", UpLimit, (int32_t)0);
        persist.Transfer("DownLimit", DownLimit, (int32_t)0);
        persist.Transfer("Scrolling", Scrolling, true);
        persist.Transfer("ComputeOffsetsOnActivate", ComputeOffsetsOnActivate, false);
        persist.Transfer("MinX", MinX, 0.0f);
        persist.Transfer("MinY", MinY, 0.0f);
        persist.Transfer("MaxX", MaxX, 0.0f);
        persist.Transfer("MaxY", MaxY, 0.0f);
        persist.Transfer("StepX", StepX, 0.0f);
        persist.Transfer("StepY", StepY, 0.0f);
        persist.Transfer("DimensionsX", DimensionsX, 0.0f);
        persist.Transfer("DimensionsY", DimensionsY, 0.0f);
        persist.Transfer("SliderLeft", SliderLeft, (int32_t)0);
        persist.Transfer("SliderRight", SliderRight, (int32_t)0);
        persist.TransferEnum("Action", Action, 0);
        persist.TransferEnum("ActionOnBack", ActionOnBack, 0);
        persist.TransferEnum("ActionOnSelected", ActionOnSelected, 0);
        persist.TransferEnum("ActionOnUnselected", ActionOnUnselected, 0);
        persist.TransferEnum("ActionOnDestruction", ActionOnDestruction, 0);
        persist.TransferEnum("ActionOnLeftClicked", ActionOnLeftClicked, 0);
        persist.TransferEnum("ActionOnLeftUnclicked", ActionOnLeftUnclicked, 0);
        persist.TransferEnum("ActionOnLeftHeld", ActionOnLeftHeld, 0);
        persist.TransferEnum("ActionOnRightClicked", ActionOnRightClicked, 0);
        persist.TransferEnum("ActionOnDropped", ActionOnDropped, 0);
        persist.TransferEnum("ActionOnDroppedNowhere", ActionOnDroppedNowhere, 0);
        persist.TransferEnum("PreAction", PreAction, 0);
        persist.TransferEnum("ActionOnDraggedUp", ActionOnDraggedUp, 0);
        persist.TransferEnum("ActionOnDraggedDown", ActionOnDraggedDown, 0);
        persist.TransferEnum("ActionOnLeftClickedAbove", ActionOnLeftClickedAbove, 0);
        persist.TransferEnum("ActionOnLeftClickedUnder", ActionOnLeftClickedUnder, 0);
        persist.Transfer("InputDelay", InputDelay, 0.2f);
        persist.Transfer("DrawFromViewport", DrawFromViewport, false);
        persist.Transfer("TextBankIndex", TextBankIndex, (uint32_t)0);
        persist.Transfer("ActionText", ActionText, (int32_t)0);
        persist.Transfer("KeyText", KeyText, (int32_t)0);
        persist.Transfer("Redefiner", Redefiner, (int32_t)0);
        persist.Transfer("UndefinedWarning", UndefinedWarning, (int32_t)0);
        persist.TransferMap("ActionMap", ActionMap, (uint32_t)0);
        persist.TransferMap("ActionMapAliases", ActionMapAliases, (uint32_t)0);
        persist.TransferVector("ActionOrder", ActionOrder);
        persist.Transfer("EditBoxParentIsButton", EditBoxParentIsButton, false);
        persist.Transfer("PasswordBox", PasswordBox, false);
        persist.Transfer("EditBoxCharLimit", EditBoxCharLimit, (int32_t)0);
        persist.Transfer("EditBoxUsesIME", EditBoxUsesIME, false);
        persist.Transfer("MovieFilename", MovieFilename, std::wstring(L""));
        persist.Transfer("DisallowSpaceAsFirstChar", DisallowSpaceAsFirstChar, false);
        persist.Transfer("LayerIndependant", LayerIndependant, false);
        persist.TransferVector("SwappingStates", SwappingStates);
        persist.TransferVector("SwappingTimes", SwappingTimes);
        persist.Transfer("BastardChild", BastardChild, false);
        persist.TransferEnum("Alignement", Alignement, 0);
        persist.Transfer("RandomSwap", RandomSwap, false);
        persist.Transfer("UseRelativeZoom", UseRelativeZoom, false);
        persist.Transfer("UseRelativePosition", UseRelativePosition, false);
        persist.Transfer("HoveredState", HoveredState, (int32_t)3);
        persist.Transfer("LeftClickedState", LeftClickedState, (int32_t)3);
        persist.Transfer("RightClickedState", RightClickedState, (int32_t)3);
        persist.TransferVector("ShapeChildren", ShapeChildren);
        persist.Transfer("ViewAreaTLX", ViewAreaTLX, (int32_t)0);
        persist.Transfer("ViewAreaTLY", ViewAreaTLY, (int32_t)0);
        persist.Transfer("ViewAreaBRX", ViewAreaBRX, (int32_t)640);
        persist.Transfer("ViewAreaBRY", ViewAreaBRY, (int32_t)480);
        persist.Transfer("UseViewArea", UseViewArea, false);
        persist.Transfer("PartOfListTree", PartOfListTree, true);
        persist.Transfer("PCStyle", PCStyle, false);
        persist.Transfer("Sprite2DFlag", Sprite2DFlag, (int32_t)2);
    }
};


class CUIMiscThingsDef : public CDefObject {
public:
    std::wstring SpaceSeparator, CommaSeparator, NewLineSeparator, OpenBracket, CloseBracket, Positive;
    std::wstring WeaponValueString, WeaponAugString, WeaponAugNone, WeaponWeightString;
    std::wstring WeaponLightString, WeaponHeavyString, WeaponKillsString, WeaponCatMeleeString;
    std::wstring WeaponCatRangedString, WeaponDamageString, TradeCostString, ColonSeparator;
    std::wstring TradeProfitString, TradeLossString, TradeAlreadyOwnsString, TradeNumberInStockString;
    std::wstring TradeDeliveryString, TradeNoDeliveryString, TradeDaysString, TradeBuyString;
    std::wstring TradeSellString, TradeWantedString, QuestFailedString, FailedString, SucceededString;
    std::wstring Plus, Minus;
    uint32_t CoreGraphic = 0, VignetteGraphic = 0, OptionalGraphic = 0, FeatGraphic = 0;
    std::wstring ObjectsRewardString, NoneString, CheckGuildString, QuestStartingString;
    C2DVector RingCenter = { 0,0 }, PCRingCenter = { 0,0 }, WorldMapOffset = { 0,0 };
    float WorldMapWidth = 0, WorldMapHeight = 0;
    std::wstring YouString, OwnString, NoString, HousesString, HouseString, InString, ShopsString, ShopString;
    std::wstring ThereString, AreString, IsString, ForString, SaleString, GeneralString, TatooString, BarberString, TitleString, LevelString;
    uint32_t TotalSpellsInPalettes = 0x12, TotalSpellsInContainer = 3, TotalAssignableThings = 8;
    std::wstring LogBookBasicsCategoryString, LogBookObjectsCategoryString, LogBookTownsCategoryString, LogBookHeroCategoryString;
    std::wstring LogBookCombatCategoryString, LogBookQuestCategoryString, LogBookStoryCategoryString;
    std::wstring LogBookBasicsCategoryNameString, LogBookObjectsCategoryNameString, LogBookTownsCategoryNameString;
    std::wstring LogBookHeroCategoryNameString, LogBookCombatCategoryNameString, LogBookQuestCategoryNameString, LogBookStoryCategoryNameString;

    std::map<std::string, uint32_t> MapPaths;
    std::map<std::string, uint32_t> MiniMapGraphics;

    std::string SoundUpDown, SoundSlider, SoundBack, SoundForward, SoundError, SoundExit;
    C2DVector HeroDollTL = { 310,33 }, HeroDollBR = { 560,300 };
    float HeroDollSphereRadius = 1.3f;
    C2DVector HeroDollTL_PC = { 310,33 }, HeroDollBR_PC = { 560,300 };
    float HeroDollSphereRadius_PC = 1.3f;
    C2DVector HeroDollFrameTL_PC = { 0,0 };
    float HeroDollFrameEmulateListOffset = 0;
    uint32_t QuestStartScreenMusic = 2, QuestCompleteScreenMusic = 3, QuestFailureScreenMusic = 9, DeathScreenMusic = 14;
    std::string CountUpSound;
    float DigitCountTime = 3.0f;
    uint32_t SaveHeroGraphicIndex = 0;

    std::string SoundKeyboardUp, SoundKeyboardDown, SoundKeyboardLeft, SoundKeyboardRight;
    std::string SoundKeyboardEnterCharacter, SoundKeyboardDeleteCharacter, SoundKeyboardDone;
    std::wstring FrontEndMusic;
    int32_t KeyboardSmallKeyGraphic = 0, KeyboardLargeKeyGraphic = 0;
    float TimeInSecsForFade = 0, BackBufferFilterSaturation = 0, BackBufferFilterContrast = 0, BackBufferFilterBrightness = 0;
    float BackBufferFilterTintR = 0, BackBufferFilterTintG = 0, BackBufferFilterTintB = 0, BackBufferFilterTintScale = 0;
    float BackBufferDiffuseScale = 0, BackBufferAmbientScale = 0, MinimumFilterColor = 0;

    std::string GetClassName() const override { return "UI_MISC_THINGS_DEF"; }
    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* p = dynamic_cast<const std::remove_pointer<decltype(this)>::type*>(parentObject)) {
            std::string myName = m_InstantiationName;
            *this = *p;
            m_InstantiationName = myName;
        }
    }

    void Transfer(CPersistContext& persist) override {
        persist.Transfer("SpaceSeparator", SpaceSeparator, std::wstring(L""));
        persist.Transfer("CommaSeparator", CommaSeparator, std::wstring(L""));
        persist.Transfer("NewLineSeparator", NewLineSeparator, std::wstring(L""));
        persist.Transfer("OpenBracket", OpenBracket, std::wstring(L""));
        persist.Transfer("CloseBracket", CloseBracket, std::wstring(L""));
        persist.Transfer("Positive", Positive, std::wstring(L""));
        persist.Transfer("WeaponValueString", WeaponValueString, std::wstring(L""));
        persist.Transfer("WeaponAugString", WeaponAugString, std::wstring(L""));
        persist.Transfer("WeaponAugNone", WeaponAugNone, std::wstring(L""));
        persist.Transfer("WeaponWeightString", WeaponWeightString, std::wstring(L""));
        persist.Transfer("WeaponLightString", WeaponLightString, std::wstring(L""));
        persist.Transfer("WeaponHeavyString", WeaponHeavyString, std::wstring(L""));
        persist.Transfer("WeaponKillsString", WeaponKillsString, std::wstring(L""));
        persist.Transfer("WeaponCatMeleeString", WeaponCatMeleeString, std::wstring(L""));
        persist.Transfer("WeaponCatRangedString", WeaponCatRangedString, std::wstring(L""));
        persist.Transfer("WeaponDamageString", WeaponDamageString, std::wstring(L""));
        persist.Transfer("TradeCostString", TradeCostString, std::wstring(L""));
        persist.Transfer("ColonSeparator", ColonSeparator, std::wstring(L""));
        persist.Transfer("TradeProfitString", TradeProfitString, std::wstring(L""));
        persist.Transfer("TradeLossString", TradeLossString, std::wstring(L""));
        persist.Transfer("TradeAlreadyOwnsString", TradeAlreadyOwnsString, std::wstring(L""));
        persist.Transfer("TradeNumberInStockString", TradeNumberInStockString, std::wstring(L""));
        persist.Transfer("TradeDeliveryString", TradeDeliveryString, std::wstring(L""));
        persist.Transfer("TradeNoDeliveryString", TradeNoDeliveryString, std::wstring(L""));
        persist.Transfer("TradeDaysString", TradeDaysString, std::wstring(L""));
        persist.Transfer("TradeBuyString", TradeBuyString, std::wstring(L""));
        persist.Transfer("TradeSellString", TradeSellString, std::wstring(L""));
        persist.Transfer("TradeWantedString", TradeWantedString, std::wstring(L""));
        persist.Transfer("QuestFailedString", QuestFailedString, std::wstring(L""));
        persist.Transfer("FailedString", FailedString, std::wstring(L""));
        persist.Transfer("SucceededString", SucceededString, std::wstring(L""));
        persist.Transfer("Plus", Plus, std::wstring(L""));
        persist.Transfer("Minus", Minus, std::wstring(L""));
        persist.Transfer("CoreGraphic", CoreGraphic, (uint32_t)0);
        persist.Transfer("VignetteGraphic", VignetteGraphic, (uint32_t)0);
        persist.Transfer("OptionalGraphic", OptionalGraphic, (uint32_t)0);
        persist.Transfer("FeatGraphic", FeatGraphic, (uint32_t)0);
        persist.Transfer("ObjectsRewardString", ObjectsRewardString, std::wstring(L""));
        persist.Transfer("NoneString", NoneString, std::wstring(L""));
        persist.Transfer("CheckGuildString", CheckGuildString, std::wstring(L""));
        persist.Transfer("QuestStartingString", QuestStartingString, std::wstring(L""));
        persist.Transfer("RingCenterX", RingCenter.x, 0.0f);
        persist.Transfer("RingCenterY", RingCenter.y, 0.0f);
        persist.Transfer("PCRingCenterX", PCRingCenter.x, 0.0f);
        persist.Transfer("PCRingCenterY", PCRingCenter.y, 0.0f);
        persist.Transfer("WorldMapOffsetX", WorldMapOffset.x, 0.0f);
        persist.Transfer("WorldMapOffsetY", WorldMapOffset.y, 0.0f);
        persist.Transfer("WorldMapWidth", WorldMapWidth, 0.0f);
        persist.Transfer("WorldMapHeight", WorldMapHeight, 0.0f);
        persist.Transfer("YouString", YouString, std::wstring(L""));
        persist.Transfer("OwnString", OwnString, std::wstring(L""));
        persist.Transfer("NoString", NoString, std::wstring(L""));
        persist.Transfer("HousesString", HousesString, std::wstring(L""));
        persist.Transfer("HouseString", HouseString, std::wstring(L""));
        persist.Transfer("InString", InString, std::wstring(L""));
        persist.Transfer("ShopsString", ShopsString, std::wstring(L""));
        persist.Transfer("ShopString", ShopString, std::wstring(L""));
        persist.Transfer("ThereString", ThereString, std::wstring(L""));
        persist.Transfer("AreString", AreString, std::wstring(L""));
        persist.Transfer("IsString", IsString, std::wstring(L""));
        persist.Transfer("ForString", ForString, std::wstring(L""));
        persist.Transfer("SaleString", SaleString, std::wstring(L""));
        persist.Transfer("GeneralString", GeneralString, std::wstring(L""));
        persist.Transfer("TatooString", TatooString, std::wstring(L""));
        persist.Transfer("BarberString", BarberString, std::wstring(L""));
        persist.Transfer("TitleString", TitleString, std::wstring(L""));
        persist.Transfer("LevelString", LevelString, std::wstring(L""));
        persist.Transfer("TotalSpellsInPalettes", TotalSpellsInPalettes, (uint32_t)0x12);
        persist.Transfer("TotalSpellsInContainer", TotalSpellsInContainer, (uint32_t)3);
        persist.Transfer("TotalAssignableThings", TotalAssignableThings, (uint32_t)8);
        persist.Transfer("LogBookBasicsCategoryString", LogBookBasicsCategoryString, std::wstring(L""));
        persist.Transfer("LogBookObjectsCategoryString", LogBookObjectsCategoryString, std::wstring(L""));
        persist.Transfer("LogBookTownsCategoryString", LogBookTownsCategoryString, std::wstring(L""));
        persist.Transfer("LogBookHeroCategoryString", LogBookHeroCategoryString, std::wstring(L""));
        persist.Transfer("LogBookCombatCategoryString", LogBookCombatCategoryString, std::wstring(L""));
        persist.Transfer("LogBookQuestCategoryString", LogBookQuestCategoryString, std::wstring(L""));
        persist.Transfer("LogBookStoryCategoryString", LogBookStoryCategoryString, std::wstring(L""));
        persist.Transfer("LogBookBasicsCategoryNameString", LogBookBasicsCategoryNameString, std::wstring(L""));
        persist.Transfer("LogBookObjectsCategoryNameString", LogBookObjectsCategoryNameString, std::wstring(L""));
        persist.Transfer("LogBookTownsCategoryNameString", LogBookTownsCategoryNameString, std::wstring(L""));
        persist.Transfer("LogBookHeroCategoryNameString", LogBookHeroCategoryNameString, std::wstring(L""));
        persist.Transfer("LogBookCombatCategoryNameString", LogBookCombatCategoryNameString, std::wstring(L""));
        persist.Transfer("LogBookQuestCategoryNameString", LogBookQuestCategoryNameString, std::wstring(L""));
        persist.Transfer("LogBookStoryCategoryNameString", LogBookStoryCategoryNameString, std::wstring(L""));
        persist.TransferMap("MapPaths", MapPaths, (uint32_t)0);
        persist.Transfer("SoundUpDown", SoundUpDown, "");
        persist.Transfer("SoundSlider", SoundSlider, "");
        persist.Transfer("SoundBack", SoundBack, "");
        persist.Transfer("SoundForward", SoundForward, "");
        persist.Transfer("SoundError", SoundError, "");
        persist.Transfer("SoundExit", SoundExit, "");

        persist.Transfer("HeroDollTLX", HeroDollTL.x, 310.0f);
        persist.Transfer("HeroDollTLY", HeroDollTL.y, 33.0f);
        persist.Transfer("HeroDollBRX", HeroDollBR.x, 560.0f);
        persist.Transfer("HeroDollBRY", HeroDollBR.y, 300.0f);
        persist.Transfer("HeroDollSphereRadius", HeroDollSphereRadius, 1.3f);
        persist.Transfer("HeroDollTLX_PC", HeroDollTL_PC.x, 310.0f);
        persist.Transfer("HeroDollTLY_PC", HeroDollTL_PC.y, 33.0f);
        persist.Transfer("HeroDollBRX_PC", HeroDollBR_PC.x, 560.0f);
        persist.Transfer("HeroDollBRY_PC", HeroDollBR_PC.y, 300.0f);
        persist.Transfer("HeroDollSphereRadius_PC", HeroDollSphereRadius_PC, 1.3f);
        persist.Transfer("HeroDollFrameTLX_PC", HeroDollFrameTL_PC.x, 0.0f);
        persist.Transfer("HeroDollFrameTLY_PC", HeroDollFrameTL_PC.y, 0.0f);
        persist.Transfer("HeroDollFrameEmulateListOffset", HeroDollFrameEmulateListOffset, 0.0f);
        persist.Transfer("QuestStartScreenMusic", QuestStartScreenMusic, (uint32_t)2);
        persist.Transfer("QuestCompleteScreenMusic", QuestCompleteScreenMusic, (uint32_t)3);
        persist.Transfer("QuestFailureScreenMusic", QuestFailureScreenMusic, (uint32_t)9);
        persist.Transfer("DeathScreenMusic", DeathScreenMusic, (uint32_t)14);
        persist.Transfer("CountUpSound", CountUpSound, std::string(""));
        persist.Transfer("DigitCountTime", DigitCountTime, 3.0f);
        persist.Transfer("SaveHeroGraphicIndex", SaveHeroGraphicIndex, (uint32_t)0);

        persist.TransferMap("MiniMapGraphics", MiniMapGraphics, (uint32_t)0);
        persist.Transfer("SoundKeyboardUp", SoundKeyboardUp, "");
        persist.Transfer("SoundKeyboardDown", SoundKeyboardDown, "");
        persist.Transfer("SoundKeyboardLeft", SoundKeyboardLeft, "");
        persist.Transfer("SoundKeyboardRight", SoundKeyboardRight, "");
        persist.Transfer("SoundKeyboardEnterCharacter", SoundKeyboardEnterCharacter, "");
        persist.Transfer("SoundKeyboardDeleteCharacter", SoundKeyboardDeleteCharacter, "");
        persist.Transfer("SoundKeyboardDone", SoundKeyboardDone, "");

        persist.Transfer("FrontEndMusic", FrontEndMusic, std::wstring(L""));
        persist.Transfer("KeyboardSmallKeyGraphic", KeyboardSmallKeyGraphic, (int32_t)0);
        persist.Transfer("KeyboardLargeKeyGraphic", KeyboardLargeKeyGraphic, (int32_t)0);
        persist.Transfer("TimeInSecsForFade", TimeInSecsForFade, 0.0f);
        persist.Transfer("BackBufferFilterSaturation", BackBufferFilterSaturation, 0.0f);
        persist.Transfer("BackBufferFilterContrast", BackBufferFilterContrast, 0.0f);
        persist.Transfer("BackBufferFilterBrightness", BackBufferFilterBrightness, 0.0f);
        persist.Transfer("BackBufferFilterTintR", BackBufferFilterTintR, 0.0f);
        persist.Transfer("BackBufferFilterTintG", BackBufferFilterTintG, 0.0f);
        persist.Transfer("BackBufferFilterTintB", BackBufferFilterTintB, 0.0f);
        persist.Transfer("BackBufferFilterTintScale", BackBufferFilterTintScale, 0.0f);
        persist.Transfer("BackBufferDiffuseScale", BackBufferDiffuseScale, 0.0f);
        persist.Transfer("BackBufferAmbientScale", BackBufferAmbientScale, 0.0f);
        persist.Transfer("MinimumFilterColor", MinimumFilterColor, 0.0f);
    }
};

class CUIIconsDef : public CDefObject {
public:
    uint32_t IconFriendRequestReceived = 0, IconFriendRequestReceivedOn = 0;
    uint32_t IconFriendRequestSent = 0, IconFriendRequestSentOn = 0;
    uint32_t IconGameInviteReceived = 0, IconGameInviteReceivedOn = 0;
    uint32_t IconGameInviteSent = 0, IconGameInviteSentOn = 0;
    uint32_t IconMute = 0, IconMuteOn = 0;
    uint32_t IconOnline = 0, IconOnlineOn = 0;
    uint32_t IconPasscodeBlank = 0, IconPasscodeFilled = 0;
    uint32_t IconTV = 0, IconTVOn = 0;
    uint32_t IconVoice = 0, IconVoiceOn = 0;
    uint32_t IconWait1 = 0, IconWait2 = 0, IconWait3 = 0, IconWait4 = 0;
    uint32_t IconProgress = 0, IconProgressOn = 0;
    uint32_t IconA = 0, IconB = 0, IconX = 0, IconY = 0;
    uint32_t IconBlank = 0, IconUpArrow = 0, IconDownArrow = 0, IconListHighlight = 0;

    std::string GetClassName() const override { return "UI_ICONS_DEF"; }
    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* p = dynamic_cast<const std::remove_pointer<decltype(this)>::type*>(parentObject)) {
            std::string myName = m_InstantiationName;
            *this = *p;
            m_InstantiationName = myName;
        }
    }

    void Transfer(CPersistContext& persist) override {
        persist.Transfer("IconFriendRequestReceived", IconFriendRequestReceived, (uint32_t)0);
        persist.Transfer("IconFriendRequestReceivedOn", IconFriendRequestReceivedOn, (uint32_t)0);
        persist.Transfer("IconFriendRequestSent", IconFriendRequestSent, (uint32_t)0);
        persist.Transfer("IconFriendRequestSentOn", IconFriendRequestSentOn, (uint32_t)0);
        persist.Transfer("IconGameInviteReceived", IconGameInviteReceived, (uint32_t)0);
        persist.Transfer("IconGameInviteReceivedOn", IconGameInviteReceivedOn, (uint32_t)0);
        persist.Transfer("IconGameInviteSent", IconGameInviteSent, (uint32_t)0);
        persist.Transfer("IconGameInviteSentOn", IconGameInviteSentOn, (uint32_t)0);
        persist.Transfer("IconMute", IconMute, (uint32_t)0);
        persist.Transfer("IconMuteOn", IconMuteOn, (uint32_t)0);
        persist.Transfer("IconOnline", IconOnline, (uint32_t)0);
        persist.Transfer("IconOnlineOn", IconOnlineOn, (uint32_t)0);
        persist.Transfer("IconPasscodeBlank", IconPasscodeBlank, (uint32_t)0);
        persist.Transfer("IconPasscodeFilled", IconPasscodeFilled, (uint32_t)0);
        persist.Transfer("IconTV", IconTV, (uint32_t)0);
        persist.Transfer("IconTVOn", IconTVOn, (uint32_t)0);
        persist.Transfer("IconVoice", IconVoice, (uint32_t)0);
        persist.Transfer("IconVoiceOn", IconVoiceOn, (uint32_t)0);
        persist.Transfer("IconWait1", IconWait1, (uint32_t)0);
        persist.Transfer("IconWait2", IconWait2, (uint32_t)0);
        persist.Transfer("IconWait3", IconWait3, (uint32_t)0);
        persist.Transfer("IconWait4", IconWait4, (uint32_t)0);
        persist.Transfer("IconProgress", IconProgress, (uint32_t)0);
        persist.Transfer("IconProgressOn", IconProgressOn, (uint32_t)0);
        persist.Transfer("IconA", IconA, (uint32_t)0);
        persist.Transfer("IconB", IconB, (uint32_t)0);
        persist.Transfer("IconX", IconX, (uint32_t)0);
        persist.Transfer("IconY", IconY, (uint32_t)0);
        persist.Transfer("IconBlank", IconBlank, (uint32_t)0);
        persist.Transfer("IconUpArrow", IconUpArrow, (uint32_t)0);
        persist.Transfer("IconDownArrow", IconDownArrow, (uint32_t)0);
        persist.Transfer("IconListHighlight", IconListHighlight, (uint32_t)0);
    }
};

template<>
inline void CPersistContext::TransferVector<CActionInputControl>(
    const char* name, std::vector<CActionInputControl>& vec)
{
    if (Mode == MODE_LOAD_TEXT) {
        if (!PDefText) return;
        std::string& text = *PDefText;
        size_t nlen = std::strlen(name);
        size_t pos = 0;
        while (true) {
            size_t found = std::string::npos;
            while (pos + nlen <= text.size()) {
                size_t p = text.find(name, pos);
                if (p == std::string::npos) break;
                bool prevOk = (p == 0) || IsWordBoundary(text[p - 1]);
                bool nextOk = (p + nlen >= text.size()) || IsWordBoundary(text[p + nlen]);
                if (prevOk && nextOk) { found = p; break; }
                pos = p + 1;
            }
            if (found == std::string::npos) break;
            size_t fieldStart = found;
            size_t scanPos = found + nlen;
            SkipWS(scanPos);
            if (scanPos >= text.size()) break;
            char nextCh = text[scanPos];
            if (nextCh == '.') {
                scanPos++;
                std::string cmd = ReadIdent(scanPos);
                if (cmd == "Add") {
                    if (!SkipChar(scanPos, '(')) { pos = scanPos; continue; }
                    CActionInputControl item;
                    item.ParseInline(*this, scanPos);
                    if (!SkipChar(scanPos, ')')) { pos = scanPos; continue; }
                    size_t sc = FindSemicolon(scanPos);
                    BlankRegion(fieldStart, sc != std::string::npos ? sc : scanPos);
                    vec.push_back(item);
                    pos = fieldStart; // re-scan from start (blanked, won't re-match)
                }
                else if (cmd == "clear") {
                    SkipChar(scanPos, '(');
                    SkipChar(scanPos, ')');
                    size_t sc = FindSemicolon(scanPos);
                    BlankRegion(fieldStart, sc != std::string::npos ? sc : scanPos);
                    vec.clear();
                    pos = fieldStart;
                }
                else {
                    pos = scanPos;
                }
            }
            else {
                pos = found + 1; // Not a vector command, skip
            }
        }
        return;
    }
    if (Mode == MODE_SAVE_BINARY) {
        if (!m_ForceNoTags && vec.empty()) return; // SKIP EMPTY VECTORS

        WriteTag(name);
        PSaveStream->WriteSLONG((int32_t)vec.size());
        for (auto& item : vec) item.Transfer(*this);
    }
}

// Factory implementations
inline IDefObject* Alloc_CFrontEndDef() { return new CFrontEndDef(); }
inline IDefObject* Alloc_CControlsDef() { return new CControlSchemeDef(); }
inline IDefObject* Alloc_CEngineDef() { return new CEngineDef(); }
inline IDefObject* Alloc_CConfigOptionsDefaultsDef() { return new CConfigOptionsDefaultsDef(); }
inline IDefObject* Alloc_CEngineVideoOptionsDef() { return new CEngineVideoOptionsDef(); }
inline IDefObject* Alloc_CUIDef() { return new CUIDef(); }
inline IDefObject* Alloc_CUIMiscThingsDef() { return new CUIMiscThingsDef(); }
inline IDefObject* Alloc_CUIIconsDef() { return new CUIIconsDef(); }

