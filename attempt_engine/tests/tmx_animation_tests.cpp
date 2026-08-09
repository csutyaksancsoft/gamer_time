#include "assets/tmx_map_loader.h"
#include "game/map_world.h"

#include <cassert>
#include <filesystem>
#include <fstream>

int main() {
    const auto root = std::filesystem::temp_directory_path() / "attempt_engine_tmx_animation_test";
    std::filesystem::create_directories(root / "tiles");
    {
        std::ofstream tsx(root / "tiles" / "animated.tsx");
        tsx << R"(<tileset name="animated" tilewidth="16" tileheight="16" tilecount="3" columns="3" margin="1" spacing="2" objectalignment="bottomleft">
 <tileoffset x="2" y="3"/><image source="animated.png" width="54" height="18"/>
 <tile id="1"><animation><frame tileid="0" duration="100"/><frame tileid="2" duration="250"/></animation></tile>
</tileset>)";
    }
    {
        std::ofstream tsx(root / "tiles" / "second.tsx");
        tsx << R"(<tileset name="second" tilewidth="16" tileheight="16" tilecount="2" columns="2"><image source="second.png" width="32" height="16"/></tileset>)";
    }
    std::ofstream(root / "tiles" / "animated.png").put('\0');
    std::ofstream(root / "tiles" / "second.png").put('\0');
    const std::uint32_t transformed = 10u | assets::kTmxFlipHorizontal | assets::kTmxFlipDiagonal;
    {
        std::ofstream tmx(root / "map.tmx");
        tmx << R"(<map orientation="orthogonal" renderorder="right-down" width="2" height="1" tilewidth="16" tileheight="16">
 <tileset firstgid="1" source="tiles/animated.tsx"/><tileset firstgid="10" source="tiles/second.tsx"/>
 <layer id="1" name="ground" width="2" height="1"><properties><property name="render_phase" value="above_units"/></properties><data encoding="csv">2,)" << transformed << R"(</data></layer>
 <layer id="4" name="ordinary" width="2" height="1"><data encoding="csv">0,0</data></layer>
 <layer id="5" name="unknown-phase" width="2" height="1"><properties><property name="render_phase" value="sky"/></properties><data encoding="csv">0,0</data></layer>
 <objectgroup id="2" name="props" draworder="topdown" opacity="0.5"><object id="7" x="8" y="16" width="32" height="24" rotation="30" gid="2"/></objectgroup>
 <objectgroup id="3" name="collision_full"><object id="8" x="0" y="0"><polygon points="0,0 16,0 16,16"/></object></objectgroup>
</map>)";
    }

    const TmxMapAsset asset = assets::load_tmx_map((root / "map.tmx").string());
    assert(asset.tilesets.size() == 2);
    assert(asset.tilesets[0].margin == 1 && asset.tilesets[0].spacing == 2);
    assert(asset.tilesets[0].animations.size() == 1);
    assert(asset.object_layers[0].draw_order == "topdown");
    assert(assets::resolve_tmx_tileset_image_path(asset, 0) == (root / "tiles" / "animated.png").string());

    const MapWorld map = MapWorld::from_tmx(asset);
    const TileLayer & ground = *map.find_tile_layer("ground");
    assert(ground.atlas_indices[0] == 1);
    assert(ground.atlas_indices[1] == 3);
    assert(ground.transform_flags[1] == (assets::kTmxFlipHorizontal | assets::kTmxFlipDiagonal));
    assert(ground.render_phase == TileRenderPhase::AboveUnits);
    assert(map.find_tile_layer("ordinary")->render_phase == TileRenderPhase::BelowUnits);
    assert(map.find_tile_layer("unknown-phase")->render_phase == TileRenderPhase::BelowUnits);
    assert(map.resolve_animated_index(1, 0) == 0);
    assert(map.resolve_animated_index(1, 99) == 0);
    assert(map.resolve_animated_index(1, 100) == 2);
    assert(map.resolve_animated_index(1, 350) == 0);
    const MapObject & prop = map.find_object_layer("props")->objects[0];
    assert(prop.is_tile && prop.atlas_index == 1 && prop.opacity == 0.5f);
    assert(prop.size.x == 32 && prop.size.y == 24 && prop.rotation == 30);
    assert(prop.position.x == 10 && prop.position.y == 1); // centered map origin + anchor/alignment/offset
    std::filesystem::remove_all(root);
}
