Crane Operator: Safe Cargo Delivery
===================================

Mission
-------
Control the crane trolley on the top rail, guide the swinging cargo through
industrial timing hazards, lower it into the recessed green delivery bay, and
hold it stable until the delivery timer fills.

Build
-----
From the repository root, run:

build\build_crane_game.bat

The build output is:

crane_game.exe

in the repository root.

Run
---
Use the clean launcher:

Play_Crane_Game.bat

The launcher keeps the working directory at the repository root and temporarily
adds runtime\ to PATH so Windows can find the SDL DLLs without leaving DLL files
beside crane_game.exe.

Directly double-clicking crane_game.exe may fail on a machine where SDL2 DLLs are
not already available through the system PATH. That is normal for a dynamic SDL2
Windows build. Keeping DLLs in runtime\ plus using the launcher is the clean-root
solution.

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
crane_game.exe            Built executable in the root folder
src/                      C source code
build/                    Build and launcher scripts
assets/                   Images, animation frames, and fonts
runtime/                  SDL/runtime DLL fallbacks
docs/                     README and structure notes

Collectible Stars
-----------------
Each level has exactly 3 optional stars. Cargo must touch a star to collect it;
the trolley does not collect stars. Double and triple pendulum levels use the
final cargo mass position for pickup.

Win and Lose Rules
------------------
Win when all required delivery conditions are true for the level's hold time:
cargo center is inside the target, cargo is low enough in the recessed bay,
cargo speed is below the safe limit, route checkpoints are complete when a level
requires them, and the pendulum is stable enough.

Lose when time runs out, cargo hits a static beam/block/bay wall, cargo rises
into the HUD/ceiling area, swing angle becomes unsafe, or an unstable-zone timer
expires.
