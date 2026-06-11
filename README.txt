Crane Operator: Safe Cargo Delivery
===================================

Mission
-------
Control the crane trolley on the top rail, guide the swinging cargo through static industrial geometry, lower it into the recessed green delivery bay, and hold it stable until the delivery timer fills.

This version is a harder 6-level timing-hazard pendulum campaign. The old 10-level animated hazard campaign is not used. Difficulty comes from cable control, readable timing hazards, unstable no-stopping zones, stricter delivery thresholds, chain-motion stability, wind/magnet forces, and optional star collectibles.

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
R                         Restart level and reset stars
ESC                       Pause
F11 or Alt+Enter          Toggle fullscreen desktop mode
F3                        Cycle graphics quality

Collectible Stars
-----------------
Each level has exactly 3 optional stars. Cargo must touch a star to collect it; the trolley does not collect stars. Double and triple pendulum levels use the final cargo mass position for pickup.

Stars are drawn with lightweight SDL primitives. Collected stars become dim. Restarting a level resets that attempt's stars. Level Select shows best stars per level for the current session.

Ranks:
0 stars    Delivery Complete
1 star     Bronze Operator
2 stars    Silver Operator
3 stars    Gold Operator

Campaign total is 18 stars. After level 6, the results screen shows total stars and a campaign rank.

Physics
-------
Levels 1-2 use the original driven damped single pendulum:

theta_ddot = -(g / L) * sin(theta)
             - (trolley_ax / L) * cos(theta)
             - damping * omega

Levels 3-6 use linked point masses for double and triple pendulums. W/S changes total cable length, split evenly across all links. The stabilizer increases damping gradually; it does not freeze the cargo instantly.

For double and triple pendulum delivery, cargo speed alone is not enough. The game sums velocity magnitudes across all linked masses and requires chain motion below the level's safe threshold before the delivery timer fills.

Unstable Zones
--------------
Yellow/black unstable zones are no-stopping regions. They do not collide with the cargo. If the cargo or trolley remains inside one too long, the level fails. The HUD shows the active timer and "Keep moving" warning.

Campaign Levels
---------------
All 6 levels are unlocked in Level Select.

Level 1: Intro Crane Test
Timing Training Yard. Single pendulum, moving gate, medium bay, and stars that teach timing around a visible red gate.

Level 2: Single Pendulum Gate Trial
Magnet Corridor. Single pendulum, compact magnet pull, moving gate, fragile delivery, and risky stars around the pull zone.

Level 3: Double Pendulum Stabilization
Sweeper Timing. Double pendulum, rotating arm, moving gate, chain-motion delivery check, and stars that reward timed calm control.

Level 4: Double Pendulum Needle Corridor
Magnet Gate Double. Double pendulum, compact magnet pull, moving gate, unstable/no-stop zone, risky stars, and strict final bay.

Level 5: Triple Pendulum Height Puzzle
Triple Rotor Wind. Triple pendulum, rotating arm, wind zone, moving gate, pre-target star, and stricter chain calm.

Level 6: Triple Pendulum Final Exam
Final Timing Gauntlet. Triple pendulum boss route with magnet pull, rotating arm, moving gate, wind, unstable/no-stop zone, fragile recessed bay, and the strictest delivery thresholds.

Win and Lose Rules
------------------
Win when all required delivery conditions are true for the level's hold time:
Cargo center is inside the target, cargo is low enough in the recessed bay, cargo speed is below the safe limit, and the pendulum is stable enough.

Single pendulum levels check angle and angular velocity. Double/triple levels check total chain motion.

Lose when time runs out, cargo hits a static beam/block/bay wall, cargo rises into the HUD/ceiling area, swing angle becomes unsafe, or an unstable-zone timer expires.

Build and Run
-------------
From this folder, run:

build_crane_game.bat
crane_game.exe

The batch file looks for GCC on PATH first, then checks common MSYS2 locations. It expects SDL2 and SDL2_ttf development headers/libs for that compiler. If SDL2_image is available, existing cached PNG assets can still load; if not, the game remains playable through built-in SDL drawings.

Performance Notes
-----------------
Default graphics mode is Simple Fast. Static visuals for the crane, rail, cable, hook, cargo, background, obstacles, delivery bay, HUD, and menus are preserved.

The new stars and unstable zones are cheap SDL primitive drawings. No heavy animated PNG hazards, repeated transparent overlays, or particle spam were added.

The game uses a fixed 1280x720 logical resolution. Resized windows and fullscreen desktop mode are letterboxed when needed so gameplay is not cropped.

## Timing hazard campaign pass

The 6-level campaign now focuses on timing hazards instead of static-only layouts. Levels use magnets, rotating sweepers, moving gates, wind zones, unstable/no-stop zones, and stricter delivery thresholds. Double/triple pendulum cargo now receives wind/magnet external forces so hazards affect the final cargo mass. Star placements were replaced with risky but reachable positions, and stars no longer collect while the cargo is colliding.

## Hazard visual bugfix pass

This build replaces the buggy magnet field, sweeper, moving-gate, and unstable-zone visuals with clean SDL primitive drawings. The mechanics are preserved, but old large animated/texture effects are no longer used for these hazards in normal rendering. Magnets now appear as small machines with subtle field rings, sweepers appear as clear rotating red arms, moving gates show a faint movement path, and no-stop zones use only border strips and a compact label.
