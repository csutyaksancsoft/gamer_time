#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

struct SongConfig {
    std::string id = "main";
    std::string file = "song.wav";
    float bpm = 120.0f;
    std::int32_t first_beat_ms = 0;
    std::uint16_t subdivision = 1;
    std::uint32_t duration_ms = 0;
};

class SongCatalog {
public:
    static SongCatalog load(const std::string & directory);
    const SongConfig * find(const std::string & id) const;
    const std::vector<SongConfig> & songs() const { return songs_; }
private:
    std::vector<SongConfig> songs_;
    std::unordered_map<std::string, std::size_t> by_id_;
};

SongConfig load_song_config(const std::string & path);
std::vector<std::uint32_t> generate_beat_grid(const SongConfig & config);
