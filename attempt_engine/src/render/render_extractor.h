#pragma once

#include "render/render_world.h"

#include <string>
#include <cstdint>

class World;

class RenderExtractor {
public:
    RenderWorld build(
        const World & world,
        const CameraState & camera,
        bool show_collision_debug,
        std::string overlay_text,
        std::uint64_t animation_time_ms = 0
    ) const;
};
