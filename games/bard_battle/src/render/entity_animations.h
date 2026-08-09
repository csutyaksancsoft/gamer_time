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

enum class Role : std::uint8_t { player_1, player_2, player_3, player_4, projectile, shield, melee, count };

struct Frame { std::uint32_t tile_id=0, duration_ms=0, atlas_index=0; };
struct Strip {
    Role role=Role::player_1;
    std::filesystem::path tsx_path, image_path;
    std::uint32_t source_width=0, source_height=0, tile_count=0, columns=0, margin=0, spacing=0;
    Vec2f world_size{};
    std::vector<Frame> frames;
};

class Catalog {
public:
    static Catalog load(const std::filesystem::path & directory);
    const Strip * find(Role role) const;
    std::vector<std::string> pack(AtlasAsset & atlas, LoadedImage & image);
    const std::vector<std::string> & warnings() const { return warnings_; }
private:
    std::array<std::optional<Strip>,static_cast<std::size_t>(Role::count)> strips_{};
    std::vector<std::string> warnings_;
};

std::uint32_t frame_index(const Strip & strip, std::uint64_t elapsed_us, bool loop);
std::uint8_t player_variant(std::uint8_t team, std::uint32_t color, bool teams);

} // namespace entity_animation
