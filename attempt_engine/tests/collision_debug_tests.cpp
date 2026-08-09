#include "assets/tmx_map_loader.h"
#include "game/map_world.h"
#include "game/world.h"
#include "render/batch_builder.h"
#include "render/render_extractor.h"

#include <cassert>
#include <string>

int main() {
    const std::string fixture = std::string(ATTEMPT_ENGINE_SOURCE_ROOT) + "/attempt_engine/tests/fixtures/collision_channels.tmx";
    World world;
    world.set_map(MapWorld::from_tmx(assets::load_tmx_map(fixture)));

    const RenderWorld visible = RenderExtractor{}.build(world, {}, true, {});
    // Three rectangles (four edges) and three triangles (three edges), across all channels.
    assert(visible.debug_quads.size() == 21);
    for (const RenderDebugQuad & quad : visible.debug_quads) {
        assert((quad.flags & kInstanceFlagDebugCollision) != 0);
    }
    assert(RenderExtractor{}.build(world, {}, false, {}).debug_quads.empty());

    RenderWorld phases{};
    for (TileRenderPhase phase : {TileRenderPhase::BelowUnits, TileRenderPhase::AboveUnits,
                                  TileRenderPhase::BelowUnits, TileRenderPhase::AboveUnits}) {
        RenderTileLayer layer{};
        layer.render_phase = phase;
        layer.tiles.push_back({});
        phases.terrain_layers.push_back(layer);
    }
    phases.projected_units.push_back({});
    const RenderBatch batch = BatchBuilder{}.build(phases);
    assert(batch.terrain_layer_ranges.size() == 4);
    assert(batch.terrain_layer_ranges[0].render_phase == TileRenderPhase::BelowUnits);
    assert(batch.terrain_layer_ranges[1].render_phase == TileRenderPhase::AboveUnits);
    assert(batch.terrain_layer_ranges[2].render_phase == TileRenderPhase::BelowUnits);
    assert(batch.terrain_layer_ranges[3].render_phase == TileRenderPhase::AboveUnits);
    assert(batch.unit_instance_offset == 4 && batch.unit_instance_count == 1);
}
