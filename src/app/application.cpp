#include "app/application.h"

#include "game/map_world.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace {

constexpr const char * kDefaultMapName = "maps/grass_tileset_map.tmx";

} // namespace

Application::Application(RuntimeConfig config)
    : config_(std::move(config)) {
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
    scene_atlas_.columns = scene_atlas_image_.width / scene_atlas_.tile_width;
    scene_atlas_.rows = scene_atlas_image_.height / scene_atlas_.tile_height;
    scene_renderer_.initialize_scene_atlas(scene_atlas_, scene_atlas_image_);
    try {
        song_config_ = load_song_config(config_.asset_dir + "/audio/song.cfg");
        song_player_.load(config_.asset_dir + "/audio", song_config_);
    } catch (const std::exception &) {
        // Networking and movement remain usable until the real song asset is supplied.
    }
    network_.connect(config_.server, config_.player_name);

    initialized_ = true;
}

void Application::shutdown() {
    if (!initialized_) {
        return;
    }

    scene_renderer_.shutdown();
    network_.disconnect();
    song_player_.stop();
    platform_.shutdown();

    initialized_ = false;
    running_ = false;
}

void Application::tick_frame(float dt_seconds) {
    const InputState input = platform_.poll_input();
    if (input.quit_requested || input.escape_pressed) {
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
    if (input.toggle_fog_pressed) fog_enabled_ = !fog_enabled_;
    scene_renderer_.set_debug_modes(solid_terrain_debug_, fog_enabled_);

    network_.update();
    const net::Snapshot & snapshot = network_.snapshot();
    if (!snapshot.players.empty()) {
        std::vector<NetworkUnitState> units;
        units.reserve(snapshot.players.size());
        for (const net::PlayerState & player : snapshot.players) {
            Vec2f position = player.position;
            if (player.id == network_.player_id()) {
                if (!have_predicted_position_) { predicted_position_ = position; have_predicted_position_ = true; }
                else { predicted_position_ += (position - predicted_position_) * 0.18f; }
                position = predicted_position_;
            }
            units.push_back({player.id, position, 49u + (player.id % 5u)});
        }
        world_.replace_network_units(units);
        world_.set_local_unit(network_.player_id());
    }

    Vec2f movement{};
    movement.x = static_cast<float>(input.move_right) - static_cast<float>(input.move_left);
    movement.y = static_cast<float>(input.move_up) - static_cast<float>(input.move_down);
    movement = normalize_or_zero(movement);
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
    if (input.space_pressed) { rhythm_hud_.predict_hit(rhythm_now_us,config_.calibration_ms);network_.send_rhythm_hit(config_.calibration_ms); }
    net::SongSchedule schedule{};
    if(network_.take_song_schedule(schedule)) {
        const auto local_start = static_cast<std::uint64_t>(static_cast<std::int64_t>(schedule.server_start_us) - network_.server_offset_us());
        song_player_.schedule(local_start);
        rhythm_hud_.schedule(schedule,local_start);
    }
    song_player_.update(rhythm_now_us);
    while(network_.take_rhythm_result(last_rhythm_result_)){have_rhythm_result_=true;rhythm_hud_.apply_result(last_rhythm_result_,rhythm_now_us);}
    fog_of_war_system_.update(world_);
    RenderWorld render_world = render_extractor_.build(
        world_,
        camera_controller_.state(),
        show_collision_debug_,
        build_overlay_text()
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
    scene_renderer_.set_overlay_text(render_world.overlay_text);
    scene_renderer_.draw_frame();
    update_window_title(batch);
}

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
    overlay << "ESC quit | WASD move | SPACE rhythm | wheel zoom | F3 collision | F4 solid terrain | F5 fog\n\n";
    overlay << "Network: " << network_.status() << " | Player ID: " << network_.player_id() << '\n';
    overlay << "Server clock offset: " << network_.server_offset_us() / 1000 << " ms\n";
    overlay << "Audio: " << (song_player_.error().empty() ? "ready" : song_player_.error()) << '\n';
    if(have_rhythm_result_) overlay << "Last hit: " << net::grade_name(last_rhythm_result_.grade) << " (" << last_rhythm_result_.offset_ms << " ms) | P/G/M " << last_rhythm_result_.perfect << "/" << last_rhythm_result_.good << "/" << last_rhythm_result_.miss << '\n';
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
    overlay << "Render debug: solid terrain " << (solid_terrain_debug_ ? "on" : "off") << " | fog " << (fog_enabled_ ? "on" : "off") << '\n';
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
