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

} // namespace assets
