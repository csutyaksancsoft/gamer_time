# Scoreboard artwork

Drop a PNG named `background.png` in this folder. It is loaded at startup and stretched to the centered scoreboard panel shown while Tab is held. Any PNG dimensions are accepted; 1600x1200 or another 4:3 image is a convenient working size.

Player rows and team buttons are drawn over this image. Press F6 and hold Tab to display the exact team-button click rectangles while adjusting `Application::build_match_ui`.
