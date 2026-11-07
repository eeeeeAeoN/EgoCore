use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroSpecialMovementDef {
    #[def("TotalTimeToSprintSeconds", default = 7.0)]
    pub total_time_to_sprint_seconds: f32,
    #[def("MinimumNumberOfSecondsLeftToBeAllowedToSprint", default = 1.5)]
    pub minimum_number_of_seconds_left_to_be_allowed_to_sprint: f32,
    #[def("TotalTimeToFullyRestoreSprintBarSeconds", default = 10.0)]
    pub total_time_to_fully_restore_sprint_bar_seconds: f32,
    #[def("NoSecondsToRunBeforeSprintStarts", default = 3.0)]
    pub no_seconds_to_run_before_sprint_starts: f32,
}
