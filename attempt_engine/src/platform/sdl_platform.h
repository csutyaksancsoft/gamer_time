#pragma once

#include "platform/input_state.h"

#include <string>

struct WindowConfig {
    std::string title;
    int width = 1280;
    int height = 720;
};

struct SDL_Window;

class SdlPlatform {
public:
    SdlPlatform() = default;
    ~SdlPlatform();

    void initialize(const WindowConfig & config);
    void shutdown();

    InputState poll_input();
    SDL_Window * window() const {
        return window_;
    }

    void set_window_title(const char * title) const;

private:
    SDL_Window * window_ = nullptr;
};
