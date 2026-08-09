#pragma once

#include <SDL3/SDL_audio.h>

#include <string>

class AudioPlaybackDevice {
public:
    ~AudioPlaybackDevice();

    bool initialize();
    void shutdown();

    SDL_AudioDeviceID id() const { return device_; }
    bool ready() const { return device_ != 0; }
    const std::string & diagnostic() const { return diagnostic_; }

private:
    SDL_AudioDeviceID device_ = 0;
    std::string diagnostic_;
};
