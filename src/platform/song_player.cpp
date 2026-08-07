#include "platform/song_player.h"

#include <SDL3/SDL.h>

SongPlayer::~SongPlayer() { stop(); }

bool SongPlayer::load(const std::string & directory, const SongConfig & config) {
    stop(); error_.clear();
    Uint8 * data=nullptr; Uint32 length=0;
    const std::string path=directory + "/" + config.file;
    if (!SDL_LoadWAV(path.c_str(), &spec_, &data, &length)) { error_="Cannot load " + path + ": " + SDL_GetError(); return false; }
    pcm_.assign(data, data + length); SDL_free(data);
    stream_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec_, nullptr, nullptr);
    if(!stream_){error_="Cannot open audio output: " + std::string(SDL_GetError()); return false;}
    return true;
}

void SongPlayer::schedule(std::uint64_t local_start_us) {
    if(!stream_ || pcm_.empty()) return;
    SDL_ClearAudioStream(stream_); SDL_PutAudioStreamData(stream_, pcm_.data(), static_cast<int>(pcm_.size()));
    SDL_PauseAudioStreamDevice(stream_); scheduled_start_us_=local_start_us; armed_=true; playing_=false;
}

void SongPlayer::update(std::uint64_t now_us) {
    if(armed_ && now_us>=scheduled_start_us_){ SDL_ResumeAudioStreamDevice(stream_); actual_start_us_=now_us; armed_=false; playing_=true; }
}

void SongPlayer::stop() { if(stream_){SDL_DestroyAudioStream(stream_);stream_=nullptr;} pcm_.clear(); armed_=false;playing_=false; }
std::int64_t SongPlayer::playhead_ms(std::uint64_t now_us) const { if(!playing_) return -1; return static_cast<std::int64_t>((now_us-actual_start_us_)/1000); }
