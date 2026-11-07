use crate::DefStruct;
use crate::bytes::UnexpectedEnd;
use crate::def::UiStateDef;
use crate::enums::{
    ActionType, EngineGraphicType, Sprite2dFlags, TableExpansion, TextAlignment, UiType,
};
use crate::visit::{AsField, DefDefault, FieldRef};
use crate::wire::{DefIndex, DefString, ParseWireError, TaggedWire, WStr, Wire};
use std::collections::BTreeMap;

/// `MeshIndex` / `AnimationIndex` are polymorphic, disambiguated by the sibling
/// `Type` field. For asset-backed element types the stored `i32` is a **graphics.big
/// bank id** (a stable global asset id — e.g. `MESH_HERO_IRON_BATTLEAXE` = 8033);
/// for list/container types it is a **reference to another UI def** (the scroll-bar
/// element). Only `ui_mesh.cpp` reads the field at runtime (via `GetMeshBank`). The
/// wire encoding is a plain 4-byte `i32` in both cases; this enum records which it is
/// so the semantic differ compares bank ids raw (they match across builds) but def
/// refs by resolved name (their indices differ across index spaces).
#[derive(Debug, Clone, PartialEq)]
pub enum MeshRef {
    /// Index into an asset bank (graphics.big mesh / sprite bank). Compared raw.
    Bank(u32),
    /// Reference to another UI def. Resolved by name across index spaces.
    Def(DefIndex),
}

impl MeshRef {
    /// Whether a UI element `Type` reads MeshIndex/AnimationIndex as a **def
    /// reference** (list-style containers pointing at their scroll-bar element)
    /// rather than an asset-bank id. Derived from the corpus (`UI_TYPE_LIST` = 8,
    /// `UI_TYPE_SCROLLING_VIEWPORT` = 13); every other type uses it as a bank id.
    /// A misclassification only shows up as a semantic-ledger divergence (the bytes
    /// are unaffected), so the set is easy to extend if a new type surfaces.
    pub fn type_is_def_ref(ui_type: i32) -> bool {
        matches!(ui_type, 8 | 13)
    }

    /// Build the variant appropriate for `ui_type` from an already-resolved index.
    pub fn from_index(ui_type: i32, index: i32) -> Self {
        if Self::type_is_def_ref(ui_type) {
            MeshRef::Def(DefIndex(index))
        } else {
            MeshRef::Bank(index as u32)
        }
    }

    fn raw(&self) -> i32 {
        match self {
            MeshRef::Bank(n) => *n as i32,
            MeshRef::Def(d) => d.0,
        }
    }
}

impl Wire for MeshRef {
    fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
        // Untagged parse can't see `Type`; default to Bank. Real parses of a
        // `#[def(tag = "type_")]` field go through `TaggedWire::parse_tagged`.
        Ok(MeshRef::Bank(u32::parse(cur)?))
    }
    fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        self.raw().serialize(out)
    }
    fn wire_size(&self) -> usize {
        size_of::<i32>()
    }
}

impl TaggedWire for MeshRef {
    fn parse_tagged(cur: &mut &[u8], tag: i32) -> Result<Self, ParseWireError> {
        Ok(MeshRef::from_index(tag, i32::parse(cur)?))
    }
}

impl DefDefault for MeshRef {
    fn def_default() -> Self {
        MeshRef::Bank(0)
    }
}

