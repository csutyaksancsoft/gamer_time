#pragma once

#include "render/render_world.h"

#include <string>

class World;

class RenderExtractor {
public:
    RenderWorld build(
        const World & world,
        const CameraState & camera,
        bool show_collision_debug,
        std::string overlay_text
    ) const;
};
