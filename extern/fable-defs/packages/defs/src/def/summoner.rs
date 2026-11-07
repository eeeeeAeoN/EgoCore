use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SummonerDef {
    #[def("LightningRangeForAutomaticUse")]
    pub lightning_range_for_automatic_use: f32,
    #[def("LightningPercentChanceForRandomUse")]
    pub lightning_percent_chance_for_random_use: f32,
    #[def("LightningPercentChanceForBoastUse")]
    pub lightning_percent_chance_for_boast_use: f32,
    #[def("LightningRandomChanceTestSeconds")]
    pub lightning_random_chance_test_seconds: f32,
    #[def("LightningSecondsToWaitBeforeReuse")]
    pub lightning_seconds_to_wait_before_reuse: f32,
    #[def("ChargeRangeNear")]
    pub charge_range_near: f32,
    #[def("ChargeRangeFar")]
    pub charge_range_far: f32,
    #[def("ChargeRangeAttackInterrupt")]
    pub charge_range_attack_interrupt: f32,
    #[def("ChargeSecondsToWaitBeforeReuse")]
    pub charge_seconds_to_wait_before_reuse: f32,
    #[def("SummonerDeathExplosion")]
    pub summoner_death_explosion: DefIndex,
    #[def("LightningOrb")]
    pub lightning_orb: DefIndex,
    #[def("LightningOrbAttackFireheart")]
    pub lightning_orb_attack_fireheart: DefIndex,
    #[def("LightningOrbLifeSeconds", default = -1.0)]
    pub lightning_orb_life_seconds: f32,
}
