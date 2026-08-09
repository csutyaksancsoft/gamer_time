#include "rhythm/song_config.h"

#include "core/error.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string_view>
#include <cmath>
#include <filesystem>
#include <unordered_set>

namespace {
std::string trim(std::string value) {
    auto nonspace=[](unsigned char c){return !std::isspace(c);};
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), nonspace));
    value.erase(std::find_if(value.rbegin(), value.rend(), nonspace).base(), value.end());
    return value;
}
}

SongConfig load_song_config(const std::string & path) {
    std::ifstream file(path);
    if (!file) fail("Missing song metadata: " + path);
    SongConfig config{}; config.id.clear(); config.file.clear();
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        const auto equals = line.find('=');
        if (equals == std::string::npos) fail("Malformed song metadata line: " + line);
        const std::string key=trim(line.substr(0,equals)); const std::string value=trim(line.substr(equals+1));
        if(key=="id") config.id=value; else if(key=="file") config.file=value; else if(key=="bpm") config.bpm=std::stof(value); else if(key=="first_beat_ms") config.first_beat_ms=std::stoi(value); else if(key=="subdivision") config.subdivision=static_cast<std::uint16_t>(std::stoul(value)); else if(key=="duration_ms") config.duration_ms=static_cast<std::uint32_t>(std::stoul(value));
    }
    if(config.id.empty() || config.file.empty() || config.bpm<=0.0f || config.subdivision==0 || config.duration_ms==0) fail(path+" requires id, file, and positive bpm, subdivision, and duration_ms");
    const std::filesystem::path wav=std::filesystem::path(path).parent_path()/config.file;
    if(!std::filesystem::is_regular_file(wav)) fail("Missing song audio: "+wav.string());
    config.file=std::filesystem::weakly_canonical(wav).string();
    return config;
}

SongCatalog SongCatalog::load(const std::string & directory) {
    namespace fs=std::filesystem; SongCatalog out; std::error_code ec;
    if(!fs::is_directory(directory,ec)) fail("Songs directory does not exist: "+directory);
    std::vector<fs::path> configs;
    for(const auto & entry:fs::directory_iterator(directory)) if(entry.is_regular_file()&&entry.path().extension()==".cfg") configs.push_back(entry.path());
    std::sort(configs.begin(),configs.end());
    for(const auto & path:configs){auto song=load_song_config(path.string());if(out.by_id_.contains(song.id))fail("Duplicate song id: "+song.id);out.by_id_[song.id]=out.songs_.size();out.songs_.push_back(std::move(song));}
    if(out.songs_.empty()) fail("Song catalog is empty: "+directory);
    return out;
}
const SongConfig * SongCatalog::find(const std::string & id) const {const auto it=by_id_.find(id);return it==by_id_.end()?nullptr:&songs_[it->second];}

std::vector<std::uint32_t> generate_beat_grid(const SongConfig & config) {
    constexpr std::size_t kMaximumNotes = 4096;
    std::vector<std::uint32_t> notes;
    const double interval_ms=60000.0/(static_cast<double>(config.bpm)*config.subdivision);
    for(std::int64_t index=0;;++index){
        const auto timestamp=static_cast<std::int64_t>(std::llround(config.first_beat_ms+index*interval_ms));
        if(timestamp>=static_cast<std::int64_t>(config.duration_ms))break;
        if(timestamp>=0&&(notes.empty()||timestamp>notes.back()))notes.push_back(static_cast<std::uint32_t>(timestamp));
        if(notes.size()>kMaximumNotes)fail("Generated beat grid exceeds 4096 notes");
    }
    return notes;
}
