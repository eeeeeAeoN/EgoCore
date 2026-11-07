use crate::DefStruct;
use crate::enums::DoorTriggerType;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct DoorDef {
    #[def("TriggerType")]
    pub trigger_type: DoorTriggerType,
    #[def("TriggerRadius", default = 3.5)]
    pub trigger_radius: f32,
    #[def("OpenCollisionMesh")]
    pub open_collision_mesh: i32,
    #[def("ClosedCollisionMesh")]
    pub closed_collision_mesh: i32,
    #[def("PostVandalismCrimeIfDestroyed")]
    pub post_vandalism_crime_if_destroyed: bool,
    #[def("LockPickable", default = true)]
    pub lock_pickable: bool,
    #[def("PreventLockingByOwner")]
    pub prevent_locking_by_owner: bool,
}
