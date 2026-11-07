use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct VillagePeopleDef {
    #[def("Enable")]
    pub enable: bool,
    #[def("DesiredMaleWeight")]
    pub desired_male_weight: f32,
    #[def("DesiredFemaleWeight")]
    pub desired_female_weight: f32,
    #[def("DesiredChildWeight")]
    pub desired_child_weight: f32,
    #[def("ProbabilityOfHomsexualityPerDialogueEnabledMan")]
    pub probability_of_homsexuality_per_dialogue_enabled_man: f32,
    #[def("MaleTeacher")]
    pub male_teacher: DefIndex,
    #[def("MaleTeacher")]
    pub male_teacher2: DefIndex,
    #[def("MaleShopKeeper")]
    pub male_shop_keeper: DefIndex,
    #[def("FemaleShopKeeper")]
    pub female_shop_keeper: DefIndex,
    #[def("MaleMarketTrader")]
    pub male_market_trader: DefIndex,
    #[def("FemaleMarketTrader")]
    pub female_market_trader: DefIndex,
    #[def("MaleBarman")]
    pub male_barman: DefIndex,
    #[def("FemaleBarman")]
    pub female_barman: DefIndex,
    #[def("MaleBarmaid")]
    pub male_barmaid: DefIndex,
    #[def("FemaleBarmaid")]
    pub female_barmaid: DefIndex,
    #[def("MaleWarehouseWorker")]
    pub male_warehouse_worker: DefIndex,
    #[def("FemaleWarehouseWorker")]
    pub female_warehouse_worker: DefIndex,
    #[def("MaleServant")]
    pub male_servant: DefIndex,
    #[def("FemaleServant")]
    pub female_servant: DefIndex,
    #[def("MaleGoodApprentice")]
    pub male_good_apprentice: DefIndex,
    #[def("FemaleGoodApprentice")]
    pub female_good_apprentice: DefIndex,
    #[def("MaleEvilApprentice")]
    pub male_evil_apprentice: DefIndex,
    #[def("FemaleEvilApprentice")]
    pub female_evil_apprentice: DefIndex,
    #[def("MaleUnemployed")]
    pub male_unemployed: DefIndex,
    #[def("FemaleUnemployed")]
    pub female_unemployed: DefIndex,
    #[def("MaleChild")]
    pub male_child: DefIndex,
    #[def("FemaleChild")]
    pub female_child: DefIndex,
    #[def("GamesMaster")]
    pub games_master: DefIndex,
    #[def("Guard")]
    pub guard: DefIndex,
    #[def("MaleGhost")]
    pub male_ghost: DefIndex,
    #[def("FemaleGhost")]
    pub female_ghost: DefIndex,
    #[def("NumGuards")]
    pub num_guards: i32,
    #[def("NumGhosts")]
    pub num_ghosts: i32,
    #[def("GuardRespawnType")]
    pub guard_respawn_type: Vec<DefIndex>,
    #[def("GuardRespawnQuantity")]
    pub guard_respawn_quantity: Vec<i32>,
    #[def("GuardRespawnMaxRenownBasedOffset")]
    pub guard_respawn_max_renown_based_offset: i32,
}
