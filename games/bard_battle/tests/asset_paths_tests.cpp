#include "assets/tmx_map_loader.h"
#include "game/map_world.h"

#include <cassert>
#include <string>

int main() {
    const std::string map_path = std::string(BARD_BATTLE_SOURCE_ROOT) +
        "/games/bard_battle/assets/tiled_projects/maps/brawlers_ballad.tmx";
    const MapWorld map = MapWorld::from_tmx(assets::load_tmx_map(map_path));
    assert(map.find_object_layer("collision_full"));
    assert(map.find_object_layer("collision_shots"));
    assert(map.find_object_layer("collision_player"));
}
