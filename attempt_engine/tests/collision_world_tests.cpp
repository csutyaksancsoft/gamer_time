#include "assets/tmx_map_loader.h"
#include "game/collision_world.h"
#include "game/map_world.h"

#include <cassert>
#include <string>

namespace {
Vec2f center(const CollisionShape & shape) {
    return (shape.bounds.min + shape.bounds.max) * 0.5f;
}

CollisionWorld concave_collision() {
    TmxMapAsset asset{};
    asset.orientation = "orthogonal";
    asset.render_order = "right-down";
    asset.width = asset.height = 100;
    asset.tile_width = asset.tile_height = 10;
    TmxTilesetAsset tiles{};
    tiles.first_gid = 1;
    tiles.name = "fixture";
    tiles.tile_width = tiles.tile_height = 10;
    tiles.tile_count = tiles.columns = 1;
    asset.tilesets.push_back(tiles);

    TmxObjectAsset object{};
    object.id = 1;
    object.x = object.y = 500.0f;
    object.has_polygon = true;
    object.polygon.points = {
        {0, 0}, {100, 0}, {100, 100}, {70, 100},
        {70, 30}, {30, 30}, {30, 100}, {0, 100},
    };
    TmxObjectLayerAsset layer{};
    layer.name = "collision_player";
    layer.objects.push_back(object);
    asset.object_layers.push_back(layer);
    asset.layer_order.push_back({TmxLayerType::Object, 0});
    return CollisionWorld::from_map(MapWorld::from_tmx(asset));
}
}

int main() {
    const std::string fixture = std::string(ATTEMPT_ENGINE_SOURCE_ROOT) + "/attempt_engine/tests/fixtures/collision_channels.tmx";
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

    const CollisionWorld concave = concave_collision();
    // This rectangle lies in the concavity while still overlapping the polygon's AABB.
    assert(concave.query_bounds(CollisionChannel::Player, {{40, -90}, {60, -40}}).empty());
    // A rectangle corner inside the polygon, an edge crossing, and corner contact all collide.
    assert(concave.query_bounds(CollisionChannel::Player, {{90, -20}, {110, -10}}).size() == 1);
    assert(concave.query_bounds(CollisionChannel::Player, {{-10, -50}, {10, -40}}).size() == 1);
    assert(concave.query_bounds(CollisionChannel::Player, {{-10, -110}, {0, -100}}).size() == 1);
    // Polygon contained by the query and a query contained by the polygon both collide.
    assert(concave.query_bounds(CollisionChannel::Player, {{-10, -110}, {110, 10}}).size() == 1);
    assert(concave.query_bounds(CollisionChannel::Player, {{10, -20}, {20, -10}}).size() == 1);
    // Channel filtering is still applied before geometry testing.
    assert(concave.query_bounds(CollisionChannel::Shot, {{-10, -110}, {110, 10}}).empty());

}
