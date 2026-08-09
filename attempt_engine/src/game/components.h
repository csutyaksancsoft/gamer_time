#pragma once

#include "core/types.h"

#include <cstdint>
#include <vector>

using UnitId = std::uint32_t;

struct TransformComponent {
    Vec2f position{};
};

struct RenderComponent {
    std::uint32_t sprite_index = 0;
    Vec2f footprint{24.0f, 24.0f};
    std::uint32_t atlas_span = 0x00010001u;
    float rotation_radians = 0.0f;
    std::uint32_t transform_flags = 0;
    bool solid_color = false;
    bool circle_outline = false;
    float color[4]{1.0f,1.0f,1.0f,1.0f};
};

struct VisionComponent {
    float radius = 96.0f;
};

struct UnitComponent {
    bool selected = false;
    bool has_move_target = false;
    Vec2f move_target{};
};

struct MoveCommand {
    std::vector<UnitId> units;
    Vec2f destination{};
};
