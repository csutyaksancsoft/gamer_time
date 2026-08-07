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

#include <limits>

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

} // namespace assets
