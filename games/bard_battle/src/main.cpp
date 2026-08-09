#include "app/application.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <algorithm>

int main(int argc, char ** argv) {
    try {
        const char * base_path = SDL_GetBasePath();
        if (!base_path) {
            throw std::runtime_error(std::string("SDL_GetBasePath failed: ") + SDL_GetError());
        }

        RuntimeConfig config;
        config.shader_dir = std::string(base_path) + "shaders";
        config.asset_dir = std::string(base_path) + "assets";
        const std::string settings_path = std::string(base_path) + "client-settings.cfg";
        {
            std::ifstream settings(settings_path);
            std::string key;
            while (settings >> key) {
                if (key == "calibration_ms") settings >> config.calibration_ms;
            }
        }
        bool calibration_overridden = false;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--server" && i + 1 < argc) config.server = argv[++i];
            else if (argument == "--name" && i + 1 < argc) config.player_name = argv[++i];
            else if (argument == "--calibration-ms" && i + 1 < argc) { config.calibration_ms = static_cast<std::int16_t>(std::stoi(argv[++i])); calibration_overridden = true; }
            else throw std::runtime_error("Usage: bard_battle [--server IP] [--name Player] [--calibration-ms N] (port 27020 is automatic)");
        }
        config.calibration_ms = static_cast<std::int16_t>(std::clamp<int>(config.calibration_ms, -250, 250));
        if (calibration_overridden) {
            std::ofstream settings(settings_path, std::ios::trunc);
            if (settings) settings << "calibration_ms " << config.calibration_ms << '\n';
        }

        Application application(std::move(config));
        return application.run();
    } catch (const std::exception & e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Bard Battle - Fatal Error",
            e.what(),
            nullptr
        );
        return 1;
    }
}
