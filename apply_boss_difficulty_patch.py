from pathlib import Path
import re

GAME = Path('crane_game.c')
README = Path('README.txt')

text = GAME.read_text(encoding='utf-8')
original = text

# -----------------------------------------------------------------------------
# 0. Small type/field additions for required cargo checkpoint gates.
# -----------------------------------------------------------------------------
text = text.replace('#define MAX_UNSTABLE_ZONES 2\n', '#define MAX_UNSTABLE_ZONES 2\n#define MAX_SKILL_GATES 4\n')

if 'typedef struct {\n    FRect rect;\n    int passed;\n    const char *label;\n} SkillGate;' not in text:
    text = text.replace(
        'typedef struct {\n    FRect rect;\n    double timeLimit;\n} UnstableZone;\n',
        'typedef struct {\n    FRect rect;\n    double timeLimit;\n} UnstableZone;\n\n'
        'typedef struct {\n    FRect rect;\n    int passed;\n    const char *label;\n} SkillGate;\n'
    )

if 'SkillGate skillGates[MAX_SKILL_GATES];' not in text:
    text = text.replace(
        '    UnstableZone unstableZones[MAX_UNSTABLE_ZONES];\n    int unstableZoneCount;\n',
        '    UnstableZone unstableZones[MAX_UNSTABLE_ZONES];\n    int unstableZoneCount;\n'
        '    SkillGate skillGates[MAX_SKILL_GATES];\n    int skillGateCount;\n'
    )

if 'int missingSkillGate;' not in text:
    text = text.replace(
        '    int unstableZoneTooLong;\n    int lastStarsCollected;\n',
        '    int unstableZoneTooLong;\n    int missingSkillGate;\n    int lastStarsCollected;\n'
    )
if 'double skillGateMessageTimer;' not in text:
    text = text.replace(
        '    double starMessageTimer;\n    double lastCompletionTime;\n',
        '    double starMessageTimer;\n    double skillGateMessageTimer;\n    double lastCompletionTime;\n'
    )

# -----------------------------------------------------------------------------
# 1. Helpers for skill gates and validation. Insert before initialize_levels.
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
                printf("Warning: Level %d star %d is close to obstacle %d; verify it is reachable.\n",
                       levelNumber + 1, s + 1, o + 1);
            }
        }
    }
}

static void add_skill_gate(Level *level, FRect rect, const char *label) {
    if (level->skillGateCount < MAX_SKILL_GATES) {
        level->skillGates[level->skillGateCount].rect = rect;
        level->skillGates[level->skillGateCount].passed = 0;
        level->skillGates[level->skillGateCount].label = label;
        level->skillGateCount++;
    }
}

static void reset_skill_gates(Level *level) {
    int i;
    for (i = 0; i < level->skillGateCount; ++i) {
        level->skillGates[i].passed = 0;
    }
}

static int all_skill_gates_passed(const Level *level) {
    int i;
    for (i = 0; i < level->skillGateCount; ++i) {
        if (!level->skillGates[i].passed) {
            return 0;
        }
    }
    return 1;
}

