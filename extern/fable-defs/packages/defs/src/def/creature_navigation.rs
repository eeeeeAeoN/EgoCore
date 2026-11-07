use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CreatureNavigationDef {
    #[def("MinPathCost")]
    pub min_path_cost: f32,
    #[def("MaxPathCost")]
    pub max_path_cost: f32,
    #[def("Radius")]
    pub radius: f32,
    #[def("AvoidDynamicObstacles", default = true)]
    pub avoid_dynamic_obstacles: bool,
}
