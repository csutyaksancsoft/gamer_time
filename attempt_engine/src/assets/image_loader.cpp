#include "assets/image_loader.h"

#include <SDL3/SDL.h>

#define STB_IMAGE_STATIC
#define STBI_NO_THREAD_LOCALS
#define STBI_ONLY_PNG
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_STDIO
#define STBI_MALLOC(size) SDL_malloc(size)
#define STBI_REALLOC(pointer, size) SDL_realloc(pointer, size)
#define STBI_FREE(pointer) SDL_free(pointer)
#define STB_IMAGE_IMPLEMENTATION
#include "../../external/SDL/src/video/stb_image.h"

#include "core/error.h"
#include "assets/tmx_map_loader.h"

#include <limits>
#include <cstring>
#include <cmath>

namespace assets {

LoadedImage load_png_rgba(const std::string & image_path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::size_t encoded_size = 0;
    void * encoded_data = SDL_LoadFile(image_path.c_str(), &encoded_size);
    if (!encoded_data) {
        fail("Failed to read atlas image: " + image_path);
    }
    if (encoded_size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        SDL_free(encoded_data);
        fail("Atlas image is too large: " + image_path);
    }

    stbi_uc * pixels = stbi_load_from_memory(
        static_cast<const stbi_uc *>(encoded_data),
        static_cast<int>(encoded_size),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );
    SDL_free(encoded_data);
    if (pixels == nullptr) {
        fail("Failed to load atlas image: " + image_path);
    }

    LoadedImage image{};
    image.width = static_cast<std::uint32_t>(width);
    image.height = static_cast<std::uint32_t>(height);
    image.rgba_pixels.assign(pixels, pixels + (static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u));
    stbi_image_free(pixels);
    return image;
}

void append_bottom_row_sprites(LoadedImage & destination, const LoadedImage & source, std::uint32_t sprite_count) {
    if (destination.empty() || source.empty() || sprite_count == 0 || source.width % sprite_count != 0) {
        fail("Invalid placeholder atlas layout");
    }
    const std::uint32_t source_tile = source.width / sprite_count;
    if (source.height < source_tile || destination.width < sprite_count * 16u) {
        fail("Placeholder sprites do not fit the scene atlas");
    }
    constexpr std::uint32_t destination_tile = 16;
    const std::uint32_t old_height = destination.height;
    destination.height += destination_tile;
    destination.rgba_pixels.resize(static_cast<std::size_t>(destination.width) * destination.height * 4u, 0);
    const std::uint32_t source_y = source.height - source_tile;
    for (std::uint32_t sprite = 0; sprite < sprite_count; ++sprite) {
        for (std::uint32_t y = 0; y < destination_tile; ++y) {
            for (std::uint32_t x = 0; x < destination_tile; ++x) {
                const std::uint32_t sx = sprite * source_tile + x * source_tile / destination_tile;
                const std::uint32_t sy = source_y + y * source_tile / destination_tile;
                const std::size_t source_offset = (static_cast<std::size_t>(sy) * source.width + sx) * 4u;
                const std::size_t destination_offset = (static_cast<std::size_t>(old_height + y) * destination.width + sprite * destination_tile + x) * 4u;
                for (std::size_t channel = 0; channel < 4; ++channel) {
                    destination.rgba_pixels[destination_offset + channel] = source.rgba_pixels[source_offset + channel];
                }
            }
        }
    }
}

void append_packed_sprites(LoadedImage & destination, const LoadedImage & source, std::uint32_t sprite_count,
                           std::uint32_t tile_size, std::uint32_t columns, std::uint32_t first_index) {
    if (destination.empty() || source.empty() || sprite_count == 0 || columns == 0 ||
        source.width % sprite_count != 0 || destination.width != columns * tile_size) fail("Invalid packed sprite atlas layout");
    const std::uint32_t source_tile = source.width / sprite_count;
    if (source.height < source_tile) fail("Packed source sprites are invalid");
    const std::uint32_t needed_rows = (first_index + sprite_count + columns - 1u) / columns;
    if (needed_rows > destination.height / tile_size) {
        destination.height = needed_rows * tile_size;
        destination.rgba_pixels.resize(static_cast<std::size_t>(destination.width) * destination.height * 4u, 0);
    }
    const std::uint32_t source_y = source.height - source_tile;
    for (std::uint32_t sprite = 0; sprite < sprite_count; ++sprite) {
        const std::uint32_t index = first_index + sprite;
        const std::uint32_t dx = (index % columns) * tile_size;
        const std::uint32_t dy = (index / columns) * tile_size;
        for (std::uint32_t y = 0; y < tile_size; ++y) for (std::uint32_t x = 0; x < tile_size; ++x) {
            const std::uint32_t sx = sprite * source_tile + x * source_tile / tile_size;
            const std::uint32_t sy = source_y + y * source_tile / tile_size;
            const std::size_t src = (static_cast<std::size_t>(sy) * source.width + sx) * 4u;
            const std::size_t dst = (static_cast<std::size_t>(dy + y) * destination.width + dx + x) * 4u;
            std::memcpy(destination.rgba_pixels.data() + dst, source.rgba_pixels.data() + src, 4u);
        }
    }
}

RuntimeAtlas build_runtime_atlas(const TmxMapAsset & map) {
    if (map.tilesets.empty() || map.tile_width == 0 || map.tile_height == 0) fail("Cannot build an atlas from an empty TMX map");
    std::uint64_t total = 0;
    for (const auto & ts : map.tilesets) {
        if (ts.tile_width != map.tile_width || ts.tile_height != map.tile_height || ts.tile_count == 0 || ts.columns == 0) {
            fail("Tileset tile dimensions do not match the map: " + ts.name);
        }
        total += ts.tile_count;
    }
    if (total == 0 || total > std::numeric_limits<std::uint32_t>::max() ||
        total * map.tile_width > std::numeric_limits<std::uint32_t>::max()) fail("Runtime atlas dimensions overflow");

    RuntimeAtlas result{};
    result.atlas = AtlasAsset::from_grid_image("<combined-tmx-atlas>", map.tile_width, map.tile_height);
    result.atlas.logical_tile_count = static_cast<std::uint32_t>(total);
    result.atlas.columns = static_cast<std::uint32_t>(std::ceil(std::sqrt(static_cast<double>(total))));
    result.atlas.rows = (static_cast<std::uint32_t>(total) + result.atlas.columns - 1u) / result.atlas.columns;
    result.image.width = result.atlas.columns * map.tile_width;
    result.image.height = result.atlas.rows * map.tile_height;
    result.image.rgba_pixels.resize(static_cast<std::size_t>(result.image.width) * result.image.height * 4u, 0);

    std::uint32_t packed = 0;
    for (std::size_t set_index = 0; set_index < map.tilesets.size(); ++set_index) {
        const auto & ts = map.tilesets[set_index];
        const LoadedImage source = load_png_rgba(resolve_tmx_tileset_image_path(map, set_index));
        const std::uint64_t required_width = static_cast<std::uint64_t>(ts.margin) * 2u +
            static_cast<std::uint64_t>(ts.columns) * ts.tile_width + static_cast<std::uint64_t>(ts.columns - 1u) * ts.spacing;
        const std::uint32_t rows = (ts.tile_count + ts.columns - 1u) / ts.columns;
        const std::uint64_t required_height = static_cast<std::uint64_t>(ts.margin) * 2u +
            static_cast<std::uint64_t>(rows) * ts.tile_height + static_cast<std::uint64_t>(rows - 1u) * ts.spacing;
        if (source.width < required_width || source.height < required_height) fail("Tileset image geometry is invalid: " + ts.name);
        for (std::uint32_t local = 0; local < ts.tile_count; ++local, ++packed) {
            const std::uint32_t sx = ts.margin + (local % ts.columns) * (ts.tile_width + ts.spacing);
            const std::uint32_t sy = ts.margin + (local / ts.columns) * (ts.tile_height + ts.spacing);
            for (std::uint32_t y = 0; y < ts.tile_height; ++y) {
                const std::size_t src = (static_cast<std::size_t>(sy + y) * source.width + sx) * 4u;
                const std::uint32_t dx = (packed % result.atlas.columns) * ts.tile_width;
                const std::uint32_t dy = (packed / result.atlas.columns) * ts.tile_height;
                const std::size_t dst = (static_cast<std::size_t>(dy + y) * result.image.width + dx) * 4u;
                std::memcpy(result.image.rgba_pixels.data() + dst, source.rgba_pixels.data() + src, static_cast<std::size_t>(ts.tile_width) * 4u);
            }
        }
    }
    return result;
}

} // namespace assets
