#pragma once

#include "core/types.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace net {

constexpr std::uint32_t kProtocolVersion = 5;
constexpr std::size_t kMaxRhythmNotes = 4096;
constexpr std::size_t kMaxProjectiles = 1024;
constexpr std::uint16_t kDefaultPort = 27020;
constexpr std::size_t kMaxPlayers = 64;
constexpr float kMoveSpeed = 160.0f;

using PlayerId = std::uint32_t;

enum class MessageType : std::uint8_t {
    hello = 1,
    welcome,
    input,
    snapshot,
    clock_ping,
    clock_pong,
    ready,
    song_schedule,
    rhythm_hit,
    rhythm_result,
};

enum class RoomState : std::uint8_t { lobby, countdown, playing, free_move };
enum class RhythmGrade : std::uint8_t { perfect, good, miss };
enum class RhythmAction : std::uint8_t { shoot, shield };

struct PlayerState {
    PlayerId id = 0;
    Vec2f position{};
    Vec2f velocity{};
    std::uint32_t acknowledged_input = 0;
    std::uint32_t color = 0xffffffffu;
    std::string name;
    bool alive = true;
    float facing_angle = 0.0f;
    std::uint64_t respawn_at_us = 0;
    std::uint64_t protected_until_us = 0;
    std::uint64_t shield_until_us = 0;
};

struct ProjectileState {
    std::uint32_t id = 0;
    PlayerId owner_id = 0;
    Vec2f position{};
    Vec2f velocity{};
    float angle = 0.0f;
};

struct Snapshot {
    std::uint64_t server_time_us = 0;
    std::uint32_t server_tick = 0;
    std::vector<PlayerState> players;
    std::vector<ProjectileState> projectiles;
};

struct SongSchedule {
    std::uint64_t server_start_us = 0;
    std::uint32_t duration_ms = 0;
    float bpm = 120.0f;
    std::int32_t first_beat_ms = 0;
    std::uint16_t subdivision = 1;
    std::string song_id;
    std::vector<std::uint32_t> note_times_ms;
};

struct RhythmResult {
    RhythmGrade grade = RhythmGrade::miss;
    std::int32_t offset_ms = 0;
    std::uint32_t perfect = 0;
    std::uint32_t good = 0;
    std::uint32_t miss = 0;
    std::uint32_t note_index = UINT32_MAX;
    std::uint32_t combo = 0;
    std::uint32_t max_combo = 0;
    bool overstrum = false;
    bool shot_fired = false;
    bool shield_activated = false;
};

class Writer {
public:
    explicit Writer(MessageType type);
    void u8(std::uint8_t value);
    void u16(std::uint16_t value);
    void u32(std::uint32_t value);
    void u64(std::uint64_t value);
    void i16(std::int16_t value);
    void i32(std::int32_t value);
    void f32(float value);
    void string(std::string_view value, std::size_t max_length);
    const std::vector<std::uint8_t> & bytes() const { return bytes_; }
private:
    std::vector<std::uint8_t> bytes_;
};

class Reader {
public:
    explicit Reader(std::span<const std::uint8_t> bytes);
    MessageType type();
    std::uint8_t u8();
    std::uint16_t u16();
    std::uint32_t u32();
    std::uint64_t u64();
    std::int16_t i16();
    std::int32_t i32();
    float f32();
    std::string string(std::size_t max_length);
    bool finished() const { return offset_ == bytes_.size(); }
private:
    void require(std::size_t count) const;
    std::span<const std::uint8_t> bytes_;
    std::size_t offset_ = 0;
};

std::vector<std::uint8_t> make_hello(std::string_view name);
std::vector<std::uint8_t> make_welcome(PlayerId id, Vec2f spawn);
std::vector<std::uint8_t> make_input(std::uint32_t sequence, std::int8_t x, std::int8_t y, std::uint64_t client_time_us);
std::vector<std::uint8_t> make_snapshot(const Snapshot & snapshot);
Snapshot read_snapshot(Reader & reader);
std::vector<std::uint8_t> make_clock_ping(std::uint64_t client_send_us);
std::vector<std::uint8_t> make_clock_pong(std::uint64_t client_send_us, std::uint64_t server_receive_us, std::uint64_t server_send_us);
std::vector<std::uint8_t> make_ready();
std::vector<std::uint8_t> make_song_schedule(const SongSchedule & schedule);
SongSchedule read_song_schedule(Reader & reader);
std::vector<std::uint8_t> make_rhythm_hit(std::uint32_t sequence, std::uint64_t client_time_us, std::int16_t calibration_ms, Vec2f aim, RhythmAction action);
std::vector<std::uint8_t> make_rhythm_result(const RhythmResult & result);
RhythmResult read_rhythm_result(Reader & reader);

std::uint64_t monotonic_time_us();
const char * grade_name(RhythmGrade grade);

} // namespace net
