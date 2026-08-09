# Attempt Engine modules

Attempt Engine is a compact C++20 engine built for the games in this repository.

- `attempt_engine_core`: shared math, types, and errors
- `attempt_engine_map`: TMX/TSX loading, atlas metadata, map objects, and collision
- `attempt_engine_world`: units, navigation, selection, fog of war, and world state
- `attempt_engine_render`: render-world extraction, sorting, projection, and batching
- `attempt_engine_assets`: SDL-backed image loading
- `attempt_engine_platform`: SDL window, input, and camera support
- `attempt_engine_gpu`: Vulkan device and resource management

The engine owns neutral runtime data. Network packets, game modes, rhythm rules, menus, sounds, and game assets belong to each game. If an engine API needs game data, translate it into an engine-owned structure at the game boundary—for example, Bard Battle converts snapshot players into `ReplicatedUnitState` values.

Shaders are under `attempt_engine/shaders/`. A game compiles and packages the shaders it uses. Engine tests are under `attempt_engine/tests/` and are registered by the engine CMake file.

New games should follow the configuration steps in the repository [README](../README.md). Attempt Engine does not currently offer stable binary compatibility, dynamic plugins, scripting, or a standalone SDK installation.
