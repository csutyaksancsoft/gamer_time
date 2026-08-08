#include "render/render_safety.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

int main() {
    using namespace render_safety;

    std::uint64_t capacity = 0;
    assert(next_power_of_two(1000, 512, capacity) && capacity == 1024);
    assert(next_power_of_two(1, 512, capacity) && capacity == 512);
    assert(!next_power_of_two((std::uint64_t{1} << 63) + 1, 1, capacity));

    std::uint64_t bytes = 0;
    assert(checked_instance_bytes(123, 32, bytes) && bytes == 3936);
    assert(!checked_instance_bytes(std::numeric_limits<std::size_t>::max(), 32, bytes));

    assert(valid_draw_range(5, 5, 10));
    assert(valid_draw_range(10, 0, 10));
    assert(!valid_draw_range(8, 3, 10));
    assert(!valid_draw_range(std::numeric_limits<std::uint32_t>::max(), 2, 10));

    const Vec2f snapped = snap_camera({1.24f, -1.26f}, 2.0f);
    assert(std::abs(snapped.x - 1.0f) < 0.001f);
    assert(std::abs(snapped.y + 1.5f) < 0.001f);
    assert(select_zoom_level(1.0f, 1.0f) == 1.5f);
    assert(select_zoom_level(1.0f, -1.0f) == 0.75f);
    assert(select_zoom_level(3.0f, 1.0f) == 3.0f);

    const UvBounds uv = half_texel_uv(1, 2, 2, 32, 32);
    assert(std::abs(uv.min_u - 16.5f / 32.0f) < 0.0001f);
    assert(std::abs(uv.max_u - 31.5f / 32.0f) < 0.0001f);
    assert(std::abs(uv.min_v - 0.5f / 32.0f) < 0.0001f);
    return 0;
}
