#include "game/combat_rules.h"

#include <stdexcept>

int main() {
    const auto require=[](bool condition){if(!condition)throw std::runtime_error("combat rule assertion failed");};
    constexpr std::uint64_t now=1000000;
    const auto beat=game::beat_duration_us(120.0f);
    require(beat==500000);
    require(game::movement_locked(now,now+beat,0,true));
    require(!game::movement_locked(now,now+beat,0,false));
    require(game::movement_locked(now,0,now+beat,false));
    require(!game::movement_locked(now+beat,now+beat,now+beat,true));
    require(game::actions_locked(now,now+beat));
    require(!game::actions_locked(now+beat,now+beat));
    const auto stunned=game::resolve_shield_melee(net::ShieldMeleeMode::stun,now,beat);
    require(!stunned.killed&&stunned.shield_until==0&&stunned.stunned_until==now+2*beat);
    const auto extended=game::resolve_shield_melee(net::ShieldMeleeMode::stun,now,beat,now+3*beat);
    require(extended.stunned_until==now+3*beat);
    const auto killed=game::resolve_shield_melee(net::ShieldMeleeMode::kill,now,beat);
    require(killed.killed&&killed.shield_until==0&&killed.stunned_until==0);
    require(game::kMeleeRadius==32.0f&&game::kMeleeEffectUs==250000);
    for(const Vec2f target:std::initializer_list<Vec2f>{{40,0},{-40,0},{0,40},{0,-40}})require(game::melee_overlaps({0,0},game::kMeleeRadius,target,8.0f));
    require(!game::melee_overlaps({0,0},game::kMeleeRadius,{41,0},8.0f));
}
