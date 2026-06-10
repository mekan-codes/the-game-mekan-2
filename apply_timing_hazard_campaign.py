from pathlib import Path
import re

GAME = Path('crane_game.c')
README = Path('README.txt')

text = GAME.read_text(encoding='utf-8')
original = text

# -----------------------------------------------------------------------------
# Timing Hazard Campaign patch
# - Replaces the too-easy static layouts with timing-based hazard layouts.
# - Uses existing game systems: magnets, moving gates, sweepers, wind, unstable zones.
# - Adds external wind/magnet force support for double/triple Verlet pendulums.
# - Fixes star placement by replacing all stars with reachable positions.
# -----------------------------------------------------------------------------

# 1) Make double/triple pendulums react to wind and magnet forces.
# Existing single-pendulum physics already uses wind_acceleration_for_cargo() and
# magnet_acceleration_for_cargo(). This adds the same external horizontal force to
# the Verlet chain, strongest on the final cargo mass.
old_multi_block = '''    game->cargoInWind = 0;
    game->cargoInMagnet = 0;
    sync_chain_lengths(game);

    for (i = 0; i < game->chain.massCount; ++i) {
        ChainMass *mass = &game->chain.masses[i];
        double vx = (mass->x - mass->prevX) * damping;
        double vy = (mass->y - mass->prevY) * damping;
        double displacement = sqrt(vx * vx + vy * vy);

        if (displacement > maxDisplacement) {
            double scale = maxDisplacement / displacement;
            vx *= scale;
            vy *= scale;
        }

        mass->prevX = mass->x;
        mass->prevY = mass->y;
        mass->x += vx;
        mass->y += vy + GRAVITY * dt * dt;
    }
'''
new_multi_block = '''    game->cargoInWind = 0;
    game->cargoInMagnet = 0;
    sync_chain_lengths(game);

    /*
       External hazard forces for multi-link pendulums.
       The final cargo mass receives the strongest wind/magnet push; upper links
       receive a weaker amount so the rope reacts naturally without exploding.
    */
    {
        double externalAccel = wind_acceleration_for_cargo(game) + magnet_acceleration_for_cargo(game);
        int lastMass = game->chain.massCount - 1;
        for (i = 0; i < game->chain.massCount; ++i) {
            ChainMass *mass = &game->chain.masses[i];
            double vx = (mass->x - mass->prevX) * damping;
            double vy = (mass->y - mass->prevY) * damping;
            double displacement = sqrt(vx * vx + vy * vy);
            double forceScale = (i == lastMass) ? 1.0 : 0.35;

            if (displacement > maxDisplacement) {
                double scale = maxDisplacement / displacement;
                vx *= scale;
                vy *= scale;
            }

            mass->prevX = mass->x;
            mass->prevY = mass->y;
            mass->x += vx + externalAccel * forceScale * dt * dt;
            mass->y += vy + GRAVITY * dt * dt;
        }
    }
'''
if old_multi_block in text:
    text = text.replace(old_multi_block, new_multi_block)
elif 'externalAccel = wind_acceleration_for_cargo(game) + magnet_acceleration_for_cargo(game)' not in text:
    print('Warning: could not patch multi-pendulum hazard force block automatically.')

