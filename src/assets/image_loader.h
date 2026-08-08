#pragma once

#include "assets/atlas_asset.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

struct TmxMapAsset;

struct LoadedImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba_pixels;

    bool empty() const {
        return width == 0 || height == 0 || rgba_pixels.empty();
    }
};

struct RuntimeAtlas {
    AtlasAsset atlas;
    LoadedImage image;
};

namespace assets {

LoadedImage load_png_rgba(const std::string & image_path);
void append_bottom_row_sprites(LoadedImage & destination, const LoadedImage & source, std::uint32_t sprite_count);
void append_packed_sprites(LoadedImage & destination, const LoadedImage & source, std::uint32_t sprite_count,
                           std::uint32_t tile_size, std::uint32_t columns, std::uint32_t first_index);
RuntimeAtlas build_runtime_atlas(const TmxMapAsset & map);

} // namespace assets
