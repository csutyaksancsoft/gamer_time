#include "assets/tmx_map_loader.h"
#include "game/fog_of_war_system.h"
#include "game/map_world.h"
#include "game/world.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {

TmxMapAsset make_map(bool with_wall = false) {
    TmxMapAsset asset{};
    asset.width = 65;
    asset.height = 31;
    asset.tile_width = 17;
    asset.tile_height = 19;
    TmxTilesetAsset tileset{};
    tileset.first_gid = 1;
    tileset.name = "fog-test";
    tileset.tile_width = 17;
    tileset.tile_height = 19;
    tileset.tile_count = 1;
    tileset.columns = 1;
    asset.tilesets.push_back(tileset);
    if (with_wall) {
        TmxObjectLayerAsset layer{};
        layer.name = "collision_full";
        TmxObjectAsset wall{};
        wall.id = 1;
        wall.x = 552.5f;
        wall.y = 244.5f;
        wall.width = 16.0f;
        wall.height = 100.0f;
        layer.objects.push_back(wall);
        asset.object_layers.push_back(layer);
        asset.layer_order.push_back({TmxLayerType::Object, 0});
    }
    return asset;
}

bool visible(const World & world, std::uint32_t x, std::uint32_t y) {
    return world.fog_mask()[static_cast<std::size_t>(y) * world.fog_width() + x] == 255;
}

World revealed_from(Vec2f position, float radius = 72.0f) {
    World world;
    world.set_map(MapWorld::from_tmx(make_map()));
    const UnitId unit = world.create_unit({position}, {}, {radius}, {});
    world.set_local_unit(unit);
    FogOfWarSystem{}.update(world);
    return world;
}

} // namespace

int main() {
    World dimensions;
    dimensions.set_map(MapWorld::from_tmx(make_map()));
    // 65*17 by 31*19 pixels, rounded up to complete 8-world-unit cells.
    assert(dimensions.fog_width() == 139);
    assert(dimensions.fog_height() == 74);
    assert(dimensions.fog_mask().size() == 139u * 74u);

    const Vec2f origin = dimensions.map().origin();
    const float right = -origin.x;
    const float top = -origin.y;
    const Vec2f edge_sources[] = {
        {origin.x + 8.0f, 0.0f}, {right - 8.0f, 0.0f},
        {0.0f, origin.y + 8.0f}, {0.0f, top - 8.0f},
    };
    for (const Vec2f source : edge_sources) {
        const World world = revealed_from(source);
        std::uint32_t visible_count = 0;
        std::uint32_t fullest_row = 0;
        std::uint32_t fullest_column = 0;
        for (std::uint32_t y = 0; y < world.fog_height(); ++y) {
            std::uint32_t row_count = 0;
            for (std::uint32_t x = 0; x < world.fog_width(); ++x) {
                row_count += visible(world, x, y) ? 1u : 0u;
            }
            visible_count += row_count;
            fullest_row = std::max(fullest_row, row_count);
        }
        for (std::uint32_t x = 0; x < world.fog_width(); ++x) {
            std::uint32_t column_count = 0;
            for (std::uint32_t y = 0; y < world.fog_height(); ++y) {
                column_count += visible(world, x, y) ? 1u : 0u;
            }
            fullest_column = std::max(fullest_column, column_count);
        }
        assert(visible_count > 0 && visible_count < 320);
        assert(fullest_row < world.fog_width());
        assert(fullest_column < world.fog_height());
        assert(!visible(world, 0, 0));
        assert(!visible(world, world.fog_width() - 1, world.fog_height() - 1));
    }

    const World radius_check = revealed_from({0.0f, 0.0f}, 32.0f);
    assert(visible(radius_check, 69, 36));
    assert(!visible(radius_check, 74, 36));

    World occlusion;
    occlusion.set_map(MapWorld::from_tmx(make_map(true)));
    const UnitId unit = occlusion.create_unit({{-40.0f, 0.0f}}, {}, {112.0f}, {});
    occlusion.set_local_unit(unit);
    FogOfWarSystem{}.update(occlusion);
    assert(visible(occlusion, 67, 36)); // between the source and wall
    assert(!visible(occlusion, 75, 36)); // in range, but behind the wall
}
