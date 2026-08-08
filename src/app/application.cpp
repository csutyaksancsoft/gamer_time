#include "app/application.h"

#include "game/map_world.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <utility>

namespace {

constexpr const char * kDefaultMapName = "maps/grass_tileset_map.tmx";

} // namespace

Application::Application(RuntimeConfig config)
    : config_(std::move(config)), menu_name_(config_.player_name), menu_server_(config_.server) {
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
    scene_atlas_ = assets::build_atlas_from_tmx(map_asset, assets::resolve_tmx_tileset_image_path(map_asset));
    scene_atlas_image_ = assets::load_png_rgba(scene_atlas_.image_path);
    player_sprite_base_ = scene_atlas_.tile_count();
    const LoadedImage placeholder_atlas = assets::load_png_rgba(config_.asset_dir + "/tiles/sample_scene_atlas.png");
    assets::append_bottom_row_sprites(scene_atlas_image_, placeholder_atlas, 4);
    scene_atlas_.columns = scene_atlas_image_.width / scene_atlas_.tile_width;
    scene_atlas_.rows = scene_atlas_image_.height / scene_atlas_.tile_height;
    scene_renderer_.initialize_scene_atlas(scene_atlas_, scene_atlas_image_);
    try {
        song_config_ = load_song_config(config_.asset_dir + "/audio/song.cfg");
        song_player_.load(config_.asset_dir + "/audio", song_config_);
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
    scene_renderer_.set_debug_modes(solid_terrain_debug_, true);

    if(in_menu_){
        if(input.left_mouse_pressed){if(input.mouse_y>=245&&input.mouse_y<295)menu_focus_=MenuFocus::name;else if(input.mouse_y>=315&&input.mouse_y<365)menu_focus_=MenuFocus::server;}
        std::string & field=menu_focus_==MenuFocus::name?menu_name_:menu_server_;
        if(input.backspace_pressed&&!field.empty())field.pop_back();
        for(unsigned char c:input.text_input)if(c>=0x20&&c<=0x7e&&field.size()<128)field.push_back(static_cast<char>(c));
        const bool connect_clicked=input.left_mouse_pressed&&input.mouse_y>=390&&input.mouse_y<440;
        if(input.enter_pressed||connect_clicked){std::string host;std::uint16_t port;if(!ui::valid_player_name(menu_name_))menu_error_="Name must be 1-24 visible characters.";else if(!ui::parse_endpoint(menu_server_,host,port))menu_error_="Server must be host or host:port.";else try{network_.connect(menu_server_,menu_name_);in_menu_=false;menu_error_.clear();}catch(const std::exception&e){menu_error_=e.what();network_.disconnect();}}
        const std::array<std::uint8_t,1> visible{255};scene_renderer_.upload_frame_resources({},visible,1,1,camera_controller_.state());scene_renderer_.set_overlay_text(build_menu_text());scene_renderer_.draw_frame();return;
    }

    network_.update();
    if(network_.connected())was_connected_=true;
    if(was_connected_&&!network_.connected()){menu_error_="Disconnected: "+network_.status();in_menu_=true;was_connected_=false;const std::array<std::uint8_t,1> visible{255};scene_renderer_.upload_frame_resources({},visible,1,1,camera_controller_.state());scene_renderer_.set_overlay_text(build_menu_text());scene_renderer_.draw_frame();return;}
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
        if (!world_.collision().blocks_segment(predicted_position_, candidate)) predicted_position_ = candidate;
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
    bool ui_consumed=false;
    if(input.tab_held&&input.left_mouse_pressed&&net::team_switching_allowed(snapshot.room)&&snapshot.team_count>=2){const int segment=std::clamp(input.mouse_x*int(snapshot.team_count)/(std::max)(input.window_width,1),0,int(snapshot.team_count)-1);network_.request_team(static_cast<net::TeamId>(segment+1));ui_consumed=true;}
    if(voting&&input.left_mouse_pressed){if(input.mouse_y>=300&&input.mouse_y<350){network_.request_vote(net::VoteChoice::teams);ui_consumed=true;}else if(input.mouse_y>=360&&input.mouse_y<410){network_.request_vote(net::VoteChoice::ffa);ui_consumed=true;}}
    if(input.left_mouse_pressed&&!ui_consumed&&local_alive_){const Vec2f target=screen_to_world(input.mouse_x,input.mouse_y,input.window_width,input.window_height,camera_controller_.state());const Vec2f aim=normalize_or_zero(target-predicted_position_);rhythm_hud_.predict_hit(rhythm_now_us,config_.calibration_ms);network_.send_rhythm_hit(config_.calibration_ms,aim,net::RhythmAction::shoot);}
    else if(input.right_mouse_pressed&&local_alive_){rhythm_hud_.predict_hit(rhythm_now_us,config_.calibration_ms);network_.send_rhythm_hit(config_.calibration_ms,{},net::RhythmAction::shield);}
    net::SongSchedule schedule{};
    if(network_.take_song_schedule(schedule)) {
        const auto local_start = static_cast<std::uint64_t>(static_cast<std::int64_t>(schedule.server_start_us) - network_.server_offset_us());
        song_player_.schedule(local_start);
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
        overlay_text_visible_ ? build_overlay_text() : std::string{}
    );
    frustum_culler_.run(render_world, input.window_width, input.window_height);
    projection_system_.run(render_world, input.window_width, input.window_height);
    depth_sorter_.run(render_world);
    RenderBatch batch = batch_builder_.build(render_world);
    rhythm_hud_.append_instances(batch,input.window_width,input.window_height,rhythm_now_us);

    scene_renderer_.upload_frame_resources(
        batch,
        world_.fog_mask(),
        world_.fog_width(),
        world_.fog_height(),
        camera_controller_.state()
    );
    if(input.tab_held)scene_renderer_.set_overlay_text(build_scoreboard_text());
    else if(voting){const auto remaining=snapshot.vote_deadline_us>network_.server_time_us()?(snapshot.vote_deadline_us-network_.server_time_us()+999999)/1000000:0;std::ostringstream text;text<<"VOTE: MATCH MODE ("<<remaining<<"s)\n\nTEAMS [click y=300]  "<<unsigned(snapshot.teams_votes)<<"\nFREE-FOR-ALL [click y=360]  "<<unsigned(snapshot.ffa_votes)<<"\n\nVotes "<<unsigned(snapshot.teams_votes+snapshot.ffa_votes)<<" / "<<unsigned(snapshot.eligible_voters);scene_renderer_.set_overlay_text(text.str());}
    else scene_renderer_.set_overlay_text(render_world.overlay_text);
    scene_renderer_.draw_frame();
    update_window_title(batch);
}

std::string Application::build_menu_text() const {std::ostringstream out;out<<"GAMER TIME\n\nName"<<(menu_focus_==MenuFocus::name?" > ":"   ")<<menu_name_<<"\n\nServer/IP"<<(menu_focus_==MenuFocus::server?" > ":"   ")<<menu_server_<<"\n\nCONNECT (Enter)\n";if(!menu_error_.empty())out<<"\nERROR: "<<menu_error_<<'\n';return out.str();}

std::string Application::build_scoreboard_text() const {const auto&s=network_.snapshot();std::ostringstream out;out<<(s.mode==net::GameMode::teams?"TEAMS":"FREE-FOR-ALL")<<" SCOREBOARD\n";auto row=[&](const net::PlayerState&p){out<<ui::truncate_ellipsis(p.name,18)<<"  R "<<p.round_kills<<'/'<<p.round_deaths<<"  S "<<p.session_kills<<'/'<<p.session_deaths<<'\n';};if(s.mode==net::GameMode::teams){for(const auto&g:ui::sort_teams(s.players,s.team_count)){out<<"\nTEAM "<<unsigned(g.team)<<" - "<<g.kills<<" KILLS\n";for(const auto&p:g.players)row(p);}}else for(const auto&p:ui::sort_ffa(s.players))row(p);if(net::team_switching_allowed(s.room))out<<"\nChoose team: Red / Blue"<<(s.team_count>2?" / Green":"")<<(s.team_count>3?" / Gold":"");return out.str();}

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
