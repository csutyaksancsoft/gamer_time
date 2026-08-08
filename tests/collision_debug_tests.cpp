#include "assets/tmx_map_loader.h"
#include "game/map_world.h"
#include "game/world.h"
#include "render/render_extractor.h"

#include <cassert>
#include <string>

int main() {
    const std::string fixture = std::string(GT_SOURCE_DIR) + "/tests/fixtures/collision_channels.tmx";
    World world;
    world.set_map(MapWorld::from_tmx(assets::load_tmx_map(fixture)));

    const RenderWorld visible = RenderExtractor{}.build(world, {}, true, {});
    // Three rectangles (four edges) and three triangles (three edges), across all channels.
    assert(visible.debug_quads.size() == 21);
    for (const RenderDebugQuad & quad : visible.debug_quads) {
        assert((quad.flags & kInstanceFlagDebugCollision) != 0);
    }
    assert(RenderExtractor{}.build(world, {}, false, {}).debug_quads.empty());
}
