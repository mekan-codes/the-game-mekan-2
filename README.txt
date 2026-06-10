Crane Operator: Safe Cargo Delivery
===================================

Mission
-------
Control the crane trolley on the top rail, lower the cargo into the green recessed delivery bay, and keep it stable there for 3 continuous seconds.

This version is a clean 6-level pendulum campaign. The old animated hazard gauntlet is not part of gameplay: wind, magnet, laser, crusher, bridge, sweeper, hammer, and moving dock hazard loops are unused by the six campaign levels. Static visuals remain clear and polished, with built-in SDL drawing fallbacks for the crane, rail, rope, hook, cargo, background, buildings, delivery bay, steel beams, UI, and level scenery.

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
Space                     Stabilizer damping
R                         Restart level
ESC                       Pause
F11 or Alt+Enter          Toggle fullscreen desktop mode
F3                        Cycle graphics quality

Win screen:
Enter or N                Continue to the next level, or return to Level Select after level 6
R                         Replay level
L                         Level Select
M                         Main Menu

Lose screen:
R                         Retry level
L                         Level Select
M                         Main Menu

HUD
---
The HUD shows the level number/name, pendulum mode, time, cable length, cargo speed, objective, and delivery stability timer.

For double and triple pendulum levels, the HUD adds a short reminder to use smooth, small inputs. Intermediate joints are drawn on the cable so the linked cargo is readable at a glance.

Physics
-------
Levels 1-2 preserve the original single-pendulum model:

theta_ddot = -(g / L) * sin(theta)
             - (trolley_ax / L) * cos(theta)
             - damping * omega

The game uses semi-implicit Euler integration for the single pendulum:

omega += theta_ddot * dt
theta += omega * dt

Levels 3-6 use linked point masses for double and triple pendulums. Each physics step:
1. Advances each mass with Verlet-style position integration.
2. Applies gravity.
3. Solves distance constraints from the trolley pivot through each link.
4. Uses the final cargo mass for collision, win checks, speed, and stability.

W/S changes total cable length. Double pendulum levels split that length evenly across two segments, and triple pendulum levels split it evenly across three segments.

The stabilizer increases damping gradually. It calms motion but does not instantly freeze the cargo.

The fixed physics step is 1/120 second.

Campaign Levels
---------------
All 6 levels are unlocked in Level Select.

Level 1: Intro Crane Test
Single pendulum. Open route, wide target, and a simple overhead beam near the target.

Level 2: Hard Single Pendulum
Single pendulum. Narrow passage, ceiling restriction, recessed target, and fragile delivery threshold.

Level 3: Double Pendulum Training
Double pendulum. Semi-open training space with a wide target and visible two-link motion.

Level 4: Hard Double Pendulum
Double pendulum. Narrow corridor, low underpass, raised obstacle, recessed target, and stricter stability thresholds.

Level 5: Triple Pendulum Training
Triple pendulum. Open route, wide target, long time limit, and clearly drawn three-segment motion.

Level 6: Triple Pendulum Final Exam
Triple pendulum. Tight route, height restriction, narrow fragile bay, and fair final timing.

Win and Lose Rules
------------------
Win only when all conditions are true for 3 continuous seconds:
The cargo center is inside the green target, cargo is low enough in the recessed bay, cargo speed is below the safe delivery limit, swing angle is small, and angular velocity is low.

Lose when time runs out, the cargo hits a static steel beam, the cargo rises into the HUD/ceiling area, or the swing angle becomes extremely unsafe.

Build and Run
-------------
From this folder, run:

build_crane_game.bat
crane_game.exe

The batch file looks for GCC on PATH first, then checks common MSYS2 locations. It expects SDL2 and SDL2_ttf development headers/libs for that compiler. If SDL2_image is available, existing cached PNG assets can still load; if not, the game remains playable through the built-in SDL drawings.

The game uses a fixed 1280x720 logical resolution. Resized windows and fullscreen desktop mode are letterboxed when needed so gameplay is not cropped.

Performance Notes
-----------------
Default graphics mode is Simple Fast. It keeps the static crane, dock, cargo, rail, background, beam, and UI visuals while avoiding heavy animated hazard overlays during gameplay.

F3 cycles graphics quality at runtime. Textures are cached at startup and are not reloaded during gameplay. The main loop clamps catch-up time after stalls so a hitch does not trigger a long burst of slow updates.
