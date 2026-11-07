//! Def enum + flag types (merged frontend + game def sets).

//! Def enum types.
//!
//! Generated from the validated def spec (`fable-decomp/defs-spec.json`, itself
//! validated byte-exactly against every entry of the retail `game.bin`,
//! `frontend.bin`, and `script.bin`) plus the game's own `keyboard_keys.h`.
//! Each variant records its original C++ enumerator name — text defs refer to
//! enum values by those symbols, so they are kept for lowering and future text
//! emission.
//!
//! Two shapes:
//! - `#[derive(DefEnum)]` — a strict, closed Rust enum. Used where the compiled
//!   data provably stays inside the C++ enum table (verified by scanning all
//!   three bins). Parsing an out-of-table value is an error.
//! - `#[derive(DefFlags)]` — a bit-set newtype. Used for the "enums" the game
//!   actually ORs together (e.g. `TABLE_EXPANSION_HORIZONTAL |
//!   TABLE_EXPANSION_VERTICAL` occurs in game.bin) or that legitimately carry
//!   empty/unlisted values.

/// A def enum: a closed `i32`-repr enum with a total mapping to/from the wire
/// value and the C++ enumerator symbols used in text defs.
pub trait DefEnum: Sized + Copy {
    /// Table lookup; `None` for values outside the C++ enum.
    fn from_i32(value: i32) -> Option<Self>;
    fn to_i32(self) -> i32;
}

use crate::{DefEnum, DefFlags};

/// UI element type.
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum UiType {
    #[def("UI_TYPE_SPRITE")]
    Sprite = 0,
    #[def("UI_TYPE_MORPHING_SPRITE")]
    MorphingSprite = 1,
    #[def("UI_TYPE_TABLE")]
    Table = 2,
    #[def("UI_TYPE_MESH")]
    Mesh = 3,
    #[def("UI_TYPE_COMPOSITE")]
    Composite = 4,
    #[def("UI_TYPE_CHANGING_STATE_COMPOSITE")]
    ChangingStateComposite = 5,
    #[def("UI_TYPE_TEXT")]
    Text = 6,
    #[def("UI_TYPE_MENU_ENTRY")]
    MenuEntry = 7,
    #[def("UI_TYPE_LIST")]
    List = 8,
    #[def("UI_TYPE_VIEWPORT")]
    Viewport = 9,
    #[def("UI_TYPE_FRONTEND_SCREEN")]
    FrontendScreen = 10,
    #[def("UI_TYPE_FRONTEND_BUTTON")]
    FrontendButton = 11,
    #[def("UI_TYPE_FRONTEND_LIST")]
    FrontendList = 12,
    #[def("UI_TYPE_SCROLLING_VIEWPORT")]
    ScrollingViewport = 13,
    #[def("UI_TYPE_LIST_ARROW")]
    ListArrow = 14,
    #[def("UI_TYPE_SLIDER")]
    Slider = 15,
    #[def("UI_TYPE_TEXT_SLIDER")]
    TextSlider = 16,
    #[def("UI_TYPE_MOVIE")]
    Movie = 17,
    #[def("UI_TYPE_SWAPPING_STATE_COMPOSITE")]
    SwappingStateComposite = 18,
    #[def("UI_TYPE_SCROLLING_COMPOSITE")]
    ScrollingComposite = 19,
    #[def("UI_TYPE_TEXT_CONTAINER")]
    TextContainer = 20,
    #[def("UI_TYPE_ZOOMING_COMPOSITE")]
    ZoomingComposite = 21,
    #[def("UI_TYPE_COMPONENT_CONTAINER")]
    ComponentContainer = 22,
    #[def("UI_TYPE_SPELL_CONTAINER")]
    SpellContainer = 23,
    #[def("UI_TYPE_SPELL_CONTAINER_LIST")]
    SpellContainerList = 24,
    #[def("UI_TYPE_YESNO")]
    YesNo = 25,
    #[def("UI_TYPE_OK")]
    Ok = 26,
    #[def("UI_TYPE_PARTICLE_EFFECT")]
    ParticleEffect = 27,
    #[def("UI_TYPE_CONTROLLERDISCONNECT")]
    ControllerDisconnect = 28,
    #[def("UI_TYPE_DIRTYDISC")]
    DirtyDisc = 29,
    #[def("UI_TYPE_ICON_TEXT")]
    IconText = 30,
    #[def("UI_TYPE_DYNAMIC_LIST")]
    DynamicList = 31,
    #[def("UI_TYPE_MOUSE_POINTER")]
    MousePointer = 32,
    #[def("UI_TYPE_HOVERABLE")]
    Hoverable = 33,
    #[def("UI_TYPE_CLICKABLE")]
    Clickable = 34,
    #[def("UI_TYPE_DRAGGABLE")]
    Draggable = 35,
    #[def("UI_TYPE_DRAGGABLE_INTO")]
    DraggableInto = 36,
    #[def("UI_TYPE_EDIT_BOX")]
    EditBox = 37,
    #[def("UI_TYPE_NAVIGATION_BUTTON")]
    NavigationButton = 38,
    #[def("UI_TYPE_KEY_REDEFINER")]
    KeyRedefiner = 39,
    #[def("UI_TYPE_REDEFINER_LIST")]
    RedefinerList = 40,
    #[def("UI_TYPE_SCROLLBAR")]
    Scrollbar = 41,
    #[def("UI_TYPE_SCROLLBAR_OUTSIDE")]
    ScrollbarOutside = 42,
    #[def("UI_TYPE_SCROLLABLE_LIST")]
    ScrollableList = 43,
}

/// Table growth direction. Declared as an enum in C++ but OR-combined in real data (game.bin has `3`).
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default, DefFlags)]
#[flags(
    HORIZONTAL = 1 => "TABLE_EXPANSION_HORIZONTAL",
    VERTICAL = 2 => "TABLE_EXPANSION_VERTICAL",
)]
pub struct TableExpansion(pub i32);

/// Text alignment.
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum TextAlignment {
    #[def("LEFT")]
    Left = 0,
    #[def("CENTER")]
    Center = 1,
    #[def("RIGHT")]
    Right = 2,
}

/// Order in which a UI state change propagates.
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum StateChangeType {
    #[def("STATE_CHANGE_SIMULTANEOUS")]
    Simultaneous = 0,
    #[def("STATE_CHANGE_PARENT_FIRST")]
    ParentFirst = 1,
    #[def("STATE_CHANGE_CHILDREN_FIRST")]
    ChildrenFirst = 2,
    #[def("STATE_CHANGE_PARENT_ONLY")]
    ParentOnly = 3,
    #[def("STATE_CHANGE_CHILDREN_ONLY")]
    ChildrenOnly = 4,
}

