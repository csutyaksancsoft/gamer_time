#pragma once

#include "core/types.h"
#include "net/protocol.h"

#include <SDL3/SDL_audio.h>

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

class EffectPlayer {
public:
    ~EffectPlayer();
    void load(const std::string & root_directory);
    void play(net::SoundCue cue, Vec2f listener, Vec2f source, bool participant);
    void update(std::uint64_t now_us);
    void shutdown();
    const std::string & diagnostic() const { return diagnostic_; }
private:
    struct Clip { std::vector<float> samples; };
    struct Pool { std::vector<Clip> clips; std::size_t last = static_cast<std::size_t>(-1); };
    struct Voice { SDL_AudioStream * stream = nullptr; std::uint64_t end_us = 0; };
    std::size_t choose(Pool & pool);
    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec output_spec_{SDL_AUDIO_F32,2,48000};
    std::array<Pool,10> pools_{};
    std::vector<Voice> voices_;
    std::mt19937 random_{std::random_device{}()};
    std::string diagnostic_;
};