# 2) Replace initialize_levels with timing-hazard campaign.
new_initialize = r'''static void initialize_levels(Game *game) {
    Level *l;
    int i;
    memset(game->levels, 0, sizeof(game->levels));
    for (i = 0; i < LEVEL_COUNT; ++i) {
        game->levels[i].cargoKind = CARGO_NORMAL;
        game->levels[i].pendulumMode = PENDULUM_SINGLE;
        game->levels[i].windScale = 1.0;
        game->levels[i].magnetScale = 1.0;
        game->levels[i].dampingScale = 1.0;
        game->levels[i].requiredHoldTime = WIN_STABLE_SECONDS;
    }

    /*
       Timing-hazard campaign.
       Every level has visible openings; no cargo is expected to pass through walls.
       Hardness comes from timing magnets, rotating arms, moving gates, wind, and
       strict delivery stability. Optional stars are risky but reachable.
    */

    l = &game->levels[0];
    l->name = "Level 1: Timing Training Yard";
    l->objective = "Single pendulum: pass the moving block and land softly.";
    l->hint = "Wait for the gate to move away. Stars are optional risk.";
    l->timeLimit = 95.0;
    l->startTrolleyX = 155.0;
    l->startCableLength = 270.0;
    l->startTheta = 0.025;
    l->targetBase = (FRect){985.0, 552.0, 145.0, 82.0};
    l->stableSpeedLimit = 60.0;
    l->stableAngleLimit = 0.12;
    l->stableOmegaLimit = 0.34;
    l->lowTargetRatio = 0.54;
    add_obstacle(l, (FRect){310.0, 318.0, 275.0, 34.0});
    add_obstacle(l, (FRect){600.0, 510.0, 92.0, 76.0});
    add_obstacle(l, (FRect){790.0, 348.0, 230.0, 34.0});
    add_bay_walls(l, 514.0, 118.0);
    l->gates[l->gateCount++] = (MovingGate){(FRect){700.0, 430.0, 58.0, 145.0}, AXIS_Y, 88.0, 1.45, 0.0, "TIMED"};
    set_star(l, 0, 420.0, 400.0, 22.0);
    set_star(l, 1, 645.0, 460.0, 22.0);
    set_star(l, 2, 890.0, 430.0, 22.0);

    l = &game->levels[1];
    l->name = "Level 2: Magnet Corridor";
    l->objective = "Single pendulum: cross the magnet pull and time the gate.";
    l->hint = "The magnet pulls sideways. Do not fight it with sudden motor input.";
    l->timeLimit = 100.0;
    l->startTrolleyX = 155.0;
    l->startCableLength = 285.0;
    l->startTheta = -0.02;
    l->targetBase = (FRect){1025.0, 552.0, 118.0, 82.0};
    l->stableSpeedLimit = 44.0;
    l->stableAngleLimit = 0.082;
    l->stableOmegaLimit = 0.24;
    l->lowTargetRatio = 0.60;
    l->cargoKind = CARGO_FRAGILE;
    l->magnetScale = 1.0;
    add_obstacle(l, (FRect){250.0, 310.0, 310.0, 36.0});
    add_obstacle(l, (FRect){575.0, 508.0, 110.0, 80.0});
    add_obstacle(l, (FRect){760.0, 320.0, 260.0, 36.0});
    add_bay_walls(l, 514.0, 118.0);
    l->magnets[l->magnetCount++] = (MagnetHazard){610.0, 445.0, 190.0, 520.0, 2.4, 0.0, (FRect){570.0, 402.0, 80.0, 126.0}};
    l->gates[l->gateCount++] = (MovingGate){(FRect){880.0, 430.0, 54.0, 150.0}, AXIS_Y, 90.0, 1.65, 1.0, "GATE"};
    set_star(l, 0, 410.0, 400.0, 21.0);
    set_star(l, 1, 690.0, 395.0, 21.0);
    set_star(l, 2, 965.0, 455.0, 21.0);

    l = &game->levels[2];
    l->name = "Level 3: Sweeper Timing";
    l->objective = "Double pendulum: time the rotating arm and calm the chain.";
    l->hint = "The arm collider follows the visible rotating bar.";
    l->timeLimit = 112.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 320.0;
    l->startTheta = 0.018;
    l->targetBase = (FRect){1005.0, 552.0, 122.0, 82.0};
    l->stableSpeedLimit = 34.0;
    l->stableAngleLimit = 0.15;
    l->stableOmegaLimit = 0.48;
    l->safeChainMotion = 120.0;
    l->lowTargetRatio = 0.57;
    l->pendulumMode = PENDULUM_DOUBLE;
    l->dampingScale = 0.98;
    add_obstacle(l, (FRect){420.0, 330.0, 220.0, 34.0});
    add_obstacle(l, (FRect){610.0, 510.0, 105.0, 78.0});
    add_obstacle(l, (FRect){820.0, 350.0, 190.0, 34.0});
    add_bay_walls(l, 514.0, 118.0);
    l->sweepers[l->sweeperCount++] = (SweeperHazard){760.0, 455.0, 165.0, 18.0, 1.45, 0.0};
    l->gates[l->gateCount++] = (MovingGate){(FRect){910.0, 440.0, 52.0, 136.0}, AXIS_Y, 72.0, 1.3, 0.4, "WAIT"};
    set_star(l, 0, 465.0, 430.0, 21.0);
    set_star(l, 1, 760.0, 545.0, 21.0);
    set_star(l, 2, 948.0, 425.0, 21.0);

    l = &game->levels[3];
    l->name = "Level 4: Magnet Gate Double";
    l->objective = "Double pendulum: use magnet timing and a moving gate.";
    l->hint = "Do not park inside the yellow zone; pass when the gate opens.";
    l->timeLimit = 118.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 335.0;
    l->startTheta = -0.02;
    l->targetBase = (FRect){1055.0, 552.0, 96.0, 82.0};
    l->stableSpeedLimit = 27.0;
    l->stableAngleLimit = 0.115;
    l->stableOmegaLimit = 0.38;
    l->safeChainMotion = 86.0;
    l->requiredHoldTime = 3.6;
    l->lowTargetRatio = 0.62;
    l->cargoKind = CARGO_FRAGILE;
    l->pendulumMode = PENDULUM_DOUBLE;
    l->dampingScale = 0.92;
    add_obstacle(l, (FRect){245.0, 306.0, 285.0, 36.0});
    add_obstacle(l, (FRect){560.0, 508.0, 110.0, 82.0});
    add_obstacle(l, (FRect){760.0, 320.0, 250.0, 36.0});
    add_obstacle(l, (FRect){895.0, 504.0, 70.0, 82.0});
    add_bay_walls(l, 512.0, 120.0);
    add_unstable_zone(l, (FRect){690.0, 170.0, 165.0, 445.0}, 3.2);
    l->magnets[l->magnetCount++] = (MagnetHazard){610.0, 435.0, 185.0, 460.0, 2.2, 0.3, (FRect){572.0, 392.0, 78.0, 126.0}};
    l->gates[l->gateCount++] = (MovingGate){(FRect){850.0, 430.0, 55.0, 150.0}, AXIS_Y, 95.0, 1.75, 0.7, "TIMED"};
    set_star(l, 0, 360.0, 405.0, 20.0);
    set_star(l, 1, 660.0, 388.0, 20.0);
    set_star(l, 2, 960.0, 455.0, 20.0);

    l = &game->levels[4];
    l->name = "Level 5: Triple Rotor Wind";
    l->objective = "Triple pendulum: time the rotor, then land through wind.";
    l->hint = "Wind affects the final cargo mass. Stabilize before the bay.";
    l->timeLimit = 135.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 360.0;
    l->startTheta = 0.016;
    l->targetBase = (FRect){1022.0, 552.0, 104.0, 82.0};
    l->stableSpeedLimit = 22.0;
    l->stableAngleLimit = 0.13;
    l->stableOmegaLimit = 0.44;
    l->safeChainMotion = 62.0;
    l->requiredHoldTime = 3.8;
    l->lowTargetRatio = 0.63;
    l->pendulumMode = PENDULUM_TRIPLE;
    l->dampingScale = 0.92;
    l->windScale = 1.0;
    add_obstacle(l, (FRect){230.0, 310.0, 320.0, 38.0});
    add_obstacle(l, (FRect){575.0, 508.0, 125.0, 82.0});
    add_obstacle(l, (FRect){790.0, 315.0, 235.0, 38.0});
    add_bay_walls(l, 512.0, 120.0);
    l->sweepers[l->sweeperCount++] = (SweeperHazard){650.0, 455.0, 165.0, 18.0, 1.38, 0.6};
    l->windZones[l->windZoneCount++] = (WindZone){(FRect){870.0, 365.0, 130.0, 205.0}, 430.0, 1};
    l->gates[l->gateCount++] = (MovingGate){(FRect){760.0, 430.0, 52.0, 150.0}, AXIS_Y, 85.0, 1.45, 0.3, "GATE"};
    set_star(l, 0, 380.0, 410.0, 20.0);
    set_star(l, 1, 650.0, 540.0, 20.0);
    set_star(l, 2, 940.0, 430.0, 20.0);

    l = &game->levels[5];
    l->name = "Level 6: Final Timing Gauntlet";
    l->objective = "Triple pendulum: magnet, rotor, gate, wind, fragile bay.";
    l->hint = "This is the boss. Time hazards instead of crawling through them.";
    l->timeLimit = 145.0;
    l->startTrolleyX = 155.0;
    l->startCableLength = 375.0;
    l->startTheta = -0.016;
    l->targetBase = (FRect){1072.0, 552.0, 84.0, 82.0};
    l->stableSpeedLimit = 18.0;
    l->stableAngleLimit = 0.09;
    l->stableOmegaLimit = 0.30;
    l->safeChainMotion = 45.0;
    l->requiredHoldTime = 4.1;
    l->lowTargetRatio = 0.64;
    l->cargoKind = CARGO_FRAGILE;
    l->pendulumMode = PENDULUM_TRIPLE;
    l->dampingScale = 0.86;
    l->magnetScale = 1.0;
    l->windScale = 1.0;
    add_obstacle(l, (FRect){220.0, 306.0, 305.0, 38.0});
    add_obstacle(l, (FRect){545.0, 508.0, 120.0, 84.0});
    add_obstacle(l, (FRect){740.0, 300.0, 260.0, 38.0});
    add_obstacle(l, (FRect){900.0, 505.0, 72.0, 84.0});
    add_obstacle(l, (FRect){990.0, 355.0, 118.0, 36.0});
    add_bay_walls(l, 512.0, 120.0);
    add_unstable_zone(l, (FRect){675.0, 170.0, 155.0, 445.0}, 3.0);
    l->magnets[l->magnetCount++] = (MagnetHazard){590.0, 430.0, 180.0, 450.0, 2.4, 0.0, (FRect){552.0, 388.0, 78.0, 126.0}};
    l->sweepers[l->sweeperCount++] = (SweeperHazard){790.0, 455.0, 170.0, 18.0, 1.55, 0.2};
    l->gates[l->gateCount++] = (MovingGate){(FRect){960.0, 430.0, 54.0, 150.0}, AXIS_Y, 94.0, 1.85, 0.9, "TIMED"};
    l->windZones[l->windZoneCount++] = (WindZone){(FRect){1015.0, 390.0, 120.0, 185.0}, 420.0, -1};
    set_star(l, 0, 350.0, 405.0, 19.0);
    set_star(l, 1, 790.0, 542.0, 19.0);
    set_star(l, 2, 1032.0, 452.0, 19.0);
}
'''

