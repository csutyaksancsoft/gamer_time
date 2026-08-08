#include "app/application.h"

#include "game/map_world.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <utility>

namespace {

constexpr const char * kDefaultMapName = "tiled_projects/maps/brawlers_ballad.tmx";

} // namespace

Application::Application(RuntimeConfig config)
    : config_(std::move(config)), menu_name_(config_.player_name), menu_server_(config_.server) {
    name_cursor_=menu_name_.size();server_cursor_=menu_server_.size();
}

Application::~Application() {
    shutdown();
}

int Application::run() {
    initialize();
    running_ = true;
    last_tick_ = std::chrono::steady_clock::now();

    while (running_) {
        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<float> delta = now - last_tick_;
        last_tick_ = now;
        tick_frame(std::clamp(delta.count(), 0.0f, 0.05f));
    }

    shutdown();
    return 0;
}

void Application::initialize() {
    if (initialized_) {
        return;
    }

    platform_.initialize(config_);
    scene_renderer_.initialize(platform_.window(), config_.shader_dir);
    const std::string map_path = config_.asset_dir + "/" + kDefaultMapName;
    const TmxMapAsset map_asset = assets::load_tmx_map(map_path);
    world_.set_map(MapWorld::from_tmx(map_asset));
    RuntimeAtlas runtime_atlas = assets::build_runtime_atlas(map_asset);
    scene_atlas_ = std::move(runtime_atlas.atlas);
    scene_atlas_image_ = std::move(runtime_atlas.image);
    player_sprite_base_ = scene_atlas_.tile_count();
    const LoadedImage placeholder_atlas = assets::load_png_rgba(config_.asset_dir + "/tiled_projects/tiles/shared/sample_scene_atlas.png");
    assets::append_packed_sprites(scene_atlas_image_, placeholder_atlas, 4, scene_atlas_.tile_width, scene_atlas_.columns, player_sprite_base_);
    scene_atlas_.logical_tile_count += 4;
    scene_atlas_.columns = scene_atlas_image_.width / scene_atlas_.tile_width;
    scene_atlas_.rows = scene_atlas_image_.height / scene_atlas_.tile_height;
    scene_renderer_.initialize_scene_atlas(scene_atlas_, scene_atlas_image_);
    auto load_ui_image=[&](const std::string&relative,ui::Color fallback){const std::string path=config_.asset_dir+relative;if(std::filesystem::exists(path))return assets::load_png_rgba(path);LoadedImage image{};image.width=image.height=1;image.rgba_pixels={static_cast<std::uint8_t>(fallback.r*255),static_cast<std::uint8_t>(fallback.g*255),static_cast<std::uint8_t>(fallback.b*255),static_cast<std::uint8_t>(fallback.a*255)};return image;};
    scene_renderer_.initialize_ui_images(load_ui_image("/ui/menu/background.png",{0.025f,0.035f,0.075f,0.97f}),load_ui_image("/ui/scoreboard/background.png",{0.025f,0.035f,0.075f,0.96f}));
    try {
        song_catalog_ = SongCatalog::load(config_.asset_dir + "/audio/songs");
    } catch (const std::exception &) {
        // Networking and movement remain usable until the real song asset is supplied.
    }
    effect_player_.load(config_.asset_dir + "/audio/sfx");
    initialized_ = true;
}

void Application::shutdown() {
    if (!initialized_) {
        return;
    }

    scene_renderer_.shutdown();
    network_.disconnect();
    song_player_.stop();
    effect_player_.shutdown();
    platform_.shutdown();

    initialized_ = false;
    running_ = false;
}

