#pragma once

#include "net/protocol.h"

#include <algorithm>
#include <cstdint>

namespace game {

constexpr float kMeleeRadius = 32.0f;
constexpr std::uint64_t kMeleeEffectUs = 250000;

inline std::uint64_t beat_duration_us(float bpm) {
    return static_cast<std::uint64_t>(60000000.0f / std::max(bpm, 1.0f));
}

inline bool actions_locked(std::uint64_t now, std::uint64_t stunned_until) {
    return now < stunned_until;
}

inline bool movement_locked(std::uint64_t now, std::uint64_t shield_until,
                            std::uint64_t stunned_until, bool shield_freeze) {
    return actions_locked(now, stunned_until) || (shield_freeze && now < shield_until);
}

struct ShieldMeleeResolution {
    bool killed = false;
    std::uint64_t shield_until = 0;
    std::uint64_t stunned_until = 0;
};

inline ShieldMeleeResolution resolve_shield_melee(net::ShieldMeleeMode mode,
                                                   std::uint64_t now,
                                                   std::uint64_t beat_us,
                                                   std::uint64_t existing_stun = 0) {
    if (mode == net::ShieldMeleeMode::kill) return {true, 0, 0};
    return {false, 0, std::max(existing_stun, now + 2 * beat_us)};
}

inline bool melee_overlaps(Vec2f center, float radius, Vec2f target, float target_radius) {
    const float combined = radius + target_radius;
    return length_squared(target - center) <= combined * combined;
}

} // namespace game
