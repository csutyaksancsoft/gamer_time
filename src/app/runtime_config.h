#pragma once

#include <string>
#include <cstdint>

struct RuntimeConfig {
    std::string shader_dir = "shaders";
    std::string asset_dir = "assets";
    std::string server = "127.0.0.1:27020";
    std::string player_name;
    std::int16_t calibration_ms = 0;
    int initial_width = 1280;
    int initial_height = 720;
};
