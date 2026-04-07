#pragma once
#include "StringParser.h"
#include "BinaryStreams.h"
#include <map>
#include <string>


// --- frontend.bin ----

// --- SAFE SYMBOL RESOLVER ---
inline uint32_t ResolveSymbol(const std::string& str, const std::map<std::string, int>& symbolMap) {
    auto it = symbolMap.find(str);
    if (it != symbolMap.end()) return it->second;
    try { return std::stoi(str); }
    catch (...) { return 0; }
}

// --- SUBCLASSES& STRUCTURES ---
struct CActionInputControl {
    int32_t GameAction = 0;
    int32_t ControllerType = 0;
    int32_t KeyboardKey = 0;
    int32_t XboxButton = 0;
    int32_t MouseButton = 0;
    C2DVector ControlDirection = { 0.0f, 0.0f };
};

struct CRectF { float TLX = 0.0f, TLY = 0.0f, BRX = 0.0f, BRY = 0.0f; };
struct CRect { int32_t TLX = 0, TLY = 0, BRX = 0, BRY = 0; };
struct CColor { float R = 1.0f, G = 1.0f, B = 1.0f, A = 1.0f; };

struct CUIStateDef {
    uint32_t GraphicIndex = 0;
    C2DVector Position = { 0.0f, 0.0f };
    C2DVector Zoom = { 1.0f, 1.0f };
    CColor Colour;
    float UpdateTime = -1.0f;
    int32_t StateChangeType = 0;
    bool LinearChange = false;
    uint32_t StateChangeFlag = 7;
    std::vector<uint32_t> ChildrenNotAffected;

    void SerializeOut(CDataOutputStream* os) const {
        os->WriteULONG(GraphicIndex); os->Write2DVector(&Position); os->Write2DVector(&Zoom);
        os->WriteFloat(Colour.R); os->WriteFloat(Colour.G); os->WriteFloat(Colour.B); os->WriteFloat(Colour.A);
        os->WriteFloat(UpdateTime); os->WriteSLONG(StateChangeType); os->WriteBool(LinearChange);
        os->WriteULONG(StateChangeFlag); os->WriteVectorUint32(ChildrenNotAffected);
    }
};

// --- CLASSES ---

class IDefObject {
public:
    virtual ~IDefObject() = default;
    virtual void ParseFromText(CStringParser& parser, const std::map<std::string, int>& symbolMap) = 0;
    virtual void SerializeOut(CDataOutputStream* os) const = 0;
    virtual std::string GetClassName() const = 0;
    virtual std::string GetInstantiationName() const = 0;
    virtual void SetInstantiationName(const std::string& name) = 0;
    virtual void CopyFrom(const IDefObject* parentObject) = 0;
};

class CFrontEndDef : public IDefObject {
public:
    std::string m_InstantiationName;

    std::vector<std::string> vAttractModeMovie;
    int32_t ErrorMessageBackgroundGraphic = 0;
    int32_t ButtonABigGraphic = 0;
    int32_t ButtonBBigGraphic = 0;

    std::string GetClassName() const override { return "FRONT_END"; }
    std::string GetInstantiationName() const override { return m_InstantiationName; }
    void SetInstantiationName(const std::string& name) override { m_InstantiationName = name; }

    void CopyFrom(const IDefObject* parentObject) override {
        const CFrontEndDef* parent = dynamic_cast<const CFrontEndDef*>(parentObject);
        if (parent) {
            vAttractModeMovie = parent->vAttractModeMovie;
            ErrorMessageBackgroundGraphic = parent->ErrorMessageBackgroundGraphic;
            ButtonABigGraphic = parent->ButtonABigGraphic;
            ButtonBBigGraphic = parent->ButtonBBigGraphic;
        }
    }
    void ParseFromText(CStringParser& parser, const std::map<std::string, int>& symbolMap) override {
        CParsedItem item;

        if (parser.PeekNextItem(item) && item.Type == EParsedItemType::Identifier) {
            std::string propName = item.StringValue;

            if (propName == "vAttractModeMovie") {
                parser.ReadNextItem(item);

                if (parser.SkipPastString("(")) {
                    std::string movieName;
                    if (parser.ReadNextItemAsQuotedString(movieName)) {
                        vAttractModeMovie.push_back(movieName);
                    }
                }
                parser.SkipPastString(";");
            }
            else if (propName == "ErrorMessageBackgroundGraphic") {
                parser.ReadNextItem(item);

                std::string enumName;
                if (parser.ReadAsString(enumName)) {
                    ErrorMessageBackgroundGraphic = ResolveSymbol(enumName, symbolMap);
                }
                parser.SkipPastString(";");
            }
            else if (propName == "ButtonABigGraphic") {
                parser.ReadNextItem(item);
                std::string enumName;
                if (parser.ReadAsString(enumName)) {
                    ButtonABigGraphic = ResolveSymbol(enumName, symbolMap);
                }
                parser.SkipPastString(";");
            }
            else if (propName == "ButtonBBigGraphic") {
                parser.ReadNextItem(item);
                std::string enumName;
                if (parser.ReadAsString(enumName)) {
                    ButtonBBigGraphic = ResolveSymbol(enumName, symbolMap);
                }
                parser.SkipPastString(";");
            }
        }
    }

    void SerializeOut(CDataOutputStream* os) const override {
        os->WriteSLONG(vAttractModeMovie.size());
        for (const auto& item : vAttractModeMovie) {
            os->WriteString(item);
        }
        os->WriteSLONG(ErrorMessageBackgroundGraphic);
        os->WriteSLONG(ButtonABigGraphic);
        os->WriteSLONG(ButtonBBigGraphic);
    }
};

class CControlsDef : public IDefObject {
public:
    std::string m_InstantiationName;
    std::vector<CActionInputControl> Controls;
    bool ToggleZTarget = false;
    bool ToggleSpells = false;
    bool ToggleSneak = false;
    bool ToggleExpressionMenu = false;
    bool ToggleExpressionShift = false;
    bool FlourishNeedsAttackButtonHeld = false;

    std::string GetClassName() const override { return "CONTROL_SCHEME"; }
    std::string GetInstantiationName() const override { return m_InstantiationName; }
    void SetInstantiationName(const std::string& name) override { m_InstantiationName = name; }

    void CopyFrom(const IDefObject* parentObject) override {
        const CControlsDef* parent = dynamic_cast<const CControlsDef*>(parentObject);
        if (parent) {
            Controls = parent->Controls;
            ToggleZTarget = parent->ToggleZTarget;
            ToggleSpells = parent->ToggleSpells;
            ToggleSneak = parent->ToggleSneak;
            ToggleExpressionMenu = parent->ToggleExpressionMenu;
            ToggleExpressionShift = parent->ToggleExpressionShift;
            FlourishNeedsAttackButtonHeld = parent->FlourishNeedsAttackButtonHeld;
        }
    }

    void ParseFromText(CStringParser& parser, const std::map<std::string, int>& symbolMap) override {
        CParsedItem item;

        if (parser.PeekNextItem(item) && item.Type == EParsedItemType::Identifier) {
            std::string propName = item.StringValue;

            if (propName == "Controls") {
                parser.ReadNextItem(item);

                if (parser.SkipPastString(".Add(CActionInputControl(")) {
                    CActionInputControl ctrl;
                    std::string tempStr;

                    if (parser.ReadAsString(tempStr)) ctrl.GameAction = ResolveSymbol(tempStr, symbolMap);
                    parser.SkipPastString(",");

                    std::string typeStr;
                    if (parser.ReadAsString(typeStr)) ctrl.ControllerType = ResolveSymbol(typeStr, symbolMap);
                    parser.SkipPastString(",");

                    if (parser.ReadAsString(tempStr)) {
                        int32_t btnVal = ResolveSymbol(tempStr, symbolMap);
                        if (typeStr == "CONTROLLER_KEYBOARD") ctrl.KeyboardKey = btnVal;
                        else if (typeStr == "CONTROLLER_MOUSE") ctrl.MouseButton = btnVal;
                        else ctrl.XboxButton = btnVal;
                    }

                    if (parser.PeekNextItem(item) && item.StringValue == ",") {
                        parser.ReadNextItem(item);
                        if (parser.SkipPastString("C2DCoordF(")) {
                            ctrl.ControlDirection.x = parser.ReadAsFloat();
                            parser.SkipPastString(",");
                            ctrl.ControlDirection.y = parser.ReadAsFloat();
                            parser.SkipPastString(")");
                        }
                    }

                    parser.SkipPastString("));");
                    Controls.push_back(ctrl);
                }
            }
            else if (propName == "ToggleZTarget" || propName == "ToggleSpells" ||
                propName == "ToggleSneak" || propName == "ToggleExpressionMenu" ||
                propName == "ToggleExpressionShift" || propName == "FlourishNeedsAttackButtonHeld") {

                parser.ReadNextItem(item);

                std::string boolStr;
                if (parser.ReadAsString(boolStr)) {
                    bool val = (boolStr == "TRUE" || boolStr == "true");

                    if (propName == "ToggleZTarget") ToggleZTarget = val;
                    else if (propName == "ToggleSpells") ToggleSpells = val;
                    else if (propName == "ToggleSneak") ToggleSneak = val;
                    else if (propName == "ToggleExpressionMenu") ToggleExpressionMenu = val;
                    else if (propName == "ToggleExpressionShift") ToggleExpressionShift = val;
                    else if (propName == "FlourishNeedsAttackButtonHeld") FlourishNeedsAttackButtonHeld = val;
                }
                parser.SkipPastString(";");
            }
        }
    }

