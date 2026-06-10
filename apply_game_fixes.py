from pathlib import Path
import re

GAME = Path('crane_game.c')
README = Path('README.txt')

text = GAME.read_text(encoding='utf-8')
original = text

# -----------------------------------------------------------------------------
# Helper functions: star validation and obstacle-distance checks.
# -----------------------------------------------------------------------------
helpers = r'''
static double distance_point_to_frect(double x, double y, FRect rect) {
    double closestX = clamp_double(x, rect.x, rect.x + rect.w);
    double closestY = clamp_double(y, rect.y, rect.y + rect.h);
    double dx = x - closestX;
    double dy = y - closestY;
    return sqrt(dx * dx + dy * dy);
}

static int star_too_close_to_rect(const StarCollectible *star, FRect rect, double margin) {
    return distance_point_to_frect(star->x, star->y, rect) <= star->radius + margin;
}

static void validate_level_stars(const Level *level, int levelNumber) {
    int s;
    int o;
    for (s = 0; s < STAR_COUNT; ++s) {
        const StarCollectible *star = &level->stars[s];
        if (star->radius <= 0.0) {
            printf("Warning: Level %d star %d has no radius.\n", levelNumber + 1, s + 1);
            continue;
        }
        for (o = 0; o < level->obstacleCount; ++o) {
            if (star_too_close_to_rect(star, level->obstacles[o], 18.0)) {
                printf("Warning: Level %d star %d is too close to obstacle %d. Move it if it feels impossible.\n",
                       levelNumber + 1, s + 1, o + 1);
            }
        }
    }
}
'''

marker = "static void initialize_levels(Game *game) {"
if "static double distance_point_to_frect" not in text:
    text = text.replace(marker, helpers + "\n" + marker)

