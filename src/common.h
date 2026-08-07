#pragma once

#include "core/error.h"

#include <vulkan/vulkan.h>

constexpr int kInitialWidth = 1280;
constexpr int kInitialHeight = 720;
constexpr int kMaxFramesInFlight = 2;

inline void check_vk(VkResult result, const char* msg) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(msg);
    }
}