    void SerializeOut(CDataOutputStream* os) const override {
        os->WriteSLONG((int32_t)Controls.size());

        for (const auto& ctrl : Controls) {
            os->WriteSLONG(ctrl.GameAction);
            os->WriteSLONG(ctrl.ControllerType);
            os->WriteSLONG(ctrl.KeyboardKey);
            os->WriteSLONG(ctrl.XboxButton);
            os->WriteSLONG(ctrl.MouseButton);
            os->Write2DVector(&ctrl.ControlDirection);
        }

        os->WriteBool(ToggleZTarget);
        os->WriteBool(ToggleSpells);
        os->WriteBool(ToggleSneak);
        os->WriteBool(ToggleExpressionMenu);
        os->WriteBool(ToggleExpressionShift);
        os->WriteBool(FlourishNeedsAttackButtonHeld);
    }
};

class CEngineDef : public IDefObject {
public:
    std::string m_InstantiationName;

    float LODErrorTolerance = 0.0f;
    float CharacterLODErrorTolerance = 0.0f;
    float LODErrorFactor = 0.0f;
    float SeaHeight = 0.0f;
    int32_t LocalDetailBooleanAlphaDefaultAlphaRef = 0;
    int32_t DefaultPrimitiveAlphaRef = 0;

    float GamePrimitiveDefaultFadeStart = 0.0f;
    float GamePrimitiveDefaultFadeRangeRatio = 0.0f;
    float LocalDetailDefaultFadeStart = 0.0f;
    float LocalDetailDefaultFadeRangeRatio = 0.0f;

    int32_t TestStaticMesh = 0;
    int32_t TestAnimatedMesh = 0;
    int32_t TestAnim = 0;
    int32_t TestGraphic = 0;

    float FOV_2D = 0.0f;

    int32_t InvalidTextureStandin = 0;
    int32_t InvalidThemeStandin = 0;

    std::string GetClassName() const override { return "ENGINE"; }
    std::string GetInstantiationName() const override { return m_InstantiationName; }
    void SetInstantiationName(const std::string& name) override { m_InstantiationName = name; }

    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* parent = dynamic_cast<const CEngineDef*>(parentObject)) {
            *this = *parent;
        }
    }

    void ParseFromText(CStringParser& parser, const std::map<std::string, int>& symbolMap) override {
        CParsedItem item;

        if (parser.PeekNextItem(item) && item.Type == EParsedItemType::Identifier) {
            std::string prop = item.StringValue;

            if (prop == "LODErrorTolerance" || prop == "CharacterLODErrorTolerance" ||
                prop == "LODErrorFactor" || prop == "SeaHeight" ||
                prop == "GamePrimitiveDefaultFadeStart" || prop == "GamePrimitiveDefaultFadeRangeRatio" ||
                prop == "LocalDetailDefaultFadeStart" || prop == "LocalDetailDefaultFadeRangeRatio" ||
                prop == "FOV_2D") {

                parser.ReadNextItem(item);
                float val = parser.ReadAsFloat();

                if (prop == "LODErrorTolerance") LODErrorTolerance = val;
                else if (prop == "CharacterLODErrorTolerance") CharacterLODErrorTolerance = val;
                else if (prop == "LODErrorFactor") LODErrorFactor = val;
                else if (prop == "SeaHeight") SeaHeight = val;
                else if (prop == "GamePrimitiveDefaultFadeStart") GamePrimitiveDefaultFadeStart = val;
                else if (prop == "GamePrimitiveDefaultFadeRangeRatio") GamePrimitiveDefaultFadeRangeRatio = val;
                else if (prop == "LocalDetailDefaultFadeStart") LocalDetailDefaultFadeStart = val;
                else if (prop == "LocalDetailDefaultFadeRangeRatio") LocalDetailDefaultFadeRangeRatio = val;
                else if (prop == "FOV_2D") FOV_2D = val;

                parser.SkipPastString(";");
            }
            else if (prop == "LocalDetailBooleanAlphaDefaultAlphaRef" || prop == "DefaultPrimitiveAlphaRef") {
                parser.ReadNextItem(item);
                int32_t val = parser.ReadAsInteger();

                if (prop == "LocalDetailBooleanAlphaDefaultAlphaRef") LocalDetailBooleanAlphaDefaultAlphaRef = val;
                else if (prop == "DefaultPrimitiveAlphaRef") DefaultPrimitiveAlphaRef = val;

                parser.SkipPastString(";");
            }
            else if (prop == "TestStaticMesh" || prop == "TestAnimatedMesh" ||
                prop == "TestAnim" || prop == "TestGraphic" ||
                prop == "InvalidTextureStandin" || prop == "InvalidThemeStandin") {

                parser.ReadNextItem(item);
                std::string enumName;
                if (parser.ReadAsString(enumName)) {
                    int32_t val = ResolveSymbol(enumName, symbolMap);
                    if (prop == "TestStaticMesh") TestStaticMesh = val;
                    else if (prop == "TestAnimatedMesh") TestAnimatedMesh = val;
                    else if (prop == "TestAnim") TestAnim = val;
                    else if (prop == "TestGraphic") TestGraphic = val;
                    else if (prop == "InvalidTextureStandin") InvalidTextureStandin = val;
                    else if (prop == "InvalidThemeStandin") InvalidThemeStandin = val;
                }
                parser.SkipPastString(";");
            }
        }
    }

    void SerializeOut(CDataOutputStream* os) const override {
        os->WriteFloat(LODErrorTolerance);
        os->WriteFloat(CharacterLODErrorTolerance);
        os->WriteFloat(LODErrorFactor);
        os->WriteFloat(SeaHeight);
        os->WriteSLONG(LocalDetailBooleanAlphaDefaultAlphaRef);
        os->WriteSLONG(DefaultPrimitiveAlphaRef);
        os->WriteFloat(GamePrimitiveDefaultFadeStart);
        os->WriteFloat(GamePrimitiveDefaultFadeRangeRatio);
        os->WriteFloat(LocalDetailDefaultFadeStart);
        os->WriteFloat(LocalDetailDefaultFadeRangeRatio);
        os->WriteSLONG(TestStaticMesh);
        os->WriteSLONG(TestAnimatedMesh);
        os->WriteSLONG(TestAnim);
        os->WriteSLONG(TestGraphic);
        os->WriteFloat(FOV_2D);
        os->WriteSLONG(InvalidTextureStandin);
        os->WriteSLONG(InvalidThemeStandin);
    }
};

class CConfigOptionsDefaultsDef : public IDefObject {
public:
    std::string m_InstantiationName;

    uint32_t Antialiasing = 0;
    int32_t ResolutionWidth = 1024, MinResolutionWidth = 640;
    int32_t ResolutionHeight = 768, MinResolutionHeight = 480;
    int32_t BitDepth = 16;

    float TextureDetail = 1.0f, MaxTextureDetail = 3.0f;
    float ShadowDetail = 1.0f, MaxShadowDetail = 3.0f;
    float MeshDetail = 1.0f, MaxMeshDetail = 3.0f;
    float EffectsDetail = 1.0f, MaxEffectsDetail = 3.0f;