/// UI action fired by buttons and menu entries.
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ActionType {
    #[def("UI_ACTION_TYPE_NONE")]
    TypeNone = 0,
    #[def("UI_ACTION_TYPE_WIELD")]
    TypeWield = 1,
    #[def("UI_ACTION_TYPE_AUGMENT")]
    TypeAugment = 2,
    #[def("UI_ACTION_TYPE_CHANGESTATE")]
    TypeChangestate = 3,
    #[def("UI_ACTION_TYPE_TAKE_BOAST")]
    TypeTakeBoast = 4,
    #[def("UI_ACTION_TYPE_TELEPORT")]
    TypeTeleport = 5,
    #[def("UI_ACTION_TYPE_TAKE_QUEST")]
    TypeTakeQuest = 6,
    #[def("UI_ACTION_TYPE_BUY_STAT")]
    TypeBuyStat = 7,
    #[def("UI_ACTION_TYPE_BUY_ABILITY")]
    TypeBuyAbility = 8,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_OPTIONS")]
    TypeNextScreenOptions = 9,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_LIVE_AWARE")]
    TypeNextScreenLiveAware = 10,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_EXTRAS")]
    TypeNextScreenExtras = 11,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_AUDIO_OPTIONS")]
    TypeNextScreenAudioOptions = 12,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_VIDEO_OPTIONS")]
    TypeNextScreenVideoOptions = 13,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_CAMERA_OPTIONS")]
    TypeNextScreenCameraOptions = 14,
    #[def("UI_ACTION_TYPE_NEW_GAME")]
    TypeNewGame = 15,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_PROFILES")]
    TypeNextScreenProfiles = 16,
    #[def("UI_ACTION_TYPE_LOAD_GAME")]
    TypeLoadGame = 17,
    #[def("UI_ACTION_TYPE_LOGIN")]
    TypeLogin = 18,
    #[def("UI_ACTION_TYPE_FRIENDS")]
    TypeFriends = 19,
    #[def("UI_ACTION_TYPE_APPEAR_OFFLINE")]
    TypeAppearOffline = 20,
    #[def("UI_ACTION_TYPE_DELETE")]
    TypeDelete = 21,
    #[def("UI_ACTION_TYPE_ADD_CHILD")]
    TypeAddChild = 22,
    #[def("UI_ACTION_TYPE_ADD_CHILD_AUGMENTATION")]
    TypeAddChildAugmentation = 23,
    #[def("UI_ACTION_TYPE_DELETE_ALL")]
    TypeDeleteAll = 24,
    #[def("UI_ACTION_TYPE_USE_ITEM")]
    TypeUseItem = 25,
    #[def("UI_ACTION_TYPE_ITEM_ASSIGN_LEFT")]
    TypeItemAssignLeft = 26,
    #[def("UI_ACTION_TYPE_ITEM_ASSIGN_RIGHT")]
    TypeItemAssignRight = 27,
    #[def("UI_ACTION_TYPE_ITEM_ASSIGN_DOWN")]
    TypeItemAssignDown = 28,
    #[def("UI_ACTION_TYPE_ITEM_ASSIGN_UP")]
    TypeItemAssignUp = 29,
    #[def("UI_ACTION_TYPE_CLOTHING_WEAR")]
    TypeClothingWear = 30,
    #[def("UI_ACTION_TYPE_UPDATE_MANNEQUIN_CLOTHING")]
    TypeUpdateMannequinClothing = 31,
    #[def("UI_ACTION_TYPE_BUY")]
    TypeBuy = 36,
    #[def("UI_ACTION_TYPE_SELL")]
    TypeSell = 37,
    #[def("UI_ACTION_TYPE_DELETE_PREVIOUS")]
    TypeDeletePrevious = 38,
    #[def("UI_ACTION_TYPE_RESET_CLOTHING_MANNEQUIN")]
    TypeResetClothingMannequin = 39,
    #[def("UI_ACTION_TYPE_SET_BRIGHTNESS")]
    TypeSetBrightness = 40,
    #[def("UI_ACTION_TYPE_SET_SOUND")]
    TypeSetSound = 41,
    #[def("UI_ACTION_TYPE_SET_MUSIC")]
    TypeSetMusic = 42,
    #[def("UI_ACTION_TYPE_SET_CAMERA_ROTATION")]
    TypeSetCameraRotation = 43,
    #[def("UI_ACTION_TYPE_SET_CAMERA_UP_DOWN")]
    TypeSetCameraUpDown = 44,
    #[def("UI_ACTION_TYPE_SET_VIBRATION")]
    TypeSetVibration = 45,
    #[def("UI_ACTION_TYPE_SET_HEADPHONES")]
    TypeSetHeadphones = 46,
    #[def("UI_ACTION_TYPE_SET_SUBTITLES")]
    TypeSetSubtitles = 47,
    #[def("UI_ACTION_TYPE_OPEN_MAP")]
    TypeOpenMap = 48,
    #[def("UI_ACTION_TYPE_OPEN_ABILITIES")]
    TypeOpenAbilities = 49,
    #[def("UI_ACTION_TYPE_OPEN_WEAPONS")]
    TypeOpenWeapons = 50,
    #[def("UI_ACTION_TYPE_OPEN_STATS")]
    TypeOpenStats = 51,
    #[def("UI_ACTION_TYPE_OPEN_CLOTHING")]
    TypeOpenClothing = 52,
    #[def("UI_ACTION_TYPE_OPEN_ITEMS")]
    TypeOpenItems = 53,
    #[def("UI_ACTION_TYPE_CLOSE_MAP")]
    TypeCloseMap = 54,
    #[def("UI_ACTION_TYPE_CLOSE_ABILITIES")]
    TypeCloseAbilities = 55,
    #[def("UI_ACTION_TYPE_CLOSE_WEAPONS")]
    TypeCloseWeapons = 56,
    #[def("UI_ACTION_TYPE_CLOSE_STATS")]
    TypeCloseStats = 57,
    #[def("UI_ACTION_TYPE_CLOSE_CLOTHING")]
    TypeCloseClothing = 58,
    #[def("UI_ACTION_TYPE_CLOSE_ITEMS")]
    TypeCloseItems = 59,
    #[def("UI_ACTION_TYPE_CLOSE_MENU")]
    TypeCloseMenu = 60,
    #[def("UI_ACTION_TYPE_GUILD_SEAL_RECALL")]
    TypeGuildSealRecall = 61,
    #[def("UI_ACTION_TYPE_LOAD")]
    TypeLoad = 62,
    #[def("UI_ACTION_TYPE_SAVE")]
    TypeSave = 63,
    #[def("UI_ACTION_TYPE_CLOSE_PAUSE_MENU")]
    TypeClosePauseMenu = 64,
    #[def("UI_ACTION_TYPE_PLAY_MOVIE")]
    TypePlayMovie = 65,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_PROFILES_SAVED_GAMES")]
    TypeNextScreenProfilesSavedGames = 66,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_CREDITS")]
    TypeNextScreenCredits = 67,
    #[def("UI_ACTION_TYPE_CONSTRUCT_WEAPONS_LIST")]
    TypeConstructWeaponsList = 68,
    #[def("UI_ACTION_TYPE_CONSTRUCT_CLOTHING_LIST")]
    TypeConstructClothingList = 69,
    #[def("UI_ACTION_TYPE_CONSTRUCT_ITEMS_LIST")]
    TypeConstructItemsList = 70,
    #[def("UI_ACTION_TYPE_CONSTRUCT_SKILLS_LIST")]
    TypeConstructSkillsList = 71,
    #[def("UI_ACTION_TYPE_CONSTRUCT_QUESTS_LIST")]
    TypeConstructQuestsList = 72,
    #[def("UI_ACTION_TYPE_CONSTRUCT_MAP_LIST")]
    TypeConstructMapList = 73,
    #[def("UI_ACTION_TYPE_CONSTRUCT_STATS_LIST")]
    TypeConstructStatsList = 74,
    #[def("UI_ACTION_TYPE_CHANGE_CHILD_STATE")]
    TypeChangeChildState = 75,
    #[def("UI_ACTION_TYPE_CONSTRUCT_EXPERIENCE_LIST")]
    TypeConstructExperienceList = 76,
    #[def("UI_ACTION_TYPE_CONSTRUCT_PERSONALITY_LIST")]
    TypeConstructPersonalityList = 77,
    #[def("UI_ACTION_TYPE_SET_HUD")]
    TypeSetHud = 78,
    #[def("UI_ACTION_TYPE_CLOSE_QUICK_ACCESS_MENU")]
    TypeCloseQuickAccessMenu = 79,
    #[def("UI_ACTION_TYPE_CLOSE_TELEPORT_MENU")]
    TypeCloseTeleportMenu = 80,
    #[def("UI_ACTION_TYPE_BUY_BULK")]
    TypeBuyBulk = 81,
    #[def("UI_ACTION_TYPE_SELL_BULK")]
    TypeSellBulk = 82,
    #[def("UI_ACTION_TYPE_SET_TUTORIALS")]
    TypeSetTutorials = 83,
    #[def("UI_ACTION_TYPE_SET_SHOW_BUDDY_NAMES")]
    TypeSetShowBuddyNames = 84,
    #[def("UI_ACTION_TYPE_QUIT_TO_FRONT_END")]
    TypeQuitToFrontEnd = 85,
    #[def("UI_ACTION_TYPE_PREVIOUS_SCREEN")]
    TypePreviousScreen = 86,
    #[def("UI_ACTION_TYPE_ACCEPT_CHANGES")]
    TypeAcceptChanges = 87,
    #[def("UI_ACTION_TYPE_SET_DIALOGUE")]
    TypeSetDialogue = 88,
    #[def("UI_ACTION_TYPE_XLIVE")]
    TypeXlive = 89,
    #[def("UI_ACTION_TYPE_XLIVE_BACK")]
    TypeXliveBack = 90,
    #[def("UI_ACTION_TYPE_LOGOUT")]
    TypeLogout = 91,
    #[def("UI_ACTION_TYPE_DEMOS")]
    TypeDemos = 92,
    #[def("UI_ACTION_TYPE_SET_GUILD_MASTER")]
    TypeSetGuildMaster = 93,
    #[def("UI_ACTION_TYPE_SET_HUD_TOOLTIPS")]
    TypeSetHudTooltips = 94,
    #[def("UI_ACTION_TYPE_UNDO_BUY_ABILITY")]
    TypeUndoBuyAbility = 200,
    #[def("UI_ACTION_TYPE_UNDO_BUY_STAT")]
    TypeUndoBuyStat = 201,
    #[def("UI_ACTION_TYPE_GIVE_EXCLUSIVE_INPUT")]
    TypeGiveExclusiveInput = 202,
    #[def("UI_ACTION_TYPE_REMOVE_EXCLUSIVE_INPUT")]
    TypeRemoveExclusiveInput = 203,
    #[def("UI_ACTION_TYPE_SEND_BACK_EVENT")]
    TypeSendBackEvent = 204,
    #[def("UI_ACTION_TYPE_SHOW_SPELL_SELECTION")]
    TypeShowSpellSelection = 205,
    #[def("UI_ACTION_TYPE_SET_SPELL_TO_ASSIGN")]
    TypeSetSpellToAssign = 206,
    #[def("UI_ACTION_TYPE_TELEPORT_TO_BOAST")]
    TypeTeleportToBoast = 207,
    #[def("UI_ACTION_TYPE_DROP_QUEST")]
    TypeDropQuest = 208,
    #[def("UI_ACTION_TYPE_ASSIGN_SPELL")]
    TypeAssignSpell = 209,
    #[def("UI_ACTION_TYPE_UNASSIGN_SPELL")]
    TypeUnassignSpell = 210,
    #[def("UI_ACTION_TYPE_RESET_ASSIGNED_SPELLS")]
    TypeResetAssignedSpells = 211,
    #[def("UI_ACTION_TYPE_CHANGE_ALPHA")]
    TypeChangeAlpha = 212,
    #[def("UI_ACTION_TYPE_CHANGE_NUMBER")]
    TypeChangeNumber = 213,
    #[def("UI_ACTION_TYPE_DELETE_PROFILE")]
    TypeDeleteProfile = 214,
    #[def("UI_ACTION_TYPE_GO_TO_DELETE_PROFILE_SCREEN")]
    TypeGoToDeleteProfileScreen = 215,
    #[def("UI_ACTION_TYPE_POP_MAP_STATE")]
    TypePopMapState = 216,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_INVALID_PROFILE")]
    TypeNextScreenInvalidProfile = 219,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_INVALID_SAVE")]
    TypeNextScreenInvalidSave = 220,
    #[def("UI_ACTION_TYPE_FREE_SPACE_ON_T")]
    TypeFreeSpaceOnT = 221,
    #[def("UI_ACTION_TYPE_FREE_SPACE_ON_U")]
    TypeFreeSpaceOnU = 222,
    #[def("UI_ACTION_TYPE_FLASH_DPAD_DOWN")]
    TypeFlashDpadDown = 223,
    #[def("UI_ACTION_TYPE_FLASH_DPAD_UP")]
    TypeFlashDpadUp = 224,
    #[def("UI_ACTION_TYPE_FLASH_DPAD_LEFT")]
    TypeFlashDpadLeft = 225,
    #[def("UI_ACTION_TYPE_FLASH_DPAD_RIGHT")]
    TypeFlashDpadRight = 226,
    #[def("UI_ACTION_TYPE_STOP_DPAD_FLASHING")]
    TypeStopDpadFlashing = 227,
    #[def("UI_ACTION_TYPE_CONSTRUCT_LOGBOOK_LIST")]
    TypeConstructLogbookList = 228,
    #[def("UI_ACTION_TYPE_GO_TO_MAIN_MENU_FROM_START")]
    TypeGoToMainMenuFromStart = 229,
    #[def("UI_ACTION_TYPE_SEND_OWNED_EVENT")]
    TypeSendOwnedEvent = 230,
    #[def("UI_ACTION_TYPE_SEND_EVENT")]
    TypeSendEvent = 231,
    #[def("UI_ACTION_TYPE_MOVE_COMPONENT")]
    TypeMoveComponent = 232,
    #[def("UI_ACTION_TYPE_EXIT_LIVE_GUI")]
    TypeExitLiveGui = 233,
    #[def("UI_ACTION_TYPE_OPEN_PC_SKILLS_MENU")]
    TypeOpenPcSkillsMenu = 234,
    #[def("UI_ACTION_TYPE_CONSTRUCT_EXPRESSIONS_LIST")]
    TypeConstructExpressionsList = 235,
    #[def("UI_ACTION_TYPE_SCROLL_ACTIVE_LIST_UP")]
    TypeScrollActiveListUp = 236,
    #[def("UI_ACTION_TYPE_SCROLL_ACTIVE_LIST_DOWN")]
    TypeScrollActiveListDown = 237,
    #[def("UI_ACTION_TYPE_OPEN_PC_INVENTORY_MENU")]
    TypeOpenPcInventoryMenu = 238,
    #[def("UI_ACTION_TYPE_OPEN_PC_OPTIONS_MENU")]
    TypeOpenPcOptionsMenu = 239,
    #[def("UI_ACTION_TYPE_CONSTRUCT_PC_STYLE_CARDS_LIST")]
    TypeConstructPcStyleCardsList = 240,
    #[def("UI_ACTION_TYPE_OPEN_PC_BUY_TRADING_GOODS_LIST")]
    TypeOpenPcBuyTradingGoodsList = 241,
    #[def("UI_ACTION_TYPE_CLOSE_PC_BUY_TRADING_GOODS_LIST")]
    TypeClosePcBuyTradingGoodsList = 242,
    #[def("UI_ACTION_TYPE_CONSTRUCT_MAGIC_LIST_PC")]
    TypeConstructMagicListPc = 243,
    #[def("UI_ACTION_TYPE_ASSIGN_SPELL_PC")]
    TypeAssignSpellPc = 244,
    #[def("UI_ACTION_TYPE_OBSERVE_EVENT")]
    TypeObserveEvent = 245,
    #[def("UI_ACTION_TYPE_SET_SPELL_TO_ASSIGN_PC")]
    TypeSetSpellToAssignPc = 246,
    #[def("UI_ACTION_TYPE_RESET_ASSIGNED_SPELLS_PC")]
    TypeResetAssignedSpellsPc = 247,
    #[def("UI_ACTION_TYPE_UNASSIGN_SPELL_PC")]
    TypeUnassignSpellPc = 248,
    #[def("UI_ACTION_TYPE_IGNORE_EVENT")]
    TypeIgnoreEvent = 249,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_PROFILES_FOR_DELETE")]
    TypeNextScreenProfilesForDelete = 250,
    #[def("UI_ACTION_TYPE_ASSIGN_EXPRESSION_ITEM_PC")]
    TypeAssignExpressionItemPc = 251,
    #[def("UI_ACTION_TYPE_UNASSIGN_EXPRESSION_ITEM_PC")]
    TypeUnassignExpressionItemPc = 252,
    #[def("UI_ACTION_TYPE_SET_EXPRESSION_ITEM_TO_ASSIGN_PC")]
    TypeSetExpressionItemToAssignPc = 253,
    #[def("UI_ACTION_TYPE_OPEN_PC_MAP_MENU")]
    TypeOpenPcMapMenu = 254,
    #[def("UI_ACTION_TYPE_OPEN_PC_STATUS_MENU")]
    TypeOpenPcStatusMenu = 255,
    #[def("UI_ACTION_TYPE_OPEN_PC_LOG_BOOK")]
    TypeOpenPcLogBook = 256,
    #[def("UI_ACTION_TYPE_CONSTRUCT_PC_QUESTS_LIST")]
    TypeConstructPcQuestsList = 257,
    #[def("UI_ACTION_TYPE_SET_RESOLUTION")]
    TypeSetResolution = 258,
    #[def("UI_ACTION_TYPE_SET_SHADOW_DETAIL")]
    TypeSetShadowDetail = 259,
    #[def("UI_ACTION_TYPE_SET_DECALS")]
    TypeSetDecals = 260,
    #[def("UI_ACTION_TYPE_SET_WEATHER_EFFECTS")]
    TypeSetWeatherEffects = 261,
    #[def("UI_ACTION_TYPE_SET_VERTICAL_SYNC")]
    TypeSetVerticalSync = 262,
    #[def("UI_ACTION_TYPE_SET_GLOW_EFFECTS")]
    TypeSetGlowEffects = 263,
    #[def("UI_ACTION_TYPE_SET_REVERSE_STEREO")]
    TypeSetReverseStereo = 264,
    #[def("UI_ACTION_TYPE_SET_ANTIALIASING")]
    TypeSetAntialiasing = 265,
    #[def("UI_ACTION_TYPE_SET_TEXTURE_DETAIL")]
    TypeSetTextureDetail = 266,
    #[def("UI_ACTION_TYPE_SET_MESH_RESOLUTION")]
    TypeSetMeshResolution = 267,
    #[def("UI_ACTION_TYPE_SET_WATER_REFLECTION")]
    TypeSetWaterReflection = 268,
    #[def("UI_ACTION_TYPE_SET_LANDSCAPE_DETAIL")]
    TypeSetLandscapeDetail = 269,
    #[def("UI_ACTION_TYPE_SET_PARTICLE_DETAIL")]
    TypeSetParticleDetail = 270,
    #[def("UI_ACTION_TYPE_ADD_OBSERVER")]
    TypeAddObserver = 271,
    #[def("UI_ACTION_TYPE_SCROLL_ACTIVE_LIST_UNTIL_CHILD_CONTAINING")]
    TypeScrollActiveListUntilChildContaining = 272,
    #[def("UI_ACTION_TYPE_GO_BACK")]
    TypeGoBack = 273,
    #[def("UI_ACTION_TYPE_CLOSE_ACTIVE_MENU")]
    TypeCloseActiveMenu = 274,
    #[def("UI_ACTION_TYPE_REMOVE_OBSERVER")]
    TypeRemoveObserver = 275,
    #[def("UI_ACTION_TYPE_TAVERN_GAME_BET_DOWN")]
    TypeTavernGameBetDown = 276,
    #[def("UI_ACTION_TYPE_TAVERN_GAME_BET_UP")]
    TypeTavernGameBetUp = 277,
    #[def("UI_ACTION_TYPE_RESPAWN")]
    TypeRespawn = 278,
    #[def("UI_ACTION_TYPE_CONTINUE")]
    TypeContinue = 279,
    #[def("UI_ACTION_TYPE_OPEN_PC_SELL_TRADING_GOODS_LIST")]
    TypeOpenPcSellTradingGoodsList = 280,
    #[def("UI_ACTION_TYPE_OPEN_PC_WANTED_TRADING_GOODS_LIST")]
    TypeOpenPcWantedTradingGoodsList = 281,
    #[def("UI_ACTION_TYPE_CONSTRUCT_PC_EXPERIENCE_LIST")]
    TypeConstructPcExperienceList = 282,
    #[def("UI_ACTION_TYPE_GO_TO_REDEFINE_KEYS_MENU")]
    TypeGoToRedefineKeysMenu = 283,
    #[def("UI_ACTION_TYPE_RESET_KEYS")]
    TypeResetKeys = 284,
    #[def("UI_ACTION_TYPE_ASSIGN_SPELL_TO_ITEM_SLOT_PC")]
    TypeAssignSpellToItemSlotPc = 285,
    #[def("UI_ACTION_TYPE_CONSTRUCT_STYLE_CARDS_LIST")]
    TypeConstructStyleCardsList = 286,
    #[def("UI_ACTION_TYPE_SCROLL_DESCRIPTION_DOWN")]
    TypeScrollDescriptionDown = 287,
    #[def("UI_ACTION_TYPE_SCROLL_DESCRIPTION_UP")]
    TypeScrollDescriptionUp = 288,
    #[def("UI_ACTION_TYPE_CONSTRUCT_PC_PERSONALITY_LIST")]
    TypeConstructPcPersonalityList = 289,
    #[def("UI_ACTION_TYPE_SEND_OWNED_EVENT_FORCE_OBSERVATION")]
    TypeSendOwnedEventForceObservation = 291,
    #[def("UI_ACTION_TYPE_GO_TO_MAIN_MENU_FROM_PROFILE_LIST")]
    TypeGoToMainMenuFromProfileList = 292,
    #[def("UI_ACTION_TYPE_NEW_PROFILE")]
    TypeNewProfile = 293,
    #[def("UI_ACTION_TYPE_NEW_PROFILE_RETURN_PRESSED")]
    TypeNewProfileReturnPressed = 294,
    #[def("UI_ACTION_TYPE_NEW_PROFILE_ESCAPE_PRESSED")]
    TypeNewProfileEscapePressed = 295,
    #[def("UI_ACTION_TYPE_QUIT_GAME")]
    TypeQuitGame = 296,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_OPTIONS_SUB_MENU")]
    TypeNextScreenOptionsSubMenu = 297,
    #[def("UI_ACTION_TYPE_SET_MESH_DETAIL")]
    TypeSetMeshDetail = 298,
    #[def("UI_ACTION_TYPE_SET_EFFECTS_DETAIL")]
    TypeSetEffectsDetail = 299,
    #[def("UI_ACTION_TYPE_SET_SCREEN_ASPECT_RATIO")]
    TypeSetScreenAspectRatio = 300,
    #[def("UI_ACTION_TYPE_APPLY_PROFILE_VALUES")]
    TypeApplyProfileValues = 301,
    #[def("UI_ACTION_TYPE_CLOSE_ACTIVE_TRADE_MENU")]
    TypeCloseActiveTradeMenu = 302,
    #[def("UI_ACTION_TYPE_SCROLL_ACTIVE_LIST_DOWN_MAX")]
    TypeScrollActiveListDownMax = 303,
    #[def("UI_ACTION_TYPE_SCROLL_ACTIVE_LIST_UP_MAX")]
    TypeScrollActiveListUpMax = 304,
    #[def("UI_ACTION_TYPE_SCROLL_DESCRIPTION_DOWN_MAX")]
    TypeScrollDescriptionDownMax = 305,
    #[def("UI_ACTION_TYPE_SCROLL_DESCRIPTION_UP_MAX")]
    TypeScrollDescriptionUpMax = 306,
    #[def("UI_ACTION_TYPE_SCROLL_DESCRIPTION_DOWN_ONE_PIXEL")]
    TypeScrollDescriptionDownOnePixel = 307,
    #[def("UI_ACTION_TYPE_SCROLL_DESCRIPTION_UP_ONE_PIXEL")]
    TypeScrollDescriptionUpOnePixel = 308,
    #[def("UI_ACTION_TYPE_CLOSE_TRADE_MENU_IF_LEAF")]
    TypeCloseTradeMenuIfLeaf = 309,
    #[def("UI_ACTION_TYPE_EXIT_LIVE_GUI_IF_LEAF")]
    TypeExitLiveGuiIfLeaf = 310,
    #[def("UI_ACTION_TYPE_RESET_KEYS_WASD")]
    TypeResetKeysWasd = 311,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_OPTIONS_SCOREBOARD")]
    TypeNextScreenOptionsScoreboard = 312,
    #[def("UI_ACTION_TYPE_SET_CONTROL_METHOD")]
    TypeSetControlMethod = 313,
    #[def("UI_ACTION_TYPE_GOTO_QUIT_PROMPT")]
    TypeGotoQuitPrompt = 314,
    #[def("UI_ACTION_TYPE_PC_QUIT_TO_FRONT_END")]
    TypePcQuitToFrontEnd = 315,
    #[def("UI_ACTION_TYPE_PC_ACCEPT_VIDEO_CHANGES")]
    TypePcAcceptVideoChanges = 316,
    #[def("UI_ACTION_TYPE_SET_REFRESH_RATE")]
    TypeSetRefreshRate = 317,
    #[def("UI_ACTION_TYPE_SET_CAMERA_SENSITIVITY")]
    TypeSetCameraSensitivity = 318,
    #[def("UI_ACTION_TYPE_SET_BOW_CAMERA")]
    TypeSetBowCamera = 319,
    #[def("UI_ACTION_TYPE_SET_CAMERA_RESETTING")]
    TypeSetCameraResetting = 320,
    #[def("UI_ACTION_TYPE_GOTO_ABOUT_SCREEN")]
    TypeGotoAboutScreen = 321,
    #[def("UI_ACTION_TYPE_CLOSE_FRAME")]
    TypeCloseFrame = 322,
    #[def("UI_ACTION_TYPE_SET_SHOW_TARGETING_STATUS")]
    TypeSetShowTargetingStatus = 323,
    #[def("UI_ACTION_TYPE_RESTORE_DEFAULTS_GAMEPLAY")]
    TypeRestoreDefaultsGameplay = 324,
    #[def("UI_ACTION_TYPE_RESTORE_DEFAULTS_VIDEO")]
    TypeRestoreDefaultsVideo = 325,
    #[def("UI_ACTION_TYPE_RESTORE_DEFAULTS_AUDIO")]
    TypeRestoreDefaultsAudio = 326,
    #[def("UI_ACTION_TYPE_SET_EXPRESSION_ITEM_TO_ASSIGN_SWAPPING_PC")]
    TypeSetExpressionItemToAssignSwappingPc = 327,
    #[def("UI_ACTION_TYPE_SET_SPELL_TO_ASSIGN_SWAPPING_PC")]
    TypeSetSpellToAssignSwappingPc = 328,
    #[def("UI_ACTION_TYPE_SET_SPELL_TO_ASSIGN_IN_ITEMS_SWAPPING_PC")]
    TypeSetSpellToAssignInItemsSwappingPc = 329,
    #[def("UI_ACTION_ADD_MESH_CHILD")]
    AddMeshChild = 2000,
    #[def("UI_ACTION_LOAD_WEAPON_DESC")]
    LoadWeaponDesc = 2001,
    #[def("UI_ACTION_LOAD_CLOTHING_DESC")]
    LoadClothingDesc = 2002,
    #[def("UI_ACTION_LOAD_ITEM_DESC")]
    LoadItemDesc = 2003,
    #[def("UI_ACTION_LOAD_WEAPON_MENU_ENTRY_NAME")]
    LoadWeaponMenuEntryName = 2004,
    #[def("UI_ACTION_LOAD_CLOTHING_MENU_ENTRY_NAME")]
    LoadClothingMenuEntryName = 2005,
    #[def("UI_ACTION_LOAD_ITEM_MENU_ENTRY_NAME")]
    LoadItemMenuEntryName = 2006,
    #[def("UI_ACTION_LOAD_SELL_ITEM_DESC")]
    LoadSellItemDesc = 2007,
    #[def("UI_ACTION_LOAD_SELL_ACTION_MENU")]
    LoadSellActionMenu = 2008,
    #[def("UI_ACTION_LOAD_BUY_ITEM_DESC")]
    LoadBuyItemDesc = 2009,
    #[def("UI_ACTION_LOAD_BUY_ACTION_MENU")]
    LoadBuyActionMenu = 2010,
    #[def("UI_ACTION_LOAD_WANTED_ITEM_DESC")]
    LoadWantedItemDesc = 2011,
    #[def("UI_ACTION_LOAD_QUICK_MENU_EXPRESSIONS_ICON")]
    LoadQuickMenuExpressionsIcon = 2012,
    #[def("UI_ACTION_LOAD_FILE_DESC")]
    LoadFileDesc = 2013,
    #[def("UI_ACTION_LOAD_FILE_SAVEGAME_MINIMAP")]
    LoadFileSavegameMinimap = 2014,
    #[def("UI_ACTION_REMOVE_MESH_CHILD")]
    RemoveMeshChild = 2015,
    #[def("UI_ACTION_UNLOAD_WEAPON_DESC")]
    UnloadWeaponDesc = 2016,
    #[def("UI_ACTION_UNLOAD_CLOTHING_DESC")]
    UnloadClothingDesc = 2017,
    #[def("UI_ACTION_UNLOAD_ITEM_DESC")]
    UnloadItemDesc = 2018,
    #[def("UI_ACTION_UNLOAD_SELL_ITEM_DESC")]
    UnloadSellItemDesc = 2019,
    #[def("UI_ACTION_UNLOAD_SELL_ACTION_MENU")]
    UnloadSellActionMenu = 2020,
    #[def("UI_ACTION_UNLOAD_BUY_ITEM_DESC")]
    UnloadBuyItemDesc = 2021,
    #[def("UI_ACTION_UNLOAD_BUY_ACTION_MENU")]
    UnloadBuyActionMenu = 2022,
    #[def("UI_ACTION_UNLOAD_WANTED_ITEM_DESC")]
    UnloadWantedItemDesc = 2023,
    #[def("UI_ACTION_TYPE_CHEAT_MORALITY")]
    TypeCheatMorality = 3000,
    #[def("UI_ACTION_TYPE_CHEAT_RENOWN")]
    TypeCheatRenown = 3001,
    #[def("UI_ACTION_TYPE_CLOSE_BOAST_MENU")]
    TypeCloseBoastMenu = 3002,
    #[def("UI_ACTION_TYPE_PLAY_SOUND")]
    TypePlaySound = 3003,
    #[def("UI_ACTION_TYPE_TAKE_QUEST_FOR_BOAST")]
    TypeTakeQuestForBoast = 3004,
    #[def("UI_ACTION_TYPE_OPEN_PC_MSN_CHAT")]
    TypeOpenPcMsnChat = 3005,
    #[def("UI_ACTION_TYPE_ACTIVATE_MSN_CONVERSATION")]
    TypeActivateMsnConversation = 3006,
    #[def("UI_ACTION_TYPE_SEND_MESSAGE")]
    TypeSendMessage = 3007,
    #[def("UI_ACTION_TYPE_SELECT_CONTACT")]
    TypeSelectContact = 3008,
    #[def("UI_ACTION_TYPE_SCROLL_VIEWPORT_UP")]
    TypeScrollViewportUp = 3009,
    #[def("UI_ACTION_TYPE_SCROLL_VIEWPORT_DOWN")]
    TypeScrollViewportDown = 3010,
    #[def("UI_ACTION_TYPE_SCROLL_LIST_DOWN")]
    TypeScrollListDown = 3011,
    #[def("UI_ACTION_TYPE_SCROLL_LIST_UP")]
    TypeScrollListUp = 3012,
    #[def("UI_ACTION_TYPE_OPEN_SCOREBOARD")]
    TypeOpenScoreboard = 3013,
    #[def("UI_ACTION_TYPE_CHOOSE_CLAN")]
    TypeChooseClan = 3014,
    #[def("UI_ACTION_TYPE_PHOTO_CAPTION")]
    TypePhotoCaption = 3015,
    #[def("UI_ACTION_TYPE_DISCARD_PHOTO")]
    TypeDiscardPhoto = 3016,
    #[def("UI_ACTION_TYPE_GO_TO_SCOREBOARD_SCREEN")]
    TypeGoToScoreboardScreen = 3017,
    #[def("UI_ACTION_TYPE_ACTIVATE_EDITBOX")]
    TypeActivateEditbox = 3018,
    #[def("UI_ACTION_TYPE_DEACTIVATE_EDITBOX")]
    TypeDeactivateEditbox = 3019,
    #[def("UI_ACTION_TYPE_SET_EDITBOX_VALUES")]
    TypeSetEditboxValues = 3020,
    #[def("UI_ACTION_TYPE_SCOREBOARD_OK")]
    TypeScoreboardOk = 3021,
    #[def("UI_ACTION_TYPE_LOAD_PHOTO")]
    TypeLoadPhoto = 3022,
    #[def("UI_ACTION_TYPE_SCROLL_PHOTO_DOWN")]
    TypeScrollPhotoDown = 3023,
    #[def("UI_ACTION_TYPE_SCROLL_PHOTO_UP")]
    TypeScrollPhotoUp = 3024,
    #[def("UI_ACTION_TYPE_NEXT_SCREEN_START")]
    TypeNextScreenStart = 3025,
    #[def("UI_ACTION_TYPE_OPEN_PHOTOJOURNAL")]
    TypeOpenPhotojournal = 4000,
    #[def("UI_ACTION_TYPE_OPEN_PHOTO")]
    TypeOpenPhoto = 4001,
    #[def("UI_ACTION_TYPE_EXIT_PHOTOJOURNAL_CAPTURE")]
    TypeExitPhotojournalCapture = 4002,
}

/// Sprite slot of a UI table (key of `UiDef::sprites`).
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum TableSprites {
    #[def("TABLE_SPRITES_TOP_LEFT")]
    TopLeft = 0,
    #[def("TABLE_SPRITES_TOP_RIGHT")]
    TopRight = 1,
    #[def("TABLE_SPRITES_BOTTOM_LEFT")]
    BottomLeft = 2,
    #[def("TABLE_SPRITES_BOTTOM_RIGHT")]
    BottomRight = 3,
    #[def("TABLE_SPRITES_TOP_MIDDLE")]
    TopMiddle = 4,
    #[def("TABLE_SPRITES_BOTTOM_MIDDLE")]
    BottomMiddle = 5,
    #[def("TABLE_SPRITES_MIDDLE_LEFT")]
    MiddleLeft = 6,
    #[def("TABLE_SPRITES_MIDDLE_RIGHT")]
    MiddleRight = 7,
    #[def("TABLE_SPRITES_SEPARATION_BOTTOM")]
    SeparationBottom = 8,
    #[def("TABLE_SPRITES_SEPARATION_TOP")]
    SeparationTop = 9,
    #[def("TABLE_SPRITES_SEPARATION_LEFT")]
    SeparationLeft = 10,
    #[def("TABLE_SPRITES_SEPARATION_RIGHT")]
    SeparationRight = 11,
    #[def("TABLE_SPRITES_CROSS")]
    Cross = 12,
    #[def("TABLE_SPRITES_AMOUNT")]
    Amount = 13,
}

/// Engine graphic/mesh kind.
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum EngineGraphicType {
    #[def("ENGINE_GRAPHIC_NULL")]
    EngineGraphicNull = 0,
    #[def("ENGINE_GRAPHIC_SPRITE")]
    EngineGraphicSprite = 1,
    #[def("ENGINE_GRAPHIC_3DSPRITE")]
    EngineGraphic3dsprite = 2,
    #[def("ENGINE_GRAPHIC_GENERATED_EFFECT")]
    EngineGraphicGeneratedEffect = 3,
    #[def("ENGINE_GRAPHIC_ANIMATING_MESH")]
    EngineGraphicAnimatingMesh = 4,
    #[def("ENGINE_GRAPHIC_STATIC_MESH")]
    EngineGraphicStaticMesh = 5,
    #[def("MAX_NO_ENGINE_GRAPHIC_TYPES")]
    MaxNoEngineGraphicTypes = 6,
}

/// 2D sprite render flags. `0` (no flags) occurs in real data.
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default, DefFlags)]
#[flags(
    CENTRE_ON_POS = 1 => "ENGINE_2D_SPRITE_CENTRE_ON_POS",
    ENABLE_FILTERING = 2 => "ENGINE_2D_SPRITE_ENABLE_FILTERING",
)]
pub struct Sprite2dFlags(pub i32);

