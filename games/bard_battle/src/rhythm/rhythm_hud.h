#pragma once

#include "net/protocol.h"
#include "render/render_world.h"

#include <cstdint>
#include <string>
#include <vector>

class RhythmHud {
public:
    void schedule(const net::SongSchedule & schedule, std::uint64_t local_start_us);
    void predict_hit(std::uint64_t now_us, std::int16_t calibration_ms);
    void apply_result(const net::RhythmResult & result, std::uint64_t now_us);
    void append_instances(RenderBatch & batch, int width, int height, std::uint64_t now_us) const;
    std::string feedback(std::uint64_t now_us) const;
    const net::SongSchedule & schedule_state() const { return schedule_; }

private:
    net::SongSchedule schedule_{};
    std::uint64_t local_start_us_ = 0;
    std::vector<std::uint8_t> note_states_; // 0 upcoming, 1 pending, 2 judged
    net::RhythmResult last_result_{};
    std::uint64_t feedback_time_us_ = 0;
    bool scheduled_ = false;
    std::uint32_t pending_note_ = UINT32_MAX;
};
