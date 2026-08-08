#include "assets/tmx_map_loader.h"
#include "game/collision_world.h"
#include "game/map_world.h"

#include <cassert>
#include <string>

namespace {
Vec2f center(const CollisionShape & shape) {
    return (shape.bounds.min + shape.bounds.max) * 0.5f;
}
}

int main() {
    const std::string fixture = std::string(GT_SOURCE_DIR) + "/tests/fixtures/collision_channels.tmx";
    const CollisionWorld collision = CollisionWorld::from_map(MapWorld::from_tmx(assets::load_tmx_map(fixture)));

    // Two valid geometries from each exact layer; malformed, point, and tile objects are inert.
    assert(collision.shapes().size() == 6);
    assert(collision.polygon_count() == 6); // debug extraction iterates this unfiltered collection

    for (const CollisionShape & shape : collision.shapes()) {
        const Vec2f point = center(shape);
        assert(collision.blocks_point(CollisionChannel::Player, point));
        assert(collision.blocks_segment(CollisionChannel::Player, {point.x - 40.0f, point.y}, point));

        const bool full = shape.source_layer_name == "collision_full";
        const bool shot = shape.source_layer_name == "collision_shots";
        assert(collision.blocks_point(CollisionChannel::Shot, point) == (full || shot));
        assert(collision.blocks_point(CollisionChannel::Vision, point) == full);

        const CollisionBounds local{{point.x - 1.0f, point.y - 1.0f}, {point.x + 1.0f, point.y + 1.0f}};
        assert(collision.query_bounds(CollisionChannel::Player, local).size() == 1);
        assert(collision.query_bounds(CollisionChannel::Shot, local).size() == (full || shot ? 1u : 0u));
        assert(collision.query_bounds(CollisionChannel::Vision, local).size() == (full ? 1u : 0u));
    }

    // Legacy/case-mismatched layers and semantic properties create no shapes.
    const CollisionBounds legacy_area{{60.0f, -100.0f}, {320.0f, 100.0f}};
    assert(collision.query_bounds(CollisionChannel::Player, legacy_area).empty());

    const std::string default_map_path = std::string(GT_SOURCE_DIR) + "/assets/tiled_projects/maps/brawlers_ballad.tmx";
    const MapWorld default_map = MapWorld::from_tmx(assets::load_tmx_map(default_map_path));
    assert(default_map.find_object_layer("collision_full"));
    assert(default_map.find_object_layer("collision_shots"));
    assert(default_map.find_object_layer("collision_player"));
}