    std::string GetClassName() const override { return "CONFIG_OPTIONS_DEFAULTS_DEF"; }
    std::string GetInstantiationName() const override { return m_InstantiationName; }
    void SetInstantiationName(const std::string& name) override { m_InstantiationName = name; }

    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* parent = dynamic_cast<const CConfigOptionsDefaultsDef*>(parentObject)) {
            *this = *parent;
        }
    }

    void ParseFromText(CStringParser& parser, const std::map<std::string, int>& symbolMap) override {
        CParsedItem item;
        if (parser.PeekNextItem(item) && item.Type == EParsedItemType::Identifier) {
            std::string prop = item.StringValue;

            if (prop == "Antialiasing" || prop == "ResolutionWidth" || prop == "ResolutionHeight" ||
                prop == "BitDepth" || prop == "MinResolutionWidth" || prop == "MinResolutionHeight") {

                parser.ReadNextItem(item);
                int32_t val = parser.ReadAsInteger();

                if (prop == "Antialiasing") Antialiasing = val;
                else if (prop == "ResolutionWidth") ResolutionWidth = val;
                else if (prop == "ResolutionHeight") ResolutionHeight = val;
                else if (prop == "BitDepth") BitDepth = val;
                else if (prop == "MinResolutionWidth") MinResolutionWidth = val;
                else if (prop == "MinResolutionHeight") MinResolutionHeight = val;

                parser.SkipPastString(";");
            }
            else if (prop == "TextureDetail" || prop == "MaxTextureDetail" || prop == "ShadowDetail" ||
                prop == "MaxShadowDetail" || prop == "MeshDetail" || prop == "MaxMeshDetail" ||
                prop == "EffectsDetail" || prop == "MaxEffectsDetail") {

                parser.ReadNextItem(item);
                float val = parser.ReadAsFloat();

                if (prop == "TextureDetail") TextureDetail = val;
                else if (prop == "MaxTextureDetail") MaxTextureDetail = val;
                else if (prop == "ShadowDetail") ShadowDetail = val;
                else if (prop == "MaxShadowDetail") MaxShadowDetail = val;
                else if (prop == "MeshDetail") MeshDetail = val;
                else if (prop == "MaxMeshDetail") MaxMeshDetail = val;
                else if (prop == "EffectsDetail") EffectsDetail = val;
                else if (prop == "MaxEffectsDetail") MaxEffectsDetail = val;

                parser.SkipPastString(";");
            }
        }
    }

    void SerializeOut(CDataOutputStream* os) const override {
        os->WriteULONG(Antialiasing);
        os->WriteSLONG(ResolutionWidth);
        os->WriteSLONG(ResolutionHeight);
        os->WriteSLONG(BitDepth);
        os->WriteFloat(TextureDetail);
        os->WriteFloat(MaxTextureDetail);
        os->WriteFloat(ShadowDetail);
        os->WriteFloat(MaxShadowDetail);
        os->WriteFloat(MeshDetail);
        os->WriteFloat(MaxMeshDetail);
        os->WriteFloat(EffectsDetail);
        os->WriteFloat(MaxEffectsDetail);
        os->WriteSLONG(MinResolutionWidth);
        os->WriteSLONG(MinResolutionHeight);
    }
};

class CEngineVideoOptionsDef : public IDefObject {
public:
    std::string m_InstantiationName;
    int32_t HiresTextureMemory = 0;

    float LODErrorTolerance = 0.0f;
    float CharacterLODErrorTolerance = 0.0f;
    float DrawDistanceMultiplier = 0.0f;
    float DrawDistanceMinimum = 0.0f;
    float DrawDistanceMaximum = 0.0f;
    float RepeatedMeshDrawDistanceFactor = 0.0f;
    float MinimumZSpriteAsMeshDistance = 0.0f;
    float MaximumZSpriteAsMeshDistance = 0.0f;
    float ZSpriteDrawDistanceMultiplier = 0.0f;

    int32_t ShadowBufferSize = 1024;
    float ShadowDistanceScale = 1.0f;

    bool Enable2DDisplacement = false;
    bool Enable3DDisplacement = false;
    bool EnableGlow = false;
    bool EnableRadialBlur = false;
    bool EnableWaterReflection = false;
    bool EnableWeatherEffects = false;
    bool EnableColourFilter = false;

    float WeatherDensity = 1.0f;
    bool EnableRepeatedMeshes = true;

    std::string GetClassName() const override { return "ENGINE_VIDEO_OPTIONS"; }
    std::string GetInstantiationName() const override { return m_InstantiationName; }
    void SetInstantiationName(const std::string& name) override { m_InstantiationName = name; }

    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* parent = dynamic_cast<const CEngineVideoOptionsDef*>(parentObject)) {
            *this = *parent;
        }
    }

    void ParseFromText(CStringParser& parser, const std::map<std::string, int>& symbolMap) override {
        CParsedItem item;

        if (parser.PeekNextItem(item) && item.Type == EParsedItemType::Identifier) {
            std::string prop = item.StringValue;

            if (prop == "HiresTextureMemory" || prop == "ShadowBufferSize") {
                parser.ReadNextItem(item);
                int32_t val = parser.ReadAsInteger();

                if (prop == "HiresTextureMemory") HiresTextureMemory = val;
                else if (prop == "ShadowBufferSize") ShadowBufferSize = val;

                parser.SkipPastString(";");
            }
            else if (prop == "LODErrorTolerance" || prop == "CharacterLODErrorTolerance" ||
                prop == "DrawDistanceMultiplier" || prop == "DrawDistanceMinimum" ||
                prop == "DrawDistanceMaximum" || prop == "RepeatedMeshDrawDistanceFactor" ||
                prop == "MinimumZSpriteAsMeshDistance" || prop == "MaximumZSpriteAsMeshDistance" ||
                prop == "ZSpriteDrawDistanceMultiplier" || prop == "ShadowDistanceScale" ||
                prop == "WeatherDensity") {

                parser.ReadNextItem(item);
                float val = parser.ReadAsFloat();

                if (prop == "LODErrorTolerance") LODErrorTolerance = val;
                else if (prop == "CharacterLODErrorTolerance") CharacterLODErrorTolerance = val;
                else if (prop == "DrawDistanceMultiplier") DrawDistanceMultiplier = val;
                else if (prop == "DrawDistanceMinimum") DrawDistanceMinimum = val;
                else if (prop == "DrawDistanceMaximum") DrawDistanceMaximum = val;
                else if (prop == "RepeatedMeshDrawDistanceFactor") RepeatedMeshDrawDistanceFactor = val;
                else if (prop == "MinimumZSpriteAsMeshDistance") MinimumZSpriteAsMeshDistance = val;
                else if (prop == "MaximumZSpriteAsMeshDistance") MaximumZSpriteAsMeshDistance = val;
                else if (prop == "ZSpriteDrawDistanceMultiplier") ZSpriteDrawDistanceMultiplier = val;
                else if (prop == "ShadowDistanceScale") ShadowDistanceScale = val;
                else if (prop == "WeatherDensity") WeatherDensity = val;

                parser.SkipPastString(";");
            }
            else if (prop == "Enable2DDisplacement" || prop == "Enable3DDisplacement" ||
                prop == "EnableGlow" || prop == "EnableRadialBlur" ||
                prop == "EnableWaterReflection" || prop == "EnableWeatherEffects" ||
                prop == "EnableColourFilter" || prop == "EnableRepeatedMeshes") {

                parser.ReadNextItem(item);
                std::string boolStr;
                if (parser.ReadAsString(boolStr)) {
                    bool val = (boolStr == "TRUE" || boolStr == "true");

                    if (prop == "Enable2DDisplacement") Enable2DDisplacement = val;
                    else if (prop == "Enable3DDisplacement") Enable3DDisplacement = val;
                    else if (prop == "EnableGlow") EnableGlow = val;
                    else if (prop == "EnableRadialBlur") EnableRadialBlur = val;
                    else if (prop == "EnableWaterReflection") EnableWaterReflection = val;
                    else if (prop == "EnableWeatherEffects") EnableWeatherEffects = val;
                    else if (prop == "EnableColourFilter") EnableColourFilter = val;
                    else if (prop == "EnableRepeatedMeshes") EnableRepeatedMeshes = val;
                }
                parser.SkipPastString(";");
            }
        }
    }

    void SerializeOut(CDataOutputStream* os) const override {
        os->WriteSLONG(HiresTextureMemory);
        os->WriteFloat(LODErrorTolerance);
        os->WriteFloat(CharacterLODErrorTolerance);
        os->WriteFloat(DrawDistanceMultiplier);
        os->WriteFloat(DrawDistanceMinimum);
        os->WriteFloat(DrawDistanceMaximum);
        os->WriteFloat(RepeatedMeshDrawDistanceFactor);
        os->WriteFloat(MinimumZSpriteAsMeshDistance);
        os->WriteFloat(MaximumZSpriteAsMeshDistance);
        os->WriteFloat(ZSpriteDrawDistanceMultiplier);
        os->WriteSLONG(ShadowBufferSize);
        os->WriteFloat(ShadowDistanceScale);
        os->WriteBool(Enable2DDisplacement);
        os->WriteBool(Enable3DDisplacement);
        os->WriteBool(EnableGlow);
        os->WriteBool(EnableRadialBlur);
        os->WriteBool(EnableWaterReflection);
        os->WriteBool(EnableWeatherEffects);
        os->WriteBool(EnableColourFilter);
        os->WriteFloat(WeatherDensity);
        os->WriteBool(EnableRepeatedMeshes);
    }
};

