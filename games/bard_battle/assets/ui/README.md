# UI customization

- `menu/background.png` is the full main-menu panel image.
- `scoreboard/background.png` is the full centered scoreboard image.
- Both images are optional. Solid-color fallback panels are used when they are absent.
- Press **F6** to toggle translucent green click-region debugging.
- Click-region geometry and visible controls share the same `ui::Rect`, so the debug box is the real clickable area.
- Layout positions are in `Application::build_menu_ui` and `Application::build_match_ui`.

The images are copied beside the executable by the existing CMake asset-copy step. Restart the client after replacing a PNG because UI textures are loaded at startup.
