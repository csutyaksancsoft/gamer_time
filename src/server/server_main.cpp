#include "assets/tmx_map_loader.h"
#include "core/error.h"
#include "game/collision_world.h"
#include "game/map_world.h"
#include "game/match_rules.h"
#include "net/protocol.h"
#include "rhythm/song_config.h"
#include "rhythm/rhythm_judgment.h"
#include "server/server_config.h"

#include <enet/enet.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <initializer_list>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <random>

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
    bool alive = true;
    float facing_angle = 0.0f;
    std::uint64_t respawn_at_us = 0;
    std::uint64_t protected_until_us = 0;
    std::uint64_t shield_until_us = 0;
    net::TeamId team = net::kFirstTeam;
    std::uint32_t round_kills=0,round_deaths=0,session_kills=0,session_deaths=0;
};

struct Projectile {std::uint32_t id=0;net::PlayerId owner_id=0;Vec2f position{};Vec2f velocity{};std::uint64_t spawn_us=0;float angle=0.0f;};
constexpr float kProjectileSpeed=480.0f,kProjectileRadius=6.0f,kPlayerRadius=8.0f;
constexpr std::uint64_t kProjectileLifetimeUs=2000000,kRespawnDelayUs=5000000,kProtectionUs=1000000;

float segment_distance_squared(Vec2f a,Vec2f b,Vec2f point){const Vec2f d=b-a;const float denom=length_squared(d);const float t=denom>0.0f?std::clamp(dot(point-a,d)/denom,0.0f,1.0f):0.0f;return length_squared(point-(a+d*t));}

bool terminal_line_ready() {
#ifdef _WIN32
    return false;
#else
    fd_set set; FD_ZERO(&set); FD_SET(STDIN_FILENO, &set); timeval timeout{};
    return select(STDIN_FILENO + 1, &set, nullptr, nullptr, &timeout) > 0;
#endif
}

