#pragma once

#include "app/runtime_config.h"
#include "assets/atlas_asset.h"
#include "assets/image_loader.h"
#include "assets/tmx_map_loader.h"
#include "game/fog_of_war_system.h"
#include "net/client.h"
#include "game/world.h"
#include "platform/camera_controller.h"
#include "platform/effect_player.h"
#include "platform/sdl_platform.h"
#include "platform/song_player.h"
#include "rhythm/song_config.h"
#include "rhythm/rhythm_hud.h"
#include "render/batch_builder.h"
#include "render/depth_sorter.h"
#include "render/frustum_culler.h"
#include "render/projection_system.h"
#include "render/render_extractor.h"
#include "render/render_world.h"
#include "render/scene_renderer.h"
#include "ui/ui_model.h"

#include <chrono>
#include <string>

class Application {
public:
    explicit Application(RuntimeConfig config);
    ~Application();

    int run();

private:
    void initialize();
    void shutdown();
    void tick_frame(float dt_seconds);
    void update_window_title(const RenderBatch & batch) const;
    std::string build_overlay_text() const;
    std::string build_menu_text() const;
    std::string build_scoreboard_text() const;

    RuntimeConfig config_;
    SdlPlatform platform_;
    CameraController camera_controller_;
    World world_;
    net::Client network_;
    SongPlayer song_player_;
    EffectPlayer effect_player_;
    SongConfig song_config_{};
    RhythmHud rhythm_hud_;
    FogOfWarSystem fog_of_war_system_;
    RenderExtractor render_extractor_;
    FrustumCuller frustum_culler_;
    ProjectionSystem projection_system_;
    DepthSorter depth_sorter_;
    BatchBuilder batch_builder_;
    SceneRenderer scene_renderer_;
    bool running_ = false;
    bool initialized_ = false;
    bool show_collision_debug_ = true;
    bool solid_terrain_debug_ = false;
    bool overlay_text_visible_ = false;
    AtlasAsset scene_atlas_;
    LoadedImage scene_atlas_image_;
    std::uint32_t player_sprite_base_ = 0;
    std::chrono::steady_clock::time_point last_tick_{};
    Vec2f predicted_position_{};
    bool have_predicted_position_ = false;
    float input_send_accumulator_ = 0.0f;
    net::RhythmResult last_rhythm_result_{};
    bool have_rhythm_result_ = false;
    bool local_alive_ = true;
    std::uint64_t local_respawn_at_us_ = 0;
    bool in_menu_ = true;
    enum class MenuFocus { name, server } menu_focus_ = MenuFocus::name;
    std::string menu_name_;
    std::string menu_server_;
    std::string menu_error_;
    bool was_connected_ = false;
};