/// Bindable game action.
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum GameAction {
    #[def("GAME_ACTION_NULL")]
    Null = 0,
    #[def("GAME_ACTION_LOCK_TARGET")]
    LockTarget = 1,
    #[def("GAME_ACTION_OPEN_INVENTORY")]
    OpenInventory = 2,
    #[def("GAME_ACTION_OPEN_IN_GAME_MENU")]
    OpenInGameMenu = 3,
    #[def("GAME_ACTION_TOGGLE_MINI_MAP")]
    ToggleMiniMap = 4,
    #[def("GAME_ACTION_PAUSE")]
    Pause = 5,
    #[def("GAME_ACTION_INTERACT")]
    Interact = 6,
    #[def("GAME_ACTION_BLOCK")]
    Block = 7,
    #[def("GAME_ACTION_SPECIAL_ATTACK")]
    SpecialAttack = 8,
    #[def("GAME_ACTION_ATTACK")]
    Attack = 9,
    #[def("GAME_ACTION_FIRE_RANGED_WEAPON")]
    FireRangedWeapon = 10,
    #[def("GAME_ACTION_UNARMED_ATTACK")]
    UnarmedAttack = 11,
    #[def("GAME_ACTION_ARMED_MELEE_ATTACK")]
    ArmedMeleeAttack = 12,
    #[def("GAME_ACTION_UNSHEATHE_MELEE_WEAPON")]
    UnsheatheMeleeWeapon = 13,
    #[def("GAME_ACTION_UNSHEATHE_RANGED_WEAPON")]
    UnsheatheRangedWeapon = 14,
    #[def("GAME_ACTION_SHEATHE_MELEE_WEAPON")]
    SheatheMeleeWeapon = 15,
    #[def("GAME_ACTION_SHEATHE_RANGED_WEAPON")]
    SheatheRangedWeapon = 16,
    #[def("GAME_ACTION_TOGGLE_SILENT_MOVE")]
    ToggleSilentMove = 17,
    #[def("GAME_ACTION_TOGGLE_CINEMATIC_AND_USER_CAMERA")]
    ToggleCinematicAndUserCamera = 18,
    #[def("GAME_ACTION_TOGGLE_FIRST_PERSON_VIEW")]
    ToggleFirstPersonView = 19,
    #[def("GAME_ACTION_SKIP_PAST_TEXT")]
    SkipPastText = 20,
    #[def("GAME_ACTION_SKIP_CUT_SCENE")]
    SkipCutScene = 21,
    #[def("GAME_ACTION_ANSWER_QUESTION_YES")]
    AnswerQuestionYes = 22,
    #[def("GAME_ACTION_ANSWER_QUESTION_NO")]
    AnswerQuestionNo = 23,
    #[def("GAME_ACTION_ANSWER_QUESTION_THIRD")]
    AnswerQuestionThird = 24,
    #[def("GAME_ACTION_INVENTORY_SELECT")]
    InventorySelect = 25,
    #[def("GAME_ACTION_ATTRACT_EXPERIENCE_ORBS")]
    AttractExperienceOrbs = 26,
    #[def("GAME_ACTION_ROTATE_GUI_SCREENS_LEFT")]
    RotateGuiScreensLeft = 27,
    #[def("GAME_ACTION_ROTATE_GUI_SCREENS_RIGHT")]
    RotateGuiScreensRight = 28,
    #[def("GAME_ACTION_JUMP")]
    Jump = 29,
    #[def("GAME_ACTION_SPRINT")]
    Sprint = 30,
    #[def("GAME_ACTION_RUN")]
    Run = 31,
    #[def("GAME_ACTION_SNEAK")]
    Sneak = 32,
    #[def("GAME_ACTION_INVENTORY_A")]
    InventoryA = 33,
    #[def("GAME_ACTION_INVENTORY_B")]
    InventoryB = 34,
    #[def("GAME_ACTION_INVENTORY_X")]
    InventoryX = 35,
    #[def("GAME_ACTION_INVENTORY_Y")]
    InventoryY = 36,
    #[def("GAME_ACTION_INVENTORY_UP")]
    InventoryUp = 37,
    #[def("GAME_ACTION_INVENTORY_DOWN")]
    InventoryDown = 38,
    #[def("GAME_ACTION_INVENTORY_LEFT")]
    InventoryLeft = 39,
    #[def("GAME_ACTION_INVENTORY_RIGHT")]
    InventoryRight = 40,
    #[def("GAME_ACTION_INVENTORY_WHITE")]
    InventoryWhite = 41,
    #[def("GAME_ACTION_TAVERN_GAMES_INSTRUCTIONS")]
    TavernGamesInstructions = 42,
    #[def("GAME_ACTION_FISHING_REEL_IN")]
    FishingReelIn = 43,
    #[def("GAME_ACTION_FISHING_CANCEL")]
    FishingCancel = 44,
    #[def("GAME_ACTION_TOGGLE_FIRST_PERSON_TARGETING")]
    ToggleFirstPersonTargeting = 45,
    #[def("GAME_ACTION_FIRST_PERSON_TARGET_LOCK")]
    FirstPersonTargetLock = 46,
    #[def("GAME_ACTION_FIRST_PERSON_ZOOM_IN")]
    FirstPersonZoomIn = 47,
    #[def("GAME_ACTION_GENERAL_LEAVE_PLAYER_MODE")]
    GeneralLeavePlayerMode = 48,
    #[def("GAME_ACTION_DEBUG_JUMP_1")]
    DebugJump1 = 49,
    #[def("GAME_ACTION_DEBUG_JUMP_2")]
    DebugJump2 = 50,
    #[def("GAME_ACTION_DEBUG_CAMERA")]
    DebugCamera = 51,
    #[def("GAME_ACTION_DEBUG_SHIFT")]
    DebugShift = 52,
    #[def("GAME_ACTION_TAKE_PHOTO_FOR_PHOTOJOURNAL")]
    TakePhotoForPhotojournal = 53,
    #[def("GAME_ACTION_ASSIGNABLE_SPECIAL_MOVE")]
    AssignableSpecialMove = 54,
    #[def("GAME_ACTION_QUICK_ACCESS_ITEM")]
    QuickAccessItem = 55,
    #[def("GAME_ACTION_CONTEXT_SENSITIVE_ITEM")]
    ContextSensitiveItem = 56,
    #[def("GAME_ACTION_CYCLE_THROUGH_SPELLS")]
    CycleThroughSpells = 57,
    #[def("GAME_ACTION_COIN_GOLF_CANCEL_AIM")]
    CoinGolfCancelAim = 58,
    #[def("GAME_ACTION_CONFIRM_RESET_TO_FRONT_END")]
    ConfirmResetToFrontEnd = 59,
    #[def("GAME_ACTION_MOVEMENT")]
    Movement = 60,
    #[def("GAME_ACTION_CAMERA_ROTATE")]
    CameraRotate = 61,
    #[def("GAME_ACTION_CAMERA_ROTATE_LEFT")]
    CameraRotateLeft = 62,
    #[def("GAME_ACTION_CAMERA_ROTATE_RIGHT")]
    CameraRotateRight = 63,
    #[def("GAME_ACTION_CAMERA_ZOOM_IN")]
    CameraZoomIn = 64,
    #[def("GAME_ACTION_CAMERA_ZOOM_OUT")]
    CameraZoomOut = 65,
    #[def("GAME_ACTION_ORACLE_MINIGAME_UP")]
    OracleMinigameUp = 66,
    #[def("GAME_ACTION_ORACLE_MINIGAME_DOWN")]
    OracleMinigameDown = 67,
    #[def("GAME_ACTION_ORACLE_MINIGAME_LEFT")]
    OracleMinigameLeft = 68,
    #[def("GAME_ACTION_ORACLE_MINIGAME_RIGHT")]
    OracleMinigameRight = 69,
    #[def("GAME_ACTION_ORACLE_MINIGAME_QUIT")]
    OracleMinigameQuit = 70,
    #[def("GAME_ACTION_MOVE_MOUSE_ON_GUI")]
    MoveMouseOnGui = 71,
    #[def("GAME_ACTION_TOGGLE_LIVE_GUI")]
    ToggleLiveGui = 72,
    #[def("GAME_ACTION_OPEN_EXPRESSION_MENU")]
    OpenExpressionMenu = 73,
    #[def("GAME_ACTION_TOGGLE_DEACTIVATE_LOCK_TARGET")]
    ToggleDeactivateLockTarget = 74,
    #[def("GAME_ACTION_FIRST_PERSON_LOOKAROUND")]
    FirstPersonLookaround = 75,
    #[def("GAME_ACTION_INVENTORY_UNSELECT")]
    InventoryUnselect = 76,
    #[def("GAME_ACTION_CAMERA_MOVE_DOUBLE_AXIS")]
    CameraMoveDoubleAxis = 77,
    #[def("GAME_ACTION_CHARGE_GUILD_SEAL")]
    ChargeGuildSeal = 78,
    #[def("GAME_ACTION_TAVERN_GAME_MOVEMENT")]
    TavernGameMovement = 79,
    #[def("GAME_ACTION_TAVERN_GAME_CAMERA")]
    TavernGameCamera = 80,
    #[def("GAME_ACTION_TAVERN_GAME_ACTION_BUTTON")]
    TavernGameActionButton = 81,
    #[def("GAME_ACTION_TAVERN_GAME_ALTERNATE_BUTTON")]
    TavernGameAlternateButton = 82,
    #[def("GAME_ACTION_TAVERN_GAME_QUIT")]
    TavernGameQuit = 83,
    #[def("GAME_ACTION_PROJECTILE_TARGETING_ANALOGUE_ZOOM")]
    ProjectileTargetingAnalogueZoom = 84,
    #[def("GAME_ACTION_TOGGLE_PASSIVE_AGGRESSIVE_MODE")]
    TogglePassiveAggressiveMode = 85,
    #[def("GAME_ACTION_ACTIVATE_SPELL_MODE")]
    ActivateSpellMode = 86,
    #[def("GAME_ACTION_EXPRESSION_SHIFT")]
    ExpressionShift = 87,
    #[def("GAME_ACTION_SCROLL_DESCRIPTION_UP")]
    ScrollDescriptionUp = 88,
    #[def("GAME_ACTION_SCROLL_DESCRIPTION_DOWN")]
    ScrollDescriptionDown = 89,
    #[def("GAME_ACTION_OPEN_WEAPONS_MENU")]
    OpenWeaponsMenu = 90,
    #[def("GAME_ACTION_OPEN_CLOTHING_MENU")]
    OpenClothingMenu = 91,
    #[def("GAME_ACTION_OPEN_ITEMS_MENU")]
    OpenItemsMenu = 92,
    #[def("GAME_ACTION_OPEN_CURRENT_QUESTS_MENU")]
    OpenCurrentQuestsMenu = 93,
    #[def("GAME_ACTION_CYCLE_THROUGH_SPELLS_KEYBOARD")]
    CycleThroughSpellsKeyboard = 94,
    #[def("GAME_ACTION_TOGGLE_KILL_EVERYTHING_MODE")]
    ToggleKillEverythingMode = 95,
    #[def("GAME_ACTION_OPEN_MAGIC_MENU")]
    OpenMagicMenu = 96,
    #[def("GAME_ACTION_OPEN_EXPRESSIONS_MENU")]
    OpenExpressionsMenu = 97,
    #[def("GAME_ACTION_OPEN_PERSONALITY_MENU")]
    OpenPersonalityMenu = 98,
    #[def("GAME_ACTION_OPEN_LOGBOOK_MENU")]
    OpenLogbookMenu = 99,
    #[def("GAME_ACTION_OPEN_MAP_MENU")]
    OpenMapMenu = 100,
    #[def("GAME_ACTION_SCROLL_MENU")]
    ScrollMenu = 101,
    #[def("GAME_ACTION_ZOOM_MAP_OUT")]
    ZoomMapOut = 102,
    #[def("GAME_ACTION_ZOOM_MAP_IN")]
    ZoomMapIn = 103,
    #[def("GAME_ACTION_SCROLL_MAP_LEFT")]
    ScrollMapLeft = 104,
    #[def("GAME_ACTION_SCROLL_MAP_RIGHT")]
    ScrollMapRight = 105,
    #[def("GAME_ACTION_SCROLL_MAP_DOWN")]
    ScrollMapDown = 106,
    #[def("GAME_ACTION_SCROLL_MAP_UP")]
    ScrollMapUp = 107,
    #[def("GAME_ACTION_OPTIONS_SLIDER_LEFT")]
    OptionsSliderLeft = 108,
    #[def("GAME_ACTION_OPTIONS_SLIDER_RIGHT")]
    OptionsSliderRight = 109,
    #[def("GAME_ACTION_DIGITAL_ANALOGUE_ZOOM_IN")]
    DigitalAnalogueZoomIn = 110,
    #[def("GAME_ACTION_DIGITAL_ANALOGUE_ZOOM_OUT")]
    DigitalAnalogueZoomOut = 111,
    #[def("GAME_ACTION_TOGGLE_VIEW_HERO_MODE")]
    ToggleViewHeroMode = 112,
    #[def("GAME_ACTION_CENTRE_CAMERA")]
    CentreCamera = 113,
    #[def("GAME_ACTION_BETTING")]
    Betting = 114,
    #[def("GAME_ACTION_COUNT")]
    Count = 115,
    // Anniversary retail additions (values 116-127, names not recovered)
    #[def("GAME_ACTION_116")]
    GameAction116 = 116,
    #[def("GAME_ACTION_117")]
    GameAction117 = 117,
    #[def("GAME_ACTION_118")]
    GameAction118 = 118,
    #[def("GAME_ACTION_119")]
    GameAction119 = 119,
    #[def("GAME_ACTION_120")]
    GameAction120 = 120,
    #[def("GAME_ACTION_121")]
    GameAction121 = 121,
    #[def("GAME_ACTION_122")]
    GameAction122 = 122,
    #[def("GAME_ACTION_123")]
    GameAction123 = 123,
    #[def("GAME_ACTION_124")]
    GameAction124 = 124,
    #[def("GAME_ACTION_125")]
    GameAction125 = 125,
    #[def("GAME_ACTION_126")]
    GameAction126 = 126,
    #[def("GAME_ACTION_127")]
    GameAction127 = 127,
}

/// Input controller kind.
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ControllerType {
    #[def("CONTROLLER_NONE")]
    None = 0,
    #[def("CONTROLLER_XBOX_PAD")]
    XboxPad = 1,
    #[def("CONTROLLER_KEYBOARD")]
    Keyboard = 2,
    #[def("CONTROLLER_MOUSE")]
    Mouse = 3,
}

/// Xbox pad button.
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum XboxControllerButton {
    #[def("XBOX_PAD_UNDEFINED_BUTTON")]
    UndefinedButton = 0,
    #[def("XBOX_PAD_X_BUTTON")]
    XButton = 1,
    #[def("XBOX_PAD_Y_BUTTON")]
    YButton = 2,
    #[def("XBOX_PAD_BLACK_BUTTON")]
    BlackButton = 3,
    #[def("XBOX_PAD_A_BUTTON")]
    AButton = 4,
    #[def("XBOX_PAD_B_BUTTON")]
    BButton = 5,
    #[def("XBOX_PAD_WHITE_BUTTON")]
    WhiteButton = 6,
    #[def("XBOX_PAD_LEFT_TRIGGER")]
    LeftTrigger = 7,
    #[def("XBOX_PAD_RIGHT_TRIGGER")]
    RightTrigger = 8,
    #[def("XBOX_PAD_LEFT_STICK_BUTTON")]
    LeftStickButton = 9,
    #[def("XBOX_PAD_RIGHT_STICK_BUTTON")]
    RightStickButton = 10,
    #[def("XBOX_PAD_START_BUTTON")]
    StartButton = 11,
    #[def("XBOX_PAD_BACK_BUTTON")]
    BackButton = 12,
    #[def("XBOX_PAD_DPAD_UP_BUTTON")]
    DpadUpButton = 13,
    #[def("XBOX_PAD_DPAD_DOWN_BUTTON")]
    DpadDownButton = 14,
    #[def("XBOX_PAD_DPAD_LEFT_BUTTON")]
    DpadLeftButton = 15,
    #[def("XBOX_PAD_DPAD_RIGHT_BUTTON")]
    DpadRightButton = 16,
    #[def("XBOX_PAD_LEFT_ANALOGUE_STICK")]
    LeftAnalogueStick = 17,
    #[def("XBOX_PAD_RIGHT_ANALOGUE_STICK")]
    RightAnalogueStick = 18,
}

/// Mouse button or movement binding.
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum MouseButtonControl {
    #[def("MOUSE_BUTTON_NULL_CONTROL")]
    ButtonNullControl = 0,
    #[def("MOUSE_BUTTON_LEFT_CONTROL")]
    ButtonLeftControl = 1,
    #[def("MOUSE_BUTTON_RIGHT_CONTROL")]
    ButtonRightControl = 2,
    #[def("MOUSE_BUTTON_MIDDLE_CONTROL")]
    ButtonMiddleControl = 3,
    #[def("MOUSE_MOVEMENT")]
    Movement = 4,
    #[def("MOUSE_WHEEL_MOVEMENT")]
    WheelMovement = 5,
    #[def("MOUSE_WHEEL_MOVEMENT_UP")]
    WheelMovementUp = 6,
    #[def("MOUSE_WHEEL_MOVEMENT_DOWN")]
    WheelMovementDown = 7,
    #[def("MOUSE_BUTTON_4_CONTROL")]
    Button4Control = 8,
    #[def("MOUSE_BUTTON_5_CONTROL")]
    Button5Control = 9,
    #[def("MOUSE_BUTTON_6_CONTROL")]
    Button6Control = 10,
    #[def("MOUSE_BUTTON_7_CONTROL")]
    Button7Control = 11,
    #[def("MOUSE_BUTTON_8_CONTROL")]
    Button8Control = 12,
}

/// Keyboard key binding.
///
/// C++ `EInputKey` (`Data/Defs/keyboard_keys.h`). The `NO_INPUT_KEYS` count
/// enumerator is omitted.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum InputKey {
    #[def("KB_NULL")]
    Null = 0,
    #[def("KB_ESC")]
    Esc = 1,
    #[def("KB_1")]
    Num1 = 2,
    #[def("KB_2")]
    Num2 = 3,
    #[def("KB_3")]
    Num3 = 4,
    #[def("KB_4")]
    Num4 = 5,
    #[def("KB_5")]
    Num5 = 6,
    #[def("KB_6")]
    Num6 = 7,
    #[def("KB_7")]
    Num7 = 8,
    #[def("KB_8")]
    Num8 = 9,
    #[def("KB_9")]
    Num9 = 10,
    #[def("KB_0")]
    Num0 = 11,
    #[def("KB_MINUS")]
    Minus = 12,
    #[def("KB_EQUALS")]
    Equals = 13,
    #[def("KB_BACKSPACE")]
    Backspace = 14,
    #[def("KB_TAB")]
    Tab = 15,
    #[def("KB_Q")]
    Q = 16,
    #[def("KB_W")]
    W = 17,
    #[def("KB_E")]
    E = 18,
    #[def("KB_R")]
    R = 19,
    #[def("KB_T")]
    T = 20,
    #[def("KB_Y")]
    Y = 21,
    #[def("KB_U")]
    U = 22,
    #[def("KB_I")]
    I = 23,
    #[def("KB_O")]
    O = 24,
    #[def("KB_P")]
    P = 25,
    #[def("KB_LBRACKET")]
    Lbracket = 26,
    #[def("KB_RBRACKET")]
    Rbracket = 27,
    #[def("KB_RETURN")]
    Return = 28,
    #[def("KB_LCONTROL")]
    Lcontrol = 29,
    #[def("KB_A")]
    A = 30,
    #[def("KB_S")]
    S = 31,
    #[def("KB_D")]
    D = 32,
    #[def("KB_F")]
    F = 33,
    #[def("KB_G")]
    G = 34,
    #[def("KB_H")]
    H = 35,
    #[def("KB_J")]
    J = 36,
    #[def("KB_K")]
    K = 37,
    #[def("KB_L")]
    L = 38,
    #[def("KB_SEMICOLON")]
    Semicolon = 39,
    #[def("KB_APOSTROPHE")]
    Apostrophe = 40,
    #[def("KB_HASH")]
    Hash = 41,
    #[def("KB_LSHIFT")]
    Lshift = 42,
    #[def("KB_BACKSLASH")]
    Backslash = 43,
    #[def("KB_Z")]
    Z = 44,
    #[def("KB_X")]
    X = 45,
    #[def("KB_C")]
    C = 46,
    #[def("KB_V")]
    V = 47,
    #[def("KB_B")]
    B = 48,
    #[def("KB_N")]
    N = 49,
    #[def("KB_M")]
    M = 50,
    #[def("KB_COMMA")]
    Comma = 51,
    #[def("KB_FULLSTOP")]
    Fullstop = 52,
    #[def("KB_SLASH")]
    Slash = 53,
    #[def("KB_RSHIFT")]
    Rshift = 54,
    #[def("KB_PMULTIPLY")]
    Pmultiply = 55,
    #[def("KB_LALT")]
    Lalt = 56,
    #[def("KB_SPACE")]
    Space = 57,
    #[def("KB_CAPSLOCK")]
    Capslock = 58,
    #[def("KB_F1")]
    F1 = 59,
    #[def("KB_F2")]
    F2 = 60,
    #[def("KB_F3")]
    F3 = 61,
    #[def("KB_F4")]
    F4 = 62,
    #[def("KB_F5")]
    F5 = 63,
    #[def("KB_F6")]
    F6 = 64,
    #[def("KB_F7")]
    F7 = 65,
    #[def("KB_F8")]
    F8 = 66,
    #[def("KB_F9")]
    F9 = 67,
    #[def("KB_F10")]
    F10 = 68,
    #[def("KB_NUMLOCK")]
    Numlock = 69,
    #[def("KB_SCROLLLOCK")]
    Scrolllock = 70,
    #[def("KB_P7")]
    P7 = 71,
    #[def("KB_P8")]
    P8 = 72,
    #[def("KB_P9")]
    P9 = 73,
    #[def("KB_PMINUS")]
    Pminus = 74,
    #[def("KB_P4")]
    P4 = 75,
    #[def("KB_P5")]
    P5 = 76,
    #[def("KB_P6")]
    P6 = 77,
    #[def("KB_PPLUS")]
    Pplus = 78,
    #[def("KB_P1")]
    P1 = 79,
    #[def("KB_P2")]
    P2 = 80,
    #[def("KB_P3")]
    P3 = 81,
    #[def("KB_P0")]
    P0 = 82,
    #[def("KB_PFULLSTOP")]
    Pfullstop = 83,
    #[def("KB_F11")]
    F11 = 84,
    #[def("KB_F12")]
    F12 = 85,
    #[def("KB_F13")]
    F13 = 86,
    #[def("KB_F14")]
    F14 = 87,
    #[def("KB_F15")]
    F15 = 88,
    #[def("KB_KANA")]
    Kana = 89,
    #[def("KB_CONVERT")]
    Convert = 90,
    #[def("KB_NOCONVERT")]
    Noconvert = 91,
    #[def("KB_YEN")]
    Yen = 92,
    #[def("KB_PEQUALS")]
    Pequals = 93,
    #[def("KB_CIRCUMFLEX")]
    Circumflex = 94,
    #[def("KB_AT")]
    At = 95,
    #[def("KB_COLON")]
    Colon = 96,
    #[def("KB_UNDERLINE")]
    Underline = 97,
    #[def("KB_KANJI")]
    Kanji = 98,
    #[def("KB_STOP")]
    Stop = 99,
    #[def("KB_AX")]
    Ax = 100,
    #[def("KB_UNLABELED")]
    Unlabeled = 101,
    #[def("KB_PENTER")]
    Penter = 102,
    #[def("KB_RCONTROL")]
    Rcontrol = 103,
    #[def("KB_PCOMMA")]
    Pcomma = 104,
    #[def("KB_PDIVIDE")]
    Pdivide = 105,
    #[def("KB_SYSRQ")]
    Sysrq = 106,
    #[def("KB_RALT")]
    Ralt = 107,
    #[def("KB_HOME")]
    Home = 108,
    #[def("KB_UP")]
    Up = 109,
    #[def("KB_PAGEUP")]
    Pageup = 110,
    #[def("KB_LEFT")]
    Left = 111,
    #[def("KB_RIGHT")]
    Right = 112,
    #[def("KB_END")]
    End = 113,
    #[def("KB_DOWN")]
    Down = 114,
    #[def("KB_PAGEDOWN")]
    Pagedown = 115,
    #[def("KB_INSERT")]
    Insert = 116,
    #[def("KB_DELETE")]
    Delete = 117,
    #[def("KB_LWIN")]
    Lwin = 118,
    #[def("KB_RWIN")]
    Rwin = 119,
    #[def("KB_APPS")]
    Apps = 120,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn enum_round_trip_and_symbols() {
        assert_eq!(UiType::from_i32(32), Some(UiType::MousePointer));
        assert_eq!(UiType::MousePointer.to_i32(), 32);
        assert_eq!(UiType::MousePointer.symbol(), "UI_TYPE_MOUSE_POINTER");
        assert_eq!(
            UiType::from_symbol("UI_TYPE_MOUSE_POINTER"),
            Some(UiType::MousePointer)
        );
        assert_eq!(UiType::from_i32(99), None);
    }

    #[test]
    fn controller_dispatch_values() {
        // The CActionInputControl slot dispatch relies on these exact values.
        assert_eq!(ControllerType::XboxPad.to_i32(), 1);
        assert_eq!(ControllerType::Keyboard.to_i32(), 2);
        assert_eq!(ControllerType::Mouse.to_i32(), 3);
    }

    #[test]
    fn keyboard_keys_sequential_start() {
        assert_eq!(InputKey::from_i32(0), Some(InputKey::Null));
        assert_eq!(InputKey::from_i32(12), Some(InputKey::Minus));
    }

    #[test]
    fn flags_lossless() {
        // game.bin contains OR'd expansion values; must round-trip.
        let both = TableExpansion::from_i32(3);
        assert!(both.contains(TableExpansion::HORIZONTAL));
        assert!(both.contains(TableExpansion::VERTICAL));
        assert_eq!(both.to_i32(), 3);
        assert_eq!(
            (TableExpansion::HORIZONTAL | TableExpansion::VERTICAL).to_i32(),
            3
        );
        assert!(Sprite2dFlags::from_i32(0).is_empty());
    }
}

