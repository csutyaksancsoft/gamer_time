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
    if (found == voters_.end()) return false;
    if (found->second == choice) return true;
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
void SongVote::begin(std::span<const net::PlayerId> ids,std::span<const std::string> candidates,std::uint64_t now){candidates_.assign(candidates.begin(),candidates.end());voters_.clear();for(auto id:ids)if(id)voters_[id]=0xff;deadline_us_=now+ModeVote::duration_us;active_=true;}
bool SongVote::submit(net::PlayerId id,std::uint8_t choice){auto it=voters_.find(id);if(!active_||it==voters_.end()||choice>=candidates_.size())return false;it->second=choice;return true;}
void SongVote::disconnect(net::PlayerId id){voters_.erase(id);}
std::uint8_t SongVote::choice(net::PlayerId id)const{auto it=voters_.find(id);return it==voters_.end()?0xff:it->second;}
std::vector<std::uint8_t> SongVote::totals()const{std::vector<std::uint8_t> out(candidates_.size());for(auto [id,v]:voters_){(void)id;if(v<out.size())++out[v];}return out;}
std::string SongVote::result(std::mt19937&rng)const{auto votes=totals();const auto high=*std::max_element(votes.begin(),votes.end());std::vector<std::size_t> tied;for(std::size_t i=0;i<votes.size();++i)if(votes[i]==high)tied.push_back(i);std::uniform_int_distribution<std::size_t>d(0,tied.size()-1);return candidates_[tied[d(rng)]];}

bool projectile_can_hit(net::GameMode mode, bool friendly_fire, net::TeamId owner, net::TeamId target) {
    return mode == net::GameMode::ffa || owner == net::kNoTeam || target == net::kNoTeam || owner != target || friendly_fire;
}
bool kill_is_awarded(net::GameMode mode, net::TeamId killer, net::TeamId victim) {
    return mode == net::GameMode::ffa || killer == net::kNoTeam || victim == net::kNoTeam || killer != victim;
}

} // namespace game
