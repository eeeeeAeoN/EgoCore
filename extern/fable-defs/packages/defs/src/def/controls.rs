use crate::DefStruct;
use crate::values::ActionInputControl;

/// `CONTROL_SCHEME` — `CControlsDef`.
#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ControlsDef {
    #[def("Controls")]
    pub controls: Vec<ActionInputControl>,
    #[def("ToggleZTarget")]
    pub toggle_z_target: bool,
    #[def("ToggleSpells")]
    pub toggle_spells: bool,
    #[def("ToggleSneak")]
    pub toggle_sneak: bool,
    #[def("ToggleExpressionMenu")]
    pub toggle_expression_menu: bool,
    #[def("ToggleExpressionShift")]
    pub toggle_expression_shift: bool,
    #[def("FlourishNeedsAttackButtonHeld")]
    pub flourish_needs_attack_button_held: bool,
}