class CUIDef : public IDefObject {
public:
    std::string m_InstantiationName;

    uint32_t Type = 0;
    std::vector<uint32_t> Children;
    uint32_t MeshIndex = 0;
    std::string TextValue;
    std::string Font;
    float Height = 0.0f, Width = 0.0f;
    uint32_t ExpansionType = 1;
    std::map<uint32_t, uint32_t> Sprites;
    std::vector<uint32_t> HorizontalSeparations, VerticalSeparations;
    std::vector<CUIStateDef> States;
    bool TextLineBreak = true, ScaleText = true, Independant = false;
    uint32_t MeshType = 5;
    std::vector<uint32_t> NonScrollingChildren;
    CRectF TextWindow;
    int32_t Layer = 0;
    float Angle = 0.0f;
    bool PositionIsCenter = false;
    float ScrollingSpeed = 1.0f;
    bool Wrapping = true, Inverted = false;
    C2DVector PositionOffset = { 0.0f, 0.0f };
    uint8_t AlphaOffset = 0;
    C3DVector Up = { 0.0f, 0.0f, 1.0f }, Forward = { 0.0f, 1.0f, 0.0f }, RotationAxis = { 0.0f, 1.0f, 0.0f };
    float RotationSpeed = 0.0f;
    uint32_t AnimationIndex = 0;
    int32_t DownArrow = 0, UpArrow = 0, UpLimit = 0, DownLimit = 0;
    bool Scrolling = true, ComputeOffsetsOnActivate = false;
    C2DVector Min = { 0,0 }, Max = { 0,0 }, Step = { 0,0 }, Dimensions = { 0,0 };
    int32_t SliderLeft = 0, SliderRight = 0;

    uint32_t Action = 0, ActionOnBack = 0, ActionOnSelected = 0, ActionOnUnselected = 0, ActionOnDestruction = 0;
    uint32_t ActionOnLeftClicked = 0, ActionOnLeftUnclicked = 0, ActionOnLeftHeld = 0, ActionOnRightClicked = 0;
    uint32_t ActionOnDropped = 0, ActionOnDroppedNowhere = 0, PreAction = 0, ActionOnDraggedUp = 0;
    uint32_t ActionOnDraggedDown = 0, ActionOnLeftClickedAbove = 0, ActionOnLeftClickedUnder = 0;

    float InputDelay = 0.2f;
    bool DrawFromViewport = false;
    uint32_t TextBankIndex = 0;
    int32_t ActionText = 0, KeyText = 0, Redefiner = 0, UndefinedWarning = 0;

    std::map<uint32_t, std::string> ActionMap;
    std::map<uint32_t, uint32_t> ActionMapAliases;
    std::vector<uint32_t> ActionOrder;

    bool EditBoxParentIsButton = false, PasswordBox = false;
    int32_t EditBoxCharLimit = 0;
    bool EditBoxUsesIME = false;
    std::string MovieFilename;
    bool DisallowSpaceAsFirstChar = false, LayerIndependant = false;
    std::vector<uint32_t> SwappingStates;
    std::vector<float> SwappingTimes;
    bool BastardChild = false;
    uint32_t Alignement = 0;
    bool RandomSwap = false, UseRelativeZoom = false, UseRelativePosition = false;
    int32_t HoveredState = 3, LeftClickedState = 3, RightClickedState = 3;
    std::vector<uint32_t> ShapeChildren;
    CRect ViewArea = { 0,0,640,480 };
    bool UseViewArea = false, PartOfListTree = true, PCStyle = false;
    uint32_t Sprite2DFlag = 2;

    std::string GetClassName() const override { return "UI"; }
    std::string GetInstantiationName() const override { return m_InstantiationName; }
    void SetInstantiationName(const std::string& name) override { m_InstantiationName = name; }
    void CopyFrom(const IDefObject* parentObject) override { if (const auto* p = dynamic_cast<const CUIDef*>(parentObject)) *this = *p; }

    void ParseFromText(CStringParser& parser, const std::map<std::string, int>& symbolMap) override {
        CParsedItem item;
        if (!parser.PeekNextItem(item) || item.Type != EParsedItemType::Identifier) return;
        std::string prop = item.StringValue;

        if (prop == "Children" || prop == "ShapeChildren" || prop == "ActionOrder" ||
            prop == "SwappingStates" || prop == "HorizontalSeparations" || prop == "VerticalSeparations" ||
            prop == "NonScrollingChildren" || prop == "Sprites" || prop == "ActionMap" || prop == "ActionMapAliases") {

            parser.ReadNextItem(item);
            parser.SkipPastString("[");
            std::string keyStr; parser.ReadAsIdentifierOrNumber(keyStr);
            uint32_t key = ResolveSymbol(keyStr, symbolMap);
            parser.SkipPastString("]");

            if (prop == "ActionMap") {
                std::string valStr;
                if (parser.ReadNextItemAsQuotedString(valStr)) ActionMap[key] = valStr;
            }
            else {
                std::string valStr; parser.ReadAsIdentifierOrNumber(valStr);
                uint32_t val = ResolveSymbol(valStr, symbolMap);

                if (prop == "Sprites") Sprites[key] = val;
                else if (prop == "ActionMapAliases") ActionMapAliases[key] = val;
                else {
                    std::vector<uint32_t>* targetVec = nullptr;
                    if (prop == "Children") targetVec = &Children;
                    else if (prop == "ShapeChildren") targetVec = &ShapeChildren;
                    else if (prop == "ActionOrder") targetVec = &ActionOrder;
                    else if (prop == "SwappingStates") targetVec = &SwappingStates;

                    if (targetVec) {
                        if (key >= targetVec->size()) targetVec->resize(key + 1);
                        (*targetVec)[key] = val;
                    }
                }
            }
            parser.SkipPastString(";");
            return;
        }
        if (prop == "States") {
            parser.ReadNextItem(item);
            parser.SkipPastString("[");
            std::string keyStr; parser.ReadAsIdentifierOrNumber(keyStr);
            uint32_t index = ResolveSymbol(keyStr, symbolMap);
            parser.SkipPastString("].");

            if (index >= States.size()) States.resize(index + 1);

            std::string subProp; parser.ReadAsString(subProp);
            if (subProp == "GraphicIndex") {
                std::string valStr; parser.ReadAsIdentifierOrNumber(valStr);
                States[index].GraphicIndex = ResolveSymbol(valStr, symbolMap);
            }
            else if (subProp == "PositionX") States[index].Position.x = parser.ReadAsFloat();
            else if (subProp == "PositionY") States[index].Position.y = parser.ReadAsFloat();
            else if (subProp == "ZoomX") States[index].Zoom.x = parser.ReadAsFloat();
            else if (subProp == "ZoomY") States[index].Zoom.y = parser.ReadAsFloat();
            else if (subProp == "ColourA") States[index].Colour.A = parser.ReadAsFloat();

            parser.SkipPastString(";");
            return;
        }
        parser.ReadNextItem(item);
        if (prop == "Font") parser.ReadNextItemAsQuotedString(Font);
        else if (prop == "TextValue") parser.ReadNextItemAsQuotedString(TextValue);
        else if (prop == "MovieFilename") parser.ReadNextItemAsQuotedString(MovieFilename);
        else if (prop == "Type" || prop == "MeshIndex" || prop == "AnimationIndex" || prop == "Action" || prop == "Layer" || prop == "MeshType" || prop == "ExpansionType") {
            std::string valStr; parser.ReadAsIdentifierOrNumber(valStr);
            uint32_t val = ResolveSymbol(valStr, symbolMap);
            if (prop == "Type") Type = val; else if (prop == "MeshIndex") MeshIndex = val; else if (prop == "AnimationIndex") AnimationIndex = val;
            else if (prop == "Action") Action = val; else if (prop == "Layer") Layer = (int32_t)val; else if (prop == "MeshType") MeshType = val;
            else if (prop == "ExpansionType") ExpansionType = val;
        }
        else if (prop == "Height" || prop == "Width" || prop == "ScrollingSpeed" || prop == "PositionOffsetX" || prop == "PositionOffsetY" || prop == "TextWindowBRX" || prop == "TextWindowBRY") {
            float val = parser.ReadAsFloat();
            if (prop == "Height") Height = val; else if (prop == "Width") Width = val; else if (prop == "ScrollingSpeed") ScrollingSpeed = val;
            else if (prop == "PositionOffsetX") PositionOffset.x = val; else if (prop == "PositionOffsetY") PositionOffset.y = val;
            else if (prop == "TextWindowBRX") TextWindow.BRX = val; else if (prop == "TextWindowBRY") TextWindow.BRY = val;
        }
        else if (prop == "Independant" || prop == "Scrolling" || prop == "Wrapping" || prop == "TextLineBreak" || prop == "ScaleText") {
            std::string boolStr; parser.ReadAsString(boolStr);
            bool val = (boolStr == "TRUE" || boolStr == "BTRUE" || boolStr == "true");
            if (prop == "Independant") Independant = val; else if (prop == "Scrolling") Scrolling = val; else if (prop == "Wrapping") Wrapping = val;
            else if (prop == "TextLineBreak") TextLineBreak = val; else if (prop == "ScaleText") ScaleText = val;
        }

        parser.SkipPastString(";");
    }

