use crate::DefStruct;
use crate::def::{
    values::AtmosBankEntry, values::MusicEntry, values::MusicSetEntry,
    values::ReverbEnvironmentEntry, values::SoundBankEntry,
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SoundDef {
    #[def("SoundBankEntries")]
    pub sound_bank_entries: Vec<SoundBankEntry>,
    #[def("AtmosBankEntries")]
    pub atmos_bank_entries: Vec<AtmosBankEntry>,
    #[def("MusicEntries")]
    pub music_entries: Vec<MusicEntry>,
    #[def("MusicSetEntries")]
    pub music_set_entries: Vec<MusicSetEntry>,
    #[def("ReverbEnvironmentEntries")]
    pub reverb_environment_entries: Vec<ReverbEnvironmentEntry>,
    #[def("AtmosListenerDampingMin")]
    pub atmos_listener_damping_min: f32,
    #[def("AtmosListenerDampingRange")]
    pub atmos_listener_damping_range: f32,
    #[def("DistanceModelCurve")]
    pub distance_model_curve: f32,
}