static int count_passed_skill_gates(const Level *level) {
    int i;
    int count = 0;
    for (i = 0; i < level->skillGateCount; ++i) {
        if (level->skillGates[i].passed) {
            count++;
        }
    }
    return count;
}
'''

if 'static void add_skill_gate(Level *level' not in text:
    marker = 'static void initialize_levels(Game *game) {'
    text = text.replace(marker, helpers + '\n' + marker)

# Remove duplicate validation helpers if previous patch inserted them but not gates.
text = re.sub(r'\nstatic double distance_point_to_frect\(double x, double y, FRect rect\) \{.*?static int count_passed_skill_gates\(const Level \*level\) \{.*?\n\}\n\nstatic double distance_point_to_frect',
              '\nstatic double distance_point_to_frect', text, flags=re.S)

# -----------------------------------------------------------------------------
# 2. Replace level campaign: harder geometry + mandatory gates + reachable stars.
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
       Boss-difficulty campaign pass.
       Important design rule: completion now requires passing visible cargo gates.
       This prevents the old cheese strategy of moving slowly straight to the target.
       Optional stars create the harder 3-star route.
    */

    l = &game->levels[0];
    l->name = "Level 1: Intro Precision Test";
    l->objective = "Pass the gate, collect stars, then pocket the cargo.";
    l->hint = "Blue gates are mandatory cargo checkpoints; stars are optional.";
    l->timeLimit = 90.0;
    l->startTrolleyX = 165.0;
    l->startCableLength = 270.0;
    l->startTheta = 0.03;
    l->targetBase = (FRect){980.0, 552.0, 150.0, 82.0};
    l->stableSpeedLimit = 62.0;
    l->stableAngleLimit = 0.115;
    l->stableOmegaLimit = 0.34;
    l->lowTargetRatio = 0.54;
    add_obstacle(l, (FRect){780.0, 350.0, 248.0, 34.0});
    add_obstacle(l, (FRect){670.0, 520.0, 80.0, 58.0});
    add_bay_walls(l, 514.0, 118.0);
    add_skill_gate(l, (FRect){430.0, 392.0, 92.0, 82.0}, "GATE 1");
    set_star(l, 0, 500.0, 382.0, 23.0);
    set_star(l, 1, 735.0, 480.0, 23.0);
    set_star(l, 2, 916.0, 316.0, 22.0);

    l = &game->levels[1];
    l->name = "Level 2: Low-High-Low Trial";
    l->objective = "Pass all gates: low, lift, low, then fragile bay.";
    l->hint = "One cable length cannot clear the route.";
    l->timeLimit = 90.0;
    l->startTrolleyX = 160.0;
    l->startCableLength = 275.0;
    l->startTheta = -0.035;
    l->targetBase = (FRect){1040.0, 552.0, 104.0, 82.0};
    l->stableSpeedLimit = 40.0;
    l->stableAngleLimit = 0.070;
    l->stableOmegaLimit = 0.22;
    l->lowTargetRatio = 0.60;
    l->cargoKind = CARGO_FRAGILE;
    add_obstacle(l, (FRect){245.0, 305.0, 315.0, 38.0});
    add_obstacle(l, (FRect){555.0, 506.0, 130.0, 82.0});
    add_obstacle(l, (FRect){735.0, 318.0, 282.0, 38.0});
    add_obstacle(l, (FRect){925.0, 492.0, 52.0, 42.0});
    add_bay_walls(l, 514.0, 118.0);
    add_skill_gate(l, (FRect){372.0, 382.0, 86.0, 82.0}, "LOW");
    add_skill_gate(l, (FRect){610.0, 426.0, 78.0, 68.0}, "LIFT");
    add_skill_gate(l, (FRect){840.0, 390.0, 80.0, 72.0}, "LOW");
    set_star(l, 0, 415.0, 400.0, 22.0);
    set_star(l, 1, 628.0, 452.0, 22.0);
    set_star(l, 2, 1002.0, 448.0, 21.0);

    l = &game->levels[2];
    l->name = "Level 3: Double Calm Gates";
    l->objective = "Double pendulum: pass gates and calm the whole chain.";
    l->hint = "The target will not accept delivery until both gates are passed.";
    l->timeLimit = 110.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 320.0;
    l->startTheta = 0.02;
    l->targetBase = (FRect){1000.0, 552.0, 132.0, 82.0};
    l->stableSpeedLimit = 36.0;
    l->stableAngleLimit = 0.15;
    l->stableOmegaLimit = 0.50;
    l->safeChainMotion = 125.0;
    l->lowTargetRatio = 0.56;
    l->pendulumMode = PENDULUM_DOUBLE;
    l->dampingScale = 0.96;
    add_obstacle(l, (FRect){490.0, 342.0, 235.0, 34.0});
    add_obstacle(l, (FRect){800.0, 340.0, 180.0, 34.0});
    add_obstacle(l, (FRect){820.0, 500.0, 84.0, 82.0});
    add_bay_walls(l, 514.0, 118.0);
    add_skill_gate(l, (FRect){455.0, 410.0, 84.0, 80.0}, "CALM 1");
    add_skill_gate(l, (FRect){742.0, 430.0, 86.0, 80.0}, "CALM 2");
    set_star(l, 0, 458.0, 430.0, 22.0);
    set_star(l, 1, 690.0, 505.0, 22.0);
    set_star(l, 2, 926.0, 430.0, 21.0);

    l = &game->levels[3];
    l->name = "Level 4: Double Needle Boss";
    l->objective = "Double pendulum: pass the timed needle corridor.";
    l->hint = "The unstable zone is no-stop. Pass gates before delivery.";
    l->timeLimit = 115.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 335.0;
    l->startTheta = -0.025;
    l->targetBase = (FRect){1062.0, 552.0, 88.0, 82.0};
    l->stableSpeedLimit = 25.0;
    l->stableAngleLimit = 0.105;
    l->stableOmegaLimit = 0.36;
    l->safeChainMotion = 72.0;
    l->requiredHoldTime = 3.7;
    l->lowTargetRatio = 0.62;
    l->cargoKind = CARGO_FRAGILE;
    l->pendulumMode = PENDULUM_DOUBLE;
    l->dampingScale = 0.90;
    add_obstacle(l, (FRect){240.0, 305.0, 270.0, 38.0});
    add_obstacle(l, (FRect){420.0, 506.0, 90.0, 82.0});
    add_obstacle(l, (FRect){540.0, 305.0, 240.0, 38.0});
    add_obstacle(l, (FRect){710.0, 474.0, 84.0, 108.0});
    add_obstacle(l, (FRect){840.0, 348.0, 168.0, 38.0});
    add_bay_walls(l, 512.0, 120.0);
    add_unstable_zone(l, (FRect){590.0, 155.0, 170.0, 470.0}, 3.1);
    add_skill_gate(l, (FRect){340.0, 386.0, 78.0, 76.0}, "ENTRY");
    add_skill_gate(l, (FRect){632.0, 410.0, 78.0, 74.0}, "NO STOP");
    add_skill_gate(l, (FRect){910.0, 430.0, 76.0, 74.0}, "SLOT");
    set_star(l, 0, 335.0, 414.0, 21.0);
    set_star(l, 1, 655.0, 444.0, 21.0);
    set_star(l, 2, 922.0, 492.0, 21.0);

    l = &game->levels[4];
    l->name = "Level 5: Triple Height Gauntlet";
    l->objective = "Triple pendulum: low, lift, low, then calm.";
    l->hint = "Every cable change wakes the chain. Brake before the final bay.";
    l->timeLimit = 132.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 360.0;
    l->startTheta = 0.018;
    l->targetBase = (FRect){1025.0, 552.0, 106.0, 82.0};
    l->stableSpeedLimit = 21.0;
    l->stableAngleLimit = 0.135;
    l->stableOmegaLimit = 0.46;
    l->safeChainMotion = 60.0;
    l->requiredHoldTime = 3.8;
    l->lowTargetRatio = 0.62;
    l->pendulumMode = PENDULUM_TRIPLE;
    l->dampingScale = 0.92;
    add_obstacle(l, (FRect){235.0, 314.0, 315.0, 38.0});
    add_obstacle(l, (FRect){570.0, 508.0, 128.0, 82.0});
    add_obstacle(l, (FRect){732.0, 318.0, 260.0, 38.0});
    add_obstacle(l, (FRect){902.0, 506.0, 62.0, 82.0});
    add_bay_walls(l, 512.0, 120.0);
    add_skill_gate(l, (FRect){360.0, 390.0, 78.0, 76.0}, "LOW");
    add_skill_gate(l, (FRect){625.0, 430.0, 76.0, 70.0}, "LIFT");
    add_skill_gate(l, (FRect){850.0, 392.0, 76.0, 74.0}, "LOW");
    set_star(l, 0, 380.0, 415.0, 21.0);
    set_star(l, 1, 638.0, 454.0, 21.0);
    set_star(l, 2, 970.0, 430.0, 21.0);

    l = &game->levels[5];
    l->name = "Level 6: Triple Final Boss";
    l->objective = "Triple pendulum: boss route, no-stop zone, fragile bay.";
    l->hint = "This is intentionally tryhard: pass gates, collect risky stars, then land softly.";
    l->timeLimit = 145.0;
    l->startTrolleyX = 155.0;
    l->startCableLength = 375.0;
    l->startTheta = -0.018;
    l->targetBase = (FRect){1074.0, 552.0, 82.0, 82.0};
    l->stableSpeedLimit = 17.0;
    l->stableAngleLimit = 0.085;
    l->stableOmegaLimit = 0.28;
    l->safeChainMotion = 44.0;
    l->requiredHoldTime = 4.2;
    l->lowTargetRatio = 0.64;
    l->cargoKind = CARGO_FRAGILE;
    l->pendulumMode = PENDULUM_TRIPLE;
    l->dampingScale = 0.88;
    add_obstacle(l, (FRect){210.0, 306.0, 320.0, 38.0});
    add_obstacle(l, (FRect){548.0, 508.0, 122.0, 84.0});
    add_obstacle(l, (FRect){742.0, 296.0, 265.0, 38.0});
    add_obstacle(l, (FRect){804.0, 504.0, 84.0, 88.0});
    add_obstacle(l, (FRect){902.0, 356.0, 130.0, 36.0});
    add_obstacle(l, (FRect){960.0, 500.0, 58.0, 86.0});
    add_bay_walls(l, 512.0, 120.0);
    add_unstable_zone(l, (FRect){668.0, 155.0, 158.0, 470.0}, 3.0);
    add_skill_gate(l, (FRect){350.0, 386.0, 72.0, 72.0}, "LOW");
    add_skill_gate(l, (FRect){620.0, 432.0, 72.0, 66.0}, "LIFT");
    add_skill_gate(l, (FRect){732.0, 420.0, 72.0, 66.0}, "MOVE");
    add_skill_gate(l, (FRect){1010.0, 430.0, 64.0, 72.0}, "FINAL");
    set_star(l, 0, 356.0, 410.0, 20.0);
    set_star(l, 1, 735.0, 446.0, 20.0);
    set_star(l, 2, 1030.0, 452.0, 20.0);

    for (i = 0; i < LEVEL_COUNT; ++i) {
        validate_level_stars(&game->levels[i], i);
    }
}
'''
text = re.sub(r'static void initialize_levels\(Game \*game\) \{.*?\n\}\n\nstatic void update_cargo_position',
              new_initialize + '\n\nstatic void update_cargo_position', text, flags=re.S)

