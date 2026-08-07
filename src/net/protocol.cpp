#include "net/protocol.h"

#include "core/error.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>

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
    for (std::size_t i = 0; i < std::min(s.players.size(), kMaxPlayers); ++i) { const auto & p=s.players[i]; w.u32(p.id); w.f32(p.position.x); w.f32(p.position.y); w.f32(p.velocity.x); w.f32(p.velocity.y); w.u32(p.acknowledged_input); w.u32(p.color); w.string(p.name,24); }
    return w.bytes();
}
Snapshot read_snapshot(Reader & r) {
    Snapshot s{}; s.server_time_us=r.u64(); s.server_tick=r.u32(); const auto count=r.u8(); if(count>kMaxPlayers) fail("Too many players in snapshot"); s.players.reserve(count);
    for(std::uint8_t i=0;i<count;++i){ PlayerState p{}; p.id=r.u32(); p.position={r.f32(),r.f32()}; p.velocity={r.f32(),r.f32()}; p.acknowledged_input=r.u32(); p.color=r.u32(); p.name=r.string(24); s.players.push_back(std::move(p)); } return s;
}
std::vector<std::uint8_t> make_clock_ping(std::uint64_t t) { Writer w(MessageType::clock_ping); w.u64(t); return w.bytes(); }
std::vector<std::uint8_t> make_clock_pong(std::uint64_t c, std::uint64_t r, std::uint64_t s) { Writer w(MessageType::clock_pong); w.u64(c); w.u64(r); w.u64(s); return w.bytes(); }
std::vector<std::uint8_t> make_ready() { Writer w(MessageType::ready); return w.bytes(); }
std::vector<std::uint8_t> make_song_schedule(const SongSchedule & s) { Writer w(MessageType::song_schedule); w.u64(s.server_start_us); w.u32(s.duration_ms); w.f32(s.bpm); w.i32(s.first_beat_ms); w.u16(s.subdivision); w.string(s.song_id, 64); const auto count=static_cast<std::uint32_t>(std::min(s.note_times_ms.size(),kMaxRhythmNotes)); w.u32(count); for(std::uint32_t i=0;i<count;++i)w.u32(s.note_times_ms[i]); return w.bytes(); }
SongSchedule read_song_schedule(Reader & r) { SongSchedule s{}; s.server_start_us=r.u64(); s.duration_ms=r.u32(); s.bpm=r.f32(); s.first_beat_ms=r.i32(); s.subdivision=r.u16(); s.song_id=r.string(64); const auto count=r.u32(); if(count>kMaxRhythmNotes)fail("Too many rhythm notes"); s.note_times_ms.reserve(count); for(std::uint32_t i=0;i<count;++i){const auto t=r.u32();if(t>s.duration_ms||(!s.note_times_ms.empty()&&t<=s.note_times_ms.back()))fail("Invalid rhythm note chart");s.note_times_ms.push_back(t);} return s; }
std::vector<std::uint8_t> make_rhythm_hit(std::uint32_t seq, std::uint64_t t, std::int16_t calibration) { Writer w(MessageType::rhythm_hit); w.u32(seq); w.u64(t); w.i16(calibration); return w.bytes(); }
std::vector<std::uint8_t> make_rhythm_result(const RhythmResult & r) { Writer w(MessageType::rhythm_result); w.u8(static_cast<std::uint8_t>(r.grade)); w.i32(r.offset_ms); w.u32(r.perfect); w.u32(r.good); w.u32(r.miss); w.u32(r.note_index); w.u32(r.combo); w.u32(r.max_combo); w.u8(r.overstrum?1:0); return w.bytes(); }
RhythmResult read_rhythm_result(Reader & r) { RhythmResult v{}; v.grade=static_cast<RhythmGrade>(r.u8()); v.offset_ms=r.i32(); v.perfect=r.u32(); v.good=r.u32(); v.miss=r.u32(); v.note_index=r.u32(); v.combo=r.u32(); v.max_combo=r.u32(); v.overstrum=r.u8()!=0; return v; }
std::uint64_t monotonic_time_us() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
const char * grade_name(RhythmGrade grade) { switch(grade){case RhythmGrade::perfect:return "Perfect";case RhythmGrade::good:return "Good";default:return "Miss";} }

} // namespace net
