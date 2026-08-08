# Main menu artwork

Drop a PNG named `background.png` in this folder. It is loaded at startup and stretched to the centered main-menu panel. Transparent areas show the game clear color. Any PNG dimensions are accepted; 1600x1200 or another 4:3 image is a convenient working size.

The name, server, and Connect controls are drawn over this image. Press F6 in game to display their exact clickable rectangles while adjusting the percentages in `Application::build_menu_ui`.
