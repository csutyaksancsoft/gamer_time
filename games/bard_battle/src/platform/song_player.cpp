#include "platform/song_player.h"
#include "platform/audio_playback_device.h"

#include <SDL3/SDL.h>
#include <filesystem>

SongPlayer::~SongPlayer() { stop(); }

bool SongPlayer::load(const AudioPlaybackDevice & device, const std::string & directory, const SongConfig & config) {
    stop(); error_.clear();
    if (!device.ready()) { error_="Shared playback device is unavailable"; return false; }
    Uint8 * data=nullptr; Uint32 length=0;
    const std::string path=std::filesystem::path(config.file).is_absolute()?config.file:(std::filesystem::path(directory)/config.file).string();
    if (!SDL_LoadWAV(path.c_str(), &spec_, &data, &length)) { error_="Cannot load " + path + ": " + SDL_GetError(); return false; }
    pcm_.assign(data, data + length); SDL_free(data);
    stream_=SDL_CreateAudioStream(&spec_, &spec_);
    if(!stream_){error_="Cannot create music stream: " + std::string(SDL_GetError()); return false;}
    device_=device.id();
    return true;
}

void SongPlayer::schedule(std::uint64_t local_start_us) {
    if(!stream_ || pcm_.empty()) return;
    SDL_UnbindAudioStream(stream_);
    if(!SDL_ClearAudioStream(stream_)){error_="Cannot clear music stream: "+std::string(SDL_GetError());return;}
    if(!SDL_PutAudioStreamData(stream_,pcm_.data(),static_cast<int>(pcm_.size()))||!SDL_FlushAudioStream(stream_)){error_="Cannot queue music: "+std::string(SDL_GetError());return;}
    scheduled_start_us_=local_start_us;armed_=true;playing_=false;
}

void SongPlayer::update(std::uint64_t now_us) {
    if(armed_ && now_us>=scheduled_start_us_){
        if(!SDL_BindAudioStream(device_,stream_)){error_="Cannot bind music stream: "+std::string(SDL_GetError());armed_=false;return;}
        actual_start_us_=now_us;armed_=false;playing_=true;
    }
}

void SongPlayer::stop() { if(stream_){SDL_DestroyAudioStream(stream_);stream_=nullptr;}device_=0;pcm_.clear();armed_=false;playing_=false; }
std::int64_t SongPlayer::playhead_ms(std::uint64_t now_us) const { if(!playing_) return -1; return static_cast<std::int64_t>((now_us-actual_start_us_)/1000); }