text = re.sub(r'static void initialize_levels\(Game \*game\) \{.*?\n\}\n\nstatic void update_cargo_position',
              new_initialize + '\n\nstatic void update_cargo_position', text, flags=re.S)

# 3) Stars must not collect while colliding, and collision should be checked before star pickup.
text = re.sub(r'static void update_star_collection\(Game \*game\) \{.*?\n\}\n\nstatic void update_unstable_zones',
              r'''static void update_star_collection(Game *game) {
    Level *level = &game->levels[game->selectedLevel];
    int i;

    /* Do not reward clipping into a wall/rotor/gate to pick a star. */
    if (cargo_hits_any_obstacle(game)) {
        return;
    }

    for (i = 0; i < STAR_COUNT; ++i) {
        StarCollectible *star = &level->stars[i];
        double dx = game->cargo.x - star->x;
        double dy = game->cargo.y - star->y;
        double pickupRadius = star->radius + 12.0;
        if (!star->collected && dx * dx + dy * dy <= pickupRadius * pickupRadius) {
            star->collected = 1;
            game->starMessageTimer = 1.35;
        }
    }
}

static void update_unstable_zones''', text, flags=re.S)

text = re.sub(r'\n    update_star_collection\(game\);\n    update_unstable_zones\(game, dt\);',
              '\n    update_unstable_zones(game, dt);', text, count=1)
