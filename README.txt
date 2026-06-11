Crane Operator: Safe Cargo Delivery
===================================

Mission
-------
Control the crane trolley on the top rail, guide the swinging cargo through
industrial timing hazards, lower it into the recessed green delivery bay, and
hold it stable until the delivery timer fills.

This project is a 6-level crane physics campaign. Difficulty comes from cable
control, moving gates, rotating sweepers, magnet pull, wind, unstable no-stop
zones, route checkpoints, stabilizer energy, and optional star collectibles.

Build and Run
-------------
From this folder, run:

build_crane_game.bat
crane_game.exe

The batch file looks for GCC on PATH first, then checks common MSYS2 locations.
It expects SDL2 and SDL2_ttf development headers/libs for that compiler.
SDL2_image is optional. If SDL2_image is available, PNG assets under assets/
are loaded. If not, the game remains playable through built-in SDL drawings.

Controls
--------
Menu:
Up/Down or Left/Right     Move selection
Enter                     Confirm
ESC                       Back or quit
Mouse                     Hover and left-click buttons
1-6                       Start levels 1-6 from Level Select

Gameplay:
A/D or Left/Right         Apply motor force to the trolley
W/S or Up/Down            Shorten or lengthen the total cable
Space                     Stabilizer damping, limited by stabilizer energy
R                         Restart level and reset stars
ESC                       Pause
F11 or Alt+Enter          Toggle fullscreen desktop mode
F3                        Cycle graphics quality

Project Structure
-----------------
Root files:
crane_game.c              Main game source
build_crane_game.bat      Windows build script
crane_game.exe            Built executable
README.txt                This file
DejaVuSansMono.ttf        Simple root font fallback

Assets:
assets/backgrounds/       Yard background image
assets/world/             Crane, cargo, delivery, beam, and world sprites
assets/effects/           Small wind/effect sprites
assets/ui/                Menu and HUD UI sprites
assets/hazards/           Static hazard sprites
assets/animations/        Clean frame folders for optional animated assets
assets/fonts/             Font fallback copy

Runtime:
runtime/                  Optional DLL fallback copies used by the build script

Documentation:
docs/PROJECT_STRUCTURE.txt

Collectible Stars
-----------------
Each level has exactly 3 optional stars. Cargo must touch a star to collect it;
the trolley does not collect stars. Double and triple pendulum levels use the
final cargo mass position for pickup.

Ranks:
0 stars    Delivery Complete
1 star     Bronze Operator
2 stars    Silver Operator
3 stars    Gold Operator

Campaign total is 18 stars. After level 6, the results screen shows total stars
and a campaign rank.

Win and Lose Rules
------------------
Win when all required delivery conditions are true for the level's hold time:
cargo center is inside the target, cargo is low enough in the recessed bay,
cargo speed is below the safe limit, route checkpoints are complete when a level
requires them, and the pendulum is stable enough.

Single pendulum levels check angle and angular velocity. Double/triple levels
check total chain motion.

Lose when time runs out, cargo hits a static beam/block/bay wall, cargo rises
into the HUD/ceiling area, swing angle becomes unsafe, or an unstable-zone timer
expires.

Notes
-----
Assets now live under assets/. Optional runtime DLL fallbacks live under
runtime/. Assignment9 and Assignment10 were reference folders only and are not
needed by the final organized project after the required font/DLL files are
copied into assets/fonts/ and runtime/.