impl AsField for MeshRef {
    fn as_field(&mut self) -> FieldRef<'_> {
        // Dispatch to the existing FieldRef kinds so the differ needs no new arm:
        // Bank -> U32 (raw compare), Def -> DefIndex (name-resolved compare).
        match self {
            MeshRef::Bank(n) => FieldRef::U32(n),
            MeshRef::Def(d) => FieldRef::DefIndex(d),
        }
    }
}

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct UiDef {
    #[def("Type", default = UiType::Composite)]
    pub type_: UiType,
    #[def("Children")]
    pub children: Vec<DefIndex>,
    // Polymorphic (graphics-bank id vs UI-def ref) — see [`MeshRef`]; disambiguated
    // by the sibling `Type` (the `tag`).
    #[def("MeshIndex", tag = "type_")]
    pub mesh_index: MeshRef,
    #[def("TextValue")]
    pub text_value: WStr,
    // `CDefString` (a name-table offset), not a raw int — the retail
    // `CUIElementDef::Transfer` writes `Font` as a `CDefString` and default-inits
    // it via `SetDefaultString("ENG_ARIAL_16")`. The default seeding lives in
    // `lower_ui` (offset 224 in retail's names.bin).
    #[def("Font")]
    pub font: DefString,
    #[def("Height")]
    pub height: f32,
    #[def("Width")]
    pub width: f32,
    #[def("ExpansionType", default = TableExpansion(1))] // HORIZONTAL
    pub expansion_type: TableExpansion,
    #[def("Sprites")]
    pub sprites: BTreeMap<i32, DefIndex>,
    #[def("HorizontalSeparations")]
    pub horizontal_separations: Vec<u32>,
    #[def("VerticalSeparations")]
    pub vertical_separations: Vec<u32>,
    #[def("States")]
    pub states: Vec<UiStateDef>,
    #[def("TextLineBreak", default = true)]
    pub text_line_break: bool,
    #[def("ScaleText", default = true)]
    pub scale_text: bool,
    #[def("Independant")]
    pub independant: bool,
    #[def("MeshType", default = EngineGraphicType::EngineGraphicStaticMesh)]
    pub mesh_type: EngineGraphicType,
    #[def("NonScrollingChildren")]
    pub non_scrolling_children: Vec<DefIndex>,
    #[def("TextWindowTLX")]
    pub text_window_tlx: f32,
    #[def("TextWindowTLY")]
    pub text_window_tly: f32,
    #[def("TextWindowBRX")]
    pub text_window_brx: f32,
    #[def("TextWindowBRY")]
    pub text_window_bry: f32,
    #[def("Layer")]
    pub layer: i32,
    #[def("Angle")]
    pub angle: f32,
    #[def("PositionIsCenter")]
    pub position_is_center: bool,
    #[def("ScrollingSpeed", default = 1.0)]
    pub scrolling_speed: f32,
    #[def("Wrapping", default = true)]
    pub wrapping: bool,
    #[def("Inverted")]
    pub inverted: bool,
    #[def("PositionOffsetX")]
    pub position_offset_x: f32,
    #[def("PositionOffsetY")]
    pub position_offset_y: f32,
    #[def("AlphaOffset")]
    pub alpha_offset: u32,
    #[def("UpX")]
    pub up_x: f32,
    #[def("UpY")]
    pub up_y: f32,
    #[def("UpZ", default = 1.0)]
    pub up_z: f32,
    #[def("ForwardX")]
    pub forward_x: f32,
    #[def("ForwardY", default = 1.0)]
    pub forward_y: f32,
    #[def("ForwardZ")]
    pub forward_z: f32,
    #[def("RotationAxisX")]
    pub rotation_axis_x: f32,
    #[def("RotationAxisY", default = 1.0)]
    pub rotation_axis_y: f32,
    #[def("RotationAxisZ")]
    pub rotation_axis_z: f32,
    #[def("RotationSpeed")]
    pub rotation_speed: f32,
    #[def("AnimationIndex", tag = "type_")]
    pub animation_index: MeshRef,
    #[def("DownArrow")]
    pub down_arrow: DefIndex,
    #[def("UpArrow")]
    pub up_arrow: DefIndex,
    #[def("UpLimit")]
    pub up_limit: i32,
    #[def("DownLimit")]
    pub down_limit: i32,
    #[def("Scrolling", default = true)]
    pub scrolling: bool,
    #[def("ComputeOffsetsOnActivate")]
    pub compute_offsets_on_activate: bool,
    #[def("MinX")]
    pub min_x: f32,
    #[def("MinY")]
    pub min_y: f32,
    #[def("MaxX")]
    pub max_x: f32,
    #[def("MaxY")]
    pub max_y: f32,
    #[def("StepX")]
    pub step_x: f32,
    #[def("StepY")]
    pub step_y: f32,
    #[def("DimensionsX")]
    pub dimensions_x: f32,
    #[def("DimensionsY")]
    pub dimensions_y: f32,
    #[def("SliderLeft")]
    pub slider_left: DefIndex,
    #[def("SliderRight")]
    pub slider_right: DefIndex,
    #[def("Action")]
    pub action: ActionType,
    #[def("ActionOnBack")]
    pub action_on_back: ActionType,
    #[def("ActionOnSelected")]
    pub action_on_selected: ActionType,
    #[def("ActionOnUnselected")]
    pub action_on_unselected: ActionType,
    #[def("ActionOnDestruction")]
    pub action_on_destruction: ActionType,
    #[def("ActionOnLeftClicked")]
    pub action_on_left_clicked: ActionType,
    #[def("ActionOnLeftUnclicked")]
    pub action_on_left_unclicked: ActionType,
    #[def("ActionOnLeftHeld")]
    pub action_on_left_held: ActionType,
    #[def("ActionOnRightClicked")]
    pub action_on_right_clicked: ActionType,
    #[def("ActionOnDropped")]
    pub action_on_dropped: ActionType,
    #[def("ActionOnDroppedNowhere")]
    pub action_on_dropped_nowhere: ActionType,
    #[def("PreAction")]
    pub pre_action: ActionType,
    #[def("ActionOnDraggedUp")]
    pub action_on_dragged_up: ActionType,
    #[def("ActionOnDraggedDown")]
    pub action_on_dragged_down: ActionType,
    #[def("ActionOnLeftClickedAbove")]
    pub action_on_left_clicked_above: ActionType,
    #[def("ActionOnLeftClickedUnder")]
    pub action_on_left_clicked_under: ActionType,
    #[def("InputDelay", default = 0.2)]
    pub input_delay: f32,
    #[def("DrawFromViewport")]
    pub draw_from_viewport: bool,
    #[def("TextBankIndex")]
    pub text_bank_index: u32,
    #[def("ActionText")]
    pub action_text: DefIndex,
    #[def("KeyText")]
    pub key_text: DefIndex,
    #[def("Redefiner")]
    pub redefiner: DefIndex,
    #[def("UndefinedWarning")]
    pub undefined_warning: DefIndex,
    #[def("ActionMap")]
    pub action_map: BTreeMap<u32, String>,
    #[def("ActionMapAliases")]
    pub action_map_aliases: BTreeMap<u32, u32>,
    #[def("ActionOrder")]
    pub action_order: Vec<u32>,
    #[def("EditBoxParentIsButton")]
    pub edit_box_parent_is_button: bool,
    #[def("PasswordBox")]
    pub password_box: bool,
    #[def("EditBoxCharLimit")]
    pub edit_box_char_limit: i32,
    #[def("EditBoxUsesIME")]
    pub edit_box_uses_ime: bool,
    #[def("MovieFilename")]
    pub movie_filename: WStr,
    #[def("DisallowSpaceAsFirstChar")]
    pub disallow_space_as_first_char: bool,
    #[def("LayerIndependant")]
    pub layer_independant: bool,
    #[def("SwappingStates")]
    pub swapping_states: Vec<u32>,
    #[def("SwappingTimes")]
    pub swapping_times: Vec<f32>,
    #[def("BastardChild")]
    pub bastard_child: bool,
    #[def("Alignement")]
    pub alignement: TextAlignment,
    #[def("RandomSwap")]
    pub random_swap: bool,
    #[def("UseRelativeZoom")]
    pub use_relative_zoom: bool,
    #[def("UseRelativePosition")]
    pub use_relative_position: bool,
    #[def("HoveredState", default = 3)]
    pub hovered_state: i32,
    #[def("LeftClickedState", default = 3)]
    pub left_clicked_state: i32,
    #[def("RightClickedState", default = 3)]
    pub right_clicked_state: i32,
    #[def("ShapeChildren")]
    pub shape_children: Vec<DefIndex>,
    #[def("ViewAreaTLX")]
    pub view_area_tlx: i32,
    #[def("ViewAreaTLY")]
    pub view_area_tly: i32,
    #[def("ViewAreaBRX", default = 640)]
    pub view_area_brx: i32,
    #[def("ViewAreaBRY", default = 480)]
    pub view_area_bry: i32,
    #[def("UseViewArea")]
    pub use_view_area: bool,
    #[def("PartOfListTree", default = true)]
    pub part_of_list_tree: bool,
    #[def("PCStyle")]
    pub pc_style: bool,
    #[def("Sprite2DFlag", default = Sprite2dFlags(2))]
    pub sprite2_d_flag: Sprite2dFlags,
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Serialize a MeshRef, then parse it back under `tag` (the sibling Type).
    fn round_trip_tagged(mr: MeshRef, tag: i32) -> MeshRef {
        let mut buf = [0u8; 4];
        let mut out: &mut [u8] = &mut buf;
        mr.serialize(&mut out).unwrap();
        let mut cur: &[u8] = &buf;
        MeshRef::parse_tagged(&mut cur, tag).unwrap()
    }

    #[test]
    fn meshref_variant_is_chosen_by_type_tag() {
        // Asset-backed types (UI_TYPE_MESH=3, UI_TYPE_MOUSE_POINTER=32) => Bank.
        assert!(!MeshRef::type_is_def_ref(3));
        assert!(!MeshRef::type_is_def_ref(32));
        // Container types (UI_TYPE_LIST=8, UI_TYPE_SCROLLING_VIEWPORT=13) => Def.
        assert!(MeshRef::type_is_def_ref(8));
        assert!(MeshRef::type_is_def_ref(13));

        assert_eq!(MeshRef::from_index(3, 8033), MeshRef::Bank(8033));
        assert_eq!(MeshRef::from_index(8, 500), MeshRef::Def(DefIndex(500)));

        // Round-trips preserve the variant: the wire is a plain i32, but re-parsing
        // under the same Type tag reconstructs Bank vs Def.
        assert_eq!(
            round_trip_tagged(MeshRef::Bank(8033), 3),
            MeshRef::Bank(8033)
        );
        assert_eq!(
            round_trip_tagged(MeshRef::Def(DefIndex(500)), 8),
            MeshRef::Def(DefIndex(500))
        );
        // Same bytes, different tag => different variant (proves it's tag-driven).
        assert_eq!(
            round_trip_tagged(MeshRef::Bank(500), 8),
            MeshRef::Def(DefIndex(500))
        );
    }
}