void Application::tick_frame(float dt_seconds) {
    const InputState input = platform_.poll_input();
    if (input.quit_requested || (input.escape_pressed && in_menu_)) {
        running_ = false;
        return;
    }

    if (input.resized) {
        scene_renderer_.request_resize();
    }

    if (input.toggle_collision_debug_pressed) {
        show_collision_debug_ = !show_collision_debug_;
    }
    if (input.toggle_terrain_debug_pressed) solid_terrain_debug_ = !solid_terrain_debug_;
    if (input.toggle_text_pressed) overlay_text_visible_ = !overlay_text_visible_;
    if (input.toggle_ui_debug_pressed) ui_debug_visible_ = !ui_debug_visible_;
    scene_renderer_.set_debug_modes(solid_terrain_debug_, true);

    if(in_menu_){
        if(menu_connecting_){network_.update();if(network_.welcomed()){menu_connecting_=false;in_menu_=false;}else if(network_.status()=="disconnected"||net::monotonic_time_us()-menu_connect_started_us_>10000000ULL){menu_error_=network_.status()=="disconnected"?"Connection failed or server refused the connection.":"Connection timed out after 10 seconds.";menu_connecting_=false;network_.disconnect();}}
        ui::DrawList menu_ui=build_menu_ui(input.window_width,input.window_height);
        if(menu_connecting_){for(auto&run:menu_ui.text)if(run.text=="CONNECT")run.text="CONNECTING...";for(auto&hit:menu_ui.hits)if(hit.action==ui::Action::connect)hit.enabled=false;}
        {const float w=std::clamp(input.window_width*0.62f,520.0f,820.0f),h=std::clamp(input.window_height*0.72f,430.0f,620.0f),x=(input.window_width-w)*0.5f,y=(input.window_height-h)*0.5f;const bool name=menu_focus_==MenuFocus::name;const float field_x=x+w*0.16f,field_y=name?y+h*0.34f:y+h*0.52f;const std::size_t cursor=name?name_cursor_:server_cursor_;const bool caret_visible=(net::monotonic_time_us()/500000ULL)%2==0;if(caret_visible)menu_ui.quads.push_back({{field_x+14+cursor*16.0f,field_y+15,3,30},0,{1,1,1,1},false});}
        if(ui_debug_visible_)append_ui_debug(menu_ui);
        const ui::Action action=clicked_action(menu_ui,input);
        if(action==ui::Action::none&&input.left_mouse_pressed)menu_focus_=menu_focus_;
        else if(action==ui::Action::focus_name){menu_focus_=MenuFocus::name;name_cursor_=menu_name_.size();}
        else if(action==ui::Action::focus_server){menu_focus_=MenuFocus::server;server_cursor_=menu_server_.size();}
        if(input.tab_pressed)menu_focus_=menu_focus_==MenuFocus::name?MenuFocus::server:MenuFocus::name;
        std::string & field=menu_focus_==MenuFocus::name?menu_name_:menu_server_;std::size_t & cursor=menu_focus_==MenuFocus::name?name_cursor_:server_cursor_;cursor=std::min(cursor,field.size());
        if(action==ui::Action::focus_name||action==ui::Action::focus_server){const float w=std::clamp(input.window_width*0.62f,520.0f,820.0f),x=(input.window_width-w)*0.5f,character_x=x+w*0.16f+14;const float relative=(input.mouse_x-character_x)/16.0f;cursor=static_cast<std::size_t>(std::clamp(std::lround(relative),0L,static_cast<long>(field.size())));}
        if(input.left_pressed&&cursor>0)--cursor;
        if(input.right_pressed&&cursor<field.size())++cursor;
        if(input.home_pressed)cursor=0;
        if(input.end_pressed)cursor=field.size();
        if(input.backspace_pressed&&cursor>0){field.erase(cursor-1,1);--cursor;}
        if(input.delete_pressed&&cursor<field.size())field.erase(cursor,1);
        for(unsigned char c:input.text_input)if(c>=0x20&&c<=0x7e&&field.size()<128){field.insert(field.begin()+static_cast<std::ptrdiff_t>(cursor),static_cast<char>(c));++cursor;}
        const bool connect_clicked=action==ui::Action::connect;
        if(menu_connecting_)menu_error_=network_.connected()?"Connected; waiting for server welcome...":"Connecting to "+menu_server_+"...";
        if(!menu_connecting_&&(input.enter_pressed||connect_clicked)){std::string host;std::uint16_t port;if(!ui::valid_player_name(menu_name_))menu_error_="Name must be 1-24 visible characters.";else if(!ui::parse_endpoint(menu_server_,host,port))menu_error_="Enter the server IP without a port (27020 is automatic).";else try{network_.connect(menu_server_,menu_name_);menu_connecting_=true;menu_connect_started_us_=net::monotonic_time_us();menu_error_="Connecting to "+menu_server_+" on port 27020...";}catch(const std::exception&e){menu_error_=e.what();network_.disconnect();}}
        scene_renderer_.set_ui_draw_list(std::move(menu_ui));const std::array<std::uint8_t,1> visible{255};scene_renderer_.upload_frame_resources({},visible,1,1,camera_controller_.state());scene_renderer_.draw_frame();return;
    }

    network_.update();
    if(network_.connected())was_connected_=true;
    if(was_connected_&&!network_.connected()){menu_error_="Disconnected: "+network_.status();in_menu_=true;was_connected_=false;auto menu_ui=build_menu_ui(input.window_width,input.window_height);if(ui_debug_visible_)append_ui_debug(menu_ui);scene_renderer_.set_ui_draw_list(std::move(menu_ui));const std::array<std::uint8_t,1> visible{255};scene_renderer_.upload_frame_resources({},visible,1,1,camera_controller_.state());scene_renderer_.draw_frame();return;}
    net::SoundEvent sound_event{};
    while(network_.take_sound_event(sound_event))effect_player_.play(sound_event.cue,predicted_position_,sound_event.position,sound_event.participant);
    const net::Snapshot & snapshot = network_.snapshot();
    if (!snapshot.players.empty()) {
        std::vector<NetworkUnitState> units;
        units.reserve(snapshot.players.size()+snapshot.projectiles.size());
        UnitId render_id=0;
        for(const net::PlayerState & player:snapshot.players){if(player.id>render_id)render_id=player.id;}
        for (const net::PlayerState & player : snapshot.players) {
            const bool just_respawned=player.id==network_.player_id()&&!local_alive_&&player.alive;
            if(player.id==network_.player_id()){local_alive_=player.alive;local_respawn_at_us_=player.respawn_at_us;if(just_respawned){predicted_position_=player.position;have_predicted_position_=true;}}
            if(!player.alive)continue;
            Vec2f position = player.position;
            if (player.id == network_.player_id()) {
                if (!have_predicted_position_) { predicted_position_ = position; have_predicted_position_ = true; }
                else { predicted_position_ += (position - predicted_position_) * 0.18f; }
                position = predicted_position_;
            }
            NetworkUnitState unit{};
            unit.id = player.id;
            unit.position = position;
            unit.sprite_index = player_sprite_base_ + ((player.id - 1u) % 4u);
            unit.rotation_radians = player.facing_angle;
            if (player.protected_until_us > snapshot.server_time_us &&
                ((snapshot.server_time_us / 100000) % 2) == 0) {
                unit.solid_color = true;
                unit.color[0] = 0.3f;
                unit.color[1] = 0.9f;
                unit.color[2] = 1.0f;
            }
            units.push_back(unit);
            if(player.shield_until_us>snapshot.server_time_us){NetworkUnitState shield{};shield.id=++render_id;shield.position=position;shield.size={40.0f,40.0f};shield.solid_color=true;shield.circle_outline=true;shield.color[0]=0.1f;shield.color[1]=0.55f;shield.color[2]=1.0f;shield.color[3]=0.42f;units.push_back(shield);}
        }
        for(const net::ProjectileState & p:snapshot.projectiles){NetworkUnitState unit{};unit.id=++render_id;unit.position=p.position;unit.sprite_index=1000001u;unit.size={18.0f,6.0f};unit.rotation_radians=p.angle;unit.solid_color=true;unit.color[0]=1.0f;unit.color[1]=0.85f;unit.color[2]=0.15f;units.push_back(unit);}
        world_.replace_network_units(units);
        world_.set_local_unit(network_.player_id());
    }

    Vec2f movement{};
    movement.x = static_cast<float>(input.move_right) - static_cast<float>(input.move_left);
    movement.y = static_cast<float>(input.move_up) - static_cast<float>(input.move_down);
    movement = local_alive_?normalize_or_zero(movement):Vec2f{};
    if (have_predicted_position_) {
        const Vec2f candidate = predicted_position_ + movement * (net::kMoveSpeed * dt_seconds);
        if (!world_.collision().blocks_segment(CollisionChannel::Player, predicted_position_, candidate)) predicted_position_ = candidate;
        camera_controller_.follow(predicted_position_);
        if (TransformComponent * transform = world_.try_transform(network_.player_id())) transform->position = predicted_position_;
    }
    camera_controller_.update(input, dt_seconds);
    input_send_accumulator_ += dt_seconds;
    if (input_send_accumulator_ >= 1.0f / 60.0f) {
        input_send_accumulator_ = 0.0f;
        network_.send_input(static_cast<std::int8_t>(movement.x * 127.0f), static_cast<std::int8_t>(movement.y * 127.0f));
    }
    const std::uint64_t rhythm_now_us=net::monotonic_time_us();
    const bool voting=snapshot.room==net::RoomState::voting;
    const auto local_player=std::find_if(snapshot.players.begin(),snapshot.players.end(),[&](const auto&p){return p.id==network_.player_id();});
    if(local_player!=snapshot.players.end()){if(pending_vote_!=net::VoteChoice::none&&local_player->vote==pending_vote_)pending_vote_=net::VoteChoice::none;if(pending_song_vote_!=0xff&&local_player->song_vote==pending_song_vote_)pending_song_vote_=0xff;if(pending_team_!=net::kNoTeam&&local_player->team==pending_team_)pending_team_=net::kNoTeam;}
    ui::DrawList match_ui=build_match_ui(input.window_width,input.window_height,input.tab_held);
    const net::VoteChoice accepted_vote=local_player==snapshot.players.end()?net::VoteChoice::none:local_player->vote;
    const net::TeamId accepted_team=local_player==snapshot.players.end()?net::kNoTeam:local_player->team;
    for(auto&hit:match_ui.hits){
        const bool teams_vote=hit.action==ui::Action::vote_teams,ffa_vote=hit.action==ui::Action::vote_ffa;
        const bool is_song=hit.action>=ui::Action::song_1&&hit.action<=ui::Action::song_8;
        const auto song_index=is_song?static_cast<std::uint8_t>(static_cast<int>(hit.action)-static_cast<int>(ui::Action::song_1)):0xff;
        const net::VoteChoice displayed_vote=pending_vote_!=net::VoteChoice::none?pending_vote_:accepted_vote;
        const bool vote_selected=(teams_vote&&displayed_vote==net::VoteChoice::teams)||(ffa_vote&&displayed_vote==net::VoteChoice::ffa);
        if((teams_vote||ffa_vote)&&pending_vote_!=net::VoteChoice::none)hit.enabled=false;
        net::TeamId button_team=net::kNoTeam;if(hit.action>=ui::Action::team_red&&hit.action<=ui::Action::team_gold)button_team=static_cast<net::TeamId>(static_cast<int>(hit.action)-static_cast<int>(ui::Action::team_red)+1);
        const net::TeamId displayed_team=pending_team_!=net::kNoTeam?pending_team_:accepted_team;
        const bool team_selected=button_team!=net::kNoTeam&&button_team==displayed_team;
        const bool song_selected=is_song&&(pending_song_vote_!=0xff?pending_song_vote_:local_player==snapshot.players.end()?0xff:local_player->song_vote)==song_index;
        if(vote_selected||team_selected||song_selected){const ui::Rect mark{hit.bounds.x+4,hit.bounds.y+4,hit.bounds.width-8,hit.bounds.height-8};const float label_y=team_selected?hit.bounds.y-18:mark.y+mark.height-21;match_ui.quads.push_back({mark,0,{1,1,1,0.24f},false});match_ui.text.push_back({pending_vote_!=net::VoteChoice::none||pending_song_vote_!=0xff||pending_team_!=net::kNoTeam?"SENT":"SELECTED",{mark.x,label_y,mark.width,18},1.1f,{1,1,1,1},ui::Align::center,true});}
    }
    if(ui_debug_visible_)append_ui_debug(match_ui);
    const ui::Action action=clicked_action(match_ui,input);
    bool ui_consumed=false;
    if(action==ui::Action::vote_teams){network_.request_vote(net::VoteChoice::teams);pending_vote_=net::VoteChoice::teams;ui_consumed=true;}else if(action==ui::Action::vote_ffa){network_.request_vote(net::VoteChoice::ffa);pending_vote_=net::VoteChoice::ffa;ui_consumed=true;}else if(action>=ui::Action::song_1&&action<=ui::Action::song_8){pending_song_vote_=static_cast<std::uint8_t>(static_cast<int>(action)-static_cast<int>(ui::Action::song_1));network_.request_song_vote(pending_song_vote_);ui_consumed=true;}else if(action>=ui::Action::team_red&&action<=ui::Action::team_gold){pending_team_=static_cast<net::TeamId>(static_cast<int>(action)-static_cast<int>(ui::Action::team_red)+1);network_.request_team(pending_team_);ui_consumed=true;}
    if(input.left_mouse_pressed&&!ui_consumed&&local_alive_){const Vec2f target=screen_to_world(input.mouse_x,input.mouse_y,input.window_width,input.window_height,camera_controller_.state());const Vec2f aim=normalize_or_zero(target-predicted_position_);rhythm_hud_.predict_hit(rhythm_now_us,config_.calibration_ms);network_.send_rhythm_hit(config_.calibration_ms,aim,net::RhythmAction::shoot);}
    else if(input.right_mouse_pressed&&local_alive_){rhythm_hud_.predict_hit(rhythm_now_us,config_.calibration_ms);network_.send_rhythm_hit(config_.calibration_ms,{},net::RhythmAction::shield);}
    net::SongSchedule schedule{};
    if(network_.take_song_schedule(schedule)) {
        const auto local_start = static_cast<std::uint64_t>(static_cast<std::int64_t>(schedule.server_start_us) - network_.server_offset_us());
        const auto * config=song_catalog_?song_catalog_->find(schedule.song_id):nullptr;
        if(config&&song_player_.load(config_.asset_dir+"/audio/songs",*config))song_player_.schedule(local_start);
        rhythm_hud_.schedule(schedule,local_start);
    }
    song_player_.update(rhythm_now_us);
    effect_player_.update(rhythm_now_us);
    while(network_.take_rhythm_result(last_rhythm_result_)){have_rhythm_result_=true;rhythm_hud_.apply_result(last_rhythm_result_,rhythm_now_us);}
    fog_of_war_system_.update(world_);
    RenderWorld render_world = render_extractor_.build(
        world_,
        camera_controller_.state(),
        show_collision_debug_,
        overlay_text_visible_ ? build_overlay_text() : std::string{},
        rhythm_now_us / 1000u
    );
    frustum_culler_.run(render_world, input.window_width, input.window_height);
    projection_system_.run(render_world, input.window_width, input.window_height);
    depth_sorter_.run(render_world);
    RenderBatch batch = batch_builder_.build(render_world);
    rhythm_hud_.append_instances(batch,input.window_width,input.window_height,rhythm_now_us);

    scene_renderer_.set_ui_draw_list(std::move(match_ui));scene_renderer_.upload_frame_resources(
        batch,
        world_.fog_mask(),
        world_.fog_width(),
        world_.fog_height(),
        camera_controller_.state()
    );
    if(!input.tab_held&&!voting&&overlay_text_visible_)scene_renderer_.set_overlay_text(render_world.overlay_text);
    scene_renderer_.draw_frame();
    update_window_title(batch);
}

