#pragma once

#include "net/protocol.h"

#include <cstdint>
#include <cstdlib>

namespace rhythm {
inline net::RhythmGrade grade_offset(std::int32_t offset_ms) {
    const auto absolute=std::abs(offset_ms);
    if(absolute<=35)return net::RhythmGrade::perfect;
    if(absolute<=80)return net::RhythmGrade::good;
    return net::RhythmGrade::miss;
}
inline bool note_expired(std::int64_t song_time_ms,std::uint32_t note_time_ms){return song_time_ms>static_cast<std::int64_t>(note_time_ms)+80;}
}