text = text.replace(
'''    if (cargo_hits_any_obstacle(game)) {
        game->state = STATE_LOSE;
        game->lossReason = LOSS_COLLISION;
        game->loseIndex = 0;
        return;
    }

    update_stability_and_win(game, dt);
''',
'''    if (cargo_hits_any_obstacle(game)) {
        game->state = STATE_LOSE;
        game->lossReason = LOSS_COLLISION;
        game->loseIndex = 0;
        return;
    }

    update_star_collection(game);
    update_stability_and_win(game, dt);
''')

# 4) Cleaner unstable zones: no giant diagonal screen scratches.
new_unstable = r'''static void drawUnstableZones(App *app, const Level *level) {
    int i;
    for (i = 0; i < level->unstableZoneCount; ++i) {
        const UnstableZone *zone = &level->unstableZones[i];
        int stripe;
        FRect topStrip = {zone->rect.x, zone->rect.y, zone->rect.w, 18.0};
        FRect bottomStrip = {zone->rect.x, zone->rect.y + zone->rect.h - 18.0, zone->rect.w, 18.0};
        FRect centerPanel = {zone->rect.x + 8.0, zone->rect.y + zone->rect.h * 0.5 - 24.0,
                             zone->rect.w - 16.0, 48.0};

        fill_rect(app, zone->rect, color_rgba(255, 202, 50, 10));
        outline_rect(app, zone->rect, color_rgba(255, 210, 68, 130));
        fill_rect(app, topStrip, color_rgba(255, 202, 50, 68));
        fill_rect(app, bottomStrip, color_rgba(255, 202, 50, 68));
        for (stripe = (int)zone->rect.x - 24; stripe < (int)(zone->rect.x + zone->rect.w + 24); stripe += 34) {
            draw_thick_line(app, (double)stripe, topStrip.y + topStrip.h,
                            (double)stripe + 22.0, topStrip.y, 2, color_rgba(48, 43, 31, 150));
            draw_thick_line(app, (double)stripe, bottomStrip.y + bottomStrip.h,
                            (double)stripe + 22.0, bottomStrip.y, 2, color_rgba(48, 43, 31, 150));
        }
        fill_rect(app, centerPanel, color_rgba(255, 225, 82, 42));
        outline_rect(app, centerPanel, color_rgba(92, 69, 25, 110));
        draw_text_center(app, app->fontSmall, "UNSTABLE ZONE",
                         (int)(centerPanel.x + centerPanel.w * 0.5),
                         (int)(centerPanel.y + 6.0), color_rgba(64, 49, 14, 255));
        draw_text_center(app, app->fontSmall, "KEEP MOVING",
                         (int)(centerPanel.x + centerPanel.w * 0.5),
                         (int)(centerPanel.y + 27.0), color_rgba(64, 49, 14, 255));
    }
}
'''
text = re.sub(r'static void drawUnstableZones\(App \*app, const Level \*level\) \{.*?\n\}\n\nstatic void drawStarCollectible',
              new_unstable + '\nstatic void drawStarCollectible', text, flags=re.S)