std::string Application::build_menu_text() const {std::ostringstream out;out<<"GAMER TIME\n\nName"<<(menu_focus_==MenuFocus::name?" > ":"   ")<<menu_name_<<"\n\nServer/IP"<<(menu_focus_==MenuFocus::server?" > ":"   ")<<menu_server_<<"\n\nCONNECT (Enter)\n";if(!menu_error_.empty())out<<"\nERROR: "<<menu_error_<<'\n';return out.str();}

std::string Application::build_scoreboard_text() const {const auto&s=network_.snapshot();std::ostringstream out;out<<(s.mode==net::GameMode::teams?"TEAMS":"FREE-FOR-ALL")<<" SCOREBOARD\n";auto row=[&](const net::PlayerState&p){out<<ui::truncate_ellipsis(p.name,18)<<"  R "<<p.round_kills<<'/'<<p.round_deaths<<"  S "<<p.session_kills<<'/'<<p.session_deaths<<'\n';};if(s.mode==net::GameMode::teams){for(const auto&g:ui::sort_teams(s.players,s.team_count)){out<<"\nTEAM "<<unsigned(g.team)<<" - "<<g.kills<<" KILLS\n";for(const auto&p:g.players)row(p);}}else for(const auto&p:ui::sort_ffa(s.players))row(p);if(net::team_switching_allowed(s.room))out<<"\nChoose team: Red / Blue"<<(s.team_count>2?" / Green":"")<<(s.team_count>3?" / Gold":"");return out.str();}

