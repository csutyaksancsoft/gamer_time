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
    world_.seed_test_units();

    initialized_ = true;
}

void Application::shutdown() {
    if (!initialized_) {
        return;
    }

    scene_renderer_.shutdown();
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

    camera_controller_.update(input, dt_seconds);
    selection_system_.update(world_, input, camera_controller_.state());

    if (input.right_mouse_pressed && !world_.selected_units().empty()) {
        MoveCommand command{};
        command.units = world_.selected_units();
        command.destination = SelectionSystem::screen_to_world(input, camera_controller_.state());
        world_.command_queue().push(std::move(command));
    }

    world_.command_queue().apply(world_);
    navigation_system_.update(world_, dt_seconds);
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
    title += " | selected: ";
    title += std::to_string(world_.selected_units().size());
    platform_.set_window_title(title.c_str());
}

std::string Application::build_overlay_text() const {
    std::ostringstream overlay;
    const CameraState & camera = camera_controller_.state();

    overlay << "GAMER_TIME RTS FRAMEWORK\n";
    overlay << "ESC quit | F3 collision debug | click select | right click move | WASD/Arrows pan | wheel zoom\n\n";
    overlay << "Map size: " << world_.map().width() << "x" << world_.map().height() << " tiles\n";
    overlay << "Units: " << world_.unit_count() << '\n';
    overlay << "Tile layers: " << world_.map().tile_layers().size() << '\n';
    overlay << "Object layers: " << world_.map().object_layers().size() << '\n';
    overlay << "Terrain tiles: " << world_.map().total_tile_count() << '\n';
    overlay << "Collision polygons: " << world_.collision().polygon_count() << '\n';
    overlay << "Collision debug: " << (show_collision_debug_ ? "on" : "off") << '\n';
    overlay << "Selected: " << world_.selected_units().size() << '\n';
    overlay << "Camera: (" << static_cast<int>(camera.world_center.x) << ", " << static_cast<int>(camera.world_center.y) << ") zoom " << camera.zoom << "\n";
    overlay << "Fog cells visible: " << std::count(world_.fog_mask().begin(), world_.fog_mask().end(), static_cast<std::uint8_t>(255)) << "\n";
    overlay << "Uploaded instances: " << scene_renderer_.resources().staged_instances().size() << "\n";
    overlay << "Scene atlas grid: " << scene_atlas_.columns << "x" << scene_atlas_.rows << "\n";
    overlay << "Fog texture size: " << scene_renderer_.resources().fog_texture().width << "x" << scene_renderer_.resources().fog_texture().height << '\n';

    return overlay.str();
}
