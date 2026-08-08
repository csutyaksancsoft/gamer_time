#include "game/match_rules.h"

#include <algorithm>

namespace game {

void ModeVote::begin(std::span<const net::PlayerId> players, std::uint64_t now) {
    voters_.clear();
    for (const auto id : players) if (id != 0) voters_.emplace(id, net::VoteChoice::none);
    deadline_us_ = now + duration_us;
    active_ = true;
}

bool ModeVote::submit(net::PlayerId id, net::VoteChoice choice) {
    if (!active_ || (choice != net::VoteChoice::teams && choice != net::VoteChoice::ffa)) return false;
    const auto found = voters_.find(id);
    if (found == voters_.end() || found->second != net::VoteChoice::none) return false;
    found->second = choice;
    return true;
}

void ModeVote::disconnect(net::PlayerId id) { voters_.erase(id); }
bool ModeVote::complete(std::uint64_t now) const {
    return active_ && now >= deadline_us_;
}
net::GameMode ModeVote::result() const { return teams_count() > ffa_count() ? net::GameMode::teams : net::GameMode::ffa; }
std::uint8_t ModeVote::eligible_count() const { return static_cast<std::uint8_t>(voters_.size()); }
std::uint8_t ModeVote::teams_count() const { return static_cast<std::uint8_t>(std::count_if(voters_.begin(), voters_.end(), [](const auto & p){return p.second==net::VoteChoice::teams;})); }
std::uint8_t ModeVote::ffa_count() const { return static_cast<std::uint8_t>(std::count_if(voters_.begin(), voters_.end(), [](const auto & p){return p.second==net::VoteChoice::ffa;})); }
net::VoteChoice ModeVote::choice(net::PlayerId id) const { const auto found=voters_.find(id);return found==voters_.end()?net::VoteChoice::none:found->second; }

bool projectile_can_hit(net::GameMode mode, bool friendly_fire, net::TeamId owner, net::TeamId target) {
    return mode == net::GameMode::ffa || owner == net::kNoTeam || target == net::kNoTeam || owner != target || friendly_fire;
}
bool kill_is_awarded(net::GameMode mode, net::TeamId killer, net::TeamId victim) {
    return mode == net::GameMode::ffa || killer == net::kNoTeam || victim == net::kNoTeam || killer != victim;
}

} // namespace game