std::pair<std::string,std::uint16_t> split_endpoint(const std::string & endpoint) {
    return {endpoint,net::kServerPort};
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

template<class Players> Vec2f safe_spawn_for(std::uint32_t index,const MapWorld & map,const CollisionWorld & collision,const Players & players){for(std::uint32_t offset=0;offset<map.width()*map.height();++offset){const Vec2f p=spawn_for(index+offset,map,collision);bool clear=true;for(const auto & entry:players){const auto & other=entry.second;if(other.active&&other.alive&&length_squared(p-other.position)<64.0f*64.0f){clear=false;break;}}if(clear)return p;}return spawn_for(index,map,collision);}

bool can_observe(const Player & observer,Vec2f position,const CollisionWorld & collision){return observer.active&&observer.alive&&length_squared(position-observer.position)<=net::kVisionRadius*net::kVisionRadius&&!collision.blocks_segment(observer.position,position);}

template<class Players> void emit_sound(Players & players,const CollisionWorld & collision,std::uint32_t & next_event_id,net::SoundCue cue,net::PlayerId source,net::PlayerId target,Vec2f position,std::initializer_list<net::PlayerId> participants,bool observers=true){const auto id=next_event_id++;const auto now=net::monotonic_time_us();for(auto & [peer,listener]:players){const bool participant=std::find(participants.begin(),participants.end(),listener.id)!=participants.end();if(!participant&&(!observers||!can_observe(listener,position,collision)))continue;send_packet(peer,net::make_sound_event({id,cue,source,target,position,now,participant}),true);}}

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
    for(int i=1;i<argc;++i)if(std::string_view(argv[i])=="--help"){std::cout<<server::help();return 0;}
    const auto config=server::parse_startup(argc,argv);const auto catalog=SongCatalog::load(config.songs_dir);if(!config.force_song.empty()&&!catalog.find(config.force_song))fail("Unknown --force-song ID: "+config.force_song);auto settings=server::runtime_settings(config);server::ShuffleBag shuffle(catalog);
    const std::string bind=config.bind;
    const MapWorld map=MapWorld::from_tmx(assets::load_tmx_map(config.map)); const CollisionWorld collision=CollisionWorld::from_map(map);
    if(enet_initialize()!=0) fail("ENet initialization failed");
    const auto [host_name,port]=split_endpoint(bind); ENetAddress address{}; address.port=port;
    if(host_name=="0.0.0.0") address.host=ENET_HOST_ANY; else if(enet_address_set_host(&address,host_name.c_str())!=0) fail("Invalid bind address");
    ENetHost * host=enet_host_create(&address,net::kMaxPlayers,2,0,0); if(!host) fail("Could not create server socket");
    std::unordered_map<ENetPeer*,Player> players;std::vector<Projectile> projectiles;net::PlayerId next_id=1;std::uint32_t next_projectile_id=1,next_sound_event_id=1; std::uint32_t tick=0; bool running=true; net::RoomState room=net::RoomState::lobby;net::GameMode mode=settings.mode;game::ModeVote vote;game::SongVote song_vote;net::SongSchedule song{};server::RoundConfig active_round=server::defaults_for(settings,catalog.songs().size());std::vector<std::string> candidates;std::string selected_song;std::mt19937 random{std::random_device{}()};
    auto schedule_song=[&](const std::string&id,std::uint64_t at){const auto*cfg=catalog.find(id);if(!cfg)fail("Selected song vanished from catalog: "+id);const auto chart=generate_beat_grid(*cfg);song={at+3000000ULL,cfg->duration_ms,cfg->bpm,cfg->first_beat_ms,cfg->subdivision,cfg->id,chart};selected_song=id;room=net::RoomState::countdown;for(auto &[peer,p]:players){p.round_kills=p.round_deaths=0;p.score={};p.judged_notes.assign(chart.size(),0);p.next_expiring_note=0;p.active=true;send_packet(peer,net::make_song_schedule(song),true);}std::cout<<"Selected "<<id<<"; countdown started\n";};
    std::cout<<"Server listening on "<<bind<<':'<<net::kServerPort<<" with "<<catalog.songs().size()<<" songs. Commands: status, songs, start [options], stop, kick <id>, quit\n";
    auto next_tick=std::chrono::steady_clock::now();
    while(running){
        ENetEvent event{}; while(enet_host_service(host,&event,0)>0){
            if(event.type==ENET_EVENT_TYPE_DISCONNECT){auto found=players.find(event.peer);if(found!=players.end()){std::cout<<found->second.name<<" disconnected\n";vote.disconnect(found->second.id);song_vote.disconnect(found->second.id);players.erase(found);}}
            else if(event.type==ENET_EVENT_TYPE_RECEIVE){
                try{net::Reader reader({event.packet->data,event.packet->dataLength});const auto type=reader.type();auto found=players.find(event.peer);
                    if(type==net::MessageType::hello&&found==players.end()&&players.size()<net::kMaxPlayers){if(reader.u32()!=net::kProtocolVersion)fail("Protocol mismatch");Player player{};player.id=next_id++;player.name=reader.string(24);if(player.name.empty())player.name="Player";player.position=spawn_for(player.id,map,collision);player.color=0xff000000u|((player.id*2654435761u)&0x00ffffffu);player.active=room!=net::RoomState::playing&&room!=net::RoomState::countdown;players.emplace(event.peer,player);send_packet(event.peer,net::make_welcome(player.id,player.position),true);std::cout<<player.name<<" joined as "<<player.id<<(player.active?"":" (waiting for next round)")<<"\n";}
                    else if(found!=players.end()&&type==net::MessageType::input){found->second.input_sequence=reader.u32();const auto x=static_cast<std::int8_t>(reader.u8()),y=static_cast<std::int8_t>(reader.u8());found->second.input_x=found->second.alive?x:0;found->second.input_y=found->second.alive?y:0;reader.u64();}
                    else if(found!=players.end()&&type==net::MessageType::ready)found->second.ready=true;
                    else if(found!=players.end()&&type==net::MessageType::vote_request){const auto choice=net::read_vote_request(reader);if(room!=net::RoomState::voting||!vote.submit(found->second.id,choice))fail("Vote rejected");}
                    else if(found!=players.end()&&type==net::MessageType::song_vote_request){const auto choice=net::read_song_vote_request(reader);if(room!=net::RoomState::voting||!song_vote.submit(found->second.id,choice))fail("Song vote rejected");}
                    else if(found!=players.end()&&type==net::MessageType::team_request){const auto team=net::read_team_request(reader);if(!net::team_switching_allowed(room)||!net::valid_team(team,settings.team_count))fail("Team selection rejected");found->second.team=team;}
                    else if(type==net::MessageType::clock_ping){const auto client=reader.u64();const auto receive=net::monotonic_time_us();send_packet(event.peer,net::make_clock_pong(client,receive,net::monotonic_time_us()),false);}
                    else if(found!=players.end()&&type==net::MessageType::rhythm_hit){reader.u32();const auto hit=reader.u64();const auto calibration=std::clamp<int>(reader.i16(),-250,250);Vec2f aim={reader.f32(),reader.f32()};const auto action=static_cast<net::RhythmAction>(reader.u8());if(action!=net::RhythmAction::shoot&&action!=net::RhythmAction::shield)fail("Invalid rhythm action");net::RhythmResult result{};if(room==net::RoomState::playing&&found->second.alive)result=grade_hit(found->second,hit,static_cast<std::int16_t>(calibration),song);else{result=found->second.score;result.grade=net::RhythmGrade::miss;++result.miss;result.combo=0;found->second.score=result;}const float aim_length=length(aim);const bool valid_aim=std::isfinite(aim.x)&&std::isfinite(aim.y)&&aim_length>0.999f&&aim_length<1.001f;const bool successful=result.grade!=net::RhythmGrade::miss&&found->second.alive&&room==net::RoomState::playing;result.shot_fired=successful&&action==net::RhythmAction::shoot&&valid_aim;result.shield_activated=successful&&action==net::RhythmAction::shield;if(result.shield_activated){const float shield_bpm=song.bpm>1.0f?song.bpm:1.0f;found->second.shield_until_us=net::monotonic_time_us()+static_cast<std::uint64_t>(60000000.0f/shield_bpm);}if(result.shot_fired){aim=aim/aim_length;found->second.facing_angle=std::atan2(aim.y,aim.x);projectiles.push_back({next_projectile_id++,found->second.id,found->second.position+aim*14.0f,aim*kProjectileSpeed,net::monotonic_time_us(),found->second.facing_angle});}const auto cue=action==net::RhythmAction::shoot?(result.shot_fired?net::SoundCue::shot_success:net::SoundCue::shot_failure):(result.shield_activated?net::SoundCue::shield_success:net::SoundCue::shield_failure);emit_sound(players,collision,next_sound_event_id,cue,found->second.id,0,found->second.position,{found->second.id},result.shot_fired||result.shield_activated);found->second.score=result;send_packet(event.peer,net::make_rhythm_result(result),true);}
                }catch(const std::exception & e){std::cerr<<"Rejected packet: "<<e.what()<<"\n";}
                enet_packet_destroy(event.packet);
            }
        }
        constexpr float dt=1.0f/60.0f;
        for(auto & [peer,player]:players){if(!player.active||!player.alive)continue;Vec2f direction=normalize_or_zero({player.input_x/127.0f,player.input_y/127.0f});player.velocity=direction*net::kMoveSpeed;Vec2f candidate=player.position+player.velocity*dt;if(collision.blocks_segment(player.position,candidate)){candidate=player.position;player.velocity={};}
            bool blocked=false;for(const auto & [other_peer,other]:players){if(other_peer!=peer&&other.active&&other.alive&&length_squared(candidate-other.position)<16.0f*16.0f){blocked=true;break;}}if(!blocked)player.position=candidate;else player.velocity={};}
        const auto now=net::monotonic_time_us();if(room==net::RoomState::voting&&((vote.active()&&vote.complete(now))||(song_vote.active()&&song_vote.complete(now)))){if(vote.active()){mode=vote.result();vote.finish();}if(song_vote.active()){selected_song=song_vote.result(random);song_vote.finish();}schedule_song(selected_song,now);}if(room==net::RoomState::countdown&&now>=song.server_start_us)room=net::RoomState::playing;
        for(auto & [peer,p]:players){(void)peer;if(p.active&&!p.alive&&now>=p.respawn_at_us){p.position=safe_spawn_for(p.id+tick,map,collision,players);p.velocity={};p.input_x=p.input_y=0;p.alive=true;p.respawn_at_us=0;p.protected_until_us=now+kProtectionUs;}}
        for(auto it=projectiles.begin();it!=projectiles.end();){const Vec2f next=it->position+it->velocity*dt;bool destroy=now-it->spawn_us>=kProjectileLifetimeUs||collision.blocks_segment(it->position,next);if(!destroy){auto owner=std::find_if(players.begin(),players.end(),[&](const auto&e){return e.second.id==it->owner_id;});for(auto & [peer,p]:players){(void)peer;if(!p.active||!p.alive||p.id==it->owner_id||now<p.protected_until_us)continue;if(owner!=players.end()&&!game::projectile_can_hit(mode,settings.friendly_fire,owner->second.team,p.team))continue;if(segment_distance_squared(it->position,next,p.position)<(kProjectileRadius+kPlayerRadius)*(kProjectileRadius+kPlayerRadius)){destroy=true;if(now<p.shield_until_us){emit_sound(players,collision,next_sound_event_id,net::SoundCue::shot_hit_shield,it->owner_id,p.id,p.position,{it->owner_id,p.id});}else{p.alive=false;p.velocity={};p.input_x=p.input_y=0;p.respawn_at_us=now+kRespawnDelayUs;p.protected_until_us=0;p.shield_until_us=0;++p.round_deaths;++p.session_deaths;if(owner!=players.end()&&game::kill_is_awarded(mode,owner->second.team,p.team)){++owner->second.round_kills;++owner->second.session_kills;}emit_sound(players,collision,next_sound_event_id,net::SoundCue::death,it->owner_id,p.id,p.position,{it->owner_id,p.id});}break;}}}if(destroy)it=projectiles.erase(it);else{it->position=next;++it;}}
        if(room==net::RoomState::playing){for(auto & [peer,p]:players){while(p.next_expiring_note<song.note_times_ms.size()&&now>song.server_start_us+(static_cast<std::uint64_t>(song.note_times_ms[p.next_expiring_note])+80)*1000ULL){const auto index=p.next_expiring_note++;if(index<p.judged_notes.size()&&!p.judged_notes[index]){p.judged_notes[index]=1;p.score.grade=net::RhythmGrade::miss;p.score.offset_ms=80;p.score.note_index=static_cast<std::uint32_t>(index);p.score.combo=0;p.score.overstrum=false;p.score.shot_fired=false;p.score.shield_activated=false;++p.score.miss;send_packet(peer,net::make_rhythm_result(p.score),true);}}}}
        if(room==net::RoomState::playing&&now>=song.server_start_us+song.duration_ms*1000ULL)room=net::RoomState::free_move;
        if(tick%3==0){net::Snapshot snapshot{};
snapshot.server_time_us=now;
snapshot.server_tick=tick;
snapshot.room=room;
snapshot.mode=mode;
snapshot.vote_deadline_us=vote.active()?vote.deadline_us():(song_vote.active()?song_vote.deadline_us():0);
song.vote_deadline_us=snapshot.vote_deadline_us;
snapshot.eligible_voters=vote.active()?vote.eligible_count():(song_vote.active()?song_vote.eligible_count():0);
snapshot.teams_votes=vote.active()?vote.teams_count():0;
snapshot.ffa_votes=vote.active()?vote.ffa_count():0;
snapshot.team_count=settings.team_count;
snapshot.friendly_fire=settings.friendly_fire;
snapshot.mode_vote_enabled=vote.active();
snapshot.song_vote_enabled=song_vote.active();
snapshot.song_candidates=candidates;
snapshot.song_votes=song_vote.active()?song_vote.totals():std::vector<std::uint8_t>{};
snapshot.selected_song_id=selected_song;for(const auto & [peer,p]:players){(void)peer;if(p.active){net::PlayerState state{};state.id=p.id;state.position=p.position;state.velocity=p.velocity;state.acknowledged_input=p.input_sequence;state.color=p.color;state.name=p.name;state.alive=p.alive;state.facing_angle=p.facing_angle;state.respawn_at_us=p.respawn_at_us;state.protected_until_us=p.protected_until_us;state.shield_until_us=p.shield_until_us;state.team=p.team;state.vote=vote.active()?vote.choice(p.id):net::VoteChoice::none;state.song_vote=song_vote.active()?song_vote.choice(p.id):0xff;state.round_kills=p.round_kills;state.round_deaths=p.round_deaths;state.session_kills=p.session_kills;state.session_deaths=p.session_deaths;
snapshot.players.push_back(state);}}for(const auto & p:projectiles)snapshot.projectiles.push_back({p.id,p.owner_id,p.position,p.velocity,p.angle});const auto bytes=net::make_snapshot(snapshot);for(auto & [peer,p]:players){(void)p;send_packet(peer,bytes,false);}}
        if(terminal_line_ready()){std::string line;std::getline(std::cin,line);try{if(line=="quit")running=false;
else if(line=="songs"){for(const auto&s:catalog.songs())std::cout<<s.id<<"  "<<s.bpm<<" BPM  "<<s.duration_ms<<" ms  "<<s.file<<'\n';}
else if(line=="status")std::cout<<"catalog="<<catalog.songs().size()<<" players="<<players.size()<<" room="<<static_cast<int>(room)<<" settings(mode-vote="<<settings.mode_vote<<", song-vote="<<settings.song_vote<<", mode="<<(settings.mode==net::GameMode::teams?"teams":"ffa")<<", count="<<unsigned(settings.song_count)<<", force-song="<<(settings.force_song.empty()?"off":settings.force_song)<<", teams="<<unsigned(settings.team_count)<<", friendly-fire="<<settings.friendly_fire<<") active(mode-vote="<<vote.active()<<", song-vote="<<song_vote.active()<<") candidates="<<candidates.size()<<" selected-mode="<<(mode==net::GameMode::teams?"teams":"ffa")<<" selected-song="<<(selected_song.empty()?"none":selected_song)<<" remaining="<<((room==net::RoomState::voting&&song.vote_deadline_us>now)?(song.vote_deadline_us-now)/1000000:0)<<"s\n";
else if(line=="stop")room=net::RoomState::free_move;
else if(line.rfind("set ",0)==0)std::cout<<"Updated "<<server::apply_set_command(line,settings,catalog)<<"\n";
else if(line.rfind("start",0)==0){const auto defaults=server::defaults_for(settings,catalog.songs().size());const auto round=server::parse_start_command(line,defaults,catalog);const bool all_ready=std::all_of(players.begin(),players.end(),[](const auto & entry){return entry.second.ready;});if(players.empty()||!all_ready||room==net::RoomState::voting||room==net::RoomState::countdown||room==net::RoomState::playing)std::cout<<"Start refused: all connected clients must be ready and the room idle\n";else{active_round=round;const auto prepared=server::prepare_round(round,shuffle);candidates=prepared.candidates;if(!prepared.selected_song.empty())selected_song=prepared.selected_song;std::vector<net::PlayerId> eligible;for(const auto&[peer,p]:players){(void)peer;if(p.active)eligible.push_back(p.id);}if(!round.mode_vote)mode=round.mode;if(round.mode_vote)vote.begin(eligible,now);if(round.song_vote)song_vote.begin(eligible,candidates,now);if(prepared.voting){room=net::RoomState::voting;std::cout<<"Voting started for 15 seconds\n";}else schedule_song(selected_song,now);}}else if(line.rfind("kick ",0)==0){const auto id=static_cast<net::PlayerId>(std::stoul(line.substr(5)));for(auto & [peer,p]:players)if(p.id==id){enet_peer_disconnect(peer,0);break;}}else if(line=="help")std::cout<<server::help();else std::cout<<"Unknown command; type help\n";}catch(const std::exception&e){std::cout<<"Command rejected: "<<e.what()<<'\n';}}
        enet_host_flush(host);++tick;next_tick+=std::chrono::microseconds(16667);std::this_thread::sleep_until(next_tick);
    }
    enet_host_destroy(host);enet_deinitialize();return 0;
} catch(const std::exception & e){std::cerr<<"Fatal server error: "<<e.what()<<'\n';return 1;}