// ── generated game.bin enums ──────────────────────────────────

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ActionRegisteredType {
    #[def("ACTION_NULL")]
    NULL = 0,
    #[def("ACTION_JACK_OF_BLADES_HIT_RESPONSE")]
    JACKOFBLADESHITRESPONSE = 1,
    #[def("ACTION_SCORPION_KING_HIT_RESPONSE")]
    SCORPIONKINGHITRESPONSE = 2,
    #[def("ACTION_MAZE_CAUSE_FORCE_PUSH_HIT_RESPONSE")]
    MAZECAUSEFORCEPUSHHITRESPONSE = 3,
    #[def("ACTION_ROCK_TROLL_DEATH")]
    ROCKTROLLDEATH = 4,
    #[def("ACTION_KING_SCORPION_DEATH")]
    KINGSCORPIONDEATH = 5,
    #[def("ACTION_WASP_QUEEN_DEATH")]
    WASPQUEENDEATH = 6,
    #[def("ACTION_COMBAT_GENERIC_PROJECTILE_WEAPON_AIM")]
    COMBATGENERICPROJECTILEWEAPONAIM = 7,
    #[def("ACTION_COMBAT_GENERIC_PROJECTILE_WEAPON_FIRE")]
    COMBATGENERICPROJECTILEWEAPONFIRE = 8,
    #[def("ACTION_COMBAT_GENERIC_PROJECTILE_WEAPON_LOAD")]
    COMBATGENERICPROJECTILEWEAPONLOAD = 9,
    #[def("ACTION_COMBAT_GENERIC_LEADER_COMMAND")]
    COMBATGENERICLEADERCOMMAND = 10,
    #[def("ACTION_COMBAT_GENERIC_BOAST")]
    COMBATGENERICBOAST = 11,
    #[def("ACTION_COMBAT_UNBLOCKABLE_ATTACK")]
    COMBATUNBLOCKABLEATTACK = 12,
    #[def("ACTION_COMBAT_HOP_BACK")]
    COMBATHOPBACK = 13,
    #[def("ACTION_COMBAT_STRAFE_FORWARD")]
    COMBATSTRAFEFORWARD = 14,
    #[def("ACTION_COMBAT_STRAFE_BACKWARD")]
    COMBATSTRAFEBACKWARD = 15,
    #[def("ACTION_COMBAT_STRAFE_LEFT")]
    COMBATSTRAFELEFT = 16,
    #[def("ACTION_COMBAT_STRAFE_RIGHT")]
    COMBATSTRAFERIGHT = 17,
    #[def("ACTION_COMBAT_STRAFE_BACK_LEFT")]
    COMBATSTRAFEBACKLEFT = 18,
    #[def("ACTION_COMBAT_STRAFE_BACK_RIGHT")]
    COMBATSTRAFEBACKRIGHT = 19,
    #[def("ACTION_COMBAT_CHARGE")]
    COMBATCHARGE = 20,
    #[def("ACTION_COMBAT_IDLE")]
    COMBATIDLE = 21,
    #[def("ACTION_BREAK_INTO_MELEE")]
    BREAKINTOMELEE = 22,
    #[def("ACTION_COMBAT_SUMMON_CREATURES")]
    COMBATSUMMONCREATURES = 23,
    #[def("ACTION_COMBAT_ATTACK_LUNGE")]
    COMBATATTACKLUNGE = 24,
    #[def("ACTION_COMBAT_ATTACK_MAIN")]
    COMBATATTACKMAIN = 25,
    #[def("ACTION_COMBAT_ATTACK_KNOCKDOWN")]
    COMBATATTACKKNOCKDOWN = 26,
    #[def("ACTION_COMBAT_ATTACK_SIDE")]
    COMBATATTACKSIDE = 27,
    #[def("ACTION_COMBAT_ATTACK_SHORT_RANGE")]
    COMBATATTACKSHORTRANGE = 28,
    #[def("ACTION_BANDIT_KING_HIT_RESPONSE")]
    BANDITKINGHITRESPONSE = 29,
    #[def("ACTION_BANDIT_KING_STUCK_HIT_RESPONSE")]
    BANDITKINGSTUCKHITRESPONSE = 30,
    #[def("ACTION_HOBBE_SPELLCASTER_AIM")]
    HOBBESPELLCASTERAIM = 31,
    #[def("ACTION_HOBBE_SPELLCASTER_FIRE")]
    HOBBESPELLCASTERFIRE = 32,
    #[def("ACTION_HOBBE_LUNGE")]
    HOBBELUNGE = 33,
    #[def("ACTION_TENTACLE_HIT_RESPONSE")]
    TENTACLEHITRESPONSE = 34,
    #[def("ACTION_SCREAMER_DIE")]
    SCREAMERDIE = 35,
    #[def("ACTION_SCREAMER_DRAIN_ATTACK")]
    SCREAMERDRAINATTACK = 36,
    #[def("ACTION_SCREAMER_DRAIN_OUT_OF")]
    SCREAMERDRAINOUTOF = 37,
    #[def("ACTION_SCREAMER_ADVANCE")]
    SCREAMERADVANCE = 38,
    #[def("ACTION_SCREAMER_BACK_OFF")]
    SCREAMERBACKOFF = 39,
    #[def("ACTION_SCREAMER_IDLE")]
    SCREAMERIDLE = 40,
    #[def("ACTION_COMBAT_BODGE_SIDE_ATTACK")]
    COMBATBODGESIDEATTACK = 41,
    #[def("ACTION_COMBAT_TURN_STRIKE_LEFT")]
    COMBATTURNSTRIKELEFT = 42,
    #[def("ACTION_COMBAT_TURN_STRIKE_RIGHT")]
    COMBATTURNSTRIKERIGHT = 43,
    #[def("ACTION_COMBAT_CHARGE_STRIKE")]
    COMBATCHARGESTRIKE = 44,
    #[def("ACTION_NYMPH_GET_HIT")]
    NYMPHGETHIT = 45,
    #[def("ACTION_NYMPH_GET_HIT_DIE")]
    NYMPHGETHITDIE = 46,
    #[def("ACTION_BALVERINE_LUNGE_ATTACK")]
    BALVERINELUNGEATTACK = 47,
    #[def("ACTION_BALVERINE_BREAK_OFF_FROM_COMBAT")]
    BALVERINEBREAKOFFFROMCOMBAT = 48,
    #[def("ACTION_BALVERINE_BREAK_OFF_FROM_COMBAT_LONG")]
    BALVERINEBREAKOFFFROMCOMBATLONG = 49,
    #[def("ACTION_SUMMONER_FLAME_SLICE")]
    SUMMONERFLAMESLICE = 50,
    #[def("ACTION_SUMMONER_UNSHEATHE_STRIKE")]
    SUMMONERUNSHEATHESTRIKE = 51,
    #[def("ACTION_SUMMONER_STRIKE")]
    SUMMONERSTRIKE = 52,
    #[def("ACTION_BATTLE_CHARGE")]
    BATTLECHARGE = 53,
    #[def("ACTION_SUMMONER_DIE")]
    SUMMONERDIE = 54,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ClockHandType {
    #[def("CLOCKHAND_SECOND")]
    SECOND = 0,
    #[def("CLOCKHAND_MINUTE")]
    MINUTE = 1,
    #[def("CLOCKHAND_HOUR")]
    HOUR = 2,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ClothingSuitPart {
    #[def("CLOTHING_SUIT_NULL")]
    NULL = 0,
    #[def("CLOTHING_SUIT_HEAD")]
    HEAD = 1,
    #[def("CLOTHING_SUIT_BODY")]
    BODY = 2,
    #[def("CLOTHING_SUIT_HANDS")]
    HANDS = 3,
    #[def("CLOTHING_SUIT_LEGS")]
    LEGS = 4,
    #[def("CLOTHING_SUIT_FEET")]
    FEET = 5,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CombatCreatureType {
    #[def("CREATURE_TYPE_HERO")]
    HERO = 0,
    #[def("CREATURE_TYPE_RIVAL_HERO")]
    RIVALHERO = 1,
    #[def("CREATURE_TYPE_FODDER_CREATURE")]
    FODDERCREATURE = 2,
    #[def("CREATURE_TYPE_COMBAT_HUMANOID")]
    COMBATHUMANOID = 3,
    #[def("CREATURE_TYPE_COMBAT_ANIMAL")]
    COMBATANIMAL = 4,
    #[def("CREATURE_TYPE_VILLAGER_MALE")]
    VILLAGERMALE = 5,
    #[def("CREATURE_TYPE_VILLAGER_FEMALE")]
    VILLAGERFEMALE = 6,
    #[def("CREATURE_TYPE_VILLAGER_CHILD")]
    VILLAGERCHILD = 7,
    #[def("CREATURE_TYPE_GUARD")]
    GUARD = 8,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CombatSequenceInterruptionType {
    #[def("COMBAT_SEQUENCE_INTERRUPT_NULL")]
    INTERRUPTNULL = 0,
    #[def("COMBAT_SEQUENCE_INTERRUPTABLE")]
    INTERRUPTABLE = 1,
    #[def("COMBAT_SEQUENCE_INTERRUPTABLE_DUE_TO_ZONE_CHANGE")]
    INTERRUPTABLEDUETOZONECHANGE = 2,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CombatSequenceIsValidType {
    #[def("COMBAT_SEQUENCE_IS_VALID_NULL")]
    VALIDNULL = 0,
    #[def("COMBAT_SEQUENCE_IS_VALID_TARGET_BLOCKING")]
    VALIDTARGETBLOCKING = 1,
    #[def("COMBAT_SEQUENCE_IS_VALID_LOAD_RANGED_WEAPON")]
    VALIDLOADRANGEDWEAPON = 2,
    #[def("COMBAT_SEQUENCE_IS_VALID_FIRE_AT_TARGET")]
    VALIDFIREATTARGET = 3,
    #[def("COMBAT_SEQUENCE_IS_VALID_IS_TARGET_HEALTH_OVER_75")]
    VALIDISTARGETHEALTHOVER75 = 4,
    #[def("COMBAT_SEQUENCE_IS_VALID_IS_MY_HEALTH_BELOW_30")]
    VALIDISMYHEALTHBELOW30 = 5,
    #[def("COMBAT_SEQUENCE_IS_VALID_IS_BALVERINE_ABLE_TO_LUNGE")]
    VALIDISBALVERINEABLETOLUNGE = 6,
    #[def("COMBAT_SEQUENCE_IS_VALID_ABLE_TO_SUMMON")]
    VALIDABLETOSUMMON = 7,
    #[def("COMBAT_SEQUENCE_IS_TARGET_IN_LINE_OF_SIGHT")]
    TARGETINLINEOFSIGHT = 8,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CombatSequenceOnStartModuleType {
    #[def("COMBAT_SEQUENCE_ON_START_NULL")]
    NULL = 0,
    #[def("COMBAT_SEQUENCE_ON_START_CONTINUE_AIMING")]
    CONTINUEAIMING = 1,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CombatSequenceOnStopModuleType {
    #[def("COMBAT_SEQUENCE_ON_STOP_NULL")]
    NULL = 0,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CombatSequenceType {
    #[def("COMBAT_SEQUENCE_NULL")]
    NULL = 0,
    #[def("COMBAT_SEQUENCE_MELEE")]
    MELEE = 1,
    #[def("COMBAT_SEQUENCE_BREAK_INTO_MELEE")]
    BREAKINTOMELEE = 2,
    #[def("COMBAT_SEQUENCE_BOAST")]
    BOAST = 3,
    #[def("COMBAT_SEQUENCE_LEADER")]
    LEADER = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CombatStrikeRecoilStyle {
    #[def("RECOIL_BREAK_HANDEDNESS")]
    RECOILBREAKHANDEDNESS = 0,
    #[def("RECOIL_MAINTATIN_HANDEDNESS")]
    RECOILMAINTATINHANDEDNESS = 1,
    #[def("RECOIL_NONE")]
    RECOILNONE = 2,
    #[def("MAX_NUMBER_OF_RECOIL_TYPES")]
    MAXNUMBEROFRECOILTYPES = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CompositeBlendType {
    #[def("COMPOSITE_BLEND_NULL")]
    NULL = 0,
    #[def("COMPOSITE_BLEND_ADDITIVE")]
    ADDITIVE = 1,
    #[def("COMPOSITE_BLEND_ALPHA")]
    ALPHA = 2,
    #[def("COMPOSITE_BLEND_SOLID")]
    SOLID = 3,
    #[def("COMPOSITE_BLEND_MULTIPLY")]
    MULTIPLY = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ContextSensitiveType {
    #[def("CONTEXT_SENSITIVE_NULL")]
    NULL = 0,
    #[def("CONTEXT_SENSITIVE_GUILD_SEAL")]
    GUILDSEAL = 1,
    #[def("CONTEXT_SENSITIVE_LAMP")]
    LAMP = 2,
    #[def("CONTEXT_SENSITIVE_HEALTH")]
    HEALTH = 3,
    #[def("CONTEXT_SENSITIVE_MANA")]
    MANA = 4,
    #[def("CONTEXT_SENSITIVE_EXPRESSION")]
    EXPRESSION = 5,
    #[def("CONTEXT_SENSITIVE_OPINION_EXPRESSION")]
    OPINIONEXPRESSION = 6,
    #[def("CONTEXT_SENSITIVE_GIFT")]
    GIFT = 7,
    #[def("CONTEXT_SENSITIVE_MARKER")]
    MARKER = 8,
    #[def("CONTEXT_SENSITIVE_TROPHY")]
    TROPHY = 9,
    #[def("CONTEXT_SENSITIVE_SCRIPT")]
    SCRIPT = 10,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ControlledMovementType {
    #[def("CONTROLLED_MOVEMENT_NULL")]
    NULL = 0,
    #[def("CONTROLLED_MOVEMENT_WALKING")]
    WALKING = 1,
    #[def("CONTROLLED_MOVEMENT_FLYING")]
    FLYING = 2,
    #[def("CONTROLLED_MOVEMENT_FIRST_PERSON")]
    FIRSTPERSON = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CreatureAbility {
    #[def("CREATURE_ABILITY_TYPE_ATTACK")]
    CREATUREABILITYTYPEATTACK = 0,
    #[def("CREATURE_ABILITY_TYPE_FLOURISH")]
    CREATUREABILITYTYPEFLOURISH = 1,
    #[def("CREATURE_ABILITY_TYPE_FLOURISH_360_CW")]
    CREATUREABILITYTYPEFLOURISH360CW = 2,
    #[def("CREATURE_ABILITY_TYPE_FLOURISH_360_ACW")]
    CREATUREABILITYTYPEFLOURISH360ACW = 3,
    #[def("CREATURE_ABILITY_TYPE_FLOURISH_UPTHRUST_LEFT")]
    CREATUREABILITYTYPEFLOURISHUPTHRUSTLEFT = 4,
    #[def("CREATURE_ABILITY_TYPE_FLOURISH_UPTHRUST_RIGHT")]
    CREATUREABILITYTYPEFLOURISHUPTHRUSTRIGHT = 5,
    #[def("CREATURE_ABILITY_TYPE_FLOURISH_MAXIMUM_DAMAGE")]
    CREATUREABILITYTYPEFLOURISHMAXIMUMDAMAGE = 6,
    #[def("CREATURE_ABILITY_TYPE_BREAK_BLOCK")]
    CREATUREABILITYTYPEBREAKBLOCK = 7,
    #[def("MAX_NUMBER_OF_CREATURE_ABILITIES")]
    MAXNUMBEROFCREATUREABILITIES = 8,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CreatureGeneratorGenerateType {
    #[def("GENERATE_NORMAL")]
    NORMAL = 0,
    #[def("GENERATE_AMBUSH_DROP_IN")]
    AMBUSHDROPIN = 1,
    #[def("GENERATE_AMBUSH_JUMP_OUT")]
    AMBUSHJUMPOUT = 2,
    #[def("GENERATE_GENERATOR_ANIMATION")]
    GENERATORANIMATION = 3,
    #[def("GENERATE_UNDEAD")]
    UNDEAD = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default, DefFlags)]
#[flags(
    NULL = 0 => "CREATURE_INTERACTION_NULL",
    CONVERSATION = 1 => "CREATURE_INTERACTION_CONVERSATION",
    TAG = 2 => "CREATURE_INTERACTION_TAG",
    MULTI_TAG = 4 => "CREATURE_INTERACTION_MULTI_TAG",
    PURCHASING = 8 => "CREATURE_INTERACTION_PURCHASING",
)]
pub struct CreatureInteractionType(pub i32);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CreatureType {
    #[def("NOT_HUMAN")]
    NOTHUMAN = 0,
    #[def("HUMAN_CHILD")]
    HUMANCHILD = 1,
    #[def("HUMAN_ADULT")]
    HUMANADULT = 2,
    #[def("HUMAN_ELDERLY")]
    HUMANELDERLY = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum CrimeSeverity {
    #[def("CRIME_SEVERITY_NONE")]
    NONE = 0,
    #[def("CRIME_SEVERITY_MINOR")]
    MINOR = 1,
    #[def("CRIME_SEVERITY_MODERATE")]
    MODERATE = 2,
    #[def("CRIME_SEVERITY_SERIOUS")]
    SERIOUS = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum DamageAttribute {
    #[def("DAMAGE_NULL")]
    NULL = -1,
    #[def("DAMAGE_MELEE")]
    MELEE = 0,
    #[def("DAMAGE_MELEE_UNARMED")]
    MELEEUNARMED = 1,
    #[def("DAMAGE_LIGHTNING")]
    LIGHTNING = 2,
    #[def("DAMAGE_FIRE")]
    FIRE = 3,
    #[def("DAMAGE_PROJECTILE")]
    PROJECTILE = 4,
    #[def("DAMAGE_EXPLOSION")]
    EXPLOSION = 5,
    #[def("DAMAGE_DRAIN")]
    DRAIN = 6,
    #[def("DAMAGE_DRAIN_HEAL")]
    DRAINHEAL = 7,
    #[def("DAMAGE_GENERIC_WILL")]
    GENERICWILL = 8,
    #[def("DAMAGE_DIVINE_WRATH")]
    DIVINEWRATH = 9,
    #[def("DAMAGE_UNHOLY_POWER")]
    UNHOLYPOWER = 10,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum DoorTriggerType {
    #[def("DOOR_TRIGGER_ON_PERSON")]
    ONPERSON = 0,
    #[def("DOOR_TRIGGER_MANUAL")]
    MANUAL = 1,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ExpressionInventoryType {
    #[def("EXPRESSION_INVENTORY_SOCIAL")]
    EXPRESSIONINVENTORYSOCIAL = 0,
    #[def("EXPRESSION_INVENTORY_RENOWN")]
    EXPRESSIONINVENTORYRENOWN = 1,
    #[def("EXPRESSION_INVENTORY_ALIGNMENT")]
    EXPRESSIONINVENTORYALIGNMENT = 2,
    #[def("EXPRESSION_INVENTORY_STEALTH")]
    EXPRESSIONINVENTORYSTEALTH = 3,
    #[def("NUM_INVENTORY_EXPRESSIONS")]
    NUMINVENTORYEXPRESSIONS = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum FeatAttackType {
    #[def("FAT_ATTACK_ANY")]
    ANY = 0,
    #[def("FAT_ATTACK_SWORD")]
    SWORD = 1,
    #[def("FAT_ATTACK_BOW")]
    BOW = 2,
    #[def("FAT_ATTACK_HANDS")]
    HANDS = 3,
    #[def("FAT_ATTACK_WILL")]
    WILL = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum GameEventType {
    #[def("GAME_EVENT_NULL")]
    GAMEEVENTNULL = 0,
    #[def("GAME_EVENT_UPDATE_FRAME")]
    GAMEEVENTUPDATEFRAME = 1,
    #[def("GAME_EVENT_QUIT")]
    GAMEEVENTQUIT = 2,
    #[def("GAME_EVENT_SET_EXCLUSIVE_MODE")]
    GAMEEVENTSETEXCLUSIVEMODE = 3,
    #[def("GAME_EVENT_SET_DISPLAY_MODE")]
    GAMEEVENTSETDISPLAYMODE = 4,
    #[def("GAME_EVENT_SET_EDITOR_MODE")]
    GAMEEVENTSETEDITORMODE = 5,
    #[def("GAME_EVENT_FIRST_PERSON_VIEW_START")]
    GAMEEVENTFIRSTPERSONVIEWSTART = 6,
    #[def("GAME_EVENT_CREATURE_CHEAT")]
    GAMEEVENTCREATURECHEAT = 7,
    #[def("GAME_EVENT_PLAYER_RESPAWN")]
    GAMEEVENTPLAYERRESPAWN = 8,
    #[def("GAME_EVENT_SET_PAUSE_MODE")]
    GAMEEVENTSETPAUSEMODE = 9,
    #[def("GAME_EVENT_SET_SLOW_MOTION")]
    GAMEEVENTSETSLOWMOTION = 10,
    #[def("GAME_EVENT_SET_FREE_CAMERA_MODE")]
    GAMEEVENTSETFREECAMERAMODE = 11,
    #[def("GAME_EVENT_USE_FREE_CAMERA")]
    GAMEEVENTUSEFREECAMERA = 12,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_BLOCK")]
    GAMEEVENTCONTROLLEDCREATUREBLOCK = 13,
    #[def("GAME_EVENT_TEMP_WORLD_EVENT")]
    GAMEEVENTTEMPWORLDEVENT = 14,
    #[def("GAME_EVENT_APPLY_SCRIPTED_MAP_BRUSHES")]
    GAMEEVENTAPPLYSCRIPTEDMAPBRUSHES = 15,
    #[def("GAME_EVENT_CREATURE_MOVEMENT")]
    GAMEEVENTCREATUREMOVEMENT = 16,
    #[def("GAME_EVENT_CREATURE_USE_OBJECT")]
    GAMEEVENTCREATUREUSEOBJECT = 17,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_TALK")]
    GAMEEVENTCONTROLLEDCREATURETALK = 18,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_LEARN_EXPRESSION")]
    GAMEEVENTCONTROLLEDCREATURELEARNEXPRESSION = 19,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_DROP_OBJECT")]
    GAMEEVENTCONTROLLEDCREATUREDROPOBJECT = 20,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_ZTARGET")]
    GAMEEVENTCONTROLLEDCREATUREZTARGET = 21,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_USE_ABILITY")]
    GAMEEVENTCONTROLLEDCREATUREUSEABILITY = 22,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_START_SNEAK")]
    GAMEEVENTCONTROLLEDCREATURESTARTSNEAK = 23,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_STOP_SNEAK")]
    GAMEEVENTCONTROLLEDCREATURESTOPSNEAK = 24,
    #[def("GAME_EVENT_CLICK_PAST_TEXT")]
    GAMEEVENTCLICKPASTTEXT = 25,
    #[def("GAME_EVENT_OPEN_HERO_INFO_SCREEN")]
    GAMEEVENTOPENHEROINFOSCREEN = 26,
    #[def("GAME_EVENT_CLOSE_HERO_INFO_SCREEN")]
    GAMEEVENTCLOSEHEROINFOSCREEN = 27,
    #[def("GAME_EVENT_CLOSE_IN_GAME_MENU")]
    GAMEEVENTCLOSEINGAMEMENU = 28,
    #[def("GAME_EVENT_QUESTION_ANSWERED")]
    GAMEEVENTQUESTIONANSWERED = 29,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_START_THROW_OBJECT")]
    GAMEEVENTCONTROLLEDCREATURESTARTTHROWOBJECT = 30,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_END_THROW_OBJECT")]
    GAMEEVENTCONTROLLEDCREATUREENDTHROWOBJECT = 31,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_SHEATHE_WEAPON")]
    GAMEEVENTCONTROLLEDCREATURESHEATHEWEAPON = 32,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_UNSHEATHE_MELEE_WEAPON")]
    GAMEEVENTCONTROLLEDCREATUREUNSHEATHEMELEEWEAPON = 33,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_UNSHEATHE_RANGED_WEAPON")]
    GAMEEVENTCONTROLLEDCREATUREUNSHEATHERANGEDWEAPON = 34,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_PLAYER_INTERACTION")]
    GAMEEVENTCONTROLLEDCREATUREPLAYERINTERACTION = 35,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_GIVE_ITEM_TO_TARGET")]
    GAMEEVENTCONTROLLEDCREATUREGIVEITEMTOTARGET = 36,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_UNFREEZE_CONTROLS")]
    GAMEEVENTCONTROLLEDCREATUREUNFREEZECONTROLS = 37,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_ROLL")]
    GAMEEVENTCONTROLLEDCREATUREROLL = 38,
    #[def("GAME_EVENT_USE_QUICK_ACCESS_ITEM")]
    GAMEEVENTUSEQUICKACCESSITEM = 39,
    #[def("GAME_EVENT_USE_QUICK_ACCESS_ITEM_IN_CUTSCENE")]
    GAMEEVENTUSEQUICKACCESSITEMINCUTSCENE = 40,
    #[def("GAME_EVENT_CHARGE_QUICK_ACCESS_ITEM")]
    GAMEEVENTCHARGEQUICKACCESSITEM = 41,
    #[def("GAME_EVENT_PUT_AWAY")]
    GAMEEVENTPUTAWAY = 42,
    #[def("GAME_EVENT_CREATURE_STRAFE")]
    GAMEEVENTCREATURESTRAFE = 43,
    #[def("GAME_EVENT_MOVE_HERO_TO_REGION")]
    GAMEEVENTMOVEHEROTOREGION = 44,
    #[def("GAME_EVENT_FIRST_PERSON_TARGETING")]
    GAMEEVENTFIRSTPERSONTARGETING = 45,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_DEFAULT")]
    GAMEEVENTCONTROLLEDCREATUREDEFAULT = 46,
    #[def("GAME_EVENT_EXPRESSION_FOLLOW")]
    GAMEEVENTEXPRESSIONFOLLOW = 47,
    #[def("GAME_EVENT_EXPRESSION_WAIT")]
    GAMEEVENTEXPRESSIONWAIT = 48,
    #[def("GAME_EVENT_USE_PROJECTILE_WEAPON")]
    GAMEEVENTUSEPROJECTILEWEAPON = 49,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_JUMP")]
    GAMEEVENTCONTROLLEDCREATUREJUMP = 50,
    #[def("GAME_EVENT_SPIRIT_MOVEMENT")]
    GAMEEVENTSPIRITMOVEMENT = 51,
    #[def("GAME_EVENT_OPEN_HERO_CENTRE_DOOR")]
    GAMEEVENTOPENHEROCENTREDOOR = 52,
    #[def("GAME_EVENT_CLOSE_HERO_CENTRE_DOOR")]
    GAMEEVENTCLOSEHEROCENTREDOOR = 53,
    #[def("GAME_EVENT_SPIRIT_ATTACK")]
    GAMEEVENTSPIRITATTACK = 54,
    #[def("GAME_EVENT_CREATURE_SPRINT")]
    GAMEEVENTCREATURESPRINT = 55,
    #[def("GAME_EVENT_SPIRIT_RETURN_TO_HERO")]
    GAMEEVENTSPIRITRETURNTOHERO = 56,
    #[def("GAME_EVENT_SKIP_CUT_SCENE")]
    GAMEEVENTSKIPCUTSCENE = 57,
    #[def("GAME_EVENT_USE_PROJECTILE_WEAPON_THIRD_PERSON")]
    GAMEEVENTUSEPROJECTILEWEAPONTHIRDPERSON = 58,
    #[def("GAME_EVENT_CHARGE_UP_WILL_SPELL")]
    GAMEEVENTCHARGEUPWILLSPELL = 59,
    #[def("GAME_EVENT_LOAD_GAME_FROM_IN_GAME_MENU")]
    GAMEEVENTLOADGAMEFROMINGAMEMENU = 60,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_BLOCK_END")]
    GAMEEVENTCONTROLLEDCREATUREBLOCKEND = 61,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_DEACTIVATE_ZTARGET")]
    GAMEEVENTCONTROLLEDCREATUREDEACTIVATEZTARGET = 62,
    #[def("GAME_EVENT_REMOVE_CURRENT_MODE")]
    GAMEEVENTREMOVECURRENTMODE = 63,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_LIGHTNING")]
    GAMEEVENTCONTROLLEDCREATURELIGHTNING = 64,
    #[def("GAME_EVENT_CONTROLLED_CREATURE_DEACTIVATE_LIGHTNING")]
    GAMEEVENTCONTROLLEDCREATUREDEACTIVATELIGHTNING = 65,
    #[def("GAME_EVENT_CLOSE_LIVE_GUI")]
    GAMEEVENTCLOSELIVEGUI = 66,
    #[def("GAME_EVENT_CLOSE_PHOTO_CAPTURE")]
    GAMEEVENTCLOSEPHOTOCAPTURE = 67,
    #[def("GAME_EVENT_TAKE_THE_BLOODY_SCREENSHOT")]
    GAMEEVENTTAKETHEBLOODYSCREENSHOT = 68,
    #[def("GAME_EVENT_TOGGLE_CONSOLE")]
    GAMEEVENTTOGGLECONSOLE = 69,
    #[def("GAME_EVENT_OPEN_PC_SKILLS_MENU")]
    GAMEEVENTOPENPCSKILLSMENU = 70,
    #[def("GAME_EVENT_OPEN_PC_MSN_CHAT_MENU")]
    GAMEEVENTOPENPCMSNCHATMENU = 71,
    #[def("GAME_EVENT_OPEN_PC_INVENTORY_MENU")]
    GAMEEVENTOPENPCINVENTORYMENU = 72,
    #[def("GAME_EVENT_OPEN_PC_OPTIONS_MENU")]
    GAMEEVENTOPENPCOPTIONSMENU = 73,
    #[def("GAME_EVENT_OPEN_PC_PHOTO_JOURNAL_MENU")]
    GAMEEVENTOPENPCPHOTOJOURNALMENU = 74,
    #[def("GAME_EVENT_OPEN_PC_MAP_MENU")]
    GAMEEVENTOPENPCMAPMENU = 75,
    #[def("GAME_EVENT_OPEN_PC_BUY_TRADING_MENU")]
    GAMEEVENTOPENPCBUYTRADINGMENU = 76,
    #[def("GAME_EVENT_OPEN_PC_SELL_TRADING_MENU")]
    GAMEEVENTOPENPCSELLTRADINGMENU = 77,
    #[def("GAME_EVENT_OPEN_PC_WANTED_TRADING_MENU")]
    GAMEEVENTOPENPCWANTEDTRADINGMENU = 78,
    #[def("GAME_EVENT_OPEN_PC_STATUS_MENU")]
    GAMEEVENTOPENPCSTATUSMENU = 79,
    #[def("GAME_EVENT_OPEN_PC_SCOREBOARD")]
    GAMEEVENTOPENPCSCOREBOARD = 80,
    #[def("GAME_EVENT_DELETE_MENU_COMPONENTS")]
    GAMEEVENTDELETEMENUCOMPONENTS = 81,
    #[def("GAME_EVENT_TOGGLE_PASSIVE_AGGRESSIVE_MODE")]
    GAMEEVENTTOGGLEPASSIVEAGGRESSIVEMODE = 82,
    #[def("GAME_EVENT_DELETE_TRADE_ACTIVE_MENU")]
    GAMEEVENTDELETETRADEACTIVEMENU = 83,
    #[def("GAME_EVENT_TOGGLE_KILL_EVERYTHING_MODE")]
    GAMEEVENTTOGGLEKILLEVERYTHINGMODE = 84,
    #[def("GAME_EVENT_TOGGLE_VIEW_HERO_MODE")]
    GAMEEVENTTOGGLEVIEWHEROMODE = 85,
    #[def("GAME_EVENT_CENTRE_CAMERA")]
    GAMEEVENTCENTRECAMERA = 86,
    #[def("NO_GAME_EVENTS")]
    NOGAMEEVENTS = 87,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum GiftType {
    #[def("GIFT_TYPE_FRIENDLY")]
    FRIENDLY = 0,
    #[def("GIFT_TYPE_ROMANTIC")]
    ROMANTIC = 1,
    #[def("GIFT_TYPE_OFFENSIVE")]
    OFFENSIVE = 2,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum HeroAbility {
    #[def("HERO_ABILITY_NULL")]
    HEROABILITYNULL = 0,
    #[def("HERO_ABILITY_FORCE_PUSH")]
    HEROABILITYFORCEPUSH = 1,
    #[def("HERO_ABILITY_TIME_SPELL")]
    HEROABILITYTIMESPELL = 2,
    #[def("HERO_ABILITY_ENFLAME_SPELL")]
    HEROABILITYENFLAMESPELL = 3,
    #[def("HERO_ABILITY_PHYSICAL_SHIELD_SPELL")]
    HEROABILITYPHYSICALSHIELDSPELL = 4,
    #[def("HERO_ABILITY_TURNCOAT_SPELL")]
    HEROABILITYTURNCOATSPELL = 5,
    #[def("HERO_ABILITY_DRAIN_LIFE_SPELL")]
    HEROABILITYDRAINLIFESPELL = 6,
    #[def("HERO_ABILITY_RAISE_DEAD_SPELL")]
    HEROABILITYRAISEDEADSPELL = 7,
    #[def("HERO_ABILITY_BERSERK")]
    HEROABILITYBERSERK = 8,
    #[def("HERO_ABILITY_DOUBLE_STRIKE")]
    HEROABILITYDOUBLESTRIKE = 9,
    #[def("HERO_ABILITY_SUMMON_SPELL")]
    HEROABILITYSUMMONSPELL = 10,
    #[def("HERO_ABILITY_LIGHTNING_SPELL")]
    HEROABILITYLIGHTNINGSPELL = 11,
    #[def("HERO_ABILITY_BATTLE_CHARGE")]
    HEROABILITYBATTLECHARGE = 12,
    #[def("HERO_ABILITY_ASSASSIN_RUSH")]
    HEROABILITYASSASSINRUSH = 13,
    #[def("HERO_ABILITY_HEAL_LIFE_SPELL")]
    HEROABILITYHEALLIFESPELL = 14,
    #[def("HERO_ABILITY_GHOST_SWORD_SPELL")]
    HEROABILITYGHOSTSWORDSPELL = 15,
    #[def("HERO_ABILITY_FIREBALL_SPELL")]
    HEROABILITYFIREBALLSPELL = 16,
    #[def("HERO_ABILITY_MULTI_ARROW")]
    HEROABILITYMULTIARROW = 17,
    #[def("HERO_ABILITY_DIVINE_WRATH_SPELL")]
    HEROABILITYDIVINEWRATHSPELL = 18,
    #[def("HERO_ABILITY_UNHOLY_POWER_SPELL")]
    HEROABILITYUNHOLYPOWERSPELL = 19,
    #[def("MAX_NUMBER_OF_HERO_ABILITIES")]
    MAXNUMBEROFHEROABILITIES = 20,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum HeroAttachableAppearanceModifierType {
    #[def("APPEARANCE_HAIR")]
    APPEARANCEHAIR = 0,
    #[def("APPEARANCE_HORN")]
    APPEARANCEHORN = 1,
    #[def("APPEARANCE_CLOTHING")]
    APPEARANCECLOTHING = 2,
    #[def("NO_OF_APPEARANCE_MODIFIER_TYPES")]
    NOOFAPPEARANCEMODIFIERTYPES = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum HeroExperienceStatCategory {
    #[def("HERO_STAT_STRENGTH")]
    HEROSTATSTRENGTH = 0,
    #[def("HERO_STAT_SKILL")]
    HEROSTATSKILL = 1,
    #[def("HERO_STAT_WILL")]
    HEROSTATWILL = 2,
    #[def("NUMBER_OF_HERO_STAT_CATEGORIES")]
    NUMBEROFHEROSTATCATEGORIES = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum HeroTitle {
    #[def("TITLE_NONE")]
    NONE = 0,
    #[def("TITLE_REAPER")]
    REAPER = 1,
    #[def("TITLE_SHADOWHUNTER")]
    SHADOWHUNTER = 2,
    #[def("TITLE_MALEFICUS")]
    MALEFICUS = 3,
    #[def("TITLE_DEATHBRINGER")]
    DEATHBRINGER = 4,
    #[def("TITLE_ASSASSIN")]
    ASSASSIN = 5,
    #[def("TITLE_NECROMANCER")]
    NECROMANCER = 6,
    #[def("TITLE_AVATAR")]
    AVATAR = 7,
    #[def("TITLE_PILGRIM")]
    PILGRIM = 8,
    #[def("TITLE_LIBERATOR")]
    LIBERATOR = 9,
    #[def("TITLE_PALADIN")]
    PALADIN = 10,
    #[def("TITLE_DRUID")]
    DRUID = 11,
    #[def("TITLE_RANGER")]
    RANGER = 12,
    #[def("TITLE_RUNEMASTER")]
    RUNEMASTER = 13,
    #[def("TITLE_HOOD")]
    HOOD = 14,
    #[def("TITLE_GLADIATOR")]
    GLADIATOR = 15,
    #[def("TITLE_SABRE")]
    SABRE = 16,
    #[def("TITLE_ARROWDODGER")]
    ARROWDODGER = 17,
    #[def("TITLE_PIEMASTER")]
    PIEMASTER = 18,
    #[def("TITLE_CHICKEN_CHASER")]
    CHICKENCHASER = 19,
    #[def("TITLE_ARSEFACE")]
    ARSEFACE = 20,
    #[def("TITLE_JACK")]
    JACK = 21,
    #[def("TITLE_MAZE")]
    MAZE = 22,
    #[def("TITLE_SCARLET_ROBE")]
    SCARLETROBE = 23,
    #[def("TITLE_SCYTHE")]
    SCYTHE = 24,
    #[def("TITLE_THUNDER")]
    THUNDER = 25,
    #[def("TITLE_WHISPER")]
    WHISPER = 26,
    #[def("TITLE_TWINBLADE")]
    TWINBLADE = 27,
    #[def("TITLE_BRIAR_ROSE")]
    BRIARROSE = 28,
    #[def("TITLE_LADY_GREY")]
    LADYGREY = 29,
    #[def("TITLE_GUILDMASTER")]
    GUILDMASTER = 30,
    #[def("TITLE_SCORPION_SLAYER")]
    SCORPIONSLAYER = 31,
    #[def("TITLE_DEATH_BRINGER")]
    DEATHBRINGER_ = 32,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum HeroTrainingStatus {
    #[def("TRAINING_STATUS_GRADUATED")]
    GRADUATED = 0,
    #[def("TRAINING_STATUS_APPRENTICE")]
    APPRENTICE = 1,
    #[def("TRAINING_STATUS_BOY")]
    BOY = 2,
}

/// Idle behaviours a creature can perform (C++ `enum EIdleStateGroup`).
/// Declared as DefFlags with sequential values in the original schema;
/// converted to DefEnum — values are sequential selectors, never bitmasks.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[allow(non_camel_case_types)]
#[repr(i32)]
pub enum IdleStateGroup {
    #[def("ISG_NULL")]
    NULL = 0,
    #[def("ISG_NOT_BOTHERED")]
    NOT_BOTHERED = 1,
    #[def("ISG_CLEAN_HOME")]
    CLEAN_HOME = 2,
    #[def("ISG_SIT_HOME")]
    SIT_HOME = 3,
    #[def("ISG_SIT_OUTSIDE")]
    SIT_OUTSIDE = 4,
    #[def("ISG_SIT_AROUND_FIRE")]
    SIT_AROUND_FIRE = 5,
    #[def("ISG_WATCH_FIRE")]
    WATCH_FIRE = 6,
    #[def("ISG_GAZE_HOME")]
    GAZE_HOME = 7,
    #[def("ISG_GAZE_OUTSIDE")]
    GAZE_OUTSIDE = 8,
    #[def("ISG_LOOK_AT_INTERESTING_THINGS")]
    LOOK_AT_INTERESTING_THINGS = 9,
    #[def("ISG_WANDER_AROUND_SHOPS")]
    WANDER_AROUND_SHOPS = 10,
    #[def("ISG_KID_WANDER_NEAR_KIDS")]
    KID_WANDER_NEAR_KIDS = 11,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum LightingChannel {
    #[def("LIGHTING_CHANNEL_MAIN")]
    MAIN = 0,
    #[def("LIGHTING_CHANNEL_INDOORS")]
    INDOORS = 1,
    #[def("LIGHTING_CHANNEL_INDOORS_2")]
    INDOORS2 = 2,
    #[def("LIGHTING_CHANNEL_INDOORS_3")]
    INDOORS3 = 3,
    #[def("LIGHTING_CHANNEL_INDOORS_4")]
    INDOORS4 = 4,
    #[def("LIGHTING_CHANNEL_EPIC_SPELL")]
    EPICSPELL = 5,
    #[def("LIGHTING_CHANNEL_COUNT")]
    COUNT = 6,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum MessageEventType {
    #[def("MESSAGE_EVENT_PLAYER_GIVE_ITEM")]
    PLAYERGIVEITEM = 0,
    #[def("MESSAGE_EVENT_CALL_CHILDREN_HOME")]
    CALLCHILDRENHOME = 1,
    #[def("MESSAGE_EVENT_CALL_SPOUSE_HOME")]
    CALLSPOUSEHOME = 2,
    #[def("MESSAGE_EVENT_PLAYER_ARSON")]
    PLAYERARSON = 3,
    #[def("MESSAGE_EVENT_ATTACK")]
    ATTACK = 4,
    #[def("MESSAGE_EVENT_MURDER")]
    MURDER = 5,
    #[def("MESSAGE_EVENT_PLAYER_ARSON_EMOTIONAL_REACTION")]
    PLAYERARSONEMOTIONALREACTION = 6,
    #[def("MESSAGE_EVENT_PLAYER_ATTACK_EMOTIONAL_REACTION")]
    PLAYERATTACKEMOTIONALREACTION = 7,
    #[def("MESSAGE_EVENT_PLAYER_MURDER_EMOTIONAL_REACTION")]
    PLAYERMURDEREMOTIONALREACTION = 8,
    #[def("MESSAGE_EVENT_DEATH")]
    DEATH = 9,
    #[def("MESSAGE_EVENT_GOING_FOR_HELP")]
    GOINGFORHELP = 10,
    #[def("MESSAGE_EVENT_YELL_FOR_HELP")]
    YELLFORHELP = 11,
    #[def("MESSAGE_EVENT_YELL_I_SEE_HIM")]
    YELLISEEHIM = 12,
    #[def("MESSAGE_EVENT_CONVERSATION_START")]
    CONVERSATIONSTART = 13,
    #[def("MESSAGE_EVENT_CONVERSATION_JOIN")]
    CONVERSATIONJOIN = 14,
    #[def("MESSAGE_EVENT_CONVERSATION_END")]
    CONVERSATIONEND = 15,
    #[def("MESSAGE_EVENT_CONVERSATION_ANSWER_YES_OR_NO")]
    CONVERSATIONANSWERYESORNO = 16,
    #[def("MESSAGE_EVENT_CONVERSATION_CLICK_PAST")]
    CONVERSATIONCLICKPAST = 17,
    #[def("MESSAGE_EVENT_GAME_INFO_CLICK_PAST")]
    GAMEINFOCLICKPAST = 18,
    #[def("MESSAGE_EVENT_BUY_ITEM")]
    BUYITEM = 19,
    #[def("MESSAGE_EVENT_PLAYER_INTERACTION")]
    PLAYERINTERACTION = 20,
    #[def("MESSAGE_EVENT_GAME_OF_TAG_START")]
    GAMEOFTAGSTART = 21,
    #[def("MESSAGE_EVENT_GAME_OF_TAG_JOIN")]
    GAMEOFTAGJOIN = 22,
    #[def("MESSAGE_EVENT_GAME_OF_TAG_SOMEONE_TAGGED")]
    GAMEOFTAGSOMEONETAGGED = 23,
    #[def("MESSAGE_EVENT_HOPSCOTCH_PLAYING")]
    HOPSCOTCHPLAYING = 24,
    #[def("MESSAGE_EVENT_FOUND_CORPSE")]
    FOUNDCORPSE = 25,
    #[def("MESSAGE_EVENT_FOUND_UNCONSCIOUS_PERSON")]
    FOUNDUNCONSCIOUSPERSON = 26,
    #[def("MESSAGE_EVENT_GUARD_SEEN_PLAYER_SWORD")]
    GUARDSEENPLAYERSWORD = 27,
    #[def("MESSAGE_EVENT_THEFT")]
    THEFT = 28,
    #[def("MESSAGE_EVENT_TRESPASS")]
    TRESPASS = 29,
    #[def("MESSAGE_EVENT_DEALING_WITH_TRESPASS")]
    DEALINGWITHTRESPASS = 30,
    #[def("MESSAGE_EVENT_DEALING_WITH_NOISE_IN_HOUSE")]
    DEALINGWITHNOISEINHOUSE = 31,
    #[def("MESSAGE_EVENT_HERO_REPUTATION_EVENT")]
    HEROREPUTATIONEVENT = 32,
    #[def("MESSAGE_EVENT_CROWD_FORMING")]
    CROWDFORMING = 33,
    #[def("MESSAGE_EVENT_CROWD_DISPERSING")]
    CROWDDISPERSING = 34,
    #[def("MESSAGE_EVENT_HERO_PODIUM")]
    HEROPODIUM = 35,
    #[def("MESSAGE_EVENT_LEVEL_LOADED")]
    LEVELLOADED = 36,
    #[def("MESSAGE_EVENT_LEVEL_UNLOADED")]
    LEVELUNLOADED = 37,
    #[def("MESSAGE_EVENT_REGION_LOADED")]
    REGIONLOADED = 38,
    #[def("MESSAGE_EVENT_REGION_UNLOADED")]
    REGIONUNLOADED = 39,
    #[def("MESSAGE_EVENT_REGION_PREUNLOAD")]
    REGIONPREUNLOAD = 40,
    #[def("MESSAGE_EVENT_BOAST_MADE")]
    BOASTMADE = 41,
    #[def("MESSAGE_EVENT_EXPRESSION_PERFORMED")]
    EXPRESSIONPERFORMED = 42,
    #[def("MESSAGE_EVENT_SCRIPTED_CAMERA_EVENT")]
    SCRIPTEDCAMERAEVENT = 43,
    #[def("MESSAGE_EVENT_REQUEST_GOSSIP")]
    REQUESTGOSSIP = 44,
    #[def("MESSAGE_EVENT_REQUEST_GOSSIP_REQUEST")]
    REQUESTGOSSIPREQUEST = 45,
    #[def("MESSAGE_EVENT_REQUEST_GAME_OF_TAG")]
    REQUESTGAMEOFTAG = 46,
    #[def("MESSAGE_EVENT_APPRENTICE_PRACTICE")]
    APPRENTICEPRACTICE = 47,
    #[def("MESSAGE_EVENT_PAYMENT_REQUEST")]
    PAYMENTREQUEST = 48,
    #[def("MESSAGE_EVENT_PAYMENT_CANCELLED")]
    PAYMENTCANCELLED = 49,
    #[def("MESSAGE_EVENT_SKIP_CUT_SCENE")]
    SKIPCUTSCENE = 50,
    #[def("MESSAGE_EVENT_QUEST_COMPLETED")]
    QUESTCOMPLETED = 51,
    #[def("MESSAGE_EVENT_QUEST_FAILED")]
    QUESTFAILED = 52,
    #[def("MESSAGE_EVENT_QUEST_COMPLETED_BEFORE_SCREEN_SHOWN")]
    QUESTCOMPLETEDBEFORESCREENSHOWN = 53,
    #[def("MESSAGE_EVENT_QUEST_FAILED_BEFORE_SCREEN_SHOWN")]
    QUESTFAILEDBEFORESCREENSHOWN = 54,
    #[def("MESSAGE_EVENT_QUEST_ACCEPTED")]
    QUESTACCEPTED = 55,
    #[def("MESSAGE_EVENT_FEAT_ACCEPTED")]
    FEATACCEPTED = 56,
    #[def("MESSAGE_EVENT_HAIR_TYPE_CHANGED")]
    HAIRTYPECHANGED = 57,
    #[def("MESSAGE_EVENT_BEARD_TYPE_CHANGED")]
    BEARDTYPECHANGED = 58,
    #[def("MESSAGE_EVENT_MOUSTACHE_TYPE_CHANGED")]
    MOUSTACHETYPECHANGED = 59,
    #[def("MESSAGE_EVENT_TELEPORTER_USED")]
    TELEPORTERUSED = 60,
    #[def("MESSAGE_EVENT_GUILD_SEAL_USED")]
    GUILDSEALUSED = 61,
    #[def("MESSAGE_EVENT_GAME_SAVED_MANUALLY")]
    GAMESAVEDMANUALLY = 62,
    #[def("MESSAGE_EVENT_FISHING_GAME_FINISHED")]
    FISHINGGAMEFINISHED = 63,
    #[def("MESSAGE_EVENT_TAVERN_GAME_FINISHED")]
    TAVERNGAMEFINISHED = 64,
    #[def("MESSAGE_EVENT_HERO_REWARDED_FROM_CONTAINER")]
    HEROREWARDEDFROMCONTAINER = 65,
    #[def("MESSAGE_EVENT_HERO_SLEPT")]
    HEROSLEPT = 66,
    #[def("MESSAGE_EVENT_HERO_FIRED_RANGED_WEAPON")]
    HEROFIREDRANGEDWEAPON = 67,
    #[def("MESSAGE_EVENT_HERO_CAST_SPELL")]
    HEROCASTSPELL = 68,
    #[def("MESSAGE_EVENT_HERO_PICKED_POCKET")]
    HEROPICKEDPOCKET = 69,
    #[def("MESSAGE_EVENT_HERO_PICKED_LOCK")]
    HEROPICKEDLOCK = 70,
    #[def("MESSAGE_EVENT_HERO_STOLEN_OBJECT")]
    HEROSTOLENOBJECT = 71,
    #[def("MESSAGE_EVENT_CHEST_OPENING_CANCELLED")]
    CHESTOPENINGCANCELLED = 72,
    #[def("MESSAGE_EVENT_LEAVING_QUEST_START_SCREEN")]
    LEAVINGQUESTSTARTSCREEN = 73,
    #[def("MESSAGE_EVENT_LEAVING_EXPERIENCE_SPEND_SCREEN")]
    LEAVINGEXPERIENCESPENDSCREEN = 74,
    #[def("MESSAGE_EVENT_ACTION_MODE_BUTTON_PRESSED")]
    ACTIONMODEBUTTONPRESSED = 75,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum MinimapThemeType {
    #[def("MINIMAP_THEME_TYPE_NONE")]
    NONE = 0,
    #[def("MINIMAP_THEME_TYPE_GRASS")]
    GRASS = 1,
    #[def("MINIMAP_THEME_TYPE_WATER")]
    WATER = 2,
    #[def("MINIMAP_THEME_TYPE_SNOW")]
    SNOW = 3,
    #[def("MINIMAP_THEME_TYPE_CLIFF")]
    CLIFF = 4,
    #[def("MINIMAP_THEME_TYPE_EARTH")]
    EARTH = 5,
    #[def("MINIMAP_THEME_TYPE_FOLIAGE")]
    FOLIAGE = 6,
    #[def("MINIMAP_THEME_TYPE_WOOD")]
    WOOD = 7,
    #[def("MINIMAP_THEME_TYPE_BUILDING")]
    BUILDING = 8,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum NavigatorType {
    #[def("NAV_INIT_GROUND")]
    GROUND = 1,
    #[def("NAV_INIT_FLYER")]
    FLYER = 2,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum NoiseType {
    #[def("NOISE_TYPE_CONTINUOUS")]
    CONTINUOUS = 0,
    #[def("NOISE_TYPE_ONCE_ONLY")]
    ONCEONLY = 1,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default, DefFlags)]
#[flags(
    NULL = 0 => "AUGMENTATION_NULL",
    EXTRA_DAMAGE = 1 => "AUGMENTATION_EXTRA_DAMAGE",
    SILVER = 2 => "AUGMENTATION_SILVER",
    FLAME = 4 => "AUGMENTATION_FLAME",
    LIGHTNING = 8 => "AUGMENTATION_LIGHTNING",
    DIAMOND = 16 => "AUGMENTATION_DIAMOND",
    HEALTH_REGENERATION = 32 => "AUGMENTATION_HEALTH_REGENERATION",
    STAMINA_REGENERATION = 64 => "AUGMENTATION_STAMINA_REGENERATION",
    EXPERIENCE_INCREASE = 128 => "AUGMENTATION_EXPERIENCE_INCREASE",
    HOBBE_KILLER = 256 => "AUGMENTATION_HOBBE_KILLER",
    BANDIT_KILLER = 512 => "AUGMENTATION_BANDIT_KILLER",
)]
pub struct ObjectAugmentationType(pub i32);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default, DefFlags)]
#[flags(
    MORALITY = 0 => "OPINION_MORALITY",
    RENOWN = 1 => "OPINION_RENOWN",
    SCARINESS = 2 => "OPINION_SCARINESS",
    AGREEABLENESS = 3 => "OPINION_AGREEABLENESS",
    ATTRACTIVENESS = 4 => "OPINION_ATTRACTIVENESS",
    LAST = 5 => "OPINION_LAST",
)]
pub struct Opinion(pub i32);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default, DefFlags)]
#[flags(
    NONE = 0 => "OPINION_ATTITUDE_TYPE_NONE",
    RESPECT = 1 => "OPINION_ATTITUDE_TYPE_RESPECT",
    AWE = 2 => "OPINION_ATTITUDE_TYPE_AWE",
    DISDAIN = 3 => "OPINION_ATTITUDE_TYPE_DISDAIN",
    FEAR = 4 => "OPINION_ATTITUDE_TYPE_FEAR",
    FRIENDLINESS = 5 => "OPINION_ATTITUDE_TYPE_FRIENDLINESS",
    WORSHIP = 6 => "OPINION_ATTITUDE_TYPE_WORSHIP",
    RIDICULOUS = 7 => "OPINION_ATTITUDE_TYPE_RIDICULOUS",
    OFFENSIVE = 8 => "OPINION_ATTITUDE_TYPE_OFFENSIVE",
    AGREEABLE = 9 => "OPINION_ATTITUDE_TYPE_AGREEABLE",
    UGLY = 10 => "OPINION_ATTITUDE_TYPE_UGLY",
    ATTRACTED = 11 => "OPINION_ATTITUDE_TYPE_ATTRACTED",
    LOVE = 12 => "OPINION_ATTITUDE_TYPE_LOVE",
    WIFE_FIRST = 13 => "OPINION_ATTITUDE_TYPE_WIFE_FIRST",
    WIFE_LOVE = 13 => "OPINION_ATTITUDE_TYPE_WIFE_LOVE",
    WIFE_LIKE = 14 => "OPINION_ATTITUDE_TYPE_WIFE_LIKE",
    WIFE_NEUTRAL = 15 => "OPINION_ATTITUDE_TYPE_WIFE_NEUTRAL",
    WIFE_DISLIKE = 16 => "OPINION_ATTITUDE_TYPE_WIFE_DISLIKE",
    WIFE_HATE = 17 => "OPINION_ATTITUDE_TYPE_WIFE_HATE",
    LAST = 18 => "OPINION_ATTITUDE_TYPE_LAST",
)]
pub struct OpinionAttitudeType(pub i32);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default, DefFlags)]
#[flags(
    NONE = 0 => "OPINION_DEED_TYPE_NONE",
    CRIME_WEAPON_OUT = 1 => "OPINION_DEED_TYPE_CRIME_WEAPON_OUT",
    CRIME_TRESPASS_THIRD = 2 => "OPINION_DEED_TYPE_CRIME_TRESPASS_THIRD",
    CRIME_VANDALISM = 3 => "OPINION_DEED_TYPE_CRIME_VANDALISM",
    CRIME_LOCKPICKING = 4 => "OPINION_DEED_TYPE_CRIME_LOCKPICKING",
    CRIME_PICK_POCKETS = 5 => "OPINION_DEED_TYPE_CRIME_PICK_POCKETS",
    CRIME_THEFT = 6 => "OPINION_DEED_TYPE_CRIME_THEFT",
    CRIME_ASSAULT = 7 => "OPINION_DEED_TYPE_CRIME_ASSAULT",
    CRIME_GBH = 8 => "OPINION_DEED_TYPE_CRIME_GBH",
    CRIME_MURDER = 9 => "OPINION_DEED_TYPE_CRIME_MURDER",
    CRIME_TRESPASS_FIRST = 10 => "OPINION_DEED_TYPE_CRIME_TRESPASS_FIRST",
    CRIME_TRESPASS_SECOND = 11 => "OPINION_DEED_TYPE_CRIME_TRESPASS_SECOND",
    EXPRESSION_HEROIC_STANCE = 12 => "OPINION_DEED_TYPE_EXPRESSION_HEROIC_STANCE",
    EXPRESSION_FLIRT = 13 => "OPINION_DEED_TYPE_EXPRESSION_FLIRT",
    EXPRESSION_APOLOGY_NO_CRIME = 14 => "OPINION_DEED_TYPE_EXPRESSION_APOLOGY_NO_CRIME",
    EXPRESSION_SNEER = 15 => "OPINION_DEED_TYPE_EXPRESSION_SNEER",
    EXPRESSION_EVIL_LAUGH = 16 => "OPINION_DEED_TYPE_EXPRESSION_EVIL_LAUGH",
    EXPRESSION_BATTLE_CRY = 17 => "OPINION_DEED_TYPE_EXPRESSION_BATTLE_CRY",
    EXPRESSION_PELVIC_THRUST = 18 => "OPINION_DEED_TYPE_EXPRESSION_PELVIC_THRUST",
    EXPRESSION_MIDDLE_FINGER = 19 => "OPINION_DEED_TYPE_EXPRESSION_MIDDLE_FINGER",
    EXPRESSION_BELCH = 20 => "OPINION_DEED_TYPE_EXPRESSION_BELCH",
    EXPRESSION_FART = 21 => "OPINION_DEED_TYPE_EXPRESSION_FART",
    EXPRESSION_VICTORY_PUMP = 22 => "OPINION_DEED_TYPE_EXPRESSION_VICTORY_PUMP",
    EXPRESSION_CLAP = 23 => "OPINION_DEED_TYPE_EXPRESSION_CLAP",
    EXPRESSION_GIGGLE = 24 => "OPINION_DEED_TYPE_EXPRESSION_GIGGLE",
    EXPRESSION_SHIT = 25 => "OPINION_DEED_TYPE_EXPRESSION_SHIT",
    EXPRESSION_THANKS = 26 => "OPINION_DEED_TYPE_EXPRESSION_THANKS",
    EXPRESSION_COCK_A_DOODLE_DO = 27 => "OPINION_DEED_TYPE_EXPRESSION_COCK_A_DOODLE_DO",
    EXPRESSION_CROTCH_GRAB = 28 => "OPINION_DEED_TYPE_EXPRESSION_CROTCH_GRAB",
    EXPRESSION_KISS_MY_ASS = 29 => "OPINION_DEED_TYPE_EXPRESSION_KISS_MY_ASS",
    EXPRESSION_FLAMENCO = 30 => "OPINION_DEED_TYPE_EXPRESSION_FLAMENCO",
    EXPRESSION_COSSACK = 31 => "OPINION_DEED_TYPE_EXPRESSION_COSSACK",
    EXPRESSION_AIR_GUITAR = 32 => "OPINION_DEED_TYPE_EXPRESSION_AIR_GUITAR",
    EXPRESSION_BALLET = 33 => "OPINION_DEED_TYPE_EXPRESSION_BALLET",
    EXPRESSION_SATURDAY_NIGHT_FEVER = 34 => "OPINION_DEED_TYPE_EXPRESSION_SATURDAY_NIGHT_FEVER",
    EXPRESSION_TAP = 35 => "OPINION_DEED_TYPE_EXPRESSION_TAP",
    EXPRESSION_Y = 36 => "OPINION_DEED_TYPE_EXPRESSION_Y",
    EXPRESSION_M = 37 => "OPINION_DEED_TYPE_EXPRESSION_M",
    EXPRESSION_C = 38 => "OPINION_DEED_TYPE_EXPRESSION_C",
    EXPRESSION_A = 39 => "OPINION_DEED_TYPE_EXPRESSION_A",
    EXPRESSION_THREATEN_SMALL = 40 => "OPINION_DEED_TYPE_EXPRESSION_THREATEN_SMALL",
    EXPRESSION_THREATEN_LARGE = 41 => "OPINION_DEED_TYPE_EXPRESSION_THREATEN_LARGE",
    SCRIPT_ACTION_ANNOYING_SMALL = 42 => "OPINION_DEED_TYPE_SCRIPT_ACTION_ANNOYING_SMALL",
    SCRIPT_ACTION_ANNOYING_LARGE = 43 => "OPINION_DEED_TYPE_SCRIPT_ACTION_ANNOYING_LARGE",
    SCRIPT_ACTION_NICE_SMALL = 44 => "OPINION_DEED_TYPE_SCRIPT_ACTION_NICE_SMALL",
    SCRIPT_ACTION_NICE_LARGE = 45 => "OPINION_DEED_TYPE_SCRIPT_ACTION_NICE_LARGE",
    FOLLOWER_ACCEPT = 46 => "OPINION_DEED_TYPE_FOLLOWER_ACCEPT",
    FOLLOWER_REFUSE = 47 => "OPINION_DEED_TYPE_FOLLOWER_REFUSE",
    FOLLOWER_DISMISSED = 48 => "OPINION_DEED_TYPE_FOLLOWER_DISMISSED",
    FOLLOWER_QUIT = 49 => "OPINION_DEED_TYPE_FOLLOWER_QUIT",
    FOLLOWER_ENEMYSPOTTED = 50 => "OPINION_DEED_TYPE_FOLLOWER_ENEMYSPOTTED",
    FOLLOWER_IDLEEXCITED = 51 => "OPINION_DEED_TYPE_FOLLOWER_IDLEEXCITED",
    FOLLOWER_IDLEBORED = 52 => "OPINION_DEED_TYPE_FOLLOWER_IDLEBORED",
    WITNESSED_ASSAULT_OR_GBH = 53 => "OPINION_DEED_TYPE_WITNESSED_ASSAULT_OR_GBH",
    TOO_FREQUENT_OTHER_DEED = 54 => "OPINION_DEED_TYPE_TOO_FREQUENT_OTHER_DEED",
    SHOW_TROPHY_EVIL = 55 => "OPINION_DEED_TYPE_SHOW_TROPHY_EVIL",
    SHOW_TROPHY_GOOD = 56 => "OPINION_DEED_TYPE_SHOW_TROPHY_GOOD",
    SHOW_TROPHY_BORED = 57 => "OPINION_DEED_TYPE_SHOW_TROPHY_BORED",
    KILL_BAD_GUY = 58 => "OPINION_DEED_TYPE_KILL_BAD_GUY",
    MURDER_SPOUSE = 59 => "OPINION_DEED_TYPE_MURDER_SPOUSE",
    RECEIVE_GIFT_ROMANTIC = 60 => "OPINION_DEED_TYPE_RECEIVE_GIFT_ROMANTIC",
    RECEIVE_GIFT_FRIENDLY = 61 => "OPINION_DEED_TYPE_RECEIVE_GIFT_FRIENDLY",
    RECEIVE_GIFT_OFFENSIVE = 62 => "OPINION_DEED_TYPE_RECEIVE_GIFT_OFFENSIVE",
    RECEIVE_WEDDING_RING = 63 => "OPINION_DEED_TYPE_RECEIVE_WEDDING_RING",
    BOAST_ANTICIPATION = 64 => "OPINION_DEED_TYPE_BOAST_ANTICIPATION",
    BOAST_ENCOURAGE_FIRST = 65 => "OPINION_DEED_TYPE_BOAST_ENCOURAGE_FIRST",
    BOAST_ENCOURAGE_MIDDLE = 66 => "OPINION_DEED_TYPE_BOAST_ENCOURAGE_MIDDLE",
    BOAST_ENCOURAGE_FINAL = 67 => "OPINION_DEED_TYPE_BOAST_ENCOURAGE_FINAL",
    BOAST_WELL_WISHES = 68 => "OPINION_DEED_TYPE_BOAST_WELL_WISHES",
    BOAST_ANNOYED_NO_BOASTING = 69 => "OPINION_DEED_TYPE_BOAST_ANNOYED_NO_BOASTING",
    COMMENT_AT_HERO = 70 => "OPINION_DEED_TYPE_COMMENT_AT_HERO",
    COMMENT_TO_SELF = 71 => "OPINION_DEED_TYPE_COMMENT_TO_SELF",
    COMMENT_ABOUT_HERO = 72 => "OPINION_DEED_TYPE_COMMENT_ABOUT_HERO",
    GENERIC_INCOMPREHENSION = 73 => "OPINION_DEED_TYPE_GENERIC_INCOMPREHENSION",
    HIGH_PRIORITY_INCOMPREHENSION = 74 => "OPINION_DEED_TYPE_HIGH_PRIORITY_INCOMPREHENSION",
    HUSBAND_RAGE = 75 => "OPINION_DEED_TYPE_HUSBAND_RAGE",
    TAVERN_GAME_WIN = 76 => "OPINION_DEED_TYPE_TAVERN_GAME_WIN",
    INDOORS_GREETING = 77 => "OPINION_DEED_TYPE_INDOORS_GREETING",
    APOLOGY_ACCEPTED = 78 => "OPINION_DEED_TYPE_APOLOGY_ACCEPTED",
    APOLOGY_REFUSED = 79 => "OPINION_DEED_TYPE_APOLOGY_REFUSED",
    WIFE_GREETED = 80 => "OPINION_DEED_TYPE_WIFE_GREETED",
    WIFE_TIME_SINCE_SEEING = 81 => "OPINION_DEED_TYPE_WIFE_TIME_SINCE_SEEING",
    WIFE_GIFT_RECEIVE_ALREADY_GOT = 82 => "OPINION_DEED_TYPE_WIFE_GIFT_RECEIVE_ALREADY_GOT",
    WIFE_JUSTMARRIED = 83 => "OPINION_DEED_TYPE_WIFE_JUSTMARRIED",
    WIFE_GIFT_WANTED = 84 => "OPINION_DEED_TYPE_WIFE_GIFT_WANTED",
    WIFE_WITNESSED_FLIRT = 85 => "OPINION_DEED_TYPE_WIFE_WITNESSED_FLIRT",
    WIFE_HOUSE_DRESSING_GOOD = 86 => "OPINION_DEED_TYPE_WIFE_HOUSE_DRESSING_GOOD",
    WIFE_HOUSE_DRESSING_BAD = 87 => "OPINION_DEED_TYPE_WIFE_HOUSE_DRESSING_BAD",
    WIFE_DIVORCE_WARNING = 88 => "OPINION_DEED_TYPE_WIFE_DIVORCE_WARNING",
    WIFE_DIVORCE_OCCURRED = 89 => "OPINION_DEED_TYPE_WIFE_DIVORCE_OCCURRED",
    WIFE_SEX_OFFER_TO_GO_TO_BED = 90 => "OPINION_DEED_TYPE_WIFE_SEX_OFFER_TO_GO_TO_BED",
    WIFE_SEX_COMMENT_AFTERWARDS = 91 => "OPINION_DEED_TYPE_WIFE_SEX_COMMENT_AFTERWARDS",
    LAST = 92 => "OPINION_DEED_TYPE_LAST",
)]
pub struct OpinionDeedType(pub i32);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default, DefFlags)]
#[flags(
    NONE = 0 => "OPINION_REACTION_TYPE_NONE",
    ATTITUDE_RESPECT = 1 => "OPINION_REACTION_TYPE_ATTITUDE_RESPECT",
    ATTITUDE_AWE = 2 => "OPINION_REACTION_TYPE_ATTITUDE_AWE",
    ATTITUDE_DISDAIN = 3 => "OPINION_REACTION_TYPE_ATTITUDE_DISDAIN",
    ATTITUDE_FEAR = 4 => "OPINION_REACTION_TYPE_ATTITUDE_FEAR",
    ATTITUDE_FRIENDLINESS = 5 => "OPINION_REACTION_TYPE_ATTITUDE_FRIENDLINESS",
    ATTITUDE_WORSHIP = 6 => "OPINION_REACTION_TYPE_ATTITUDE_WORSHIP",
    ATTITUDE_RIDICULOUS = 7 => "OPINION_REACTION_TYPE_ATTITUDE_RIDICULOUS",
    ATTITUDE_OFFENSIVE = 8 => "OPINION_REACTION_TYPE_ATTITUDE_OFFENSIVE",
    ATTITUDE_AGREEABLE = 9 => "OPINION_REACTION_TYPE_ATTITUDE_AGREEABLE",
    ATTITUDE_UGLY = 10 => "OPINION_REACTION_TYPE_ATTITUDE_UGLY",
    ATTITUDE_ATTRACTED = 11 => "OPINION_REACTION_TYPE_ATTITUDE_ATTRACTED",
    ATTITUDE_LOVE = 12 => "OPINION_REACTION_TYPE_ATTITUDE_LOVE",
    ATTITUDE_WIFE_LOVE = 13 => "OPINION_REACTION_TYPE_ATTITUDE_WIFE_LOVE",
    ATTITUDE_WIFE_LIKE = 14 => "OPINION_REACTION_TYPE_ATTITUDE_WIFE_LIKE",
    ATTITUDE_WIFE_NEUTRAL = 15 => "OPINION_REACTION_TYPE_ATTITUDE_WIFE_NEUTRAL",
    ATTITUDE_WIFE_DISLIKE = 16 => "OPINION_REACTION_TYPE_ATTITUDE_WIFE_DISLIKE",
    ATTITUDE_WIFE_HATE = 17 => "OPINION_REACTION_TYPE_ATTITUDE_WIFE_HATE",
    ANGRY_POINT = 18 => "OPINION_REACTION_TYPE_ANGRY_POINT",
    BACK_AWAY = 19 => "OPINION_REACTION_TYPE_BACK_AWAY",
    BELLY_LAUGH = 20 => "OPINION_REACTION_TYPE_BELLY_LAUGH",
    BOO = 21 => "OPINION_REACTION_TYPE_BOO",
    BOWING_LARGE = 22 => "OPINION_REACTION_TYPE_BOWING_LARGE",
    BOWING_SMALL = 23 => "OPINION_REACTION_TYPE_BOWING_SMALL",
    CALLING_CHILDREN = 24 => "OPINION_REACTION_TYPE_CALLING_CHILDREN",
    CHEER_LARGE = 25 => "OPINION_REACTION_TYPE_CHEER_LARGE",
    CHEER_SMALL = 26 => "OPINION_REACTION_TYPE_CHEER_SMALL",
    CLAP_LARGE = 27 => "OPINION_REACTION_TYPE_CLAP_LARGE",
    CLAP_SMALL = 28 => "OPINION_REACTION_TYPE_CLAP_SMALL",
    COMMENT_ABOUT_HERO = 29 => "OPINION_REACTION_TYPE_COMMENT_ABOUT_HERO",
    COMMENT_AT_HERO = 30 => "OPINION_REACTION_TYPE_COMMENT_AT_HERO",
    COMMENT_TO_SELF = 31 => "OPINION_REACTION_TYPE_COMMENT_TO_SELF",
    COWER = 32 => "OPINION_REACTION_TYPE_COWER",
    DISMISS = 33 => "OPINION_REACTION_TYPE_DISMISS",
    FLEE = 34 => "OPINION_REACTION_TYPE_FLEE",
    FOLLOW_CLOSE = 35 => "OPINION_REACTION_TYPE_FOLLOW_CLOSE",
    FOLLOW_FAR = 36 => "OPINION_REACTION_TYPE_FOLLOW_FAR",
    FRIENDLY_GREET = 37 => "OPINION_REACTION_TYPE_FRIENDLY_GREET",
    GET_OUT = 38 => "OPINION_REACTION_TYPE_GET_OUT",
    GROVEL_LARGE = 39 => "OPINION_REACTION_TYPE_GROVEL_LARGE",
    GROVEL_SMALL = 40 => "OPINION_REACTION_TYPE_GROVEL_SMALL",
    HERO_IMITATION_PLAY = 41 => "OPINION_REACTION_TYPE_HERO_IMITATION_PLAY",
    HEROTITLE_AT_HERO = 42 => "OPINION_REACTION_TYPE_HEROTITLE_AT_HERO",
    HEROTITLE_TO_SELF = 43 => "OPINION_REACTION_TYPE_HEROTITLE_TO_SELF",
    HIDE = 44 => "OPINION_REACTION_TYPE_HIDE",
    MARRIAGE_COMMENT = 45 => "OPINION_REACTION_TYPE_MARRIAGE_COMMENT",
    OFFER_GIFT_FRIENDLY = 46 => "OPINION_REACTION_TYPE_OFFER_GIFT_FRIENDLY",
    OFFER_GIFT_WORSHIP = 47 => "OPINION_REACTION_TYPE_OFFER_GIFT_WORSHIP",
    PICK_FIGHT = 48 => "OPINION_REACTION_TYPE_PICK_FIGHT",
    POINT = 49 => "OPINION_REACTION_TYPE_POINT",
    POINT_LAUGH = 50 => "OPINION_REACTION_TYPE_POINT_LAUGH",
    RESPECTFUL_GREET = 51 => "OPINION_REACTION_TYPE_RESPECTFUL_GREET",
    SHAKE_FIST = 52 => "OPINION_REACTION_TYPE_SHAKE_FIST",
    SHOUT_AWE = 53 => "OPINION_REACTION_TYPE_SHOUT_AWE",
    SNIGGER = 54 => "OPINION_REACTION_TYPE_SNIGGER",
    THUMBS_DOWN = 55 => "OPINION_REACTION_TYPE_THUMBS_DOWN",
    WATCH = 56 => "OPINION_REACTION_TYPE_WATCH",
    WIFE_FEELING_LOVE = 57 => "OPINION_REACTION_TYPE_WIFE_FEELING_LOVE",
    WIFE_FIRST = 57 => "OPINION_REACTION_TYPE_WIFE_FIRST",
    WIFE_GENERAL_LOVE = 58 => "OPINION_REACTION_TYPE_WIFE_GENERAL_LOVE",
    WIFE_TOHERSELF_LOVE = 59 => "OPINION_REACTION_TYPE_WIFE_TOHERSELF_LOVE",
    WIFE_CLOTHING_LOVE = 60 => "OPINION_REACTION_TYPE_WIFE_CLOTHING_LOVE",
    WIFE_FEELING_LIKE = 61 => "OPINION_REACTION_TYPE_WIFE_FEELING_LIKE",
    WIFE_GENERAL_LIKE = 62 => "OPINION_REACTION_TYPE_WIFE_GENERAL_LIKE",
    WIFE_TOHERSELF_LIKE = 63 => "OPINION_REACTION_TYPE_WIFE_TOHERSELF_LIKE",
    WIFE_CLOTHING_LIKE = 64 => "OPINION_REACTION_TYPE_WIFE_CLOTHING_LIKE",
    WIFE_FEELING_NEUTRAL = 65 => "OPINION_REACTION_TYPE_WIFE_FEELING_NEUTRAL",
    WIFE_GENERAL_NEUTRAL = 66 => "OPINION_REACTION_TYPE_WIFE_GENERAL_NEUTRAL",
    WIFE_TOHERSELF_NEUTRAL = 67 => "OPINION_REACTION_TYPE_WIFE_TOHERSELF_NEUTRAL",
    WIFE_CLOTHING_NEUTRAL = 68 => "OPINION_REACTION_TYPE_WIFE_CLOTHING_NEUTRAL",
    WIFE_FEELING_DISLIKE = 69 => "OPINION_REACTION_TYPE_WIFE_FEELING_DISLIKE",
    WIFE_GENERAL_DISLIKE = 70 => "OPINION_REACTION_TYPE_WIFE_GENERAL_DISLIKE",
    WIFE_TOHERSELF_DISLIKE = 71 => "OPINION_REACTION_TYPE_WIFE_TOHERSELF_DISLIKE",
    WIFE_CLOTHING_DISLIKE = 72 => "OPINION_REACTION_TYPE_WIFE_CLOTHING_DISLIKE",
    WIFE_FEELING_HATE = 73 => "OPINION_REACTION_TYPE_WIFE_FEELING_HATE",
    WIFE_GENERAL_HATE = 74 => "OPINION_REACTION_TYPE_WIFE_GENERAL_HATE",
    WIFE_TOHERSELF_HATE = 75 => "OPINION_REACTION_TYPE_WIFE_TOHERSELF_HATE",
    WIFE_CLOTHING_HATE = 76 => "OPINION_REACTION_TYPE_WIFE_CLOTHING_HATE",
    WIFE_JUSTMARRIED = 77 => "OPINION_REACTION_TYPE_WIFE_JUSTMARRIED",
    WIFE_GIFT_WANTED = 78 => "OPINION_REACTION_TYPE_WIFE_GIFT_WANTED",
    LAST = 79 => "OPINION_REACTION_TYPE_LAST",
    WIFE_LAST = 79 => "OPINION_REACTION_TYPE_WIFE_LAST",
)]
pub struct OpinionReactionType(pub i32);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default, DefFlags)]
#[flags(
    NONE = 0 => "OPINION_TARGETING_CONDITION_TYPE_NONE",
    NOT_TARGETED = 1 => "OPINION_TARGETING_CONDITION_TYPE_NOT_TARGETED",
    TARGETED_BUT_NOT_Z = 2 => "OPINION_TARGETING_CONDITION_TYPE_TARGETED_BUT_NOT_Z",
    NOT_Z_TARGETED = 3 => "OPINION_TARGETING_CONDITION_TYPE_NOT_Z_TARGETED",
    Z_TARGETED = 4 => "OPINION_TARGETING_CONDITION_TYPE_Z_TARGETED",
    ANY_TARGETED = 5 => "OPINION_TARGETING_CONDITION_TYPE_ANY_TARGETED",
)]
pub struct OpinionTargetingConditionType(pub i32);

/// Physical-switch trigger (C++ `enum EPSwitchTriggerType`).
/// Declared as DefFlags with sequential values in the original schema;
/// converted to DefEnum. NONE = 0 added for NULLDEF default.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
#[allow(non_camel_case_types)]
pub enum PSwitchTriggerType {
    #[def("PSWITCH_TRIGGER_NONE")]
    NONE = 0,
    #[def("PSWITCH_TRIGGER_ON_PLAYER")]
    PSWITCH_TRIGGER_ON_PLAYER = 1,
    #[def("PSWITCH_TRIGGER_ON_PLAYER_USE")]
    PSWITCH_TRIGGER_ON_PLAYER_USE = 2,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum PerceivedThingType {
    #[def("PERCEIVED_THING_FIND_PLAYER")]
    PERCEIVEDTHINGFINDPLAYER = 0,
    #[def("MAX_NO_PERCEIVED_THING_TYPES")]
    MAXNOPERCEIVEDTHINGTYPES = 1,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum PointLightChannelEffect {
    #[def("POINT_LIGHT_EFFECT_LOCAL_CHANNEL")]
    LOCALCHANNEL = 0,
    #[def("POINT_LIGHT_EFFECT_ALL_INTERNALS")]
    ALLINTERNALS = 1,
    #[def("POINT_LIGHT_EFFECT_EXTERNALS")]
    EXTERNALS = 2,
    #[def("POINT_LIGHT_EFFECT_ALL")]
    ALL = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum QuakeLength {
    #[def("QUAKE_LENGTH_SHORT")]
    SHORT = 0,
    #[def("QUAKE_LENGTH_MEDIUM")]
    MEDIUM = 1,
    #[def("QUAKE_LENGTH_LONG")]
    LONG = 2,
    #[def("QUAKE_LENGTH_CONTINUOUS")]
    CONTINUOUS = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum QuakeStrength {
    #[def("QUAKE_STRENGTH_WEAK")]
    WEAK = 0,
    #[def("QUAKE_STRENGTH_MEDIUM")]
    MEDIUM = 1,
    #[def("QUAKE_STRENGTH_STRONG")]
    STRONG = 2,
    #[def("QUAKE_STRENGTH_MADNESS")]
    MADNESS = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ReactionSpeechType {
    #[def("REACTION_SPEECH_NULL")]
    REACTIONSPEECHNULL = 0,
    #[def("REACTION_SPEECH_CALL_OVER_HERE")]
    REACTIONSPEECHCALLOVERHERE = 1,
    #[def("REACTION_SPEECH_FAWNING_GREETING")]
    REACTIONSPEECHFAWNINGGREETING = 2,
    #[def("REACTION_SPEECH_FRIENDLY_GREETING")]
    REACTIONSPEECHFRIENDLYGREETING = 3,
    #[def("REACTION_SPEECH_STRANGERS_GREETING")]
    REACTIONSPEECHSTRANGERSGREETING = 4,
    #[def("REACTION_SPEECH_NERVOUS")]
    REACTIONSPEECHNERVOUS = 5,
    #[def("REACTION_SPEECH_SCARED")]
    REACTIONSPEECHSCARED = 6,
    #[def("REACTION_SPEECH_TERRIFIED")]
    REACTIONSPEECHTERRIFIED = 7,
    #[def("REACTION_SPEECH_INSULTED")]
    REACTIONSPEECHINSULTED = 8,
    #[def("REACTION_SPEECH_ANGRY")]
    REACTIONSPEECHANGRY = 9,
    #[def("REACTION_SPEECH_HATEFUL")]
    REACTIONSPEECHHATEFUL = 10,
    #[def("REACTION_SPEECH_THREAT_OF_RETRIBUTION")]
    REACTIONSPEECHTHREATOFRETRIBUTION = 11,
    #[def("REACTION_SPEECH_PROMISE_OF_RETRIBUTION")]
    REACTIONSPEECHPROMISEOFRETRIBUTION = 12,
    #[def("REACTION_SPEECH_DISMISSIVE")]
    REACTIONSPEECHDISMISSIVE = 13,
    #[def("REACTION_SPEECH_BOTHERED")]
    REACTIONSPEECHBOTHERED = 14,
    #[def("REACTION_SPEECH_HARASSED")]
    REACTIONSPEECHHARASSED = 15,
    #[def("REACTION_SPEECH_RIDICULING")]
    REACTIONSPEECHRIDICULING = 16,
    #[def("REACTION_SPEECH_INSULTS")]
    REACTIONSPEECHINSULTS = 17,
    #[def("REACTION_SPEECH_ATTRACTED")]
    REACTIONSPEECHATTRACTED = 18,
    #[def("REACTION_SPEECH_LOVING")]
    REACTIONSPEECHLOVING = 19,
    #[def("REACTION_SPEECH_SURPRISED_AT_ABUSE")]
    REACTIONSPEECHSURPRISEDATABUSE = 20,
    #[def("REACTION_SPEECH_LOVING_AND_SHOCKED_AT_ABUSE")]
    REACTIONSPEECHLOVINGANDSHOCKEDATABUSE = 21,
    #[def("REACTION_SPEECH_UNIMPRESSED_RESPONSE_TO_THREAT")]
    REACTIONSPEECHUNIMPRESSEDRESPONSETOTHREAT = 22,
    #[def("REACTION_SPEECH_RIDICULING_RESPONSE_TO_THREAT")]
    REACTIONSPEECHRIDICULINGRESPONSETOTHREAT = 23,
    #[def("REACTION_SPEECH_LOVING_RESPONSE_TO_FLIRT")]
    REACTIONSPEECHLOVINGRESPONSETOFLIRT = 24,
    #[def("REACTION_SPEECH_ATTRACTED_RESPONSE_TO_FLIRT")]
    REACTIONSPEECHATTRACTEDRESPONSETOFLIRT = 25,
    #[def("REACTION_SPEECH_NEUTRAL_REFUSAL_OF_FLIRT")]
    REACTIONSPEECHNEUTRALREFUSALOFFLIRT = 26,
    #[def("REACTION_SPEECH_NEGATIVE_RESPONSE_TO_FLIRT")]
    REACTIONSPEECHNEGATIVERESPONSETOFLIRT = 27,
    #[def("REACTION_SPEECH_FEARFUL_REFUSAL_OF_FLIRT")]
    REACTIONSPEECHFEARFULREFUSALOFFLIRT = 28,
    #[def("REACTION_SPEECH_GRATEFUL_ACCEPTANCE_OF_BRIBE")]
    REACTIONSPEECHGRATEFULACCEPTANCEOFBRIBE = 29,
    #[def("REACTION_SPEECH_PACIFIED_ACCEPTANCE_OF_BRIBE")]
    REACTIONSPEECHPACIFIEDACCEPTANCEOFBRIBE = 30,
    #[def("REACTION_SPEECH_FRIENDLY_REJECTION_OF_BRIBE")]
    REACTIONSPEECHFRIENDLYREJECTIONOFBRIBE = 31,
    #[def("REACTION_SPEECH_UNFRIENDLY_REJECTION_OF_BRIBE")]
    REACTIONSPEECHUNFRIENDLYREJECTIONOFBRIBE = 32,
    #[def("REACTION_SPEECH_WARN_AWAY")]
    REACTIONSPEECHWARNAWAY = 33,
    #[def("REACTION_SPEECH_GIVE_AWAY")]
    REACTIONSPEECHGIVEAWAY = 34,
    #[def("REACTION_SPEECH_REPORT_CRIME")]
    REACTIONSPEECHREPORTCRIME = 35,
    #[def("REACTION_SPEECH_BODY_FOUND")]
    REACTIONSPEECHBODYFOUND = 36,
    #[def("REACTION_SPEECH_REPORT_BODY_FOUND")]
    REACTIONSPEECHREPORTBODYFOUND = 37,
    #[def("REACTION_SPEECH_GOSSIP")]
    REACTIONSPEECHGOSSIP = 38,
    #[def("REACTION_SPEECH_YAWN")]
    REACTIONSPEECHYAWN = 39,
    #[def("REACTION_SPEECH_SNORE")]
    REACTIONSPEECHSNORE = 40,
    #[def("REACTION_SPEECH_CHEER")]
    REACTIONSPEECHCHEER = 41,
    #[def("REACTION_SPEECH_SOB")]
    REACTIONSPEECHSOB = 42,
    #[def("REACTION_SPEECH_CRY_OUT")]
    REACTIONSPEECHCRYOUT = 43,
    #[def("REACTION_SPEECH_BATTLE_CRY")]
    REACTIONSPEECHBATTLECRY = 44,
    #[def("REACTION_SPEECH_LYNCH_CRY")]
    REACTIONSPEECHLYNCHCRY = 45,
    #[def("REACTION_SPEECH_WOUNDED")]
    REACTIONSPEECHWOUNDED = 46,
    #[def("REACTION_SPEECH_DIE")]
    REACTIONSPEECHDIE = 47,
    #[def("REACTION_SPEECH_GUARD_KILL")]
    REACTIONSPEECHGUARDKILL = 48,
    #[def("REACTION_SPEECH_GUARD_ARREST")]
    REACTIONSPEECHGUARDARREST = 49,
    #[def("REACTION_SPEECH_GUARD_SECURITY_SWEEP")]
    REACTIONSPEECHGUARDSECURITYSWEEP = 50,
    #[def("REACTION_SPEECH_GUARD_WARNING_1")]
    REACTIONSPEECHGUARDWARNING1 = 51,
    #[def("REACTION_SPEECH_GUARD_WARNING_2")]
    REACTIONSPEECHGUARDWARNING2 = 52,
    #[def("REACTION_SPEECH_GUARD_WARNING_3")]
    REACTIONSPEECHGUARDWARNING3 = 53,
    #[def("REACTION_SPEECH_GUARD_WARNING_END_AND_THANKS")]
    REACTIONSPEECHGUARDWARNINGENDANDTHANKS = 54,
    #[def("REACTION_SPEECH_NO_RESPECT")]
    REACTIONSPEECHNORESPECT = 55,
    #[def("MAX_NO_REACTION_SPEECH_TYPES")]
    MAXNOREACTIONSPEECHTYPES = 56,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ReverbEnvironmentType {
    #[def("REVERB_ENVIRONMENT_NULL")]
    NULL = 0,
    #[def("REVERB_ENVIRONMENT_EXTERNAL")]
    EXTERNAL = 1,
    #[def("REVERB_ENVIRONMENT_CAVE")]
    CAVE = 2,
    #[def("REVERB_ENVIRONMENT_HALL")]
    HALL = 3,
    #[def("REVERB_ENVIRONMENT_GUILD")]
    GUILD = 4,
    #[def("REVERB_ENVIRONMENT_GUILD_SMALL")]
    GUILDSMALL = 5,
    #[def("REVERB_ENVIRONMENT_SMALL_ROOM")]
    SMALLROOM = 6,
    #[def("REVERB_ENVIRONMENT_SCHOOL")]
    SCHOOL = 7,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ScriptingStateGroups {
    #[def("ESSG_NONE")]
    NONE = 0,
    #[def("ESSG_PERFORM_ACTION_PHYSICAL")]
    PERFORMACTIONPHYSICAL = 1,
    #[def("ESSG_PERFORM_ACTION_VERBAL")]
    PERFORMACTIONVERBAL = 2,
    #[def("ESSG_PERFORM_ACTION_AURAL")]
    PERFORMACTIONAURAL = 3,
    #[def("ESSG_WANDER_NEAR")]
    WANDERNEAR = 4,
    #[def("ESSG_FOLLOW_PATH")]
    FOLLOWPATH = 5,
    #[def("ESSG_FOLLOW_RANDOM")]
    FOLLOWRANDOM = 6,
    #[def("ESSG_FOLLOW_NEAREST")]
    FOLLOWNEAREST = 7,
    #[def("ESSG_WALK_TO_RANDOM")]
    WALKTORANDOM = 8,
    #[def("ESSG_WALK_TO_NEAREST_DIFFERENT")]
    WALKTONEARESTDIFFERENT = 9,
    #[def("ESSG_RUN_AT_HERO_AND_ATTACK_UNTIL_DEAD")]
    RUNATHEROANDATTACKUNTILDEAD = 10,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum Sex {
    #[def("SEX_NULL")]
    SEXNULL = 0,
    #[def("SEX_MALE")]
    SEXMALE = 1,
    #[def("SEX_FEMALE")]
    SEXFEMALE = 2,
    #[def("NO_OF_SEXES")]
    NOOFSEXES = 3,
}

/// Switch activation trigger (C++ `enum ESwitchTriggerType`).
/// Declared as DefFlags with sequential values in the original schema;
/// converted to DefEnum — values are sequential selectors, never bitmasks.
/// NONE = 0 added for NULLDEF default (C++ default-constructs to 0 uninitialised).
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
#[allow(non_camel_case_types)]
pub enum SwitchTriggerType {
    #[def("SWITCH_TRIGGER_NONE")]
    NONE = 0,
    #[def("SWITCH_TRIGGER_PLAYER_ONLY_ONCE_IN_AREA")]
    PLAYER_ONLY_ONCE_IN_AREA = 1,
    #[def("SWITCH_TRIGGER_PLAYER_ONLY_MULTIPLE_TIMES_IN_AREA")]
    PLAYER_ONLY_MULTIPLE_TIMES_IN_AREA = 2,
    #[def("SWITCH_TRIGGER_PLAYER_ONLY_RESET_WHEN_LEAVES")]
    PLAYER_ONLY_RESET_WHEN_LEAVES = 3,
    #[def("SWITCH_TRIGGER_PLAYER_SHOW_AREA_NAME")]
    PLAYER_SHOW_AREA_NAME = 4,
    #[def("SWITCH_TRIGGER_PLAYER_CHANGE_ENVIRONMENT_THEME")]
    PLAYER_CHANGE_ENVIRONMENT_THEME = 5,
    #[def("SWITCH_TRIGGER_ONCE_ON_ITEM_APPLICATION")]
    ONCE_ON_ITEM_APPLICATION = 6,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum TavernGameControlType {
    #[def("ETGCT_RELATIVE")]
    RELATIVE = 0,
    #[def("ETGCT_ABSOLUTE")]
    ABSOLUTE = 1,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum ThingCreatureProperty {
    #[def("THING_CREATURE_PROPERTY_NULL")]
    NULL = 0,
    #[def("THING_CREATURE_PROPERTY_IS_MINION")]
    ISMINION = 1,
    #[def("THING_CREATURE_PROPERTY_ANNOYABLE_BY_KIDS")]
    ANNOYABLEBYKIDS = 2,
    #[def("THING_CREATURE_PROPERTY_GUARD")]
    GUARD = 3,
    #[def("THING_CREATURE_PROPERTY_FIREFLY")]
    FIREFLY = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum TrapTriggerType {
    #[def("TRAP_TRIGGER_MANUAL")]
    MANUAL = 0,
    #[def("TRAP_TRIGGER_PROXIMITY")]
    PROXIMITY = 1,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum TrapType {
    #[def("TRAP_TYPE_TRIGGER_ONCE_ONLY")]
    TRIGGERONCEONLY = 0,
    #[def("TRAP_TYPE_TRIGGER_AND_RESET")]
    TRIGGERANDRESET = 1,
    #[def("TRAP_TYPE_TRIGGER_AND_PLAY_CONTINUOUS")]
    TRIGGERANDPLAYCONTINUOUS = 2,
    #[def("TRAP_TYPE_PLAY_CONTINUOUS")]
    PLAYCONTINUOUS = 3,
    #[def("TRAP_TYPE_TRIGGER_AND_DIE")]
    TRIGGERANDDIE = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum TutorialCategory {
    #[def("TUTORIAL_CATEGORY_NONE")]
    NONE = 0,
    #[def("TUTORIAL_CATEGORY_ABILITY_ASSIGNING")]
    ABILITYASSIGNING = 1,
    #[def("TUTORIAL_CATEGORY_ABILITY_CYCLING")]
    ABILITYCYCLING = 2,
    #[def("TUTORIAL_CATEGORY_BASIC_OBJECTS")]
    BASICOBJECTS = 3,
    #[def("TUTORIAL_CATEGORY_BED")]
    BED = 4,
    #[def("TUTORIAL_CATEGORY_BOASTING")]
    BOASTING = 5,
    #[def("TUTORIAL_CATEGORY_CAMERA")]
    CAMERA = 6,
    #[def("TUTORIAL_CATEGORY_CHARITY_SHOP")]
    CHARITYSHOP = 7,
    #[def("TUTORIAL_CATEGORY_CHEST")]
    CHEST = 8,
    #[def("TUTORIAL_CATEGORY_COMBAT_MULTIPLIER")]
    COMBATMULTIPLIER = 9,
    #[def("TUTORIAL_CATEGORY_CREATURE_DROP")]
    CREATUREDROP = 10,
    #[def("TUTORIAL_CATEGORY_DYING")]
    DYING = 11,
    #[def("TUTORIAL_CATEGORY_DEMON_DOOR")]
    DEMONDOOR = 12,
    #[def("TUTORIAL_CATEGORY_DOOR")]
    DOOR = 13,
    #[def("TUTORIAL_CATEGORY_EXPERIENCE")]
    EXPERIENCE = 14,
    #[def("TUTORIAL_CATEGORY_EXPERIENCE_SPENDING")]
    EXPERIENCESPENDING = 15,
    #[def("TUTORIAL_CATEGORY_EXPRESSION")]
    EXPRESSION = 16,
    #[def("TUTORIAL_CATEGORY_FLIRTING")]
    FLIRTING = 17,
    #[def("TUTORIAL_CATEGORY_FLOURISHING_MOVE")]
    FLOURISHINGMOVE = 18,
    #[def("TUTORIAL_CATEGORY_GOLDMARKERS")]
    GOLDMARKERS = 19,
    #[def("TUTORIAL_CATEGORY_GUILD_SEAL")]
    GUILDSEAL = 20,
    #[def("TUTORIAL_CATEGORY_INTERACTING")]
    INTERACTING = 21,
    #[def("TUTORIAL_CATEGORY_INVENTORY")]
    INVENTORY = 22,
    #[def("TUTORIAL_CATEGORY_INVENTORY_ASSIGNING")]
    INVENTORYASSIGNING = 23,
    #[def("TUTORIAL_CATEGORY_LEVELLING_UP")]
    LEVELLINGUP = 24,
    #[def("TUTORIAL_CATEGORY_MORALITY")]
    MORALITY = 25,
    #[def("TUTORIAL_CATEGORY_MOVEMENT")]
    MOVEMENT = 26,
    #[def("TUTORIAL_CATEGORY_QUEST")]
    QUEST = 27,
    #[def("TUTORIAL_CATEGORY_QUEST_CARD")]
    QUESTCARD = 28,
    #[def("TUTORIAL_CATEGORY_RENOWN")]
    RENOWN = 29,
    #[def("TUTORIAL_CATEGORY_TAKING_QUESTS")]
    TAKINGQUESTS = 30,
    #[def("TUTORIAL_CATEGORY_TELEPORTING")]
    TELEPORTING = 31,
    #[def("TUTORIAL_CATEGORY_TRADE_ITEM")]
    TRADEITEM = 32,
    #[def("TUTORIAL_CATEGORY_SEARCHING")]
    SEARCHING = 33,
    #[def("TUTORIAL_CATEGORY_SNEAK")]
    SNEAK = 34,
    #[def("TUTORIAL_CATEGORY_BUILDING_OWNERSHIP")]
    BUILDINGOWNERSHIP = 35,
    #[def("TUTORIAL_CATEGORY_FISHING_GAME")]
    FISHINGGAME = 36,
    #[def("TUTORIAL_CATEGORY_ORACLE_GAME")]
    ORACLEGAME = 37,
    #[def("TUTORIAL_CATEGORY_WORLD_MAP")]
    WORLDMAP = 38,
    #[def("TUTORIAL_CATEGORY_ALCOHOL")]
    ALCOHOL = 39,
    #[def("TUTORIAL_CATEGORY_AUGMENTATION")]
    AUGMENTATION = 40,
    #[def("TUTORIAL_CATEGORY_ARMOUR")]
    ARMOUR = 41,
    #[def("TUTORIAL_CATEGORY_BOMB")]
    BOMB = 42,
    #[def("TUTORIAL_CATEGORY_CLOTHES")]
    CLOTHES = 43,
    #[def("TUTORIAL_CATEGORY_FOOD")]
    FOOD = 44,
    #[def("TUTORIAL_CATEGORY_FISHING_ROD")]
    FISHINGROD = 45,
    #[def("TUTORIAL_CATEGORY_GIFT")]
    GIFT = 46,
    #[def("TUTORIAL_CATEGORY_HAIRSTYLE")]
    HAIRSTYLE = 47,
    #[def("TUTORIAL_CATEGORY_POTION")]
    POTION = 48,
    #[def("TUTORIAL_CATEGORY_RESURRECTION_PHIAL")]
    RESURRECTIONPHIAL = 49,
    #[def("TUTORIAL_CATEGORY_SILVER_KEY")]
    SILVERKEY = 50,
    #[def("TUTORIAL_CATEGORY_SPADE")]
    SPADE = 51,
    #[def("TUTORIAL_CATEGORY_TATTOO")]
    TATTOO = 52,
    #[def("TUTORIAL_CATEGORY_TROPHY")]
    TROPHY = 53,
    #[def("TUTORIAL_CATEGORY_WEAPON")]
    WEAPON = 54,
    #[def("TUTORIAL_CATEGORY_WEAPON_LEGENDARY")]
    WEAPONLEGENDARY = 55,
    #[def("TUTORIAL_CATEGORY_APOLOGY")]
    APOLOGY = 56,
    #[def("TUTORIAL_CATEGORY_BATTLE_CRY")]
    BATTLECRY = 57,
    #[def("TUTORIAL_CATEGORY_BELCH")]
    BELCH = 58,
    #[def("TUTORIAL_CATEGORY_EVIL_LAUGH")]
    EVILLAUGH = 59,
    #[def("TUTORIAL_CATEGORY_FART")]
    FART = 60,
    #[def("TUTORIAL_CATEGORY_FLIRT")]
    FLIRT = 61,
    #[def("TUTORIAL_CATEGORY_FOLLOW")]
    FOLLOW = 62,
    #[def("TUTORIAL_CATEGORY_GIGGLE")]
    GIGGLE = 63,
    #[def("TUTORIAL_CATEGORY_HEROIC_STANCE")]
    HEROICSTANCE = 64,
    #[def("TUTORIAL_CATEGORY_MIDDLE_FINGER")]
    MIDDLEFINGER = 65,
    #[def("TUTORIAL_CATEGORY_PELVIC_THRUST")]
    PELVICTHRUST = 66,
    #[def("TUTORIAL_CATEGORY_PICKLOCK")]
    PICKLOCK = 67,
    #[def("TUTORIAL_CATEGORY_PICKPOCKET")]
    PICKPOCKET = 68,
    #[def("TUTORIAL_CATEGORY_SHIT")]
    SHIT = 69,
    #[def("TUTORIAL_CATEGORY_SNEER")]
    SNEER = 70,
    #[def("TUTORIAL_CATEGORY_STEAL")]
    STEAL = 71,
    #[def("TUTORIAL_CATEGORY_THANKS")]
    THANKS = 72,
    #[def("TUTORIAL_CATEGORY_VICTORY_PUMP")]
    VICTORYPUMP = 73,
    #[def("TUTORIAL_CATEGORY_WAIT")]
    WAIT = 74,
    #[def("TUTORIAL_CATEGORY_COCK_A_DOODLE_DO")]
    COCKADOODLEDO = 75,
    #[def("TUTORIAL_CATEGORY_CROTCH_GRAB")]
    CROTCHGRAB = 76,
    #[def("TUTORIAL_CATEGORY_KISS_MY_ASS")]
    KISSMYASS = 77,
    #[def("TUTORIAL_CATEGORY_FLAMENCO")]
    FLAMENCO = 78,
    #[def("TUTORIAL_CATEGORY_COSSACK")]
    COSSACK = 79,
    #[def("TUTORIAL_CATEGORY_AIR_GUITAR")]
    AIRGUITAR = 80,
    #[def("TUTORIAL_CATEGORY_BALLET")]
    BALLET = 81,
    #[def("TUTORIAL_CATEGORY_SATURDAY_NIGHT_FEVER")]
    SATURDAYNIGHTFEVER = 82,
    #[def("TUTORIAL_CATEGORY_TAP")]
    TAP = 83,
    #[def("TUTORIAL_CATEGORY_Y")]
    Y = 84,
    #[def("TUTORIAL_CATEGORY_M")]
    M = 85,
    #[def("TUTORIAL_CATEGORY_C")]
    C = 86,
    #[def("TUTORIAL_CATEGORY_A")]
    A = 87,
    #[def("TUTORIAL_CATEGORY_CRIME_WEAPONOUT")]
    CRIMEWEAPONOUT = 88,
    #[def("TUTORIAL_CATEGORY_CRIME_TRESPASSING")]
    CRIMETRESPASSING = 89,
    #[def("TUTORIAL_CATEGORY_CRIME_VANDALISM")]
    CRIMEVANDALISM = 90,
    #[def("TUTORIAL_CATEGORY_CRIME_THEFT")]
    CRIMETHEFT = 91,
    #[def("TUTORIAL_CATEGORY_CRIME_ASSAULT")]
    CRIMEASSAULT = 92,
    #[def("TUTORIAL_CATEGORY_CRIME_GBH")]
    CRIMEGBH = 93,
    #[def("TUTORIAL_CATEGORY_CRIME_MURDER")]
    CRIMEMURDER = 94,
    #[def("TUTORIAL_CATEGORY_COUNT")]
    COUNT = 95,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum WallMountEffects {
    #[def("WALL_MOUNT_EFFECT_NONE")]
    NONE = 0,
    #[def("WALL_MOUNT_EFFECT_TELEPORT")]
    TELEPORT = 1,
    #[def("WALL_MOUNT_EFFECT_HEAL")]
    HEAL = 2,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum WaterType {
    #[def("WATER_TYPE_NULL")]
    NULL = 0,
    #[def("WATER_TYPE_LAKE")]
    LAKE = 1,
    #[def("WATER_TYPE_RIVER")]
    RIVER = 2,
    #[def("WATER_TYPE_SEA")]
    SEA = 3,
    #[def("WATER_TYPE_REFLECTIVE_SEA")]
    REFLECTIVESEA = 4,
    #[def("WATER_TYPE_NON_REFLECTIVE_SEA")]
    NONREFLECTIVESEA = 5,
    #[def("WATER_TYPE_OLD")]
    OLD = 6,
    #[def("WATER_TYPE_DUMMY_SHORE_POINT")]
    DUMMYSHOREPOINT = 7,
    #[def("WATER_TYPE_ICE")]
    ICE = 8,
    #[def("WATER_TYPE_COUNT")]
    COUNT = 9,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum WeaponClass {
    #[def("WC_UNARMED")]
    UNARMED = 0,
    #[def("WC_LIGHT")]
    LIGHT = 1,
    #[def("WC_HEAVY")]
    HEAVY = 2,
    #[def("WC_PROJECTILE")]
    PROJECTILE = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum WeaponType {
    #[def("WT_SWORD")]
    SWORD = 0,
    #[def("WT_AXE")]
    AXE = 1,
    #[def("WT_HAMMER")]
    HAMMER = 2,
    #[def("WT_BOW")]
    BOW = 3,
    #[def("WT_CROSSBOW")]
    CROSSBOW = 4,
    #[def("WT_BOLT")]
    BOLT = 5,
    #[def("WT_ARROW")]
    ARROW = 6,
    #[def("WT_THROWING")]
    THROWING = 7,
}

/// World-map icon graphic.  Kept as DefFlags (not DefEnum) because the
/// `UILocaleGraphicsDef.WorldMapRegionName` BTreeMap stores `UI_REGION_NAME_*`
/// text-id values (>52) keyed by these selectors, so out-of-table values are
/// legitimate in retail data (e.g. NULLDEF value 4962).
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default, DefFlags)]
#[flags(
    BANDIT_CAMP = 0 => "WORLD_MAP_NAME_GRAPHIC_BANDIT_CAMP",
    BANDIT_CAMP_TENT = 1 => "WORLD_MAP_NAME_GRAPHIC_BANDIT_CAMP_TENT",
    BANDIT_CAMP_ELITES = 2 => "WORLD_MAP_NAME_GRAPHIC_BANDIT_CAMP_ELITES",
    BANDIT_CAMP_PATH = 3 => "WORLD_MAP_NAME_GRAPHIC_BANDIT_CAMP_PATH",
    BANDIT_CAMP_GATE = 4 => "WORLD_MAP_NAME_GRAPHIC_BANDIT_CAMP_GATE",
    BOWERSTONE = 5 => "WORLD_MAP_NAME_GRAPHIC_BOWERSTONE",
    DARKWOOD = 6 => "WORLD_MAP_NAME_GRAPHIC_DARKWOOD",
    DARWOOD_SWAMP = 7 => "WORLD_MAP_NAME_GRAPHIC_DARWOOD_SWAMP",
    DARKWOOD_LAKE = 8 => "WORLD_MAP_NAME_GRAPHIC_DARKWOOD_LAKE",
    DARKWOOD_CAMP = 9 => "WORLD_MAP_NAME_GRAPHIC_DARKWOOD_CAMP",
    CHAPEL_SKORM = 10 => "WORLD_MAP_NAME_GRAPHIC_CHAPEL_SKORM",
    ANCIENT_CULLIS = 11 => "WORLD_MAP_NAME_GRAPHIC_ANCIENT_CULLIS",
    DARKWOOD_WEIR = 12 => "WORLD_MAP_NAME_GRAPHIC_DARKWOOD_WEIR",
    FISHER_CREEK = 13 => "WORLD_MAP_NAME_GRAPHIC_FISHER_CREEK",
    LYCHFIELD_GRAVEYARD = 14 => "WORLD_MAP_NAME_GRAPHIC_LYCHFIELD_GRAVEYARD",
    HEADMANS_HILL = 15 => "WORLD_MAP_NAME_GRAPHIC_HEADMANS_HILL",
    PRISON_PATH = 16 => "WORLD_MAP_NAME_GRAPHIC_PRISON_PATH",
    GIBBET_WOODS = 17 => "WORLD_MAP_NAME_GRAPHIC_GIBBET_WOODS",
    GRAVEYARD_PATH = 18 => "WORLD_MAP_NAME_GRAPHIC_GRAVEYARD_PATH",
    CIRCLE_DEAD = 19 => "WORLD_MAP_NAME_GRAPHIC_CIRCLE_DEAD",
    GREATWOOD_CAVES = 20 => "WORLD_MAP_NAME_GRAPHIC_GREATWOOD_CAVES",
    GREATWOOD_LAKE = 21 => "WORLD_MAP_NAME_GRAPHIC_GREATWOOD_LAKE",
    GREATWOOD = 22 => "WORLD_MAP_NAME_GRAPHIC_GREATWOOD",
    GUILD = 23 => "WORLD_MAP_NAME_GRAPHIC_GUILD",
    LOOKOUT_POINT = 24 => "WORLD_MAP_NAME_GRAPHIC_LOOKOUT_POINT",
    WINDMILL_HILL = 25 => "WORLD_MAP_NAME_GRAPHIC_WINDMILL_HILL",
    BOWERSTONE_JAIL = 26 => "WORLD_MAP_NAME_GRAPHIC_BOWERSTONE_JAIL",
    OAKVALE = 27 => "WORLD_MAP_NAME_GRAPHIC_OAKVALE",
    ORCHARD_FARM = 28 => "WORLD_MAP_NAME_GRAPHIC_ORCHARD_FARM",
    WITCHWOOD = 29 => "WORLD_MAP_NAME_GRAPHIC_WITCHWOOD",
    WITCHWOOD_LAKE = 30 => "WORLD_MAP_NAME_GRAPHIC_WITCHWOOD_LAKE",
    TEMPLE_AVO = 31 => "WORLD_MAP_NAME_GRAPHIC_TEMPLE_AVO",
    WITCHWOOD_STONES = 32 => "WORLD_MAP_NAME_GRAPHIC_WITCHWOOD_STONES",
    PICNIC_AREA = 33 => "WORLD_MAP_NAME_GRAPHIC_PICNIC_AREA",
    ROSE_COTTAGE = 34 => "WORLD_MAP_NAME_GRAPHIC_ROSE_COTTAGE",
    ARENA = 35 => "WORLD_MAP_NAME_GRAPHIC_ARENA",
    KNOTHOLE_GLADE = 36 => "WORLD_MAP_NAME_GRAPHIC_KNOTHOLE_GLADE",
    HOOK_COAST = 37 => "WORLD_MAP_NAME_GRAPHIC_HOOK_COAST",
    BARROW_FIELDS = 38 => "WORLD_MAP_NAME_GRAPHIC_BARROW_FIELDS",
    GREY_HOUSE = 39 => "WORLD_MAP_NAME_GRAPHIC_GREY_HOUSE",
    GREATWOOD_GORGE = 40 => "WORLD_MAP_NAME_GRAPHIC_GREATWOOD_GORGE",
    HOBBE_CAVE = 41 => "WORLD_MAP_NAME_GRAPHIC_HOBBE_CAVE",
    HEADMANS_CAVE = 42 => "WORLD_MAP_NAME_GRAPHIC_HEADMANS_CAVE",
    GREATWOOD_CULLIS = 43 => "WORLD_MAP_NAME_GRAPHIC_GREATWOOD_CULLIS",
    PRISON = 44 => "WORLD_MAP_NAME_GRAPHIC_PRISON",
    LOST_BAY = 45 => "WORLD_MAP_NAME_GRAPHIC_LOST_BAY",
    NECROPOLIS = 46 => "WORLD_MAP_NAME_GRAPHIC_NECROPOLIS",
    ARCHONS_SHRINE = 47 => "WORLD_MAP_NAME_GRAPHIC_ARCHONS_SHRINE",
    SNOWSPIRE = 48 => "WORLD_MAP_NAME_GRAPHIC_SNOWSPIRE",
    ARCHONS_FOLLY = 49 => "WORLD_MAP_NAME_GRAPHIC_ARCHONS_FOLLY",
    NORTHERN_WASTES_FOOTHILLS = 50 => "WORLD_MAP_NAME_GRAPHIC_NORTHERN_WASTES_FOOTHILLS",
    BORDELLO = 51 => "WORLD_MAP_NAME_GRAPHIC_BORDELLO",
    NUMBER = 52 => "WORLD_MAP_NAME_GRAPHIC_NUMBER",
)]
pub struct WorldMapNameGraphic(pub i32);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, DefEnum)]
#[repr(i32)]
pub enum DialogueLayer {
    #[def("DIALOGUE_LAYER_FOREGROUND")]
    FOREGROUND = 0,
    #[def("DIALOGUE_LAYER_MIDGROUND")]
    MIDGROUND = 1,
    #[def("DIALOGUE_LAYER_BACKGROUND")]
    BACKGROUND = 2,
    #[def("DIALOGUE_LAYER_LAST")]
    LAST = 3,
}
