#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct SongConfig {
    std::string id = "main";
    std::string file = "song.wav";
    float bpm = 120.0f;
    std::int32_t first_beat_ms = 0;
    std::uint16_t subdivision = 1;
    std::uint32_t duration_ms = 0;
};

SongConfig load_song_config(const std::string & path);
std::vector<std::uint32_t> generate_beat_grid(const SongConfig & config);
