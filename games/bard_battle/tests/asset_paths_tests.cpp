#include "assets/tmx_map_loader.h"
#include "game/map_world.h"
#include "render/entity_animations.h"

#include <cassert>
#include <string>

int main() {
    const std::string map_path = std::string(BARD_BATTLE_SOURCE_ROOT) +
        "/games/bard_battle/assets/tiled_projects/maps/brawlers_ballad.tmx";
    const MapWorld map = MapWorld::from_tmx(assets::load_tmx_map(map_path));
    assert(map.find_object_layer("collision_full"));
    assert(map.find_object_layer("collision_shots"));
    assert(map.find_object_layer("collision_player"));
    assert(map.find_tile_layer("Elevation")->render_phase == TileRenderPhase::AboveUnits);
    assert(map.find_tile_layer("Ramps")->render_phase == TileRenderPhase::AboveUnits);
    assert(map.find_tile_layer("base")->render_phase == TileRenderPhase::BelowUnits);
    for (int i=1;i<=4;++i) {
        assert(map.find_object_layer("respawn_zone_"+std::to_string(i)));
    }

    const auto entity_dir = std::string(BARD_BATTLE_SOURCE_ROOT) +
        "/games/bard_battle/assets/tiled_projects/entities";
    const auto animations = entity_animation::Catalog::load(entity_dir);
    assert(animations.warnings().empty());
    const auto * idle = animations.find_player(1, entity_animation::PlayerAnimation::idle);
    assert(idle->tsx_path.filename() == "idle.tsx");
    assert(idle->world_size.x == 32.0f && idle->world_size.y == 32.0f);
    assert(animations.find_player(1, entity_animation::PlayerAnimation::attack)->tsx_path.filename() == "attack.tsx");
    assert(animations.find_player(1, entity_animation::PlayerAnimation::death)->tsx_path.filename() == "death.tsx");
    assert(animations.find_player(1, entity_animation::PlayerAnimation::shield)->tsx_path.filename() == "shield.tsx");
    assert(animations.find_player(1, entity_animation::PlayerAnimation::shield_break)->tsx_path.filename() == "shield_break.tsx");
}
