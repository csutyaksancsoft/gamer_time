#include "platform/audio_playback_device.h"
#include "platform/effect_player.h"
#include "platform/song_player.h"

#include <SDL3/SDL.h>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    assert(SDL_Init(SDL_INIT_AUDIO));
    {
        AudioPlaybackDevice device;
        assert(device.initialize());

        const std::string assets = std::string(BARD_BATTLE_SOURCE_ROOT) + "/games/bard_battle/assets/audio";
        EffectPlayer effects;
        effects.load(device, assets + "/sfx");
        assert(effects.loaded_clip_count() == 4);
        assert(effects.populated_pool_count() == 2);

        SongConfig song{};
        song.file = assets + "/songs/MEMECAR-001.wav";
        SongPlayer music;
        assert(music.load(device, assets + "/songs", song));
        music.schedule(2000000);
        music.update(1000000);
        assert(!music.playing());

        effects.play(net::SoundCue::death, {}, {}, true);
        assert(effects.request_count() == 1);
        assert(effects.queued_count() == 1);
        assert(effects.last_cue() == "death");

        music.update(2000000);
        assert(music.playing());
        effects.play(net::SoundCue::shield_failure, {}, {}, true);
        assert(effects.queued_count() == 2);

        effects.play(net::SoundCue::shot_success, {}, {}, true);
        assert(effects.request_count() == 3);
        assert(effects.queued_count() == 2);
        assert(effects.diagnostic().find("no WAV clips") != std::string::npos);

        const auto invalid_root=std::filesystem::temp_directory_path()/"bard_battle_invalid_sfx";
        std::filesystem::remove_all(invalid_root);
        std::filesystem::create_directories(invalid_root/"death");
        std::ofstream(invalid_root/"death"/"bad.wav")<<"not a wave";
        EffectPlayer invalid_effects;
        invalid_effects.load(device,invalid_root.string());
        assert(invalid_effects.loaded_clip_count()==0);
        assert(invalid_effects.diagnostic().find("Skipped invalid SFX")!=std::string::npos);
        invalid_effects.shutdown();
        std::filesystem::remove_all(invalid_root);

        music.stop();
        effects.shutdown();
        device.shutdown();
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
