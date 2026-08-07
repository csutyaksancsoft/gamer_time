#include "rhythm/song_config.h"

#include "core/error.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string_view>

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
    SongConfig config{};
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        const auto equals = line.find('=');
        if (equals == std::string::npos) fail("Malformed song metadata line: " + line);
        const std::string key=trim(line.substr(0,equals)); const std::string value=trim(line.substr(equals+1));
        if(key=="id") config.id=value; else if(key=="file") config.file=value; else if(key=="bpm") config.bpm=std::stof(value); else if(key=="first_beat_ms") config.first_beat_ms=std::stoi(value); else if(key=="subdivision") config.subdivision=static_cast<std::uint16_t>(std::stoul(value)); else if(key=="duration_ms") config.duration_ms=static_cast<std::uint32_t>(std::stoul(value));
    }
    if(config.bpm<=0.0f || config.subdivision==0 || config.duration_ms==0) fail("song.cfg requires positive bpm, subdivision, and duration_ms");
    return config;
}
