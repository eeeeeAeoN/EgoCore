use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct BalverineBattleDef {
    #[def("SecondsBetweenLunges", default = 7.0)]
    pub seconds_between_lunges: f32,
    #[def("LungeAttackDamage", default = 2.0)]
    pub lunge_attack_damage: f32,
    #[def("TimeInTrees", default = 5.0)]
    pub time_in_trees: f32,
}
