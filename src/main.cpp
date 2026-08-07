#include "app/application.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

int main(int, char **) {
    try {
        const char * base_path = SDL_GetBasePath();
        if (!base_path) {
            throw std::runtime_error(std::string("SDL_GetBasePath failed: ") + SDL_GetError());
        }

        RuntimeConfig config;
        config.shader_dir = std::string(base_path) + "shaders";
        config.asset_dir = std::string(base_path) + "assets";

        Application application(std::move(config));
        return application.run();
    } catch (const std::exception & e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Gamer Time - Fatal Error",
            e.what(),
            nullptr
        );
        return 1;
    }
}
