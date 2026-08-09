#pragma once

#include "core/types.h"
#include "net/protocol.h"

#include <SDL3/SDL_audio.h>

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

class AudioPlaybackDevice;

class EffectPlayer {
public:
    ~EffectPlayer();
    void load(const AudioPlaybackDevice & device, const std::string & root_directory);
    void play(net::SoundCue cue, Vec2f listener, Vec2f source, bool participant);
    void update(std::uint64_t now_us);
    void shutdown();
    const std::string & diagnostic() const { return diagnostic_; }
    std::size_t loaded_clip_count() const { return loaded_clip_count_; }
    std::size_t populated_pool_count() const { return populated_pool_count_; }
    std::uint64_t request_count() const { return request_count_; }
    std::uint64_t queued_count() const { return queued_count_; }
    const std::string & last_cue() const { return last_cue_; }
private:
    struct Clip { std::vector<float> samples; };
    struct Pool { std::vector<Clip> clips; std::size_t last = static_cast<std::size_t>(-1); };
    struct Voice { SDL_AudioStream * stream = nullptr; std::uint64_t end_us = 0; };
    std::size_t choose(Pool & pool);
    SDL_AudioDeviceID device_ = 0; // non-owning
    SDL_AudioSpec output_spec_{SDL_AUDIO_F32,2,48000};
    std::array<Pool,10> pools_{};
    std::vector<Voice> voices_;
    std::mt19937 random_{std::random_device{}()};
    std::string diagnostic_;
    std::string last_cue_ = "none";
    std::size_t loaded_clip_count_ = 0;
    std::size_t populated_pool_count_ = 0;
    std::uint64_t request_count_ = 0;
    std::uint64_t queued_count_ = 0;
};
