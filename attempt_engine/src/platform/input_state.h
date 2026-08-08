#pragma once
#include <string>

struct InputState {
    bool quit_requested = false;
    bool resized = false;
    bool escape_pressed = false;
    bool toggle_collision_debug_pressed = false;
    bool toggle_terrain_debug_pressed = false;
    bool toggle_text_pressed = false;
    bool toggle_ui_debug_pressed = false;
    bool left_mouse_pressed = false;
    bool right_mouse_pressed = false;
    bool space_pressed = false;
    bool tab_held = false;
    bool enter_pressed = false;
    bool backspace_pressed = false;
    bool delete_pressed = false;
    bool left_pressed = false;
    bool right_pressed = false;
    bool home_pressed = false;
    bool end_pressed = false;
    bool tab_pressed = false;
    std::string text_input;
    bool move_left = false;
    bool move_right = false;
    bool move_up = false;
    bool move_down = false;
    int mouse_x = 0;
    int mouse_y = 0;
    int window_width = 0;
    int window_height = 0;
    float wheel_delta = 0.0f;
};