    void SerializeOut(CDataOutputStream* os) const override {
        os->WriteULONG(Type); os->WriteVectorUint32(Children); os->WriteULONG(MeshIndex);
        os->WriteWideString(std::wstring(TextValue.begin(), TextValue.end()));
        os->WriteString(Font);
        os->WriteFloat(Height); os->WriteFloat(Width); os->WriteULONG(ExpansionType);

        os->WriteMapUint32ToUint32(Sprites);
        os->WriteVectorUint32(HorizontalSeparations); os->WriteVectorUint32(VerticalSeparations);

        os->WriteSLONG((int32_t)States.size());
        for (const auto& state : States) state.SerializeOut(os);

        os->WriteBool(TextLineBreak); os->WriteBool(ScaleText); os->WriteBool(Independant); os->WriteULONG(MeshType);
        os->WriteVectorUint32(NonScrollingChildren);
        os->WriteFloat(TextWindow.TLX); os->WriteFloat(TextWindow.TLY); os->WriteFloat(TextWindow.BRX); os->WriteFloat(TextWindow.BRY);
        os->WriteSLONG(Layer); os->WriteFloat(Angle); os->WriteBool(PositionIsCenter); os->WriteFloat(ScrollingSpeed);
        os->WriteBool(Wrapping); os->WriteBool(Inverted); os->Write2DVector(&PositionOffset); os->Write(&AlphaOffset, 1);
        os->Write3DVector(&Up); os->Write3DVector(&Forward); os->Write3DVector(&RotationAxis); os->WriteFloat(RotationSpeed);
        os->WriteULONG(AnimationIndex);
        os->WriteSLONG(DownArrow); os->WriteSLONG(UpArrow); os->WriteSLONG(UpLimit); os->WriteSLONG(DownLimit);
        os->WriteBool(Scrolling); os->WriteBool(ComputeOffsetsOnActivate);
        os->Write2DVector(&Min); os->Write2DVector(&Max); os->Write2DVector(&Step); os->Write2DVector(&Dimensions);
        os->WriteSLONG(SliderLeft); os->WriteSLONG(SliderRight);

        os->WriteULONG(Action); os->WriteULONG(ActionOnBack); os->WriteULONG(ActionOnSelected); os->WriteULONG(ActionOnUnselected);
        os->WriteULONG(ActionOnDestruction); os->WriteULONG(ActionOnLeftClicked); os->WriteULONG(ActionOnLeftUnclicked); os->WriteULONG(ActionOnLeftHeld);
        os->WriteULONG(ActionOnRightClicked); os->WriteULONG(ActionOnDropped); os->WriteULONG(ActionOnDroppedNowhere); os->WriteULONG(PreAction);
        os->WriteULONG(ActionOnDraggedUp); os->WriteULONG(ActionOnDraggedDown); os->WriteULONG(ActionOnLeftClickedAbove); os->WriteULONG(ActionOnLeftClickedUnder);

        os->WriteFloat(InputDelay); os->WriteBool(DrawFromViewport); os->WriteULONG(TextBankIndex);
        os->WriteSLONG(ActionText); os->WriteSLONG(KeyText); os->WriteSLONG(Redefiner); os->WriteSLONG(UndefinedWarning);

        os->WriteMapUint32ToString(ActionMap); os->WriteMapUint32ToUint32(ActionMapAliases); os->WriteVectorUint32(ActionOrder);

        os->WriteBool(EditBoxParentIsButton); os->WriteBool(PasswordBox); os->WriteSLONG(EditBoxCharLimit); os->WriteBool(EditBoxUsesIME);
        os->WriteWideString(std::wstring(MovieFilename.begin(), MovieFilename.end()));
        os->WriteBool(DisallowSpaceAsFirstChar); os->WriteBool(LayerIndependant);
        os->WriteVectorUint32(SwappingStates); os->WriteVectorFloat(SwappingTimes);
        os->WriteBool(BastardChild); os->WriteULONG(Alignement);
        os->WriteBool(RandomSwap); os->WriteBool(UseRelativeZoom); os->WriteBool(UseRelativePosition);
        os->WriteSLONG(HoveredState); os->WriteSLONG(LeftClickedState); os->WriteSLONG(RightClickedState);
        os->WriteVectorUint32(ShapeChildren);
        os->WriteSLONG(ViewArea.TLX); os->WriteSLONG(ViewArea.TLY); os->WriteSLONG(ViewArea.BRX); os->WriteSLONG(ViewArea.BRY);
        os->WriteBool(UseViewArea); os->WriteBool(PartOfListTree); os->WriteBool(PCStyle); os->WriteULONG(Sprite2DFlag);
    }
};

class CUIMiscThingsDef : public IDefObject {
public:
    std::string m_InstantiationName;

    std::string SpaceSeparator, CommaSeparator, NewLineSeparator, OpenBracket, CloseBracket;
    std::string Positive, WeaponValueString, WeaponAugString, WeaponAugNone, WeaponWeightString;
    std::string WeaponLightString, WeaponHeavyString, WeaponKillsString, WeaponCatMeleeString;
    std::string WeaponCatRangedString, WeaponDamageString, TradeCostString, ColonSeparator;
    std::string TradeProfitString, TradeLossString, TradeAlreadyOwnsString, TradeNumberInStockString;
    std::string TradeDeliveryString, TradeNoDeliveryString, TradeDaysString, TradeBuyString;
    std::string TradeSellString, TradeWantedString, QuestFailedString, FailedString, SucceededString, Plus, Minus;

    uint32_t CoreGraphic = 0, VignetteGraphic = 0, OptionalGraphic = 0, FeatGraphic = 0;

    std::string ObjectsRewardString, NoneString, CheckGuildString, QuestStartingString;

    C2DVector RingCenter = { 0,0 }, PCRingCenter = { 0,0 }, WorldMapOffset = { 0,0 };
    float WorldMapWidth = 0, WorldMapHeight = 0;

    std::string YouString, OwnString, NoString, HousesString, HouseString, InString, ShopsString, ShopString;
    std::string ThereString, AreString, IsString, ForString, SaleString, GeneralString, TatooString, BarberString, TitleString, LevelString;

    uint32_t TotalSpellsInPalettes = 18, TotalSpellsInContainer = 3, TotalAssignableThings = 8;

    std::string LogBookBasicsCategoryString, LogBookObjectsCategoryString, LogBookTownsCategoryString, LogBookHeroCategoryString;
    std::string LogBookCombatCategoryString, LogBookQuestCategoryString, LogBookStoryCategoryString;
    std::string LogBookBasicsCategoryNameString, LogBookObjectsCategoryNameString, LogBookTownsCategoryNameString, LogBookHeroCategoryNameString;
    std::string LogBookCombatCategoryNameString, LogBookQuestCategoryNameString, LogBookStoryCategoryNameString;

    std::map<uint32_t, uint32_t> MapPaths;
    std::string SoundUpDown, SoundSlider, SoundBack, SoundForward, SoundError, SoundExit;

    C2DVector HeroDollTL = { 310,33 }, HeroDollBR = { 560,300 };
    float HeroDollSphereRadius = 1.3f;
    C2DVector HeroDollTL_PC = { 310,33 }, HeroDollBR_PC = { 560,300 };
    float HeroDollSphereRadius_PC = 1.3f;
    C2DVector HeroDollFrameTL_PC = { 0,0 };
    float HeroDollFrameEmulateListOffset = 0.0f;

    uint32_t QuestStartScreenMusic = 2, QuestCompleteScreenMusic = 3, QuestFailureScreenMusic = 9, DeathScreenMusic = 14;
    std::string CountUpSound;
    float DigitCountTime = 3.0f;
    uint32_t SaveHeroGraphicIndex = 0;

    std::map<std::string, uint32_t> MiniMapGraphics;

    std::string SoundKeyboardUp, SoundKeyboardDown, SoundKeyboardLeft, SoundKeyboardRight;
    std::string SoundKeyboardEnterCharacter, SoundKeyboardDeleteCharacter, SoundKeyboardDone;

    std::string FrontEndMusic;