# -----------------------------------------------------------------------------
# 3. Reset gate state and new flags.
# -----------------------------------------------------------------------------
text = text.replace('    game->unstableZoneTooLong = 0;\n    game->chainMotion = 0.0;\n',
                    '    game->unstableZoneTooLong = 0;\n    game->missingSkillGate = 0;\n    game->chainMotion = 0.0;\n')
text = text.replace('    game->starMessageTimer = 0.0;\n    game->lastCompletionTime = 0.0;\n',
                    '    game->starMessageTimer = 0.0;\n    game->skillGateMessageTimer = 0.0;\n    game->lastCompletionTime = 0.0;\n')
text = text.replace('    for (i = 0; i < STAR_COUNT; ++i) {\n        level->stars[i].collected = 0;\n    }\n',
                    '    for (i = 0; i < STAR_COUNT; ++i) {\n        level->stars[i].collected = 0;\n    }\n    reset_skill_gates(level);\n')

# -----------------------------------------------------------------------------
# 4. Star pickup cannot happen while colliding.
# -----------------------------------------------------------------------------
text = re.sub(r'static void update_star_collection\(Game \*game\) \{.*?\n\}\n\nstatic void update_unstable_zones',
              r'''static void update_star_collection(Game *game) {
    Level *level = &game->levels[game->selectedLevel];
    int i;

    /* Stars reward controlled cargo placement, not clipping through obstacles. */
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

# -----------------------------------------------------------------------------
# 5. Skill gate update, inserted before stability/win check.
# -----------------------------------------------------------------------------
skill_update = r'''
static void update_skill_gates(Game *game) {
    Level *level = &game->levels[game->selectedLevel];
    int i;
    for (i = 0; i < level->skillGateCount; ++i) {
        SkillGate *gate = &level->skillGates[i];
        if (!gate->passed && point_in_frect(game->cargo.x, game->cargo.y, gate->rect)) {
            gate->passed = 1;
            game->skillGateMessageTimer = 1.4;
        }
    }
}

'''
if 'static void update_skill_gates(Game *game)' not in text:
    text = text.replace('\nstatic void update_stability_and_win(Game *game, double dt) {', '\n' + skill_update + 'static void update_stability_and_win(Game *game, double dt) {')

# -----------------------------------------------------------------------------
# 6. Delivery requires all mandatory gates.
# -----------------------------------------------------------------------------
text = text.replace(
    '    int chainOk = level->pendulumMode == PENDULUM_SINGLE\n                || level->safeChainMotion <= 0.0\n                || game->chainMotion < level->safeChainMotion;\n    int stable = speedOk && (level->pendulumMode == PENDULUM_SINGLE ? singleSwingOk : chainOk);\n',
    '    int chainOk = level->pendulumMode == PENDULUM_SINGLE\n                || level->safeChainMotion <= 0.0\n                || game->chainMotion < level->safeChainMotion;\n    int gatesOk = all_skill_gates_passed(level);\n    int stable = speedOk && (level->pendulumMode == PENDULUM_SINGLE ? singleSwingOk : chainOk);\n'
)
text = text.replace(
    '    game->cargoTooMuchSwing = insideTarget && level->pendulumMode == PENDULUM_SINGLE && !singleSwingOk;\n    game->deliveryValid = insideTarget && stable;\n',
    '    game->cargoTooMuchSwing = insideTarget && level->pendulumMode == PENDULUM_SINGLE && !singleSwingOk;\n    game->missingSkillGate = insideTarget && stable && !gatesOk;\n    game->deliveryValid = insideTarget && stable && gatesOk;\n'
)

# -----------------------------------------------------------------------------
# 7. Update physics order: collision before stars, update gates after collision.
# -----------------------------------------------------------------------------
text = re.sub(r'\n    update_star_collection\(game\);\n    update_unstable_zones\(game, dt\);',
              '\n    update_unstable_zones(game, dt);', text, count=1)
text = text.replace(
    '    if (game->starMessageTimer > 0.0) {\n        game->starMessageTimer -= dt;\n        if (game->starMessageTimer < 0.0) {\n            game->starMessageTimer = 0.0;\n        }\n    }\n',
    '    if (game->starMessageTimer > 0.0) {\n        game->starMessageTimer -= dt;\n        if (game->starMessageTimer < 0.0) {\n            game->starMessageTimer = 0.0;\n        }\n    }\n    if (game->skillGateMessageTimer > 0.0) {\n        game->skillGateMessageTimer -= dt;\n        if (game->skillGateMessageTimer < 0.0) {\n            game->skillGateMessageTimer = 0.0;\n        }\n    }\n'
)
text = text.replace(
    '    if (cargo_hits_any_obstacle(game)) {\n        game->state = STATE_LOSE;\n        game->lossReason = LOSS_COLLISION;\n        game->loseIndex = 0;\n        return;\n    }\n\n    update_stability_and_win(game, dt);\n',
    '    if (cargo_hits_any_obstacle(game)) {\n        game->state = STATE_LOSE;\n        game->lossReason = LOSS_COLLISION;\n        game->loseIndex = 0;\n        return;\n    }\n\n    update_skill_gates(game);\n    update_star_collection(game);\n    update_stability_and_win(game, dt);\n'
)

# -----------------------------------------------------------------------------
# 8. Clean unstable zone visuals robustly.
# -----------------------------------------------------------------------------
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

# -----------------------------------------------------------------------------
# 9. Draw mandatory gates.
# -----------------------------------------------------------------------------
gate_draw = r'''
static void drawSkillGates(App *app, const Level *level) {
    int i;
    for (i = 0; i < level->skillGateCount; ++i) {
        const SkillGate *gate = &level->skillGates[i];
        SDL_Color border = gate->passed ? color_rgba(96, 230, 132, 220) : color_rgba(86, 178, 255, 220);
        SDL_Color fill = gate->passed ? color_rgba(70, 210, 110, 24) : color_rgba(70, 150, 245, 24);
        SDL_Color textColor = gate->passed ? color_rgba(34, 120, 54, 255) : color_rgba(27, 82, 137, 255);
        fill_rect(app, gate->rect, fill);
        outline_rect(app, gate->rect, border);
        outline_rect(app, (FRect){gate->rect.x + 4.0, gate->rect.y + 4.0, gate->rect.w - 8.0, gate->rect.h - 8.0}, border);
        draw_text_center(app, app->fontSmall, gate->passed ? "PASSED" : gate->label,
                         (int)(gate->rect.x + gate->rect.w * 0.5),
                         (int)(gate->rect.y + gate->rect.h * 0.5 - 8.0), textColor);
    }
}

'''
if 'static void drawSkillGates(App *app' not in text:
    text = text.replace('\nstatic void drawUnstableZones(App *app, const Level *level)', '\n' + gate_draw + 'static void drawUnstableZones(App *app, const Level *level)')
text = text.replace('    drawMovingDockTrack(app, level);\n    drawUnstableZones(app, level);\n',
                    '    drawMovingDockTrack(app, level);\n    drawUnstableZones(app, level);\n    drawSkillGates(app, level);\n')

# -----------------------------------------------------------------------------
# 10. Reduce obstacle label clutter.
# -----------------------------------------------------------------------------
text = re.sub(r'static void draw_obstacles\(App \*app, const Level \*level\) \{.*?\n\}\n\nstatic void drawUnstableZones',
              r'''static void draw_obstacles(App *app, const Level *level) {
    int i;
    for (i = 0; i < level->obstacleCount; ++i) {
        FRect obstacle = level->obstacles[i];
        const char *label = "";
        if (obstacle.w >= 180.0 && obstacle.h <= 48.0) {
            label = "LOW CLEARANCE";
        } else if (obstacle.h >= 78.0 && obstacle.w >= 72.0) {
            label = "BARRIER";
        }
        drawSteelBeam(app, obstacle, label);
    }
}

static void drawUnstableZones''', text, flags=re.S)

# -----------------------------------------------------------------------------
# 11. HUD: gates, cleaner level name, warnings.
# -----------------------------------------------------------------------------
text = text.replace('    draw_text(app, app->fontSmall, level->name, 230, 15, text);\n',
                    '    draw_text(app, app->fontSmall, level->name, 230, 15, text);\n')
text = text.replace('    snprintf(buffer, sizeof(buffer), "Stars %d/%d", stars, STAR_COUNT);\n    draw_text(app, app->fontSmall, buffer, 610, 70, color_rgba(255, 220, 92, 255));\n',
                    '    snprintf(buffer, sizeof(buffer), "Stars %d/%d", stars, STAR_COUNT);\n    draw_text(app, app->fontSmall, buffer, 610, 70, color_rgba(255, 220, 92, 255));\n    if (level->skillGateCount > 0) {\n        snprintf(buffer, sizeof(buffer), "Gates %d/%d", count_passed_skill_gates(level), level->skillGateCount);\n        draw_text(app, app->fontSmall, buffer, 710, 70, all_skill_gates_passed(level) ? color_rgba(130, 240, 150, 255) : color_rgba(132, 218, 255, 255));\n    }\n')
text = text.replace('        draw_text(app, app->fontSmall, buffer, 710, 70,\n',
                    '        draw_text(app, app->fontSmall, buffer, level->skillGateCount > 0 ? 820 : 710,\n')
text = text.replace('    } else if (game->starMessageTimer > 0.0) {\n        draw_text(app, app->fontSmall, "Star collected!", 1010, 70, color_rgba(255, 220, 92, 255));\n',
                    '    } else if (game->skillGateMessageTimer > 0.0) {\n        draw_text(app, app->fontSmall, "Gate passed!", 1010, 70, color_rgba(132, 218, 255, 255));\n    } else if (game->starMessageTimer > 0.0) {\n        draw_text(app, app->fontSmall, "Star collected!", 1010, 70, color_rgba(255, 220, 92, 255));\n    } else if (game->missingSkillGate) {\n        draw_text(app, app->fontSmall, "Pass gates first", 982, 70, color_rgba(132, 218, 255, 255));\n')

# -----------------------------------------------------------------------------
# 12. README note.
# -----------------------------------------------------------------------------
if README.exists():
    readme = README.read_text(encoding='utf-8')
    note = '''

## Boss difficulty pass

This build adds mandatory cargo checkpoint gates to prevent the old slow-drive cheese strategy. Delivery only counts after the cargo passes the required blue/green gate frames. The 6-level campaign is retuned with stricter single/double/triple pendulum stability thresholds, cleaner unstable-zone visuals, and harder optional 3-star routes. Heavy animated hazards remain disabled for performance.
'''
    if 'Boss difficulty pass' not in readme:
        README.write_text(readme.rstrip() + note, encoding='utf-8')

if text == original:
    print('Warning: no crane_game.c changes detected; file may already be patched or pattern changed.')
else:
    GAME.write_text(text, encoding='utf-8')
    print('Applied boss difficulty patch to crane_game.c')

print('Next: build_crane_game.bat')
