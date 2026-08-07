#pragma once

#include "game/command_queue.h"
#include "game/collision_world.h"
#include "game/components.h"
#include "game/map_world.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class World {
public:
    World();

    void seed_test_units();
    void replace_network_units(const std::vector<struct NetworkUnitState> & units);
    void set_map(MapWorld map);

    UnitId create_unit(
        TransformComponent transform,
        RenderComponent render,
        VisionComponent vision,
        UnitComponent unit
    );

    std::size_t unit_count() const {
        return unit_ids_.size();
    }

    const std::vector<UnitId> & unit_ids() const {
        return unit_ids_;
    }

    const std::vector<UnitId> & selected_units() const {
        return selected_units_;
    }

    void select_single(UnitId unit_id);
    void clear_selection();

    TransformComponent * try_transform(UnitId unit_id);
    const TransformComponent * try_transform(UnitId unit_id) const;
    RenderComponent * try_render(UnitId unit_id);
    const RenderComponent * try_render(UnitId unit_id) const;
    VisionComponent * try_vision(UnitId unit_id);
    const VisionComponent * try_vision(UnitId unit_id) const;
    UnitComponent * try_unit(UnitId unit_id);
    const UnitComponent * try_unit(UnitId unit_id) const;

    CommandQueue & command_queue() {
        return command_queue_;
    }

    const std::vector<std::uint8_t> & fog_mask() const {
        return fog_mask_;
    }

    std::vector<std::uint8_t> & fog_mask() {
        return fog_mask_;
    }

    std::uint32_t fog_width() const {
        return fog_width_;
    }

    std::uint32_t fog_height() const {
        return fog_height_;
    }

    const MapWorld & map() const {
        return map_;
    }

    MapWorld & map() {
        return map_;
    }

    const CollisionWorld & collision() const {
        return collision_;
    }

    void set_local_unit(UnitId unit_id) { local_unit_id_ = unit_id; }
    UnitId local_unit() const { return local_unit_id_; }

private:
    std::size_t to_index(UnitId unit_id) const;

    std::vector<UnitId> unit_ids_;
    std::vector<TransformComponent> transforms_;
    std::vector<RenderComponent> renders_;
    std::vector<VisionComponent> visions_;
    std::vector<UnitComponent> units_;
    std::vector<UnitId> selected_units_;
    CommandQueue command_queue_;
    std::vector<std::uint8_t> fog_mask_;
    std::uint32_t fog_width_ = 64;
    std::uint32_t fog_height_ = 64;
    MapWorld map_;
    CollisionWorld collision_;
    UnitId local_unit_id_ = static_cast<UnitId>(-1);
};

struct NetworkUnitState {
    UnitId id = 0;
    Vec2f position{};
    std::uint32_t sprite_index = 49;
    Vec2f size{16.0f,16.0f};
    float rotation_radians = 0.0f;
    bool solid_color = false;
    float opacity = 1.0f;
    float color[4]{1.0f,1.0f,1.0f,1.0f};
};