    int32_t KeyboardSmallKeyGraphic = 0, KeyboardLargeKeyGraphic = 0;
    float TimeInSecsForFade = 0, BackBufferFilterSaturation = 0, BackBufferFilterContrast = 0, BackBufferFilterBrightness = 0;
    float BackBufferFilterTintR = 0, BackBufferFilterTintG = 0, BackBufferFilterTintB = 0, BackBufferFilterTintScale = 0;
    float BackBufferDiffuseScale = 0, BackBufferAmbientScale = 0, MinimumFilterColor = 0;

    std::string GetClassName() const override { return "UI_MISC_THINGS_DEF"; }
    std::string GetInstantiationName() const override { return m_InstantiationName; }
    void SetInstantiationName(const std::string& name) override { m_InstantiationName = name; }
    void CopyFrom(const IDefObject* parentObject) override { if (const auto* p = dynamic_cast<const CUIMiscThingsDef*>(parentObject)) *this = *p; }

    void ParseFromText(CStringParser& parser, const std::map<std::string, int>& symbolMap) override {
        CParsedItem item;
        if (!parser.PeekNextItem(item) || item.Type != EParsedItemType::Identifier) return;
        std::string prop = item.StringValue;

        if (prop == "MapPaths") {
            parser.ReadNextItem(item);
            parser.SkipPastString("[");
            uint32_t key = parser.ReadAsInteger();
            parser.SkipPastString("]");

            std::string valStr; parser.ReadAsIdentifierOrNumber(valStr);
            uint32_t val = ResolveSymbol(valStr, symbolMap);

            MapPaths[key] = val;
            parser.SkipPastString(";");
            return;
        }

        parser.ReadNextItem(item);

        if (prop.find("String") != std::string::npos || prop.find("Separator") != std::string::npos ||
            prop.find("Bracket") != std::string::npos || prop.find("Sound") != std::string::npos ||
            prop == "Positive" || prop == "Plus" || prop == "Minus" || prop == "FrontEndMusic" || prop == "Failed" || prop == "CountUpSound") {

            std::string strVal;
            if (parser.ReadNextItemAsQuotedString(strVal)) {
                if (prop == "SpaceSeparator") SpaceSeparator = strVal; else if (prop == "CommaSeparator") CommaSeparator = strVal;
                else if (prop == "NewLineSeparator") NewLineSeparator = strVal; else if (prop == "OpenBracket") OpenBracket = strVal;
                else if (prop == "CloseBracket") CloseBracket = strVal; else if (prop == "Positive") Positive = strVal;
                else if (prop == "ColonSeparator") ColonSeparator = strVal; else if (prop == "WeaponValueString") WeaponValueString = strVal;
                else if (prop == "WeaponAugString") WeaponAugString = strVal; else if (prop == "WeaponAugNone") WeaponAugNone = strVal;
                else if (prop == "WeaponWeightString") WeaponWeightString = strVal; else if (prop == "WeaponLightString") WeaponLightString = strVal;
                else if (prop == "WeaponHeavyString") WeaponHeavyString = strVal; else if (prop == "WeaponKillsString") WeaponKillsString = strVal;
                else if (prop == "WeaponCatMeleeString") WeaponCatMeleeString = strVal; else if (prop == "WeaponCatRangedString") WeaponCatRangedString = strVal;
                else if (prop == "WeaponDamageString") WeaponDamageString = strVal; else if (prop == "TradeCostString") TradeCostString = strVal;
                else if (prop == "TradeProfitString") TradeProfitString = strVal; else if (prop == "TradeLossString") TradeLossString = strVal;
                else if (prop == "TradeAlreadyOwnsString") TradeAlreadyOwnsString = strVal; else if (prop == "TradeNumberInStockString") TradeNumberInStockString = strVal;
                else if (prop == "TradeDeliveryString") TradeDeliveryString = strVal; else if (prop == "TradeNoDeliveryString") TradeNoDeliveryString = strVal;
                else if (prop == "TradeDaysString") TradeDaysString = strVal; else if (prop == "TradeBuyString") TradeBuyString = strVal;
                else if (prop == "TradeSellString") TradeSellString = strVal; else if (prop == "TradeWantedString") TradeWantedString = strVal;
                else if (prop == "QuestFailedString") QuestFailedString = strVal; else if (prop == "FailedString") FailedString = strVal;
                else if (prop == "SucceededString") SucceededString = strVal; else if (prop == "Plus") Plus = strVal;
                else if (prop == "Minus") Minus = strVal; else if (prop == "ObjectsRewardString") ObjectsRewardString = strVal;
                else if (prop == "NoneString") NoneString = strVal; else if (prop == "CheckGuildString") CheckGuildString = strVal;
                else if (prop == "QuestStartingString") QuestStartingString = strVal; else if (prop == "YouString") YouString = strVal;
                else if (prop == "OwnString") OwnString = strVal; else if (prop == "NoString") NoString = strVal;
                else if (prop == "HousesString") HousesString = strVal; else if (prop == "HouseString") HouseString = strVal;
                else if (prop == "InString") InString = strVal; else if (prop == "ShopsString") ShopsString = strVal;
                else if (prop == "ShopString") ShopString = strVal; else if (prop == "ThereString") ThereString = strVal;
                else if (prop == "AreString") AreString = strVal; else if (prop == "IsString") IsString = strVal;
                else if (prop == "ForString") ForString = strVal; else if (prop == "SaleString") SaleString = strVal;
                else if (prop == "GeneralString") GeneralString = strVal; else if (prop == "TatooString") TatooString = strVal;
                else if (prop == "BarberString") BarberString = strVal; else if (prop == "TitleString") TitleString = strVal;
                else if (prop == "LevelString") LevelString = strVal;
                else if (prop == "LogBookBasicsCategoryString") LogBookBasicsCategoryString = strVal;
                else if (prop == "LogBookObjectsCategoryString") LogBookObjectsCategoryString = strVal;
                else if (prop == "LogBookTownsCategoryString") LogBookTownsCategoryString = strVal;
                else if (prop == "LogBookHeroCategoryString") LogBookHeroCategoryString = strVal;
                else if (prop == "LogBookCombatCategoryString") LogBookCombatCategoryString = strVal;
                else if (prop == "LogBookQuestCategoryString") LogBookQuestCategoryString = strVal;
                else if (prop == "LogBookStoryCategoryString") LogBookStoryCategoryString = strVal;
                else if (prop == "LogBookBasicsCategoryNameString") LogBookBasicsCategoryNameString = strVal;
                else if (prop == "LogBookObjectsCategoryNameString") LogBookObjectsCategoryNameString = strVal;
                else if (prop == "LogBookTownsCategoryNameString") LogBookTownsCategoryNameString = strVal;
                else if (prop == "LogBookHeroCategoryNameString") LogBookHeroCategoryNameString = strVal;
                else if (prop == "LogBookCombatCategoryNameString") LogBookCombatCategoryNameString = strVal;
                else if (prop == "LogBookQuestCategoryNameString") LogBookQuestCategoryNameString = strVal;
                else if (prop == "LogBookStoryCategoryNameString") LogBookStoryCategoryNameString = strVal;
                else if (prop == "SoundUpDown") SoundUpDown = strVal; else if (prop == "SoundSlider") SoundSlider = strVal;
                else if (prop == "SoundBack") SoundBack = strVal; else if (prop == "SoundForward") SoundForward = strVal;
                else if (prop == "SoundError") SoundError = strVal; else if (prop == "SoundExit") SoundExit = strVal;
                else if (prop == "SoundKeyboardUp") SoundKeyboardUp = strVal; else if (prop == "SoundKeyboardDown") SoundKeyboardDown = strVal;
                else if (prop == "SoundKeyboardLeft") SoundKeyboardLeft = strVal; else if (prop == "SoundKeyboardRight") SoundKeyboardRight = strVal;
                else if (prop == "SoundKeyboardEnterCharacter") SoundKeyboardEnterCharacter = strVal;
                else if (prop == "SoundKeyboardDeleteCharacter") SoundKeyboardDeleteCharacter = strVal;
                else if (prop == "SoundKeyboardDone") SoundKeyboardDone = strVal;
                else if (prop == "CountUpSound") CountUpSound = strVal; else if (prop == "FrontEndMusic") FrontEndMusic = strVal;
            }
        }
        else if (prop == "CoreGraphic" || prop == "VignetteGraphic" || prop == "OptionalGraphic" || prop == "FeatGraphic" ||
            prop == "QuestStartScreenMusic" || prop == "QuestCompleteScreenMusic" || prop == "QuestFailureScreenMusic" ||
            prop == "DeathScreenMusic" || prop == "SaveHeroGraphicIndex") {
            std::string valStr; parser.ReadAsIdentifierOrNumber(valStr);
            uint32_t val = ResolveSymbol(valStr, symbolMap);

            if (prop == "CoreGraphic") CoreGraphic = val; else if (prop == "VignetteGraphic") VignetteGraphic = val;
            else if (prop == "OptionalGraphic") OptionalGraphic = val; else if (prop == "FeatGraphic") FeatGraphic = val;
            else if (prop == "QuestStartScreenMusic") QuestStartScreenMusic = val; else if (prop == "QuestCompleteScreenMusic") QuestCompleteScreenMusic = val;
            else if (prop == "QuestFailureScreenMusic") QuestFailureScreenMusic = val; else if (prop == "DeathScreenMusic") DeathScreenMusic = val;
            else if (prop == "SaveHeroGraphicIndex") SaveHeroGraphicIndex = val;
        }
        else if (prop == "WorldMapWidth" || prop == "WorldMapHeight" || prop == "HeroDollSphereRadius" || prop == "HeroDollSphereRadius_PC" ||
            prop == "HeroDollFrameEmulateListOffset" || prop == "DigitCountTime" || prop == "TimeInSecsForFade" ||
            prop == "BackBufferFilterSaturation" || prop == "BackBufferFilterContrast" || prop == "BackBufferFilterBrightness" ||
            prop == "BackBufferFilterTintR" || prop == "BackBufferFilterTintG" || prop == "BackBufferFilterTintB" ||
            prop == "BackBufferFilterTintScale" || prop == "BackBufferDiffuseScale" || prop == "BackBufferAmbientScale" || prop == "MinimumFilterColor" ||
            prop.find("X") != std::string::npos || prop.find("Y") != std::string::npos) {
            float val = parser.ReadAsFloat();

            if (prop == "WorldMapWidth") WorldMapWidth = val; else if (prop == "WorldMapHeight") WorldMapHeight = val;
            else if (prop == "HeroDollSphereRadius") HeroDollSphereRadius = val; else if (prop == "HeroDollSphereRadius_PC") HeroDollSphereRadius_PC = val;
            else if (prop == "HeroDollFrameEmulateListOffset") HeroDollFrameEmulateListOffset = val; else if (prop == "DigitCountTime") DigitCountTime = val;
            else if (prop == "TimeInSecsForFade") TimeInSecsForFade = val; else if (prop == "BackBufferFilterSaturation") BackBufferFilterSaturation = val;
            else if (prop == "BackBufferFilterContrast") BackBufferFilterContrast = val; else if (prop == "BackBufferFilterBrightness") BackBufferFilterBrightness = val;
            else if (prop == "BackBufferFilterTintR") BackBufferFilterTintR = val; else if (prop == "BackBufferFilterTintG") BackBufferFilterTintG = val;
            else if (prop == "BackBufferFilterTintB") BackBufferFilterTintB = val; else if (prop == "BackBufferFilterTintScale") BackBufferFilterTintScale = val;
            else if (prop == "BackBufferDiffuseScale") BackBufferDiffuseScale = val; else if (prop == "BackBufferAmbientScale") BackBufferAmbientScale = val;
            else if (prop == "MinimumFilterColor") MinimumFilterColor = val;
            else if (prop == "WorldMapOffsetX") WorldMapOffset.x = val; else if (prop == "WorldMapOffsetY") WorldMapOffset.y = val;
            else if (prop == "RingCenterX") RingCenter.x = val; else if (prop == "RingCenterY") RingCenter.y = val;
            else if (prop == "PCRingCenterX") PCRingCenter.x = val; else if (prop == "PCRingCenterY") PCRingCenter.y = val;
            else if (prop == "HeroDollTLX") HeroDollTL.x = val; else if (prop == "HeroDollTLY") HeroDollTL.y = val;
            else if (prop == "HeroDollBRX") HeroDollBR.x = val; else if (prop == "HeroDollBRY") HeroDollBR.y = val;
            else if (prop == "HeroDollTLX_PC") HeroDollTL_PC.x = val; else if (prop == "HeroDollTLY_PC") HeroDollTL_PC.y = val;
            else if (prop == "HeroDollBRX_PC") HeroDollBR_PC.x = val; else if (prop == "HeroDollBRY_PC") HeroDollBR_PC.y = val;
            else if (prop == "HeroDollFrameTLX_PC") HeroDollFrameTL_PC.x = val; else if (prop == "HeroDollFrameTLY_PC") HeroDollFrameTL_PC.y = val;
        }
        else if (prop == "MiniMapGraphics") {
            CParsedItem itemBracket;
            parser.ReadNextItem(itemBracket); // Skip '['

            std::string keyStr;
            parser.ReadNextItemAsQuotedString(keyStr);
            parser.SkipPastString("]");

            std::string valStr;
            if (parser.ReadAsIdentifierOrNumber(valStr)) {
                MiniMapGraphics[keyStr] = ResolveSymbol(valStr, symbolMap);
            }
        }
        else {
            std::string valStr;
            if (parser.ReadAsIdentifierOrNumber(valStr)) {
                int32_t val = ResolveSymbol(valStr, symbolMap);

                if (prop == "TotalSpellsInPalettes") TotalSpellsInPalettes = val;
                else if (prop == "TotalSpellsInContainer") TotalSpellsInContainer = val;
                else if (prop == "TotalAssignableThings") TotalAssignableThings = val;
                else if (prop == "KeyboardSmallKeyGraphic") KeyboardSmallKeyGraphic = val;
                else if (prop == "KeyboardLargeKeyGraphic") KeyboardLargeKeyGraphic = val;
            }
        }

        parser.SkipPastString(";");
    }

