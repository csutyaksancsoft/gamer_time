#include "net/protocol.h"

#include "core/error.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
#include <cmath>

namespace net {

Writer::Writer(MessageType type) { u8(static_cast<std::uint8_t>(type)); }
void Writer::u8(std::uint8_t value) { bytes_.push_back(value); }
void Writer::u16(std::uint16_t value) { u8(value & 0xff); u8((value >> 8) & 0xff); }
void Writer::u32(std::uint32_t value) { for (int i = 0; i < 4; ++i) u8((value >> (i * 8)) & 0xff); }
void Writer::u64(std::uint64_t value) { for (int i = 0; i < 8; ++i) u8((value >> (i * 8)) & 0xff); }
void Writer::i16(std::int16_t value) { u16(static_cast<std::uint16_t>(value)); }
void Writer::i32(std::int32_t value) { u32(static_cast<std::uint32_t>(value)); }
void Writer::f32(float value) { u32(std::bit_cast<std::uint32_t>(value)); }
void Writer::string(std::string_view value, std::size_t max_length) {
    const auto length = static_cast<std::uint8_t>(std::min({value.size(), max_length, std::size_t{255}}));
    u8(length);
    bytes_.insert(bytes_.end(), value.begin(), value.begin() + length);
}

Reader::Reader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}
void Reader::require(std::size_t count) const { if (offset_ + count > bytes_.size()) fail("Truncated network message"); }
MessageType Reader::type() { return static_cast<MessageType>(u8()); }
std::uint8_t Reader::u8() { require(1); return bytes_[offset_++]; }
std::uint16_t Reader::u16() { std::uint16_t v = u8(); v |= std::uint16_t(u8()) << 8; return v; }
std::uint32_t Reader::u32() { std::uint32_t v = 0; for (int i = 0; i < 4; ++i) v |= std::uint32_t(u8()) << (i * 8); return v; }
std::uint64_t Reader::u64() { std::uint64_t v = 0; for (int i = 0; i < 8; ++i) v |= std::uint64_t(u8()) << (i * 8); return v; }
std::int16_t Reader::i16() { return static_cast<std::int16_t>(u16()); }
std::int32_t Reader::i32() { return static_cast<std::int32_t>(u32()); }
float Reader::f32() { return std::bit_cast<float>(u32()); }
std::string Reader::string(std::size_t max_length) {
    const std::size_t length = u8();
    if (length > max_length) fail("Oversized network string");
    require(length);
    std::string result(reinterpret_cast<const char *>(bytes_.data() + offset_), length);
    offset_ += length;
    return result;
}

