#pragma once
#include "net/protocol.h"
#include "rhythm/song_config.h"
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace server {
struct Config {std::string bind="0.0.0.0",map="games/bard_battle/assets/tiled_projects/maps/brawlers_ballad.tmx",songs_dir="games/bard_battle/assets/audio/songs";std::uint8_t team_count=2;bool friendly_fire=false,shield_freeze=true,mode_vote=true,song_vote=true;net::ShieldMeleeMode shield_melee=net::ShieldMeleeMode::stun;net::GameMode mode=net::GameMode::ffa;std::uint8_t song_count=3;std::string force_song;};
struct RuntimeSettings {bool mode_vote=true,song_vote=true;net::GameMode mode=net::GameMode::ffa;std::uint8_t song_count=3;std::string force_song;std::uint8_t team_count=2;bool friendly_fire=false,shield_freeze=true;net::ShieldMeleeMode shield_melee=net::ShieldMeleeMode::stun;};
struct RoundConfig {bool mode_vote=true,song_vote=true;net::GameMode mode=net::GameMode::ffa;std::uint8_t song_count=3;std::string force_song;std::vector<std::string> songs;};
Config parse_startup(int argc,char **argv);
RoundConfig defaults_for(const Config &,std::size_t catalog_size);
RuntimeSettings runtime_settings(const Config &);
RoundConfig defaults_for(const RuntimeSettings &,std::size_t catalog_size);
std::string apply_set_command(std::string_view line,RuntimeSettings &,const SongCatalog &);
RoundConfig parse_start_command(std::string_view line,const RoundConfig &,const SongCatalog &);
std::string help();
class ShuffleBag {public:explicit ShuffleBag(const SongCatalog &,std::uint32_t seed=std::random_device{}());std::vector<std::string> draw(std::size_t count);private:std::vector<std::string> all_,bag_;std::mt19937 random_;};
struct PreparedRound {bool voting=false;std::vector<std::string> candidates;std::string selected_song;};
PreparedRound prepare_round(const RoundConfig &,ShuffleBag &);
}