# 5) Reduce obstacle label clutter.
text = re.sub(r'static void draw_obstacles\(App \*app, const Level \*level\) \{.*?\n\}\n\nstatic void drawUnstableZones',
              r'''static void draw_obstacles(App *app, const Level *level) {
    int i;
    for (i = 0; i < level->obstacleCount; ++i) {
        FRect obstacle = level->obstacles[i];
        const char *label = "";
        if (obstacle.w >= 190.0 && obstacle.h <= 48.0) {
            label = "LOW CLEARANCE";
        } else if (obstacle.h >= 78.0 && obstacle.w >= 82.0) {
            label = "BARRIER";
        }
        drawSteelBeam(app, obstacle, label);
    }
}

static void drawUnstableZones''', text, flags=re.S)

# 6) README note.
if README.exists():
    readme = README.read_text(encoding='utf-8')
    note = '''

## Timing hazard campaign pass

The 6-level campaign now focuses on timing hazards instead of static-only layouts. Levels use magnets, rotating sweepers, moving gates, wind zones, unstable/no-stop zones, and stricter delivery thresholds. Double/triple pendulum cargo now receives wind/magnet external forces so hazards affect the final cargo mass. Star placements were replaced with risky but reachable positions, and stars no longer collect while the cargo is colliding.
'''
    if 'Timing hazard campaign pass' not in readme:
        README.write_text(readme.rstrip() + note, encoding='utf-8')

if text == original:
    print('Warning: no crane_game.c changes were applied. The file may already be patched or has unexpected structure.')
else:
    GAME.write_text(text, encoding='utf-8')
    print('Applied timing hazard campaign patch to crane_game.c')

print('Next: build_crane_game.bat')