std::vector<std::uint8_t> make_hello(std::string_view name) { Writer w(MessageType::hello); w.u32(kProtocolVersion); w.string(name, 24); return w.bytes(); }
std::vector<std::uint8_t> make_welcome(PlayerId id, Vec2f spawn) { Writer w(MessageType::welcome); w.u32(id); w.f32(spawn.x); w.f32(spawn.y); return w.bytes(); }
std::vector<std::uint8_t> make_input(std::uint32_t seq, std::int8_t x, std::int8_t y, std::uint64_t time) { Writer w(MessageType::input); w.u32(seq); w.u8(static_cast<std::uint8_t>(x)); w.u8(static_cast<std::uint8_t>(y)); w.u64(time); return w.bytes(); }
std::vector<std::uint8_t> make_snapshot(const Snapshot & s) {
    Writer w(MessageType::snapshot); w.u64(s.server_time_us); w.u32(s.server_tick); w.u8(static_cast<std::uint8_t>(std::min(s.players.size(), kMaxPlayers)));
    for (std::size_t i = 0; i < std::min(s.players.size(), kMaxPlayers); ++i) { const auto & p=s.players[i]; w.u32(p.id); w.f32(p.position.x); w.f32(p.position.y); w.f32(p.velocity.x); w.f32(p.velocity.y); w.u32(p.acknowledged_input); w.u32(p.color); w.string(p.name,24); w.u8(p.alive?1:0); w.f32(p.facing_angle); w.u64(p.respawn_at_us); w.u64(p.protected_until_us);w.u64(p.shield_until_us);w.u64(p.stunned_until_us);w.u8(p.team);w.u8(static_cast<std::uint8_t>(p.vote));w.u8(p.song_vote);w.u32(p.round_kills);w.u32(p.round_deaths);w.u32(p.session_kills);w.u32(p.session_deaths); }
    w.u16(static_cast<std::uint16_t>(std::min(s.projectiles.size(), kMaxProjectiles)));
    for(std::size_t i=0;i<std::min(s.projectiles.size(),kMaxProjectiles);++i){const auto & p=s.projectiles[i];w.u32(p.id);w.u32(p.owner_id);w.f32(p.position.x);w.f32(p.position.y);w.f32(p.velocity.x);w.f32(p.velocity.y);w.f32(p.angle);}
    w.u16(static_cast<std::uint16_t>(std::min(s.melee_effects.size(),kMaxMeleeEffects)));
    for(std::size_t i=0;i<std::min(s.melee_effects.size(),kMaxMeleeEffects);++i){const auto & m=s.melee_effects[i];w.u32(m.id);w.u32(m.owner_id);w.f32(m.position.x);w.f32(m.position.y);w.f32(m.radius);w.u64(m.expires_at_us);}
    w.u8(static_cast<std::uint8_t>(s.room));w.u8(static_cast<std::uint8_t>(s.mode));w.u64(s.vote_deadline_us);w.u8(s.eligible_voters);w.u8(s.teams_votes);w.u8(s.ffa_votes);w.u8(s.team_count);w.u8(s.friendly_fire?1:0);w.u8(s.shield_freeze?1:0);w.u8(static_cast<std::uint8_t>(s.shield_melee));w.u8(s.mode_vote_enabled?1:0);w.u8(s.song_vote_enabled?1:0);const auto songs=std::min(s.song_candidates.size(),kMaxSongCandidates);w.u8(static_cast<std::uint8_t>(songs));for(std::size_t i=0;i<songs;++i){w.string(s.song_candidates[i],64);w.u8(i<s.song_votes.size()?s.song_votes[i]:0);}w.string(s.selected_song_id,64);
    return w.bytes();
}
Snapshot read_snapshot(Reader & r) {
    Snapshot s{}; s.server_time_us=r.u64(); s.server_tick=r.u32(); const auto count=r.u8(); if(count>kMaxPlayers) fail("Too many players in snapshot"); s.players.reserve(count);
    const auto finite=[](Vec2f v){return std::isfinite(v.x)&&std::isfinite(v.y);};
    for(std::uint8_t i=0;i<count;++i){ PlayerState p{}; p.id=r.u32(); p.position={r.f32(),r.f32()}; p.velocity={r.f32(),r.f32()}; p.acknowledged_input=r.u32(); p.color=r.u32(); p.name=r.string(24);p.alive=r.u8()!=0;p.facing_angle=r.f32();p.respawn_at_us=r.u64();p.protected_until_us=r.u64();p.shield_until_us=r.u64();p.stunned_until_us=r.u64();p.team=r.u8();const auto vote=r.u8();p.song_vote=r.u8();if(p.team>kMaxTeam||vote>static_cast<std::uint8_t>(VoteChoice::ffa))fail("Invalid player team or vote");p.vote=static_cast<VoteChoice>(vote);p.round_kills=r.u32();p.round_deaths=r.u32();p.session_kills=r.u32();p.session_deaths=r.u32();if(!finite(p.position)||!finite(p.velocity)||!std::isfinite(p.facing_angle))fail("Non-finite player state");s.players.push_back(std::move(p)); }
    const auto projectile_count=r.u16();if(projectile_count>kMaxProjectiles)fail("Too many projectiles in snapshot");s.projectiles.reserve(projectile_count);for(std::uint16_t i=0;i<projectile_count;++i){ProjectileState p{};p.id=r.u32();p.owner_id=r.u32();p.position={r.f32(),r.f32()};p.velocity={r.f32(),r.f32()};p.angle=r.f32();if(!finite(p.position)||!finite(p.velocity)||!std::isfinite(p.angle))fail("Non-finite projectile state");s.projectiles.push_back(p);}const auto melee_count=r.u16();if(melee_count>kMaxMeleeEffects)fail("Too many melee effects in snapshot");s.melee_effects.reserve(melee_count);for(std::uint16_t i=0;i<melee_count;++i){MeleeEffectState m{};m.id=r.u32();m.owner_id=r.u32();m.position={r.f32(),r.f32()};m.radius=r.f32();m.expires_at_us=r.u64();if(!finite(m.position)||!std::isfinite(m.radius)||m.radius<=0.0f)fail("Invalid melee effect");s.melee_effects.push_back(m);}const auto room=r.u8(),mode=r.u8();if(room>static_cast<std::uint8_t>(RoomState::free_move)||mode>static_cast<std::uint8_t>(GameMode::teams))fail("Invalid match state");s.room=static_cast<RoomState>(room);s.mode=static_cast<GameMode>(mode);s.vote_deadline_us=r.u64();s.eligible_voters=r.u8();s.teams_votes=r.u8();s.ffa_votes=r.u8();s.team_count=r.u8();const auto friendly=r.u8(),freeze=r.u8(),shield_melee=r.u8(),mode_enabled=r.u8(),song_enabled=r.u8();if(friendly>1||freeze>1||shield_melee>static_cast<std::uint8_t>(ShieldMeleeMode::kill)||mode_enabled>1||song_enabled>1)fail("Invalid match flags");s.friendly_fire=friendly!=0;s.shield_freeze=freeze!=0;s.shield_melee=static_cast<ShieldMeleeMode>(shield_melee);s.mode_vote_enabled=mode_enabled!=0;s.song_vote_enabled=song_enabled!=0;const auto sc=r.u8();if(sc>kMaxSongCandidates)fail("Too many song candidates");std::uint16_t song_total=0;for(std::uint8_t i=0;i<sc;++i){const auto id=r.string(64);if(id.empty()||std::find(s.song_candidates.begin(),s.song_candidates.end(),id)!=s.song_candidates.end())fail("Invalid song candidate ID");s.song_candidates.push_back(id);s.song_votes.push_back(r.u8());song_total+=s.song_votes.back();}s.selected_song_id=r.string(64);if(s.team_count<2||s.team_count>4||s.teams_votes+s.ffa_votes>s.eligible_voters||song_total>s.eligible_voters)fail("Invalid vote state");for(const auto&p:s.players)if(p.song_vote!=0xff&&p.song_vote>=sc)fail("Invalid player song vote");return s;
}
std::vector<std::uint8_t> make_clock_ping(std::uint64_t t) { Writer w(MessageType::clock_ping); w.u64(t); return w.bytes(); }
std::vector<std::uint8_t> make_clock_pong(std::uint64_t c, std::uint64_t r, std::uint64_t s) { Writer w(MessageType::clock_pong); w.u64(c); w.u64(r); w.u64(s); return w.bytes(); }
std::vector<std::uint8_t> make_ready() { Writer w(MessageType::ready); return w.bytes(); }
std::vector<std::uint8_t> make_song_schedule(const SongSchedule & s) { Writer w(MessageType::song_schedule); w.u64(s.server_start_us); w.u32(s.duration_ms); w.f32(s.bpm); w.i32(s.first_beat_ms); w.u16(s.subdivision); w.string(s.song_id, 64); const auto count=static_cast<std::uint32_t>(std::min(s.note_times_ms.size(),kMaxRhythmNotes)); w.u32(count); for(std::uint32_t i=0;i<count;++i)w.u32(s.note_times_ms[i]); return w.bytes(); }
SongSchedule read_song_schedule(Reader & r) { SongSchedule s{}; s.server_start_us=r.u64(); s.duration_ms=r.u32(); s.bpm=r.f32(); s.first_beat_ms=r.i32(); s.subdivision=r.u16(); s.song_id=r.string(64); const auto count=r.u32(); if(count>kMaxRhythmNotes)fail("Too many rhythm notes"); s.note_times_ms.reserve(count); for(std::uint32_t i=0;i<count;++i){const auto t=r.u32();if(t>s.duration_ms||(!s.note_times_ms.empty()&&t<=s.note_times_ms.back()))fail("Invalid rhythm note chart");s.note_times_ms.push_back(t);} return s; }
std::vector<std::uint8_t> make_rhythm_hit(std::uint32_t seq, std::uint64_t t, std::int16_t calibration, Vec2f aim, RhythmAction action) { Writer w(MessageType::rhythm_hit); w.u32(seq); w.u64(t); w.i16(calibration);w.f32(aim.x);w.f32(aim.y);w.u8(static_cast<std::uint8_t>(action));return w.bytes(); }
std::vector<std::uint8_t> make_rhythm_result(const RhythmResult & r) { Writer w(MessageType::rhythm_result); w.u8(static_cast<std::uint8_t>(r.grade)); w.i32(r.offset_ms); w.u32(r.perfect); w.u32(r.good); w.u32(r.miss); w.u32(r.note_index); w.u32(r.combo); w.u32(r.max_combo); w.u8(r.overstrum?1:0);w.u8(r.shot_fired?1:0);w.u8(r.shield_activated?1:0);w.u8(r.melee_fired?1:0);return w.bytes(); }
RhythmResult read_rhythm_result(Reader & r) { RhythmResult v{}; v.grade=static_cast<RhythmGrade>(r.u8());if(v.grade>RhythmGrade::miss)fail("Invalid rhythm grade"); v.offset_ms=r.i32(); v.perfect=r.u32(); v.good=r.u32(); v.miss=r.u32(); v.note_index=r.u32(); v.combo=r.u32(); v.max_combo=r.u32();const auto overstrum=r.u8(),shot=r.u8(),shield=r.u8(),melee=r.u8();if(overstrum>1||shot>1||shield>1||melee>1)fail("Invalid rhythm result flags");v.overstrum=overstrum!=0;v.shot_fired=shot!=0;v.shield_activated=shield!=0;v.melee_fired=melee!=0;return v; }
std::vector<std::uint8_t> make_sound_event(const SoundEvent & e){Writer w(MessageType::sound_event);w.u32(e.id);w.u8(static_cast<std::uint8_t>(e.cue));w.u32(e.source_id);w.u32(e.target_id);w.f32(e.position.x);w.f32(e.position.y);w.u64(e.server_time_us);w.u8(e.participant?1:0);return w.bytes();}
SoundEvent read_sound_event(Reader & r){SoundEvent e{};e.id=r.u32();const auto cue=r.u8();if(cue>static_cast<std::uint8_t>(SoundCue::shield_failure))fail("Invalid sound cue");e.cue=static_cast<SoundCue>(cue);e.source_id=r.u32();e.target_id=r.u32();e.position={r.f32(),r.f32()};e.server_time_us=r.u64();e.participant=r.u8()!=0;if(!std::isfinite(e.position.x)||!std::isfinite(e.position.y))fail("Non-finite sound event");return e;}
std::vector<std::uint8_t> make_vote_request(VoteChoice choice){if(choice!=VoteChoice::teams&&choice!=VoteChoice::ffa)fail("Invalid vote choice");Writer w(MessageType::vote_request);w.u8(static_cast<std::uint8_t>(choice));return w.bytes();}
VoteChoice read_vote_request(Reader & r){const auto value=r.u8();if(value<static_cast<std::uint8_t>(VoteChoice::teams)||value>static_cast<std::uint8_t>(VoteChoice::ffa))fail("Invalid vote choice");return static_cast<VoteChoice>(value);}
std::vector<std::uint8_t> make_song_vote_request(std::uint8_t candidate){if(candidate>=kMaxSongCandidates)fail("Invalid song candidate");Writer w(MessageType::song_vote_request);w.u8(candidate);return w.bytes();}
std::uint8_t read_song_vote_request(Reader&r){const auto candidate=r.u8();if(candidate>=kMaxSongCandidates)fail("Invalid song candidate");return candidate;}
std::vector<std::uint8_t> make_team_request(TeamId team){if(team<kFirstTeam||team>kMaxTeam)fail("Invalid team");Writer w(MessageType::team_request);w.u8(team);return w.bytes();}
TeamId read_team_request(Reader & r){const auto team=r.u8();if(team<kFirstTeam||team>kMaxTeam)fail("Invalid team");return team;}
bool valid_team(TeamId team,std::uint8_t count){return count>=2&&count<=4&&team>=kFirstTeam&&team<=count;}
bool team_switching_allowed(RoomState room){return room==RoomState::lobby||room==RoomState::voting||room==RoomState::free_move;}
const char * sound_cue_name(SoundCue cue){switch(cue){case SoundCue::death:return "death";case SoundCue::shot_success:return "shot_success";case SoundCue::shot_failure:return "shot_failure";case SoundCue::shot_hit_player:return "shot_hit_player";case SoundCue::shot_hit_shield:return "shot_hit_shield";case SoundCue::shield_success:return "shield_success";case SoundCue::shield_failure:return "shield_failure";}return "unknown";}
std::uint64_t monotonic_time_us() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
const char * grade_name(RhythmGrade grade) { switch(grade){case RhythmGrade::perfect:return "Perfect";case RhythmGrade::good:return "Good";default:return "Miss";} }

} // namespace net
