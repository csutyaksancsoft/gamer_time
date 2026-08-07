#pragma once

#include "rhythm/song_config.h"

#include <SDL3/SDL_audio.h>

#include <cstdint>
#include <string>
#include <vector>

class SongPlayer {
public:
    ~SongPlayer();
    bool load(const std::string & audio_directory, const SongConfig & config);
    void schedule(std::uint64_t local_start_us);
    void update(std::uint64_t now_us);
    void stop();
    bool playing() const { return playing_; }
    std::int64_t playhead_ms(std::uint64_t now_us) const;
    const std::string & error() const { return error_; }
private:
    SDL_AudioStream * stream_ = nullptr;
    SDL_AudioSpec spec_{};
    std::vector<std::uint8_t> pcm_;
    std::uint64_t scheduled_start_us_ = 0;
    std::uint64_t actual_start_us_ = 0;
    bool armed_ = false;
    bool playing_ = false;
    std::string error_;
};
