#include "net/protocol.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

int main() {
    const auto require=[](bool condition){if(!condition)throw std::runtime_error("protocol assertion failed");};
    net::Snapshot source{};
    source.server_time_us=123456789;source.server_tick=42;
    source.players.push_back({7,{1.25f,-3.5f},{160.0f,0.0f},99,0xff123456u,"Player"});
    source.players[0].facing_angle=1.25f;source.players[0].respawn_at_us=999;source.players[0].protected_until_us=555;source.players[0].shield_until_us=777;
    source.projectiles.push_back({91,7,{10.0f,20.0f},{480.0f,0.0f},0.5f});
    const auto bytes=net::make_snapshot(source);net::Reader reader(bytes);
    require(reader.type()==net::MessageType::snapshot);const auto decoded=net::read_snapshot(reader);
    require(decoded.server_time_us==source.server_time_us);require(decoded.server_tick==42);require(decoded.players.size()==1);require(decoded.players[0].id==7);require(std::fabs(decoded.players[0].position.x-1.25f)<0.001f);require(decoded.players[0].name=="Player");require(decoded.players[0].respawn_at_us==999);require(decoded.players[0].shield_until_us==777);require(decoded.projectiles.size()==1);require(decoded.projectiles[0].id==91);require(std::fabs(decoded.projectiles[0].velocity.x-480.0f)<0.001f);require(reader.finished());

    net::SongSchedule schedule{5000000,180000,128.0f,125,2,"main",{125,500,875}};const auto song_bytes=net::make_song_schedule(schedule);net::Reader song_reader(song_bytes);require(song_reader.type()==net::MessageType::song_schedule);const auto decoded_song=net::read_song_schedule(song_reader);require(decoded_song.server_start_us==5000000);require(decoded_song.subdivision==2);require(decoded_song.song_id=="main");require(decoded_song.note_times_ms.size()==3);require(song_reader.finished());
    net::RhythmResult result{};result.grade=net::RhythmGrade::good;result.offset_ms=-52;result.good=1;result.note_index=7;result.combo=3;result.max_combo=5;result.shot_fired=true;const auto result_bytes=net::make_rhythm_result(result);net::Reader result_reader(result_bytes);require(result_reader.type()==net::MessageType::rhythm_result);const auto decoded_result=net::read_rhythm_result(result_reader);require(decoded_result.note_index==7);require(decoded_result.offset_ms==-52);require(decoded_result.combo==3);require(!decoded_result.overstrum);require(decoded_result.shot_fired);require(result_reader.finished());
    net::Writer excessive(net::MessageType::snapshot);excessive.u64(1);excessive.u32(1);excessive.u8(0);excessive.u16(static_cast<std::uint16_t>(net::kMaxProjectiles+1));bool rejected=false;try{net::Reader malformed(excessive.bytes());require(malformed.type()==net::MessageType::snapshot);(void)net::read_snapshot(malformed);}catch(const std::exception &){rejected=true;}require(rejected);
    std::cout<<"protocol tests passed\n";
}