# -----------------------------------------------------------------------------
# Replace all six level definitions with cleaned, harder, validated layouts.
# -----------------------------------------------------------------------------
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
       Level design pass:
       - No heavy animated hazards.
       - Stars are reachable by cargo, not placed inside collidable rectangles.
       - Hardness comes from low/high/low cable changes, corridors, unstable zones,
         stricter stability, and risky optional stars.
    */

    l = &game->levels[0];
    l->name = "Level 1: Intro Crane Test";
    l->objective = "Single pendulum: learn stars, then pocket the cargo.";
    l->hint = "Collecting all stars needs height control, but the delivery route is forgiving.";
    l->timeLimit = 105.0;
    l->startTrolleyX = 165.0;
    l->startCableLength = 270.0;
    l->startTheta = 0.03;
    l->targetBase = (FRect){972.0, 552.0, 160.0, 82.0};
    l->stableSpeedLimit = 70.0;
    l->stableAngleLimit = 0.13;
    l->stableOmegaLimit = 0.38;
    l->lowTargetRatio = 0.52;
    add_obstacle(l, (FRect){810.0, 356.0, 225.0, 34.0});
    add_obstacle(l, (FRect){700.0, 520.0, 72.0, 56.0});
    add_bay_walls(l, 514.0, 118.0);
    set_star(l, 0, 500.0, 382.0, 24.0);   /* simple route star */
    set_star(l, 1, 775.0, 485.0, 24.0);   /* requires lifting over small block */
    set_star(l, 2, 925.0, 318.0, 23.0);   /* careful ceiling star, not inside beam */

    l = &game->levels[1];
    l->name = "Level 2: Single Pendulum Gate Trial";
    l->objective = "Single pendulum: low, lift, low, then fragile bay.";
    l->hint = "One cable height cannot solve this. Change length through each gate.";
    l->timeLimit = 95.0;
    l->startTrolleyX = 160.0;
    l->startCableLength = 275.0;
    l->startTheta = -0.035;
    l->targetBase = (FRect){1038.0, 552.0, 108.0, 82.0};
    l->stableSpeedLimit = 46.0;
    l->stableAngleLimit = 0.080;
    l->stableOmegaLimit = 0.25;
    l->lowTargetRatio = 0.58;
    l->cargoKind = CARGO_FRAGILE;
    add_obstacle(l, (FRect){250.0, 318.0, 315.0, 36.0});   /* low ceiling */
    add_obstacle(l, (FRect){555.0, 504.0, 125.0, 78.0});   /* lift block */
    add_obstacle(l, (FRect){730.0, 330.0, 275.0, 36.0});   /* second low ceiling */
    add_obstacle(l, (FRect){932.0, 490.0, 50.0, 38.0});    /* side-pocket wall, not star-covered */
    add_bay_walls(l, 514.0, 118.0);
    set_star(l, 0, 415.0, 405.0, 23.0);    /* underpass star */
    set_star(l, 1, 625.0, 452.0, 23.0);    /* above lift block, reachable before collision */
    set_star(l, 2, 1005.0, 452.0, 22.0);   /* moved out of obstacle; risky pre-bay pocket */

    l = &game->levels[2];
    l->name = "Level 3: Double Pendulum Stabilization";
    l->objective = "Double pendulum: collect safely and calm the whole chain.";
    l->hint = "Cargo speed alone is not enough. The upper link must settle too.";
    l->timeLimit = 118.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 320.0;
    l->startTheta = 0.02;
    l->targetBase = (FRect){998.0, 552.0, 138.0, 82.0};
    l->stableSpeedLimit = 42.0;
    l->stableAngleLimit = 0.17;
    l->stableOmegaLimit = 0.58;
    l->safeChainMotion = 155.0;
    l->lowTargetRatio = 0.54;
    l->pendulumMode = PENDULUM_DOUBLE;
    l->dampingScale = 1.04;
    add_obstacle(l, (FRect){500.0, 350.0, 225.0, 34.0});
    add_obstacle(l, (FRect){805.0, 348.0, 175.0, 34.0});
    add_obstacle(l, (FRect){822.0, 502.0, 78.0, 80.0});
    add_bay_walls(l, 514.0, 118.0);
    set_star(l, 0, 460.0, 430.0, 23.0);   /* no longer glued to the beam */
    set_star(l, 1, 684.0, 505.0, 23.0);   /* side pocket */
    set_star(l, 2, 925.0, 432.0, 22.0);   /* calm approach star */

    l = &game->levels[3];
    l->name = "Level 4: Double Pendulum Needle Corridor";
    l->objective = "Double pendulum: thread the no-stop corridor.";
    l->hint = "The clean yellow zone is timed; do not crawl through it.";
    l->timeLimit = 125.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 335.0;
    l->startTheta = -0.025;
    l->targetBase = (FRect){1060.0, 552.0, 94.0, 82.0};
    l->stableSpeedLimit = 30.0;
    l->stableAngleLimit = 0.12;
    l->stableOmegaLimit = 0.42;
    l->safeChainMotion = 96.0;
    l->requiredHoldTime = 3.5;
    l->lowTargetRatio = 0.60;
    l->cargoKind = CARGO_FRAGILE;
    l->pendulumMode = PENDULUM_DOUBLE;
    l->dampingScale = 0.98;
    add_obstacle(l, (FRect){245.0, 318.0, 260.0, 36.0});
    add_obstacle(l, (FRect){420.0, 502.0, 88.0, 82.0});
    add_obstacle(l, (FRect){540.0, 318.0, 235.0, 36.0});
    add_obstacle(l, (FRect){712.0, 474.0, 82.0, 106.0});
    add_obstacle(l, (FRect){840.0, 356.0, 162.0, 36.0});
    add_bay_walls(l, 512.0, 120.0);
    add_unstable_zone(l, (FRect){590.0, 155.0, 170.0, 470.0}, 3.7);
    set_star(l, 0, 335.0, 414.0, 22.0);
    set_star(l, 1, 655.0, 444.0, 22.0);
    set_star(l, 2, 922.0, 492.0, 22.0);

    l = &game->levels[4];
    l->name = "Level 5: Triple Pendulum Height Puzzle";
    l->objective = "Triple pendulum: low, lift, low, calm.";
    l->hint = "Plan cable changes early. Sudden height changes wake every link.";
    l->timeLimit = 142.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 360.0;
    l->startTheta = 0.018;
    l->targetBase = (FRect){1023.0, 552.0, 112.0, 82.0};
    l->stableSpeedLimit = 26.0;
    l->stableAngleLimit = 0.15;
    l->stableOmegaLimit = 0.52;
    l->safeChainMotion = 82.0;
    l->requiredHoldTime = 3.6;
    l->lowTargetRatio = 0.60;
    l->pendulumMode = PENDULUM_TRIPLE;
    l->dampingScale = 1.02;
    add_obstacle(l, (FRect){240.0, 326.0, 310.0, 36.0});
    add_obstacle(l, (FRect){570.0, 506.0, 128.0, 80.0});
    add_obstacle(l, (FRect){732.0, 332.0, 260.0, 36.0});
    add_obstacle(l, (FRect){904.0, 506.0, 62.0, 78.0});
    add_bay_walls(l, 512.0, 120.0);
    set_star(l, 0, 380.0, 415.0, 22.0);
    set_star(l, 1, 638.0, 454.0, 22.0);
    set_star(l, 2, 970.0, 430.0, 22.0);

    l = &game->levels[5];
    l->name = "Level 6: Triple Pendulum Final Exam";
    l->objective = "Triple pendulum: boss route and fragile bay.";
    l->hint = "Low ceiling, lift block, clean no-stop zone, slot, and strict chain calm.";
    l->timeLimit = 158.0;
    l->startTrolleyX = 155.0;
    l->startCableLength = 375.0;
    l->startTheta = -0.018;
    l->targetBase = (FRect){1072.0, 552.0, 86.0, 82.0};
    l->stableSpeedLimit = 20.0;
    l->stableAngleLimit = 0.095;
    l->stableOmegaLimit = 0.32;
    l->safeChainMotion = 58.0;
    l->requiredHoldTime = 4.0;
    l->lowTargetRatio = 0.62;
    l->cargoKind = CARGO_FRAGILE;
    l->pendulumMode = PENDULUM_TRIPLE;
    l->dampingScale = 1.00;
    add_obstacle(l, (FRect){215.0, 318.0, 315.0, 36.0});
    add_obstacle(l, (FRect){548.0, 506.0, 120.0, 82.0});
    add_obstacle(l, (FRect){742.0, 306.0, 260.0, 36.0});
    add_obstacle(l, (FRect){804.0, 504.0, 84.0, 86.0});
    add_obstacle(l, (FRect){902.0, 362.0, 128.0, 34.0});
    add_obstacle(l, (FRect){962.0, 500.0, 58.0, 84.0});
    add_bay_walls(l, 512.0, 120.0);
    add_unstable_zone(l, (FRect){668.0, 155.0, 158.0, 470.0}, 3.6);
    set_star(l, 0, 356.0, 410.0, 21.0);
    set_star(l, 1, 735.0, 446.0, 21.0);
    set_star(l, 2, 1030.0, 452.0, 21.0);

    for (i = 0; i < LEVEL_COUNT; ++i) {
        validate_level_stars(&game->levels[i], i);
    }
}
'''
text = re.sub(r'static void initialize_levels\(Game \*game\) \{.*?\n\}\n\nstatic void update_cargo_position',
              new_initialize + "\n\nstatic void update_cargo_position",
              text, flags=re.S)

# -----------------------------------------------------------------------------
# Do not collect stars while colliding; collision loss should happen first.
# -----------------------------------------------------------------------------
old_update_star = r'''static void update_star_collection(Game *game) {
    Level *level = &game->levels[game->selectedLevel];
    int i;
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
'''
new_update_star = r'''static void update_star_collection(Game *game) {
    Level *level = &game->levels[game->selectedLevel];
    int i;

    /* Stars must reward controlled flying, not clipping through obstacles. */
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
'''
text = text.replace(old_update_star, new_update_star)

old_order = r'''    update_star_collection(game);
    update_unstable_zones(game, dt);
    if (game->state != STATE_PLAYING) {
        return;
    }

    if (fabs(game->cargo.theta) > MAX_SWING_RADIANS) {
        game->state = STATE_LOSE;
        game->lossReason = LOSS_SWING;
        game->loseIndex = 0;
        return;
    }

    if (cargo_hits_any_obstacle(game)) {
        game->state = STATE_LOSE;
        game->lossReason = LOSS_COLLISION;
        game->loseIndex = 0;
        return;
    }

    update_stability_and_win(game, dt);
'''
new_order = r'''    update_unstable_zones(game, dt);
    if (game->state != STATE_PLAYING) {
        return;
    }

    if (fabs(game->cargo.theta) > MAX_SWING_RADIANS) {
        game->state = STATE_LOSE;
        game->lossReason = LOSS_SWING;
        game->loseIndex = 0;
        return;
    }

    if (cargo_hits_any_obstacle(game)) {
        game->state = STATE_LOSE;
        game->lossReason = LOSS_COLLISION;
        game->loseIndex = 0;
        return;
    }

    update_star_collection(game);
    update_stability_and_win(game, dt);
'''
text = text.replace(old_order, new_order)

# -----------------------------------------------------------------------------
# Cleaner obstacle labels and unstable zones: no full-height noisy diagonal lines.
# -----------------------------------------------------------------------------
old_draw_obstacles = r'''static void draw_obstacles(App *app, const Level *level) {
    int i;
    for (i = 0; i < level->obstacleCount; ++i) {
        const char *label = (level->obstacles[i].h <= 48.0) ? "LOW CLEARANCE" : "BARRIER";
        drawSteelBeam(app, level->obstacles[i], label);
    }
}
'''
new_draw_obstacles = r'''static void draw_obstacles(App *app, const Level *level) {
    int i;
    for (i = 0; i < level->obstacleCount; ++i) {
        FRect obstacle = level->obstacles[i];
        const char *label = "";
        if (obstacle.w >= 180.0 && obstacle.h <= 48.0) {
            label = "LOW CLEARANCE";
        } else if (obstacle.h >= 78.0 && obstacle.w >= 70.0) {
            label = "BARRIER";
        }
        drawSteelBeam(app, obstacle, label);
    }
}
'''
text = text.replace(old_draw_obstacles, new_draw_obstacles)

old_unstable = r'''static void drawUnstableZones(App *app, const Level *level) {
    int i;
    for (i = 0; i < level->unstableZoneCount; ++i) {
        const UnstableZone *zone = &level->unstableZones[i];
        int stripe;
        fill_rect(app, zone->rect, color_rgba(255, 202, 50, 26));
        outline_rect(app, zone->rect, color_rgba(255, 210, 68, 190));
        for (stripe = (int)zone->rect.x - 80; stripe < (int)(zone->rect.x + zone->rect.w + 80); stripe += 36) {
            draw_thick_line(app, (double)stripe, zone->rect.y + 8.0,
                            (double)stripe + 72.0, zone->rect.y + zone->rect.h - 8.0,
                            3, color_rgba(42, 38, 28, 135));
        }
        draw_text_center(app, app->fontSmall, "UNSTABLE ZONE",
                         (int)(zone->rect.x + zone->rect.w * 0.5),
                         (int)(zone->rect.y + 18.0), color_rgba(64, 49, 14, 255));
        draw_text_center(app, app->fontSmall, "KEEP MOVING",
                         (int)(zone->rect.x + zone->rect.w * 0.5),
                         (int)(zone->rect.y + zone->rect.h - 28.0), color_rgba(64, 49, 14, 255));
    }
}
'''
new_unstable = r'''static void drawUnstableZones(App *app, const Level *level) {
    int i;
    for (i = 0; i < level->unstableZoneCount; ++i) {
        const UnstableZone *zone = &level->unstableZones[i];
        int stripe;
        FRect topStrip = {zone->rect.x, zone->rect.y, zone->rect.w, 18.0};
        FRect bottomStrip = {zone->rect.x, zone->rect.y + zone->rect.h - 18.0, zone->rect.w, 18.0};
        FRect centerPanel = {zone->rect.x + 8.0, zone->rect.y + zone->rect.h * 0.5 - 24.0,
                             zone->rect.w - 16.0, 48.0};

        fill_rect(app, zone->rect, color_rgba(255, 202, 50, 12));
        outline_rect(app, zone->rect, color_rgba(255, 210, 68, 145));
        fill_rect(app, topStrip, color_rgba(255, 202, 50, 70));
        fill_rect(app, bottomStrip, color_rgba(255, 202, 50, 70));

        for (stripe = (int)zone->rect.x - 24; stripe < (int)(zone->rect.x + zone->rect.w + 24); stripe += 34) {
            draw_thick_line(app, (double)stripe, topStrip.y + topStrip.h,
                            (double)stripe + 22.0, topStrip.y, 2, color_rgba(48, 43, 31, 155));
            draw_thick_line(app, (double)stripe, bottomStrip.y + bottomStrip.h,
                            (double)stripe + 22.0, bottomStrip.y, 2, color_rgba(48, 43, 31, 155));
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
text = text.replace(old_unstable, new_unstable)

if text == original:
    raise SystemExit('No changes were applied. The file may not match the expected version.')

GAME.write_text(text, encoding='utf-8')

if README.exists():
    readme = README.read_text(encoding='utf-8')
    note = '''

## Fix pass: star placement, visual cleanup, and harder routes

This build includes a gameplay cleanup pass:
- fixed impossible / obstacle-overlapping star placements,
- added star placement validation warnings,
- stars no longer collect while the cargo is colliding,
- cleaned the unstable-zone visuals so they no longer draw noisy full-height diagonal lines,
- reduced obstacle label clutter,
- tightened the 6-level campaign with harder static geometry and stricter double/triple-pendulum stability checks.
'''
    if 'Fix pass: star placement' not in readme:
        README.write_text(readme.rstrip() + note, encoding='utf-8')

print('Applied crane game fixes to crane_game.c')
print('Now run: build_crane_game.bat')
