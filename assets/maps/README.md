  ## Tiled Setup Instructions

  ### 1. Organize the assets

  Use one PNG and one external TSX for each tilesheet:

  assets/
  ├── maps/
  │   └── world.tmx
  └── tiles/
      ├── terrain.png
      ├── terrain.tsx
      ├── water.png
      ├── water.tsx
      ├── decorations.png
      └── decorations.tsx

  Keep every animation’s frames in the same PNG/TSX. Tiled animation frames use local tile IDs from their owning tileset and cannot span multiple TSX
  files.

  ### 2. Create the map

  In Tiled:

  1. Select File → New → New Map.
  2. Choose Orthogonal orientation.
  3. Use a finite map.
  4. Set the map tile width and height, such as 16 × 16.
  5. Save it as assets/maps/world.tmx.
  6. Configure TMX tile-layer storage as uncompressed CSV.

  The consumer will continue supporting finite, orthogonal, CSV-based TMX maps.

  ### 3. Create each external tileset

  For every tilesheet PNG:

  1. Select File → New → New Tileset.
  2. Choose Based on Tileset Image.
  3. Select the PNG.
  4. Set tile width and height equal to the map tile dimensions.
  5. Enter the correct margin and spacing if the sheet has padding.
  6. Save the tileset as an external .tsx beside the PNG.
  7. Add the external tileset to the map through the Tilesets panel.
  8. Repeat for every PNG.

  Do not choose Collection of Images. Different sheets may have different overall image dimensions and column counts, but their individual tile cells
  must match the map tile size.

  External TSX files are Tiled’s recommended format because their animations and metadata can be reused across maps. Tiled tileset documentation
  (https://doc.mapeditor.org/en/stable/manual/editing-tilesets/)

  ### 4. Define an animation

  For a sheet containing animation frames:

  1. Open its TSX for editing.
  2. Arrange the physical frames as ordinary tiles in that sheet.
  3. Select the tile that will represent the complete animation.
  4. Open View → Views and Toolbars → Tile Animation Editor if the animation panel is hidden.
  5. Drag frame tiles from the tileset into the animation frame list.
  6. Put the frames in playback order.
  7. Set each frame duration in milliseconds.
  8. Save the TSX.
  9. Enable View → Show Tile Animations to preview it in Tiled.

  The representative tile may also be the first physical frame. Runtime frame resolution will be non-recursive, so referencing it as a frame will
  display its pixels rather than restarting its animation.

  ### 5. Paint animated grid tiles

  1. Create or select a normal Tile Layer.
  2. Select the representative animated tile in the Tilesets panel.
  3. Paint it with the stamp brush exactly like a static tile.
  4. Save the TMX.

  The TMX stores one GID in each painted cell. The animation itself remains in the TSX.

  ### 6. Place animated tile objects

  1. Create an Object Layer for visual props, such as props.
  2. Keep its draw order set to Top Down.
  3. Select the representative animated tile.
  4. Select Tiled’s Insert Tile object tool.
  5. Click on the Object Layer to place it freely.
  6. Move, resize, rotate, or flip it as needed.
  7. Save the TMX.

  These tile objects will render above ordinary terrain and will be Y-sorted with players. Characters will therefore move behind or in front of props
  based on their ground position.

  Keep collision polygons on their existing collision Object Layer. Tile objects will not create or modify collision in this refactor.

  ### 7. Verify the authored map

  Before testing in-game:

  - Enable Show Tile Animations in Tiled.
  - Confirm every animation loops correctly.
  - Confirm every map tileset is external and saved.
  - Confirm TMX paths remain relative.
  - Confirm all tilesets use the map tile dimensions.
  - Confirm animation frames stay within their owning tileset.
  - Confirm visual object layers are visible.
  - Confirm the map uses uncompressed CSV tile-layer data.