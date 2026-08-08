# Attempt Engine

Attempt Engine is the small SDL3/Vulkan engine used by the games in this repository. It is intentionally tailored to this codebase rather than designed as a general-purpose engine. It provides window/input handling, TMX map loading, collision channels, world systems, render extraction, and Vulkan rendering support.

The repository currently contains one game: [Bard Battle](games/bard_battle/README.md), a networked rhythm-combat arena. The Git repository keeps its historical directory name for now; source code and build products use the Attempt Engine and Bard Battle names.

## Repository layout

```text
attempt_engine/       Engine source, shaders, tests, and engine documentation
games/bard_battle/    Bard Battle source, assets, tests, tools, and scripts
external/             Pinned third-party dependencies
CMakeLists.txt        Monorepo build configuration
```

## Configure a game

Games live under `games/<game_id>/` and provide their own `CMakeLists.txt`, source tree, assets, tests, and README. A game links only the Attempt Engine modules it uses, such as `attempt_engine_map`, `attempt_engine_world`, `attempt_engine_render`, or the SDL/Vulkan targets.

To add another game:

1. Create `games/<game_id>/` with `src/`, `assets/`, `tests/`, and a `CMakeLists.txt`.
2. Add an opt-in root CMake option and conditionally call `add_subdirectory(games/<game_id>)`.
3. Implement the game application and executable, passing a title and initial size through `WindowConfig`.
4. Link the smallest set of `attempt_engine_*` targets required by the game.
5. Add game-owned packaging rules that copy its assets while using the engine shaders it needs.

Engine code must not include headers from a game. Game code may include and link Attempt Engine freely. This is a CMake/source convention, not a runtime plugin system.

## Build and test

Configure everything:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Useful options:

- `ATTEMPT_ENGINE_BUILD_BARD_BATTLE` — include Bard Battle; defaults to `ON`
- `BARD_BATTLE_BUILD_CLIENT` — build the SDL/Vulkan client; defaults to `ON`
- `BARD_BATTLE_BUILD_SERVER` — build the server and bot; defaults to `ON`

See [attempt_engine/README.md](attempt_engine/README.md) for module details and [games/bard_battle/README.md](games/bard_battle/README.md) for game-specific setup.