    void SerializeOut(CDataOutputStream* os) const override {
        std::vector<std::string> w1 = { SpaceSeparator, CommaSeparator, NewLineSeparator, OpenBracket, CloseBracket, Positive, WeaponValueString, WeaponAugString, WeaponAugNone, WeaponWeightString, WeaponLightString, WeaponHeavyString, WeaponKillsString, WeaponCatMeleeString, WeaponCatRangedString, WeaponDamageString, TradeCostString, ColonSeparator, TradeProfitString, TradeLossString, TradeAlreadyOwnsString, TradeNumberInStockString, TradeDeliveryString, TradeNoDeliveryString, TradeDaysString, TradeBuyString, TradeSellString, TradeWantedString, QuestFailedString, FailedString, SucceededString, Plus, Minus };
        for (const auto& s : w1) os->WriteWideString(std::wstring(s.begin(), s.end()));

        os->WriteULONG(CoreGraphic); os->WriteULONG(VignetteGraphic); os->WriteULONG(OptionalGraphic); os->WriteULONG(FeatGraphic);

        std::vector<std::string> w2 = { ObjectsRewardString, NoneString, CheckGuildString, QuestStartingString };
        for (const auto& s : w2) os->WriteWideString(std::wstring(s.begin(), s.end()));

        os->Write2DVector(&RingCenter); os->Write2DVector(&PCRingCenter); os->Write2DVector(&WorldMapOffset);
        os->WriteFloat(WorldMapWidth); os->WriteFloat(WorldMapHeight);

        std::vector<std::string> w3 = { YouString, OwnString, NoString, HousesString, HouseString, InString, ShopsString, ShopString, ThereString, AreString, IsString, ForString, SaleString, GeneralString, TatooString, BarberString, TitleString, LevelString };
        for (const auto& s : w3) os->WriteWideString(std::wstring(s.begin(), s.end()));

        os->WriteULONG(TotalSpellsInPalettes); os->WriteULONG(TotalSpellsInContainer); os->WriteULONG(TotalAssignableThings);

        std::vector<std::string> w4 = { LogBookBasicsCategoryString, LogBookObjectsCategoryString, LogBookTownsCategoryString, LogBookHeroCategoryString, LogBookCombatCategoryString, LogBookQuestCategoryString, LogBookStoryCategoryString, LogBookBasicsCategoryNameString, LogBookObjectsCategoryNameString, LogBookTownsCategoryNameString, LogBookHeroCategoryNameString, LogBookCombatCategoryNameString, LogBookQuestCategoryNameString, LogBookStoryCategoryNameString };
        for (const auto& s : w4) os->WriteWideString(std::wstring(s.begin(), s.end()));

        os->WriteMapUint32ToUint32(MapPaths);

        std::vector<std::string> c1 = { SoundUpDown, SoundSlider, SoundBack, SoundForward, SoundError, SoundExit };
        for (const auto& s : c1) os->WriteCharString(s);

        os->Write2DVector(&HeroDollTL); os->Write2DVector(&HeroDollBR); os->WriteFloat(HeroDollSphereRadius);
        os->Write2DVector(&HeroDollTL_PC); os->Write2DVector(&HeroDollBR_PC); os->WriteFloat(HeroDollSphereRadius_PC);
        os->Write2DVector(&HeroDollFrameTL_PC); os->WriteFloat(HeroDollFrameEmulateListOffset);

        os->WriteULONG(QuestStartScreenMusic); os->WriteULONG(QuestCompleteScreenMusic); os->WriteULONG(QuestFailureScreenMusic); os->WriteULONG(DeathScreenMusic);

        os->WriteCharString(CountUpSound); os->WriteFloat(DigitCountTime); os->WriteULONG(SaveHeroGraphicIndex);

        os->WriteMapStringToUint32(MiniMapGraphics);

        std::vector<std::string> c2 = { SoundKeyboardUp, SoundKeyboardDown, SoundKeyboardLeft, SoundKeyboardRight, SoundKeyboardEnterCharacter, SoundKeyboardDeleteCharacter, SoundKeyboardDone };
        for (const auto& s : c2) os->WriteCharString(s);

        os->WriteWideString(std::wstring(FrontEndMusic.begin(), FrontEndMusic.end()));
        os->WriteSLONG(KeyboardSmallKeyGraphic); os->WriteSLONG(KeyboardLargeKeyGraphic);

        os->WriteFloat(TimeInSecsForFade); os->WriteFloat(BackBufferFilterSaturation); os->WriteFloat(BackBufferFilterContrast); os->WriteFloat(BackBufferFilterBrightness);
        os->WriteFloat(BackBufferFilterTintR); os->WriteFloat(BackBufferFilterTintG); os->WriteFloat(BackBufferFilterTintB); os->WriteFloat(BackBufferFilterTintScale);
        os->WriteFloat(BackBufferDiffuseScale); os->WriteFloat(BackBufferAmbientScale); os->WriteFloat(MinimumFilterColor);
    }
};