ui::Action Application::clicked_action(const ui::DrawList & list,const InputState & input) const {if(!input.left_mouse_pressed)return ui::Action::none;const float ui_mouse_y=static_cast<float>(input.window_height-input.mouse_y);for(auto it=list.hits.rbegin();it!=list.hits.rend();++it)if(it->enabled&&it->bounds.contains(static_cast<float>(input.mouse_x),ui_mouse_y))return it->action;return ui::Action::none;}

void Application::append_ui_debug(ui::DrawList & list) const {for(const auto&hit:list.hits){ui::Color c=hit.enabled?ui::Color{0.05f,1.0f,0.25f,0.28f}:ui::Color{1.0f,0.1f,0.1f,0.28f};const char*label="CLICK REGION";if(hit.action==ui::Action::focus_name)label="NAME CLICK REGION";else if(hit.action==ui::Action::focus_server)label="SERVER CLICK REGION";else if(hit.action==ui::Action::connect)label="CONNECT CLICK REGION";else if(hit.action==ui::Action::vote_teams)label="TEAMS VOTE REGION";else if(hit.action==ui::Action::vote_ffa)label="FFA VOTE REGION";list.quads.push_back({hit.bounds,0,c,false});list.text.push_back({label,{hit.bounds.x+5,hit.bounds.y+5,hit.bounds.width-10,18},1.0f,{1,1,1,1},ui::Align::left,true});}}

