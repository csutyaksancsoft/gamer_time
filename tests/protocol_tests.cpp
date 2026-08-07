#include "net/protocol.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

int main() {
    const auto require=[](bool condition){if(!condition)throw std::runtime_error("protocol assertion failed");};
    net::Snapshot source{};
    source.server_time_us=123456789;source.server_tick=42;
    source.players.push_back({7,{1.25f,-3.5f},{160.0f,0.0f},99,0xff123456u,"Player"});
    const auto bytes=net::make_snapshot(source);net::Reader reader(bytes);
    require(reader.type()==net::MessageType::snapshot);const auto decoded=net::read_snapshot(reader);
    require(decoded.server_time_us==source.server_time_us);require(decoded.server_tick==42);require(decoded.players.size()==1);require(decoded.players[0].id==7);require(std::fabs(decoded.players[0].position.x-1.25f)<0.001f);require(decoded.players[0].name=="Player");require(reader.finished());

    net::SongSchedule schedule{5000000,180000,128.0f,125,2};const auto song_bytes=net::make_song_schedule(schedule);net::Reader song_reader(song_bytes);require(song_reader.type()==net::MessageType::song_schedule);const auto decoded_song=net::read_song_schedule(song_reader);require(decoded_song.server_start_us==5000000);require(decoded_song.subdivision==2);
    std::cout<<"protocol tests passed\n";
}
