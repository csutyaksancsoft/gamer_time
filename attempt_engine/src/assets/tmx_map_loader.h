#pragma once

#include "assets/atlas_asset.h"

#include <cstdint>
#include <string>
#include <vector>

struct TmxProperty {
    std::string name;
    std::string type = "string";
    std::string value;
};

struct TmxPolygonPoint {
    float x = 0.0f;
    float y = 0.0f;
};

struct TmxPolygon {
    std::vector<TmxPolygonPoint> points;
};

struct TmxAnimationFrame {
    std::uint32_t tile_id = 0;
    std::uint32_t duration_ms = 0;
};

struct TmxTileAnimation {
    std::uint32_t tile_id = 0;
    std::vector<TmxAnimationFrame> frames;
};

struct TmxTilesetAsset {
    std::uint32_t first_gid = 1;
    std::string name;
    std::uint32_t tile_width = 0;
    std::uint32_t tile_height = 0;
    std::uint32_t tile_count = 0;
    std::uint32_t columns = 0;
    std::uint32_t margin = 0;
    std::uint32_t spacing = 0;
    std::int32_t tile_offset_x = 0;
    std::int32_t tile_offset_y = 0;
    std::string object_alignment = "unspecified";
    std::string source_path;
    std::string image_source;
    std::uint32_t image_width = 0;
    std::uint32_t image_height = 0;
    std::vector<TmxProperty> properties;
    std::vector<TmxTileAnimation> animations;
};

struct TmxLayerAsset {
    std::uint32_t id = 0;
    std::string name;
    bool visible = true;
    float opacity = 1.0f;
    std::vector<TmxProperty> properties;
};

struct TmxTileLayerAsset : TmxLayerAsset {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint32_t> gids;
};

struct TmxObjectAsset {
    std::uint32_t id = 0;
    std::string name;
    std::string type;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float rotation = 0.0f;
    bool visible = true;
    std::uint32_t gid = 0;
    bool has_polygon = false;
    bool is_point = false;
    TmxPolygon polygon;
    std::vector<TmxProperty> properties;
};

struct TmxObjectLayerAsset : TmxLayerAsset {
    std::string draw_order = "topdown";
    std::vector<TmxObjectAsset> objects;
};

enum class TmxLayerType {
    Tile,
    Object,
};

struct TmxLayerRef {
    TmxLayerType type = TmxLayerType::Tile;
    std::size_t index = 0;
};

struct TmxMapAsset {
    std::string map_path;
    std::string orientation;
    std::string render_order;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t tile_width = 0;
    std::uint32_t tile_height = 0;
    std::uint32_t next_layer_id = 0;
    std::uint32_t next_object_id = 0;
    std::vector<TmxProperty> properties;
    std::vector<TmxTilesetAsset> tilesets;
    std::vector<TmxTileLayerAsset> tile_layers;
    std::vector<TmxObjectLayerAsset> object_layers;
    std::vector<TmxLayerRef> layer_order;
};

namespace assets {

TmxMapAsset load_tmx_map(const std::string & map_path);
std::string resolve_tmx_tileset_image_path(const TmxMapAsset & map, std::size_t tileset_index = 0);
AtlasAsset build_atlas_from_tmx(const TmxMapAsset & map, const std::string & image_path, std::size_t tileset_index = 0);

constexpr std::uint32_t kTmxFlipHorizontal = 0x80000000u;
constexpr std::uint32_t kTmxFlipVertical = 0x40000000u;
constexpr std::uint32_t kTmxFlipDiagonal = 0x20000000u;
constexpr std::uint32_t kTmxHexRotate120 = 0x10000000u;
constexpr std::uint32_t kTmxTransformMask = kTmxFlipHorizontal | kTmxFlipVertical | kTmxFlipDiagonal;
constexpr std::uint32_t kTmxGidMask = ~(kTmxTransformMask | kTmxHexRotate120);

} // namespace assets