ui::DrawList Application::build_menu_ui(int width,int height) const {ui::DrawList list;const float w=std::clamp(width*0.62f,520.0f,820.0f),h=std::clamp(height*0.72f,430.0f,620.0f),x=(width-w)*0.5f,y=(height-h)*0.5f;list.quads.push_back({{x,y,w,h},0,{0.025f,0.035f,0.075f,0.97f},false});list.text.push_back({"GAMER TIME",{x,y+36,w,48},3.0f,{1,1,1,1},ui::Align::center,true});const ui::Rect name{x+w*0.16f,y+h*0.34f,w*0.68f,58};const ui::Rect server{x+w*0.16f,y+h*0.52f,w*0.68f,58};const ui::Rect connect{x+w*0.28f,y+h*0.72f,w*0.44f,64};list.quads.push_back({name,0,menu_focus_==MenuFocus::name?ui::Color{0.16f,0.28f,0.46f,1}:ui::Color{0.08f,0.11f,0.18f,1},false});list.quads.push_back({server,0,menu_focus_==MenuFocus::server?ui::Color{0.16f,0.28f,0.46f,1}:ui::Color{0.08f,0.11f,0.18f,1},false});list.quads.push_back({connect,0,{0.18f,0.52f,0.32f,1},false});list.text.push_back({"NAME",{name.x,name.y-25,name.width,20},1.4f,{1,1,1,1},ui::Align::left,true});list.text.push_back({menu_name_.empty()?"Click and type your name":menu_name_,{name.x+14,name.y+17,name.width-28,28},2.0f,{1,1,1,1},ui::Align::left,true});list.text.push_back({"SERVER / IP",{server.x,server.y-25,server.width,20},1.4f,{1,1,1,1},ui::Align::left,true});list.text.push_back({menu_server_,{server.x+14,server.y+17,server.width-28,28},2.0f,{1,1,1,1},ui::Align::left,true});list.text.push_back({"CONNECT",{connect.x,connect.y+19,connect.width,28},2.0f,{1,1,1,1},ui::Align::center,true});if(!menu_error_.empty())list.text.push_back({menu_error_,{x+30,y+h-42,w-60,25},1.3f,{1,0.3f,0.3f,1},ui::Align::center,true});list.hits.push_back({name,ui::Action::focus_name,true});list.hits.push_back({server,ui::Action::focus_server,true});list.hits.push_back({connect,ui::Action::connect,true});return list;}

