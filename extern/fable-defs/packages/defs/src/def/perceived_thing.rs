use crate::DefStruct;
use crate::enums::PerceivedThingType;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct PerceivedThingDef {
    #[def("FieldOfView", default = 90.0)]
    pub field_of_view: f32,
    #[def("ExtendedSightRadius", default = 13.0)]
    pub extended_sight_radius: f32,
    #[def("SightRadius", default = 8.0)]
    pub sight_radius: f32,
    #[def("SoundRadius", default = 10.0)]
    pub sound_radius: f32,
    #[def("SmellRadius")]
    pub smell_radius: f32,
    #[def("Type")]
    pub type_: PerceivedThingType,
    #[def("AwarenessTimer", default = 17.0)]
    pub awareness_timer: f32,
    #[def("GiveUpChaseRadius", default = 5.0)]
    pub give_up_chase_radius: f32,
    #[def("DayTimeVisibilityMultiplier", default = 1.0)]
    pub day_time_visibility_multiplier: f32,
    #[def("NightTimeVisibilityMultiplier", default = 1.0)]
    pub night_time_visibility_multiplier: f32,
}
