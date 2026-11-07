use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CoinGameObstacleDef {
    #[def("Radius")]
    pub radius: f32,
}