class CUIIconsDef : public IDefObject {
public:
    std::string m_InstantiationName;

    uint32_t IconFriendRequestReceived = 0;
    uint32_t IconFriendRequestReceivedOn = 0;
    uint32_t IconFriendRequestSent = 0;
    uint32_t IconFriendRequestSentOn = 0;
    uint32_t IconGameInviteReceived = 0;
    uint32_t IconGameInviteReceivedOn = 0;
    uint32_t IconGameInviteSent = 0;
    uint32_t IconGameInviteSentOn = 0;
    uint32_t IconMute = 0;
    uint32_t IconMuteOn = 0;
    uint32_t IconOnline = 0;
    uint32_t IconOnlineOn = 0;
    uint32_t IconPasscodeBlank = 0;
    uint32_t IconPasscodeFilled = 0;
    uint32_t IconTV = 0;
    uint32_t IconTVOn = 0;
    uint32_t IconVoice = 0;
    uint32_t IconVoiceOn = 0;
    uint32_t IconWait1 = 0;
    uint32_t IconWait2 = 0;
    uint32_t IconWait3 = 0;
    uint32_t IconWait4 = 0;
    uint32_t IconProgress = 0;
    uint32_t IconProgressOn = 0;
    uint32_t IconA = 0;
    uint32_t IconB = 0;
    uint32_t IconX = 0;
    uint32_t IconY = 0;
    uint32_t IconBlank = 0;
    uint32_t IconUpArrow = 0;
    uint32_t IconDownArrow = 0;
    uint32_t IconListHighlight = 0;

    std::string GetClassName() const override { return "UI_ICONS_DEF"; }
    std::string GetInstantiationName() const override { return m_InstantiationName; }
    void SetInstantiationName(const std::string& name) override { m_InstantiationName = name; }

    void CopyFrom(const IDefObject* parentObject) override {
        if (const auto* p = dynamic_cast<const CUIIconsDef*>(parentObject)) *this = *p;
    }

    void ParseFromText(CStringParser& parser, const std::map<std::string, int>& symbolMap) override {
        CParsedItem item;
        if (!parser.PeekNextItem(item) || item.Type != EParsedItemType::Identifier) return;
        std::string prop = item.StringValue;

        parser.ReadNextItem(item);

        std::string valStr;
        if (parser.ReadAsIdentifierOrNumber(valStr)) {
            uint32_t val = ResolveSymbol(valStr, symbolMap);

            if (prop == "IconFriendRequestReceived") IconFriendRequestReceived = val;
            else if (prop == "IconFriendRequestReceivedOn") IconFriendRequestReceivedOn = val;
            else if (prop == "IconFriendRequestSent") IconFriendRequestSent = val;
            else if (prop == "IconFriendRequestSentOn") IconFriendRequestSentOn = val;
            else if (prop == "IconGameInviteReceived") IconGameInviteReceived = val;
            else if (prop == "IconGameInviteReceivedOn") IconGameInviteReceivedOn = val;
            else if (prop == "IconGameInviteSent") IconGameInviteSent = val;
            else if (prop == "IconGameInviteSentOn") IconGameInviteSentOn = val;
            else if (prop == "IconMute") IconMute = val;
            else if (prop == "IconMuteOn") IconMuteOn = val;
            else if (prop == "IconOnline") IconOnline = val;
            else if (prop == "IconOnlineOn") IconOnlineOn = val;
            else if (prop == "IconPasscodeBlank") IconPasscodeBlank = val;
            else if (prop == "IconPasscodeFilled") IconPasscodeFilled = val;
            else if (prop == "IconTV") IconTV = val;
            else if (prop == "IconTVOn") IconTVOn = val;
            else if (prop == "IconVoice") IconVoice = val;
            else if (prop == "IconVoiceOn") IconVoiceOn = val;
            else if (prop == "IconWait1") IconWait1 = val;
            else if (prop == "IconWait2") IconWait2 = val;
            else if (prop == "IconWait3") IconWait3 = val;
            else if (prop == "IconWait4") IconWait4 = val;
            else if (prop == "IconProgress") IconProgress = val;
            else if (prop == "IconProgressOn") IconProgressOn = val;
            else if (prop == "IconA") IconA = val;
            else if (prop == "IconB") IconB = val;
            else if (prop == "IconX") IconX = val;
            else if (prop == "IconY") IconY = val;
            else if (prop == "IconBlank") IconBlank = val;
            else if (prop == "IconUpArrow") IconUpArrow = val;
            else if (prop == "IconDownArrow") IconDownArrow = val;
            else if (prop == "IconListHighlight") IconListHighlight = val;
        }

        parser.SkipPastString(";");
    }

    void SerializeOut(CDataOutputStream* os) const override {
        os->WriteULONG(IconFriendRequestReceived);
        os->WriteULONG(IconFriendRequestReceivedOn);
        os->WriteULONG(IconFriendRequestSent);
        os->WriteULONG(IconFriendRequestSentOn);
        os->WriteULONG(IconGameInviteReceived);
        os->WriteULONG(IconGameInviteReceivedOn);
        os->WriteULONG(IconGameInviteSent);
        os->WriteULONG(IconGameInviteSentOn);
        os->WriteULONG(IconMute);
        os->WriteULONG(IconMuteOn);
        os->WriteULONG(IconOnline);
        os->WriteULONG(IconOnlineOn);
        os->WriteULONG(IconPasscodeBlank);
        os->WriteULONG(IconPasscodeFilled);
        os->WriteULONG(IconTV);
        os->WriteULONG(IconTVOn);
        os->WriteULONG(IconVoice);
        os->WriteULONG(IconVoiceOn);
        os->WriteULONG(IconWait1);
        os->WriteULONG(IconWait2);
        os->WriteULONG(IconWait3);
        os->WriteULONG(IconWait4);
        os->WriteULONG(IconProgress);
        os->WriteULONG(IconProgressOn);
        os->WriteULONG(IconA);
        os->WriteULONG(IconB);
        os->WriteULONG(IconX);
        os->WriteULONG(IconY);
        os->WriteULONG(IconBlank);
        os->WriteULONG(IconUpArrow);
        os->WriteULONG(IconDownArrow);
        os->WriteULONG(IconListHighlight);
    }
};

// Factories
inline IDefObject* Alloc_CFrontEndDef() { return new CFrontEndDef(); }
inline IDefObject* Alloc_CUIMiscThingsDef() { return new CUIMiscThingsDef(); }
inline IDefObject* Alloc_CUIIconsDef() { return new CUIIconsDef(); }
inline IDefObject* Alloc_CControlsDef() { return new CControlsDef(); }
inline IDefObject* Alloc_CEngineVideoOptionsDef() { return new CEngineVideoOptionsDef(); }
inline IDefObject* Alloc_CEngineDef() { return new CEngineDef(); }
inline IDefObject* Alloc_CUIDef() { return new CUIDef(); }
inline IDefObject* Alloc_CConfigOptionsDefaultsDef() { return new CConfigOptionsDefaultsDef(); }