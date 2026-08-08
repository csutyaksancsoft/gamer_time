#pragma once

#include "core/types.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace render_safety {

inline constexpr std::array<float, 6> kZoomLevels{0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f};

inline float select_zoom_level(float current, float wheel_delta) {
    std::size_t nearest = 0;
    for (std::size_t i = 1; i < kZoomLevels.size(); ++i) {
        if (std::abs(kZoomLevels[i] - current) < std::abs(kZoomLevels[nearest] - current)) nearest = i;
    }
    if (wheel_delta > 0.0f && nearest + 1 < kZoomLevels.size()) ++nearest;
    if (wheel_delta < 0.0f && nearest > 0) --nearest;
    return kZoomLevels[nearest];
}

inline Vec2f snap_camera(Vec2f position, float zoom) {
    if (!(zoom > 0.0f) || !std::isfinite(zoom)) return position;
    return {std::round(position.x * zoom) / zoom, std::round(position.y * zoom) / zoom};
}

inline bool checked_instance_bytes(std::size_t count, std::size_t stride, std::uint64_t & bytes) {
    if (stride != 0 && count > std::numeric_limits<std::uint64_t>::max() / stride) return false;
    bytes = static_cast<std::uint64_t>(count) * stride;
    return true;
}

inline bool next_power_of_two(std::uint64_t required, std::uint64_t minimum, std::uint64_t & capacity) {
    required = required < minimum ? minimum : required;
    if (required > (std::uint64_t{1} << 63)) return false;
    capacity = 1;
    while (capacity < required) capacity <<= 1;
    return true;
}

inline bool valid_draw_range(std::uint32_t offset, std::uint32_t count, std::size_t total) {
    return offset <= total && count <= total - offset;
}

struct UvBounds { float min_u, min_v, max_u, max_v; };

inline UvBounds half_texel_uv(std::uint32_t index, std::uint32_t columns, std::uint32_t rows,
                              std::uint32_t atlas_width, std::uint32_t atlas_height) {
    const std::uint32_t col = columns ? index % columns : 0;
    const std::uint32_t row = columns ? index / columns : 0;
    const float tile_w = columns ? static_cast<float>(atlas_width) / columns : 1.0f;
    const float tile_h = rows ? static_cast<float>(atlas_height) / rows : 1.0f;
    return {(col * tile_w + 0.5f) / atlas_width, (row * tile_h + 0.5f) / atlas_height,
            ((col + 1) * tile_w - 0.5f) / atlas_width, ((row + 1) * tile_h - 0.5f) / atlas_height};
}

} // namespace render_safety
