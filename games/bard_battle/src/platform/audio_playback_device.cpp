#include "platform/audio_playback_device.h"

#include <SDL3/SDL.h>

AudioPlaybackDevice::~AudioPlaybackDevice() { shutdown(); }

bool AudioPlaybackDevice::initialize() {
    shutdown();
    diagnostic_.clear();
    const SDL_AudioSpec requested{SDL_AUDIO_F32, 2, 48000};
    device_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &requested);
    if (!device_) {
        diagnostic_ = "Cannot open default playback device: " + std::string(SDL_GetError());
        return false;
    }
    return true;
}

void AudioPlaybackDevice::shutdown() {
    if (device_) SDL_CloseAudioDevice(device_);
    device_ = 0;
}
