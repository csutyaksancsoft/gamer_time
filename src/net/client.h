#pragma once

#include "net/protocol.h"

#include <enet/enet.h>

#include <cstdint>
#include <deque>
#include <string>

namespace net {

class Client {
public:
    Client() = default;
    ~Client();
    void connect(const std::string & endpoint, const std::string & name);
    void disconnect();
    void update();
    void send_input(std::int8_t x, std::int8_t y);
    void send_rhythm_hit(std::int16_t calibration_ms, Vec2f aim, RhythmAction action);

    bool connected() const { return connected_; }
    bool welcomed() const { return player_id_ != 0; }
    PlayerId player_id() const { return player_id_; }
    Vec2f spawn() const { return spawn_; }
    const Snapshot & snapshot() const { return snapshot_; }
    std::uint32_t input_sequence() const { return input_sequence_; }
    std::int64_t server_offset_us() const { return server_offset_us_; }
    std::uint64_t server_time_us() const { return static_cast<std::uint64_t>(static_cast<std::int64_t>(monotonic_time_us()) + server_offset_us_); }
    bool take_song_schedule(SongSchedule & schedule);
    bool take_rhythm_result(RhythmResult & result);
    bool take_sound_event(SoundEvent & event);
    const std::string & status() const { return status_; }

private:
    void begin_connection();
    void send(const std::vector<std::uint8_t> & bytes, bool reliable);
    void receive(std::span<const std::uint8_t> bytes);
    ENetHost * host_ = nullptr;
    ENetPeer * peer_ = nullptr;
    bool connected_ = false;
    bool enet_initialized_ = false;
    PlayerId player_id_ = 0;
    Vec2f spawn_{};
    Snapshot snapshot_{};
    std::uint32_t input_sequence_ = 0;
    std::uint32_t rhythm_sequence_ = 0;
    std::uint64_t last_ping_us_ = 0;
    std::int64_t server_offset_us_ = 0;
    std::uint64_t best_rtt_us_ = UINT64_MAX;
    bool has_schedule_ = false;
    SongSchedule schedule_{};
    std::deque<RhythmResult> rhythm_results_;
    std::deque<SoundEvent> sound_events_;
    std::uint32_t last_sound_event_id_ = 0;
    std::string name_;
    ENetAddress address_{};
    std::uint64_t reconnect_deadline_us_ = 0;
    std::uint64_t next_reconnect_us_ = 0;
    std::string status_ = "disconnected";
};

} // namespace net
