#pragma once

#include "assets/atlas_asset.h"
#include "assets/image_loader.h"
#include "core/types.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace entity_animation {

enum class PlayerAnimation : std::uint8_t { death, idle, running, attack, melee, shield_break, shield, count };
enum class Role : std::uint8_t { projectile, melee, count };

struct Frame { std::uint32_t tile_id=0, duration_ms=0, atlas_index=0; };
struct Strip {
    std::filesystem::path tsx_path, image_path;
    std::uint32_t source_width=0, source_height=0, tile_count=0, columns=0, margin=0, spacing=0;
    Vec2f world_size{};
    std::uint16_t atlas_span_x=1, atlas_span_y=1;
    std::vector<Frame> frames;
};

class Catalog {
public:
    static Catalog load(const std::filesystem::path & directory);
    const Strip * find_player(std::uint8_t player, PlayerAnimation animation) const;
    const Strip * find_player_exact(std::uint8_t player, PlayerAnimation animation) const;
    const Strip * find(Role role) const;
    std::vector<std::string> pack(AtlasAsset & atlas, LoadedImage & image);
    const std::vector<std::string> & warnings() const { return warnings_; }
private:
    std::array<std::array<std::optional<Strip>,static_cast<std::size_t>(PlayerAnimation::count)>,4> players_{};
    std::array<std::optional<Strip>,static_cast<std::size_t>(Role::count)> effects_{};
    std::vector<std::string> warnings_;
};

std::uint32_t frame_index(const Strip & strip, std::uint64_t elapsed_us, bool loop);
std::uint64_t duration_us(const Strip & strip);
float melee_rotation_radians(std::uint64_t elapsed_us, std::uint64_t duration_us);
std::uint8_t player_folder(std::uint32_t player_id, std::uint8_t team, bool teams);

} // namespace entity_animation
