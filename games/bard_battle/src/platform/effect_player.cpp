#include "platform/effect_player.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

namespace {
constexpr std::size_t kMaxVoices=32;
constexpr std::array<const char *,10> kFolders={"death","shot_success","shot_failure","shot_hit_player","shot_hit_shield","shield_success","shield_failure","respawn","swing_attack","shield_break"};
bool wav_extension(const std::filesystem::path & path){std::string extension=path.extension().string();std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return extension==".wav";}
}

EffectPlayer::~EffectPlayer(){shutdown();}

void EffectPlayer::load(const std::string & root){shutdown();diagnostic_.clear();device_=SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&output_spec_);if(!device_){diagnostic_="Cannot open effects audio: "+std::string(SDL_GetError());return;}SDL_ResumeAudioDevice(device_);
    for(std::size_t category=0;category<kFolders.size();++category){const std::filesystem::path directory=std::filesystem::path(root)/kFolders[category];if(!std::filesystem::exists(directory))continue;std::vector<std::filesystem::path> files;for(const auto & entry:std::filesystem::directory_iterator(directory))if(entry.is_regular_file()&&wav_extension(entry.path()))files.push_back(entry.path());std::sort(files.begin(),files.end());for(const auto & path:files){SDL_AudioSpec source_spec{};Uint8 * source=nullptr;Uint32 source_length=0;if(!SDL_LoadWAV(path.string().c_str(),&source_spec,&source,&source_length)){diagnostic_="Skipped invalid SFX: "+path.filename().string();continue;}Uint8 * converted=nullptr;int converted_length=0;const bool ok=SDL_ConvertAudioSamples(&source_spec,source,static_cast<int>(source_length),&output_spec_,&converted,&converted_length);SDL_free(source);if(!ok||!converted){diagnostic_="Skipped incompatible SFX: "+path.filename().string();continue;}Clip clip{};const auto * samples=reinterpret_cast<const float *>(converted);clip.samples.assign(samples,samples+converted_length/static_cast<int>(sizeof(float)));SDL_free(converted);if(!clip.samples.empty())pools_[category].clips.push_back(std::move(clip));}}
}

std::size_t EffectPlayer::choose(Pool & pool){if(pool.clips.size()==1)return 0;std::uniform_int_distribution<int> repeat_roll(0,99);if(pool.last<pool.clips.size()&&repeat_roll(random_)<85){std::uniform_int_distribution<std::size_t> pick(0,pool.clips.size()-2);const auto value=pick(random_);return value>=pool.last?value+1:value;}std::uniform_int_distribution<std::size_t> pick(0,pool.clips.size()-1);return pick(random_);}

void EffectPlayer::play(net::SoundCue cue,Vec2f listener,Vec2f source,bool participant){if(!device_)return;auto & pool=pools_[static_cast<std::size_t>(cue)];if(pool.clips.empty())return;const std::size_t selected=choose(pool);pool.last=selected;const auto & clip=pool.clips[selected];float gain=1.0f,pan=0.0f;if(!participant){const Vec2f delta=source-listener;const float distance=std::sqrt(length_squared(delta));gain=std::clamp(1.0f-0.7f*(distance/net::kVisionRadius),0.3f,1.0f);pan=std::clamp(delta.x/net::kVisionRadius,-1.0f,1.0f);}std::vector<float> samples=clip.samples;for(std::size_t i=0;i+1<samples.size();i+=2){samples[i]*=gain*(pan>0.0f?1.0f-pan:1.0f);samples[i+1]*=gain*(pan<0.0f?1.0f+pan:1.0f);}if(voices_.size()>=kMaxVoices){SDL_DestroyAudioStream(voices_.front().stream);voices_.erase(voices_.begin());}SDL_AudioStream * stream=SDL_CreateAudioStream(&output_spec_,&output_spec_);if(!stream||!SDL_BindAudioStream(device_,stream)||!SDL_PutAudioStreamData(stream,samples.data(),static_cast<int>(samples.size()*sizeof(float)))){if(stream)SDL_DestroyAudioStream(stream);return;}SDL_FlushAudioStream(stream);const std::uint64_t frames=samples.size()/2;voices_.push_back({stream,net::monotonic_time_us()+frames*1000000ULL/static_cast<std::uint64_t>(output_spec_.freq)+50000ULL});}

void EffectPlayer::update(std::uint64_t now){for(auto it=voices_.begin();it!=voices_.end();){if(now>=it->end_us){SDL_DestroyAudioStream(it->stream);it=voices_.erase(it);}else ++it;}}
void EffectPlayer::shutdown(){for(auto & voice:voices_)if(voice.stream)SDL_DestroyAudioStream(voice.stream);voices_.clear();if(device_){SDL_CloseAudioDevice(device_);device_=0;}for(auto & pool:pools_){pool.clips.clear();pool.last=static_cast<std::size_t>(-1);}}
