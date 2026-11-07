// Compile-time checks for the packaged def model.
#include <fable/defs.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

using namespace fable::defs;

static_assert(std::is_same<decltype(CCreatureDef::InitialActions),
                           std::vector<std::int32_t>>::value,
              "CCreatureDef::InitialActions should be std::vector<int32_t>");
static_assert(std::is_same<decltype(CCreatureDef::PhonemeAnim),
                           std::map<std::string, std::int32_t>>::value,
              "CCreatureDef::PhonemeAnim should be std::map<std::string, int32_t>");
static_assert(std::is_same<decltype(CCreatureDef::BattleCrySound), CDefString>::value,
              "CCreatureDef::BattleCrySound should be CDefString");
static_assert(std::is_base_of<CThingBaseDef, CThingObjectDef>::value,
              "CThingObjectDef should inherit CThingBaseDef");

static_assert(sizeof(CDefString) == sizeof(std::int32_t), "CDefString is an i32");
static_assert(sizeof(CDefIndex) == sizeof(std::int32_t), "CDefIndex is an i32");
static_assert(sizeof(EActionRegisteredType) == sizeof(std::int32_t),
              "enums have an explicit 32-bit underlying type");

static_assert(NUISystem::UI_TYPE_SPRITE == 0, "UI_TYPE_SPRITE");
static_assert(NUISystem::TABLE_EXPANSION_HORIZONTAL == 1, "TABLE_EXPANSION_HORIZONTAL");
static_assert(NUISystem::TABLE_EXPANSION_VERTICAL == 2, "TABLE_EXPANSION_VERTICAL");

int main() {
    CCreatureDef creature{};
    creature.Inventory = 7;
    creature.InitialActions.push_back(1);
    creature.PhonemeAnim["idle"] = 2;

    if (creature.Inventory != 7) return 1;
    if (creature.InitialActions.size() != 1) return 2;
    if (creature.PhonemeAnim.size() != 1) return 3;

    CThingObjectDef object{};
    object.Health = 10.0f;
    if (object.Health != 10.0f) return 4;

    return 0;
}
