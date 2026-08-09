#include "assets/image_loader.h"
#include "assets/tmx_map_loader.h"
#include "game/map_world.h"
#include "render/entity_animations.h"

#include <array>
#include <cassert>
#include <string>

int main() {
    const std::string map_path = std::string(BARD_BATTLE_SOURCE_ROOT) +
        "/games/bard_battle/assets/tiled_projects/maps/brawlers_ballad.tmx";
    const auto map_asset = assets::load_tmx_map(map_path);
    const MapWorld map = MapWorld::from_tmx(map_asset);
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
    auto animations = entity_animation::Catalog::load(entity_dir);
    assert(animations.warnings().empty());
    const auto * idle = animations.find_player(1, entity_animation::PlayerAnimation::idle);
    assert(idle->tsx_path.filename() == "idle.tsx");
    assert(idle->world_size.x == 32.0f && idle->world_size.y == 32.0f);
    assert(animations.find_player(1, entity_animation::PlayerAnimation::attack)->tsx_path.filename() == "attack.tsx");
    assert(animations.find_player(1, entity_animation::PlayerAnimation::death)->tsx_path.filename() == "death.tsx");
    assert(animations.find_player(1, entity_animation::PlayerAnimation::shield)->tsx_path.filename() == "shield.tsx");
    assert(animations.find_player(1, entity_animation::PlayerAnimation::shield_break)->tsx_path.filename() == "shield_break.tsx");
    const auto * melee = animations.find_player_exact(1, entity_animation::PlayerAnimation::melee);
    assert(melee && melee->tsx_path.filename() == "melee.tsx");
    assert(melee->world_size.x == 64.0f && melee->world_size.y == 64.0f);
    constexpr std::array states{
        entity_animation::PlayerAnimation::death, entity_animation::PlayerAnimation::idle,
        entity_animation::PlayerAnimation::running, entity_animation::PlayerAnimation::attack,
        entity_animation::PlayerAnimation::melee, entity_animation::PlayerAnimation::shield_break,
        entity_animation::PlayerAnimation::shield};
    for(std::uint8_t player=1;player<=4;++player)for(const auto state:states)assert(animations.find_player_exact(player,state));
    const auto * running=animations.find_player_exact(1,entity_animation::PlayerAnimation::running);
    assert(running->frames.size()==3&&running->source_width==23&&running->source_height==28);

    auto runtime_atlas = assets::build_runtime_atlas(map_asset);
    assert(animations.pack(runtime_atlas.atlas, runtime_atlas.image).empty());
    melee = animations.find_player_exact(1, entity_animation::PlayerAnimation::melee);
    assert(melee && melee->atlas_span_x == 2 && melee->atlas_span_y == 2);
    const auto source = assets::load_png_rgba(melee->image_path.string());
    const auto first = melee->frames.front().atlas_index;
    const auto destination_x = (first % runtime_atlas.atlas.columns) * runtime_atlas.atlas.tile_width;
    const auto destination_y = (first / runtime_atlas.atlas.columns) * runtime_atlas.atlas.tile_height;
    for(std::uint32_t y=0;y<source.height;++y)for(std::uint32_t x=0;x<source.width;++x)for(std::uint32_t channel=0;channel<4;++channel){
        const auto source_offset=(static_cast<std::size_t>(y)*source.width+x)*4+channel;
        const auto destination_offset=(static_cast<std::size_t>(destination_y+y)*runtime_atlas.image.width+destination_x+x)*4+channel;
        assert(source.rgba_pixels[source_offset] == runtime_atlas.image.rgba_pixels[destination_offset]);
    }
}
