#pragma once

#include "net/protocol.h"

#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace game {

struct MatchParticipant {
    net::PlayerId id = 0;
    net::VoteChoice vote = net::VoteChoice::none;
    bool eligible = false;
};

class ModeVote {
public:
    static constexpr std::uint64_t duration_us = 15'000'000;
    void begin(std::span<const net::PlayerId> active_players, std::uint64_t now_us);
    bool submit(net::PlayerId player, net::VoteChoice choice);
    void disconnect(net::PlayerId player);
    bool complete(std::uint64_t now_us) const;
    net::GameMode result() const;
    bool active() const { return active_; }
    std::uint64_t deadline_us() const { return deadline_us_; }
    std::uint8_t eligible_count() const;
    std::uint8_t teams_count() const;
    std::uint8_t ffa_count() const;
    net::VoteChoice choice(net::PlayerId player) const;
    void finish() { active_ = false; }
private:
    bool active_ = false;
    std::uint64_t deadline_us_ = 0;
    std::unordered_map<net::PlayerId, net::VoteChoice> voters_;
};

bool projectile_can_hit(net::GameMode mode, bool friendly_fire, net::TeamId owner_team, net::TeamId target_team);
bool kill_is_awarded(net::GameMode mode, net::TeamId killer_team, net::TeamId victim_team);

} // namespace game
