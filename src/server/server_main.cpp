#include "assets/tmx_map_loader.h"
#include "core/error.h"
#include "game/collision_world.h"
#include "game/map_world.h"
#include "net/protocol.h"
#include "rhythm/song_config.h"
#include "rhythm/rhythm_judgment.h"

#include <enet/enet.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifndef _WIN32
#include <sys/select.h>
#include <unistd.h>
#endif

namespace {

struct Player {
    net::PlayerId id = 0;
    std::string name;
    Vec2f position{};
    Vec2f velocity{};
    std::int8_t input_x = 0;
    std::int8_t input_y = 0;
    std::uint32_t input_sequence = 0;
    std::uint32_t color = 0xffffffffu;
    bool ready = false;
    bool active = true;
    net::RhythmResult score{};
    std::vector<std::uint8_t> judged_notes;
    std::size_t next_expiring_note = 0;
};

bool terminal_line_ready() {
#ifdef _WIN32
    return false;
#else
    fd_set set; FD_ZERO(&set); FD_SET(STDIN_FILENO, &set); timeval timeout{};
    return select(STDIN_FILENO + 1, &set, nullptr, nullptr, &timeout) > 0;
#endif
}

std::pair<std::string,std::uint16_t> split_endpoint(const std::string & endpoint) {
    const auto colon=endpoint.rfind(':'); if(colon==std::string::npos) return {endpoint,net::kDefaultPort};
    return {endpoint.substr(0,colon),static_cast<std::uint16_t>(std::stoul(endpoint.substr(colon+1)))};
}

void send_packet(ENetPeer * peer, const std::vector<std::uint8_t> & bytes, bool reliable) {
    auto * packet=enet_packet_create(bytes.data(),bytes.size(),reliable?ENET_PACKET_FLAG_RELIABLE:0);
    enet_peer_send(peer,reliable?0:1,packet);
}

Vec2f spawn_for(std::uint32_t index, const MapWorld & map, const CollisionWorld & collision) {
    const Vec2f tile=map.tile_size();
    for(std::uint32_t attempt=0;attempt<map.width()*map.height();++attempt){
        const std::uint32_t n=(index*37u+attempt*17u)%(map.width()*map.height());
        const std::uint32_t x=n%map.width(), y=n/map.width();
        const Vec2f point={map.origin().x+(x+0.5f)*tile.x,map.origin().y+(y+0.5f)*tile.y};
        if(!collision.blocks_point(point)) return point;
    }
    return {};
}

net::RhythmResult grade_hit(Player & player, std::uint64_t hit_us, std::int16_t calibration_ms, const net::SongSchedule & song) {
    constexpr std::int64_t kGoodWindowMs=80;
    const std::int64_t relative_ms=(static_cast<std::int64_t>(hit_us)-static_cast<std::int64_t>(song.server_start_us))/1000-calibration_ms;
    std::size_t best=song.note_times_ms.size();std::int64_t best_abs=kGoodWindowMs+1;std::int32_t best_offset=0;
    for(std::size_t i=0;i<song.note_times_ms.size();++i){if(i<player.judged_notes.size()&&player.judged_notes[i])continue;const auto offset=static_cast<std::int32_t>(relative_ms-static_cast<std::int64_t>(song.note_times_ms[i]));const auto absolute=std::llabs(static_cast<long long>(offset));if(absolute<best_abs){best=i;best_abs=absolute;best_offset=offset;}}
    player.score.note_index=UINT32_MAX;player.score.offset_ms=best_offset;player.score.overstrum=best>=song.note_times_ms.size();
    if(best<song.note_times_ms.size()&&best_abs<=kGoodWindowMs){player.judged_notes[best]=1;player.score.note_index=static_cast<std::uint32_t>(best);player.score.overstrum=false;player.score.grade=rhythm::grade_offset(best_offset);if(player.score.grade==net::RhythmGrade::perfect)++player.score.perfect;else ++player.score.good;++player.score.combo;player.score.max_combo=std::max(player.score.max_combo,player.score.combo);}
    else{player.score.grade=net::RhythmGrade::miss;++player.score.miss;player.score.combo=0;player.score.overstrum=true;}
    return player.score;
}

} // namespace

