use crate::DefStruct;
use crate::def::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TavernGameCoinBaseDef {
    #[def("CoinIDensity")]
    pub coin_i_density: f32,
    #[def("CoinRadius")]
    pub coin_radius: f32,
    #[def("CoinEdgeRestitution")]
    pub coin_edge_restitution: f32,
    #[def("CoinEdgeFriction")]
    pub coin_edge_friction: f32,
    #[def("CoinSurfaceFriction")]
    pub coin_surface_friction: f32,
    #[def("CoinDefIndex")]
    pub coin_def_index: DefIndex,
    #[def("ArrowDefIndex")]
    pub arrow_def_index: DefIndex,
}
