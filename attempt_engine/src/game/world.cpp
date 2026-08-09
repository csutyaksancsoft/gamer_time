#include "game/world.h"
#include "game/fog_config.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace {

constexpr UnitId kInvalidUnitId = static_cast<UnitId>(-1);
constexpr std::uint32_t kFogWidth = 64;
constexpr std::uint32_t kFogHeight = 64;

} // namespace

World::World()
    : fog_mask_(static_cast<std::size_t>(kFogWidth * kFogHeight), 0) {
}

void World::set_map(MapWorld map) {
    map_ = std::move(map);
    collision_ = CollisionWorld::from_map(map_);

    const auto cells_for_extent = [](std::uint32_t tiles, float tile_size) {
        const double pixels = static_cast<double>(tiles) * static_cast<double>(tile_size);
        if (pixels <= 0.0) {
            return std::uint32_t{0};
        }
        const double cells = std::ceil(pixels / static_cast<double>(fog::kCellWorldSize));
        if (cells > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
            throw std::overflow_error("Map is too large for the fog mask");
        }
        return static_cast<std::uint32_t>(cells);
    };

    const Vec2f tile_size = map_.tile_size();
    fog_width_ = cells_for_extent(map_.width(), tile_size.x);
    fog_height_ = cells_for_extent(map_.height(), tile_size.y);
    const std::uint64_t fog_cell_count =
        static_cast<std::uint64_t>(fog_width_) * static_cast<std::uint64_t>(fog_height_);
    if (fog_cell_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::overflow_error("Fog mask is too large");
    }
    fog_mask_.assign(static_cast<std::size_t>(fog_cell_count), 0);
}

void World::seed_test_units() {
    if (!unit_ids_.empty()) {
        return;
    }

    create_unit({{-160.0f, -80.0f}}, {49, {16.0f, 16.0f}}, {96.0f}, {});
    create_unit({{-60.0f, 0.0f}}, {50, {16.0f, 16.0f}}, {96.0f}, {});
    create_unit({{60.0f, 70.0f}}, {98, {16.0f, 16.0f}}, {112.0f}, {});
    create_unit({{150.0f, -20.0f}}, {116, {16.0f, 16.0f}}, {112.0f}, {});
}

void World::replace_replicated_units(const std::vector<ReplicatedUnitState> & network_units) {
    unit_ids_.clear();
    transforms_.clear();
    renders_.clear();
    visions_.clear();
    units_.clear();
    selected_units_.clear();

    UnitId maximum_id = 0;
    for (const ReplicatedUnitState & state : network_units) {
        maximum_id = std::max(maximum_id, state.id);
    }
    const std::size_t count = network_units.empty() ? 0 : static_cast<std::size_t>(maximum_id) + 1;
    unit_ids_.reserve(network_units.size());
    transforms_.resize(count);
    renders_.resize(count);
    visions_.resize(count);
    units_.resize(count);
    for (const ReplicatedUnitState & state : network_units) {
        unit_ids_.push_back(state.id);
        transforms_[state.id] = {state.position};
        renders_[state.id].sprite_index=state.sprite_index;
        renders_[state.id].atlas_span=state.atlas_span;
        renders_[state.id].footprint=state.size;
        renders_[state.id].rotation_radians=state.rotation_radians;
        renders_[state.id].transform_flags=state.transform_flags;
        renders_[state.id].solid_color=state.solid_color;
        renders_[state.id].circle_outline=state.circle_outline;
        std::copy(std::begin(state.color),std::end(state.color),std::begin(renders_[state.id].color));
        visions_[state.id] = {state.vision_radius};
    }
}

UnitId World::create_unit(
    TransformComponent transform,
    RenderComponent render,
    VisionComponent vision,
    UnitComponent unit
) {
    const UnitId unit_id = static_cast<UnitId>(unit_ids_.size());
    unit_ids_.push_back(unit_id);
    transforms_.push_back(transform);
    renders_.push_back(render);
    visions_.push_back(vision);
    units_.push_back(unit);
    return unit_id;
}

void World::select_single(UnitId unit_id) {
    clear_selection();
    if (UnitComponent * unit = try_unit(unit_id)) {
        unit->selected = true;
        selected_units_.push_back(unit_id);
    }
}

void World::clear_selection() {
    for (UnitId unit_id : selected_units_) {
        if (UnitComponent * unit = try_unit(unit_id)) {
            unit->selected = false;
        }
    }
    selected_units_.clear();
}

TransformComponent * World::try_transform(UnitId unit_id) {
    const std::size_t index = to_index(unit_id);
    return index == static_cast<std::size_t>(kInvalidUnitId) ? nullptr : &transforms_[index];
}

const TransformComponent * World::try_transform(UnitId unit_id) const {
    const std::size_t index = to_index(unit_id);
    return index == static_cast<std::size_t>(kInvalidUnitId) ? nullptr : &transforms_[index];
}

RenderComponent * World::try_render(UnitId unit_id) {
    const std::size_t index = to_index(unit_id);
    return index == static_cast<std::size_t>(kInvalidUnitId) ? nullptr : &renders_[index];
}

const RenderComponent * World::try_render(UnitId unit_id) const {
    const std::size_t index = to_index(unit_id);
    return index == static_cast<std::size_t>(kInvalidUnitId) ? nullptr : &renders_[index];
}

VisionComponent * World::try_vision(UnitId unit_id) {
    const std::size_t index = to_index(unit_id);
    return index == static_cast<std::size_t>(kInvalidUnitId) ? nullptr : &visions_[index];
}

const VisionComponent * World::try_vision(UnitId unit_id) const {
    const std::size_t index = to_index(unit_id);
    return index == static_cast<std::size_t>(kInvalidUnitId) ? nullptr : &visions_[index];
}

UnitComponent * World::try_unit(UnitId unit_id) {
    const std::size_t index = to_index(unit_id);
    return index == static_cast<std::size_t>(kInvalidUnitId) ? nullptr : &units_[index];
}

const UnitComponent * World::try_unit(UnitId unit_id) const {
    const std::size_t index = to_index(unit_id);
    return index == static_cast<std::size_t>(kInvalidUnitId) ? nullptr : &units_[index];
}

std::size_t World::to_index(UnitId unit_id) const {
    if (unit_id >= transforms_.size()) {
        return static_cast<std::size_t>(kInvalidUnitId);
    }
    return static_cast<std::size_t>(unit_id);
}
