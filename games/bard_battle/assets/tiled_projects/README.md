# Opening the project in Tiled

1. Start Tiled and choose **File → Open File or Project**.
2. Open `bard_battle.tiled-project` from this directory.
3. In Tiled's **Project** panel, expand `maps` and open `brawlers_ballad.tmx`.
4. Edit tilesets through the categorized `tiles` folders in the same panel and save both the TSX and TMX after changes.

Open the `.tiled-project` file, not a `.tiled-session` file. Session files contain per-user editor state and may include paths from another computer. The map uses relative `../tiles/*.tsx` references, so the whole `tiled_projects` directory can be moved with the repository without relinking tilesets.

See [maps/README.md](maps/README.md) for map settings, animation, visual-object, and collision authoring rules.

## Tile folders

- `tiles/terrain/` — terrain sheets and color variants
- `tiles/water/` — water backgrounds, animation, and water rocks
- `tiles/props/` — freely placed bushes, rocks, and other scene props
- `tiles/shared/` — runtime atlases and images shared across categories
- `tiles/sources/` — editable source artwork such as Aseprite files

Keep a tileset's `.tsx` beside its PNG. Add new assets to the closest category, creating a narrowly named subfolder when a category becomes crowded.