ui::DrawList Application::build_match_ui(int width,int height,bool scoreboard) const {ui::DrawList list;
const auto&s=network_.snapshot();
if(!scoreboard&&s.room==net::RoomState::voting&&s.song_vote_enabled){const float w=std::clamp(width*0.72f,520.0f,980.0f),h=std::clamp(height*0.82f,400.0f,760.0f),x=(width-w)*0.5f,y=(height-h)*0.5f;list.quads.push_back({{x,y,w,h},0,{0.035f,0.045f,0.09f,0.98f},false});const auto remaining=s.vote_deadline_us>network_.server_time_us()?(s.vote_deadline_us-network_.server_time_us()+999999)/1000000:0;list.text.push_back({"ROUND VOTE - "+std::to_string(remaining)+"s",{x+20,y+20,w-40,35},2.0f,{1,1,1,1},ui::Align::center,true});float top=y+70;if(s.mode_vote_enabled){const float bw=(w-90)*0.5f;const ui::Rect teams{x+30,top,bw,70},ffa{x+60+bw,top,bw,70};list.quads.push_back({teams,0,{0.18f,0.36f,0.68f,1},false});list.quads.push_back({ffa,0,{0.58f,0.22f,0.24f,1},false});list.text.push_back({"TEAMS  "+std::to_string(s.teams_votes),{teams.x,teams.y+23,teams.width,25},1.5f,{1,1,1,1},ui::Align::center,true});list.text.push_back({"FFA  "+std::to_string(s.ffa_votes),{ffa.x,ffa.y+23,ffa.width,25},1.5f,{1,1,1,1},ui::Align::center,true});list.hits.push_back({teams,ui::Action::vote_teams,true});list.hits.push_back({ffa,ui::Action::vote_ffa,true});top+=90;}const std::size_t count=s.song_candidates.size(),columns=count<=3?1:2,rows=(count+columns-1)/columns;const float gap=10,bw=(w-60-gap*(columns-1))/columns,bh=std::clamp((h-(top-y)-45-gap*rows)/std::max<std::size_t>(rows,1),42.0f,76.0f);for(std::size_t i=0;i<count;++i){const ui::Rect r{x+30+(i%columns)*(bw+gap),top+(i/columns)*(bh+gap),bw,bh};list.quads.push_back({r,0,{0.16f,0.28f,0.42f,1},false});const auto total=i<s.song_votes.size()?s.song_votes[i]:0;list.text.push_back({s.song_candidates[i]+"  ["+std::to_string(total)+"]",{r.x+8,r.y+bh*0.35f,r.width-16,24},1.35f,{1,1,1,1},ui::Align::center,true});list.hits.push_back({r,static_cast<ui::Action>(static_cast<int>(ui::Action::song_1)+i),true});}return list;}
if(scoreboard){const auto layout=ui::scoreboard_layout(s.players.size(),width,height);const float x=(width-layout.panel_width)*0.5f,y=(height-layout.panel_height)*0.5f;list.quads.push_back({{x,y,layout.panel_width,layout.panel_height},0,{0.025f,0.035f,0.075f,0.96f},false});list.text.push_back({s.mode==net::GameMode::teams?"TEAMS SCOREBOARD":"FREE-FOR-ALL SCOREBOARD",{x+20,y+20,layout.panel_width-40,35},2.2f,{1,1,1,1},ui::Align::center,true});const bool teams=s.mode==net::GameMode::teams;std::vector<net::PlayerState> rows=ui::sort_ffa(s.players);const float column_width=(layout.panel_width-56)/layout.columns;for(std::size_t i=0;i<rows.size();++i){const auto&p=rows[i];const auto column=static_cast<unsigned>(i/layout.rows_per_column),row=static_cast<unsigned>(i%layout.rows_per_column);const float row_x=x+28+column*column_width,row_y=y+70+row*layout.row_height;std::ostringstream line;line<<ui::truncate_ellipsis(p.name,layout.name_characters)<<"   R "<<p.round_kills<<'/'<<p.round_deaths<<"   S "<<p.session_kills<<'/'<<p.session_deaths;if(teams)line<<"   T"<<unsigned(p.team);list.text.push_back({line.str(),{row_x,row_y,column_width-10,layout.row_height},layout.font_scale*1.5f,{1,1,1,1},ui::Align::left,true});}if(net::team_switching_allowed(s.room)){const float gap=12,footer_y=y+layout.panel_height-68,bw=(layout.panel_width-40-gap*(s.team_count-1))/s.team_count;static const char*names[]={"RED","BLUE","GREEN","GOLD"};for(std::uint8_t team=1;team<=s.team_count;++team){const ui::Rect r{x+20+(team-1)*(bw+gap),footer_y,bw,48};list.quads.push_back({r,0,team==1?ui::Color{0.62f,0.12f,0.14f,1}:team==2?ui::Color{0.10f,0.30f,0.68f,1}:team==3?ui::Color{0.10f,0.52f,0.25f,1}:ui::Color{0.72f,0.55f,0.08f,1},false});list.text.push_back({names[team-1],{r.x,r.y+14,r.width,24},1.6f,{1,1,1,1},ui::Align::center,true});list.hits.push_back({r,static_cast<ui::Action>(static_cast<int>(ui::Action::team_red)+team-1),true});}}}else if(s.room==net::RoomState::voting){const float w=std::clamp(width*0.52f,500.0f,760.0f),h=330,x=(width-w)*0.5f,y=(height-h)*0.5f;list.quads.push_back({{x,y,w,h},0,{0.035f,0.045f,0.09f,0.98f},false});const auto remaining=s.vote_deadline_us>network_.server_time_us()?(s.vote_deadline_us-network_.server_time_us()+999999)/1000000:0;list.text.push_back({"CHOOSE MATCH MODE - "+std::to_string(remaining)+"s",{x+20,y+30,w-40,35},2.0f,{1,1,1,1},ui::Align::center,true});const ui::Rect teams{x+35,y+110,(w-85)*0.5f,100},ffa{x+50+(w-85)*0.5f,y+110,(w-85)*0.5f,100};list.quads.push_back({teams,0,{0.18f,0.36f,0.68f,1},false});list.quads.push_back({ffa,0,{0.58f,0.22f,0.24f,1},false});list.text.push_back({"TEAMS\n"+std::to_string(s.teams_votes)+" votes",{teams.x,teams.y+22,teams.width,62},1.7f,{1,1,1,1},ui::Align::center,true});list.text.push_back({"FREE-FOR-ALL\n"+std::to_string(s.ffa_votes)+" votes",{ffa.x,ffa.y+22,ffa.width,62},1.7f,{1,1,1,1},ui::Align::center,true});list.hits.push_back({teams,ui::Action::vote_teams,true});list.hits.push_back({ffa,ui::Action::vote_ffa,true});list.text.push_back({std::to_string(s.teams_votes+s.ffa_votes)+" / "+std::to_string(s.eligible_voters)+" players voted",{x+20,y+265,w-40,25},1.4f,{1,1,1,1},ui::Align::center,true});}return list;}