int main(int argc,char ** argv) try {
    std::string bind="0.0.0.0:27020", map_path="assets/maps/grass_tileset_map.tmx", song_path="assets/audio/song.cfg";
    for(int i=1;i<argc;++i){const std::string arg=argv[i];if(arg=="--bind"&&i+1<argc)bind=argv[++i];else if(arg=="--map"&&i+1<argc)map_path=argv[++i];else if(arg=="--song"&&i+1<argc)song_path=argv[++i];else fail("Usage: gamer_time_server [--bind host:port] [--map path] [--song path]");}
    const MapWorld map=MapWorld::from_tmx(assets::load_tmx_map(map_path)); const CollisionWorld collision=CollisionWorld::from_map(map); const SongConfig song_config=load_song_config(song_path);
    const auto chart=generate_beat_grid(song_config);
    if(enet_initialize()!=0) fail("ENet initialization failed");
    const auto [host_name,port]=split_endpoint(bind); ENetAddress address{}; address.port=port;
    if(host_name=="0.0.0.0") address.host=ENET_HOST_ANY; else if(enet_address_set_host(&address,host_name.c_str())!=0) fail("Invalid bind address");
    ENetHost * host=enet_host_create(&address,net::kMaxPlayers,2,0,0); if(!host) fail("Could not create server socket");
    std::unordered_map<ENetPeer*,Player> players; net::PlayerId next_id=1; std::uint32_t tick=0; bool running=true; net::RoomState room=net::RoomState::lobby; net::SongSchedule song{};
    std::cout<<"Server listening on "<<bind<<". Commands: status, start, stop, kick <id>, quit\n";
    auto next_tick=std::chrono::steady_clock::now();
    while(running){
        ENetEvent event{}; while(enet_host_service(host,&event,0)>0){
            if(event.type==ENET_EVENT_TYPE_DISCONNECT){auto found=players.find(event.peer);if(found!=players.end()){std::cout<<found->second.name<<" disconnected\n";players.erase(found);}}
            else if(event.type==ENET_EVENT_TYPE_RECEIVE){
                try{net::Reader reader({event.packet->data,event.packet->dataLength});const auto type=reader.type();auto found=players.find(event.peer);
                    if(type==net::MessageType::hello&&found==players.end()&&players.size()<net::kMaxPlayers){if(reader.u32()!=net::kProtocolVersion)fail("Protocol mismatch");Player player{};player.id=next_id++;player.name=reader.string(24);if(player.name.empty())player.name="Player";player.position=spawn_for(player.id,map,collision);player.color=0xff000000u|((player.id*2654435761u)&0x00ffffffu);player.active=room!=net::RoomState::playing&&room!=net::RoomState::countdown;players.emplace(event.peer,player);send_packet(event.peer,net::make_welcome(player.id,player.position),true);std::cout<<player.name<<" joined as "<<player.id<<(player.active?"":" (waiting for next round)")<<"\n";}
                    else if(found!=players.end()&&type==net::MessageType::input){found->second.input_sequence=reader.u32();found->second.input_x=static_cast<std::int8_t>(reader.u8());found->second.input_y=static_cast<std::int8_t>(reader.u8());reader.u64();}
                    else if(found!=players.end()&&type==net::MessageType::ready)found->second.ready=true;
                    else if(type==net::MessageType::clock_ping){const auto client=reader.u64();const auto receive=net::monotonic_time_us();send_packet(event.peer,net::make_clock_pong(client,receive,net::monotonic_time_us()),false);}
                    else if(found!=players.end()&&type==net::MessageType::rhythm_hit){reader.u32();const auto hit=reader.u64();const auto calibration=std::clamp<int>(reader.i16(),-250,250);net::RhythmResult result{};if(room==net::RoomState::playing)result=grade_hit(found->second,hit,static_cast<std::int16_t>(calibration),song);else{result=found->second.score;result.grade=net::RhythmGrade::miss;++result.miss;found->second.score=result;}send_packet(event.peer,net::make_rhythm_result(result),true);}
                }catch(const std::exception & e){std::cerr<<"Rejected packet: "<<e.what()<<"\n";}
                enet_packet_destroy(event.packet);
            }
        }
        constexpr float dt=1.0f/60.0f;
        for(auto & [peer,player]:players){if(!player.active)continue;Vec2f direction=normalize_or_zero({player.input_x/127.0f,player.input_y/127.0f});player.velocity=direction*net::kMoveSpeed;Vec2f candidate=player.position+player.velocity*dt;if(collision.blocks_segment(player.position,candidate)){candidate=player.position;player.velocity={};}
            bool blocked=false;for(const auto & [other_peer,other]:players){if(other_peer!=peer&&other.active&&length_squared(candidate-other.position)<16.0f*16.0f){blocked=true;break;}}if(!blocked)player.position=candidate;else player.velocity={};}
        const auto now=net::monotonic_time_us();if(room==net::RoomState::countdown&&now>=song.server_start_us)room=net::RoomState::playing;
        if(room==net::RoomState::playing){for(auto & [peer,p]:players){while(p.next_expiring_note<song.note_times_ms.size()&&now>song.server_start_us+(static_cast<std::uint64_t>(song.note_times_ms[p.next_expiring_note])+80)*1000ULL){const auto index=p.next_expiring_note++;if(index<p.judged_notes.size()&&!p.judged_notes[index]){p.judged_notes[index]=1;p.score.grade=net::RhythmGrade::miss;p.score.offset_ms=80;p.score.note_index=static_cast<std::uint32_t>(index);p.score.combo=0;p.score.overstrum=false;++p.score.miss;send_packet(peer,net::make_rhythm_result(p.score),true);}}}}
        if(room==net::RoomState::playing&&now>=song.server_start_us+song.duration_ms*1000ULL)room=net::RoomState::free_move;
        if(tick%3==0){net::Snapshot snapshot{};snapshot.server_time_us=now;snapshot.server_tick=tick;for(const auto & [peer,p]:players){(void)peer;if(p.active)snapshot.players.push_back({p.id,p.position,p.velocity,p.input_sequence,p.color,p.name});}const auto bytes=net::make_snapshot(snapshot);for(auto & [peer,p]:players){(void)p;send_packet(peer,bytes,false);}}
        if(terminal_line_ready()){std::string line;std::getline(std::cin,line);if(line=="quit")running=false;else if(line=="status")std::cout<<players.size()<<" players, tick "<<tick<<", room "<<static_cast<int>(room)<<"\n";else if(line=="stop")room=net::RoomState::free_move;else if(line=="start"){const bool all_ready=std::all_of(players.begin(),players.end(),[](const auto & entry){return entry.second.ready;});if(players.empty()||!all_ready)std::cout<<"Start refused: all connected clients must be ready\n";else{song={now+3000000ULL,song_config.duration_ms,song_config.bpm,song_config.first_beat_ms,song_config.subdivision,song_config.id,chart};room=net::RoomState::countdown;for(auto & [peer,p]:players){p.score={};p.judged_notes.assign(chart.size(),0);p.next_expiring_note=0;p.active=true;send_packet(peer,net::make_song_schedule(song),true);}std::cout<<"Song scheduled in 3 seconds\n";}}else if(line.rfind("kick ",0)==0){const auto id=static_cast<net::PlayerId>(std::stoul(line.substr(5)));for(auto & [peer,p]:players)if(p.id==id){enet_peer_disconnect(peer,0);break;}}}
        enet_host_flush(host);++tick;next_tick+=std::chrono::microseconds(16667);std::this_thread::sleep_until(next_tick);
    }
    enet_host_destroy(host);enet_deinitialize();return 0;
} catch(const std::exception & e){std::cerr<<"Fatal server error: "<<e.what()<<'\n';return 1;}
