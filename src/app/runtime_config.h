#pragma once

#include "common.h"

#include <string>

struct RuntimeConfig {
    std::string shader_dir = "shaders";
    std::string asset_dir = "assets";
    int initial_width = kInitialWidth;
    int initial_height = kInitialHeight;
};
