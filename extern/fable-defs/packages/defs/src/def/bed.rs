use crate::DefStruct;
use crate::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct BedDef {
    #[def("GetInBedAnimName")]
    pub get_in_bed_anim_name: DefString,
    #[def("SleepInBedAnimName")]
    pub sleep_in_bed_anim_name: DefString,
    #[def("GetUpFromBedAnimName")]
    pub get_up_from_bed_anim_name: DefString,
    #[def("GetUpFromBedFastAnimName")]
    pub get_up_from_bed_fast_anim_name: DefString,
    #[def("Animating")]
    pub animating: bool,
}