void Application::update_window_title(const RenderBatch & batch) const {
    std::string title = "gamer_time | units: ";
    title += std::to_string(world_.unit_count());
    title += " | tiles: ";
    title += std::to_string(world_.map().total_tile_count());
    title += " | visible: ";
    title += std::to_string(batch.unit_instance_count);
    title += " | net: ";
    title += network_.status();
    platform_.set_window_title(title.c_str());
}

std::string Application::build_overlay_text() const {
    std::ostringstream overlay;
    const CameraState & camera = camera_controller_.state();

    overlay << "GAMER_TIME NETWORK ARENA\n";
    overlay << "ESC quit | WASD move | LEFT CLICK fire | RIGHT CLICK shield | wheel zoom | F3 collision | F4 solid terrain | F5 text\n\n";
    overlay << "Network: " << network_.status() << " | Player ID: " << network_.player_id() << '\n';
    overlay << "Server clock offset: " << network_.server_offset_us() / 1000 << " ms\n";
    overlay << "Audio: " << (song_player_.error().empty() ? "ready" : song_player_.error()) << '\n';
    if(!effect_player_.diagnostic().empty())overlay<<"SFX: "<<effect_player_.diagnostic()<<'\n';
    if(have_rhythm_result_) overlay << "Last hit: " << net::grade_name(last_rhythm_result_.grade) << " (" << last_rhythm_result_.offset_ms << " ms) | "<<(last_rhythm_result_.shot_fired?"SHOT":last_rhythm_result_.shield_activated?"SHIELD":"NO ACTION")<<" | P/G/M " << last_rhythm_result_.perfect << "/" << last_rhythm_result_.good << "/" << last_rhythm_result_.miss << '\n';
    if(!local_alive_){const auto now=network_.server_time_us();const auto remaining=local_respawn_at_us_>now?local_respawn_at_us_-now:0;overlay<<"RESPAWNING IN "<<(remaining+999999)/1000000<<"\n";}
    const std::string rhythm_feedback=rhythm_hud_.feedback(net::monotonic_time_us());
    if(!rhythm_feedback.empty())overlay<<"RHYTHM: "<<rhythm_feedback<<" | Max combo "<<last_rhythm_result_.max_combo<<'\n';
    overlay << "Map size: " << world_.map().width() << "x" << world_.map().height() << " tiles\n";
    overlay << "Units: " << world_.unit_count() << '\n';
    overlay << "Tile layers: " << world_.map().tile_layers().size() << '\n';
    overlay << "Object layers: " << world_.map().object_layers().size() << '\n';
    overlay << "Terrain tiles: " << world_.map().total_tile_count() << '\n';
    overlay << "Collision polygons: " << world_.collision().polygon_count() << '\n';
    overlay << "Collision debug: " << (show_collision_debug_ ? "on" : "off") << '\n';
    const Vec2f snapped = scene_renderer_.snapped_camera_position();
    overlay << "Render debug: solid terrain " << (solid_terrain_debug_ ? "on" : "off") << " | fog enforced\n";
    overlay << "Camera snapped: (" << snapped.x << ", " << snapped.y << ") zoom " << camera.zoom << "\n";
    overlay << "Fog cells visible: " << std::count(world_.fog_mask().begin(), world_.fog_mask().end(), static_cast<std::uint8_t>(255)) << "\n";
    const RenderBatch & batch = scene_renderer_.staged_batch();
    const auto & frame = scene_renderer_.resources().frame_instance_buffer(scene_renderer_.current_frame_index());
    overlay << "Instances: terrain " << batch.terrain_instance_count << " | units " << batch.unit_instance_count
            << " | debug " << batch.debug_instance_count << " | total " << batch.instances.size() << '\n';
    overlay << "Frame buffer: uploaded " << frame.uploaded_bytes << " B | capacity " << frame.capacity_bytes
            << " B | reallocations " << frame.reallocation_count << '\n';
    if (!scene_renderer_.frame_diagnostic().empty()) overlay << "GPU: " << scene_renderer_.frame_diagnostic() << '\n';
    overlay << "Scene atlas grid: " << scene_atlas_.columns << "x" << scene_atlas_.rows << "\n";
    overlay << "Fog texture size: " << scene_renderer_.resources().fog_texture().width << "x" << scene_renderer_.resources().fog_texture().height << '\n';

    return overlay.str();
}
