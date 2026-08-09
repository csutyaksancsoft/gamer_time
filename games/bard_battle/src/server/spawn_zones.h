#pragma once

#include "core/types.h"
#include "game/collision_world.h"
#include "game/map_world.h"
#include "net/protocol.h"

#include <array>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace server {

struct SpawnOccupant {
    Vec2f position{};
    bool living = true;
};

class SpawnZones {
public:
    static constexpr float kPlayerRadius = 8.0f;
    static constexpr float kPlayerSeparation = 64.0f;

    SpawnZones(const MapWorld & map, const CollisionWorld & collision);

    Vec2f sample(net::GameMode mode, net::TeamId team, std::mt19937 & random,
                 std::span<const SpawnOccupant> occupants = {}) const;

private:
    struct Region {
        std::string layer;
        std::uint32_t object_id = 0;
        std::vector<Vec2f> points;
        float area = 0.0f;
        bool rectangle = false;
        std::vector<Vec2f> valid_anchors;
    };

    const CollisionWorld * collision_ = nullptr;
    std::array<std::vector<Region>, 4> zones_;

    bool valid_position(const Region & region, Vec2f point) const;
    Vec2f random_point(const Region & region, std::mt19937 & random) const;
};

} // namespace server
