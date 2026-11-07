use crate::DefStruct;
use crate::def::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TrophyDef {
    #[def("MinigameInitialTimerSecs")]
    pub minigame_initial_timer_secs: f32,
    #[def("TimeBonusPerWitness")]
    pub time_bonus_per_witness: f32,
    #[def("RenownGainedPerWitnessAhead")]
    pub renown_gained_per_witness_ahead: i32,
    #[def("EvilTrophy")]
    pub evil_trophy: bool,
    #[def("FakeTrophy")]
    pub fake_trophy: bool,
    #[def("EmoteIconForAvailableWitness")]
    pub emote_icon_for_available_witness: i32,
    #[def("ShowTrophyAnimName")]
    pub show_trophy_anim_name: DefString,
    #[def("GameDebugTextScale", default = 1.0)]
    pub game_debug_text_scale: f32,
    #[def("SecsDelayPerGainedWitnessSound", default = 1.0)]
    pub secs_delay_per_gained_witness_sound: f32,
    #[def("SoundStartGame")]
    pub sound_start_game: String,
    #[def("SoundShowTrophy")]
    pub sound_show_trophy: String,
    #[def("SoundGainedWitness")]
    pub sound_gained_witness: String,
    #[def("SoundOutOfTime")]
    pub sound_out_of_time: String,
    #[def("SoundGotAllWitnesses")]
    pub sound_got_all_witnesses: String,
    #[def("SoundEndNewHighScore")]
    pub sound_end_new_high_score: String,
    #[def("SoundEndNoNewScore")]
    pub sound_end_no_new_score: String,
}
