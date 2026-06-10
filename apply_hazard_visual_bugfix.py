from pathlib import Path
import re

GAME = Path('crane_game.c')
README = Path('README.txt')

text = GAME.read_text(encoding='utf-8')
original = text

# -----------------------------------------------------------------------------
# Hazard visual bugfix pass
# Fixes the current buggy-looking hazard visuals:
# - magnet field / radiating effect looked like a random rectangle or failed asset
# - rotating sweeper sprite looked like a broken industrial beam
# - moving gate looked like a solid wall instead of a timing hazard
# - unstable zone was still too visually large/noisy
# This patch keeps the mechanics, but draws hazards with clean SDL primitives.
# -----------------------------------------------------------------------------

# 1) Force default graphics to simple-fast. This prevents old animated sheets from
# accidentally appearing when F3 is toggled or after older patches.
text = text.replace('    app->graphicsQuality = GRAPHICS_POLISHED;\n', '    app->graphicsQuality = GRAPHICS_SIMPLE_FAST;\n')
text = text.replace('    app->graphicsQuality = GRAPHICS_FULL_ANIMATED;\n', '    app->graphicsQuality = GRAPHICS_SIMPLE_FAST;\n')

# 2) Replace magnet draw function with clean primitive visual.
new_magnet = r'''static void drawMagnetHazard(App *app, const MagnetHazard *magnet, double time, double cargoX, double cargoY) {
    double pulse = magnet_pulse(magnet, time);
    double dx = cargoX - magnet->x;
    double dy = cargoY - magnet->y;
    double cargoDist = sqrt(dx * dx + dy * dy);
    int activeNearCargo = cargoDist <= magnet->radius * 1.10;
    int ringA = activeNearCargo ? 42 + (int)(pulse * 16.0) : 38;
    int ringB = activeNearCargo ? 76 + (int)(pulse * 20.0) : 62;
    FRect base = magnet->body;
    FRect tower = {base.x + base.w * 0.5 - 20.0, base.y - 62.0, 40.0, base.h + 62.0};

    /* Draw only clean rings around the physical magnet. No large PNG field sheet. */
    draw_circle_outline(app, (int)magnet->x, (int)magnet->y, ringA,
                        color_rgba(84, 190, 255, activeNearCargo ? 145 : 72));
    draw_circle_outline(app, (int)magnet->x, (int)magnet->y, ringB,
                        color_rgba(129, 86, 230, activeNearCargo ? 105 : 45));
    if (activeNearCargo) {
        draw_circle_outline(app, (int)magnet->x, (int)magnet->y, (int)(magnet->radius * 0.56),
                            color_rgba(84, 190, 255, 46));
        draw_line(app, cargoX, cargoY, magnet->x, magnet->y, color_rgba(84, 190, 255, 85));
    }

    fill_rect(app, (FRect){tower.x + 5.0, tower.y + 6.0, tower.w, tower.h}, color_rgba(0, 0, 0, 70));
    fill_rect(app, tower, color_rgba(64, 73, 83, 245));
    fill_rect(app, (FRect){tower.x + 7.0, tower.y + 8.0, tower.w - 14.0, tower.h - 16.0}, color_rgba(88, 100, 114, 245));
    outline_rect(app, tower, color_rgba(31, 38, 45, 255));

    /* U-shaped magnet head. */
    fill_rect(app, (FRect){base.x + 10.0, base.y + 6.0, base.w - 20.0, 14.0}, color_rgba(55, 65, 74, 255));
    fill_rect(app, (FRect){base.x + 10.0, base.y + 6.0, 13.0, 44.0}, color_rgba(85, 189, 238, 255));
    fill_rect(app, (FRect){base.x + base.w - 23.0, base.y + 6.0, 13.0, 44.0}, color_rgba(85, 189, 238, 255));
    fill_rect(app, (FRect){base.x + 23.0, base.y + 36.0, base.w - 46.0, 10.0}, color_rgba(47, 55, 64, 255));
    outline_rect(app, base, color_rgba(43, 50, 57, 230));

    fill_circle(app, (int)magnet->x, (int)magnet->y, 8, color_rgba(98, 208, 255, activeNearCargo ? 230 : 145));
    draw_text_center(app, app->fontSmall, activeNearCargo ? "MAGNET PULL" : "MAGNET",
                     (int)(base.x + base.w * 0.5), (int)(base.y + base.h + 8.0),
                     color_rgba(50, 80, 130, 255));

#if DEBUG_COLLIDERS
    draw_circle_outline(app, (int)magnet->x, (int)magnet->y, (int)magnet->radius, color_rgba(170, 95, 255, 110));
    outline_rect(app, magnet->body, color_rgba(170, 95, 255, 160));
#endif
}
'''
text = re.sub(r'static void drawMagnetHazard\(App \*app, const MagnetHazard \*magnet, double time, double cargoX, double cargoY\) \{.*?\n\}\n\nstatic void drawLaserHazard',
              new_magnet + '\n\nstatic void drawLaserHazard', text, flags=re.S)

# 3) Replace sweeper draw function with clear rotating arm, no sprite/animation.
new_sweeper = r'''static void drawSweeperHazard(App *app, const SweeperHazard *sweeper, double time) {
    double a = current_sweeper_angle(sweeper, time);
    double half = sweeper->length * 0.5;
    double ax = sweeper->pivotX - cos(a) * half;
    double ay = sweeper->pivotY - sin(a) * half;
    double bx = sweeper->pivotX + cos(a) * half;
    double by = sweeper->pivotY + sin(a) * half;

    /* Movement guide: readable, not a giant sprite. */
    draw_circle_outline(app, (int)sweeper->pivotX, (int)sweeper->pivotY, (int)half,
                        color_rgba(214, 82, 58, 70));
    draw_circle_outline(app, (int)sweeper->pivotX, (int)sweeper->pivotY, (int)(half + sweeper->width * 0.5),
                        color_rgba(214, 82, 58, 38));

    draw_thick_line(app, ax, ay, bx, by, (int)(sweeper->width + 8.0), color_rgba(58, 36, 32, 245));
    draw_thick_line(app, ax, ay, bx, by, (int)sweeper->width, color_rgba(211, 69, 50, 245));
    draw_thick_line(app, ax, ay, bx, by, 4, color_rgba(246, 190, 59, 245));

    fill_circle(app, (int)sweeper->pivotX, (int)sweeper->pivotY, 15, color_rgba(34, 39, 43, 255));
    fill_circle(app, (int)sweeper->pivotX, (int)sweeper->pivotY, 8, color_rgba(238, 176, 54, 255));
    fill_circle(app, (int)bx, (int)by, 8, color_rgba(232, 78, 58, 245));
    fill_circle(app, (int)ax, (int)ay, 8, color_rgba(232, 78, 58, 245));
    draw_text_center(app, app->fontSmall, "ROTATING ARM",
                     (int)sweeper->pivotX, (int)(sweeper->pivotY + half + 15.0),
                     color_rgba(135, 40, 35, 255));

#if DEBUG_COLLIDERS
    draw_thick_line(app, ax, ay, bx, by, (int)sweeper->width, color_rgba(255, 120, 120, 120));
#endif
}
'''
text = re.sub(r'static void drawSweeperHazard\(App \*app, const SweeperHazard \*sweeper, double time\) \{.*?\n\}\n\nstatic void drawMovingDockTrack',
              new_sweeper + '\n\nstatic void drawMovingDockTrack', text, flags=re.S)

# 4) Replace moving gate draw with clearer timing gate and visible movement path.
new_gate = r'''static void drawMovingGate(App *app, const MovingGate *gate, FRect gateRect, double time) {
    FRect path = gate->baseRect;
    double marker = sin(gate->omega * time + gate->phase);
    int stripeY;
    if (gate->axis == AXIS_X) {
        path.x -= fabs(gate->amplitude);
        path.w += fabs(gate->amplitude) * 2.0;
    } else if (gate->axis == AXIS_Y) {
        path.y -= fabs(gate->amplitude);
        path.h += fabs(gate->amplitude) * 2.0;
    }

    /* Movement path only: faint frame, not a collidable-looking wall. */
    fill_rect(app, path, color_rgba(224, 72, 62, 12));
    outline_rect(app, path, color_rgba(224, 72, 62, 85));
    draw_text_center(app, app->fontSmall, "WAIT FOR OPENING",
                     (int)(path.x + path.w * 0.5), (int)(path.y + 10.0), color_rgba(134, 45, 38, 180));

    fill_rect(app, (FRect){gateRect.x + 5.0, gateRect.y + 6.0, gateRect.w, gateRect.h}, color_rgba(0, 0, 0, 82));
    fill_rect(app, gateRect, color_rgba(151, 44, 42, 245));
    fill_rect(app, (FRect){gateRect.x + 6.0, gateRect.y + 6.0, gateRect.w - 12.0, gateRect.h - 12.0}, color_rgba(215, 70, 52, 245));
    fill_rect(app, (FRect){gateRect.x, gateRect.y, gateRect.w, 10.0}, color_rgba(82, 31, 34, 255));
    fill_rect(app, (FRect){gateRect.x, gateRect.y + gateRect.h - 10.0, gateRect.w, 10.0}, color_rgba(82, 31, 34, 255));
    outline_rect(app, gateRect, color_rgba(42, 27, 29, 255));

    for (stripeY = (int)gateRect.y + 20; stripeY < (int)(gateRect.y + gateRect.h - 18.0); stripeY += 42) {
        draw_thick_line(app, gateRect.x + 5.0, stripeY + 18.0, gateRect.x + gateRect.w - 5.0, stripeY, 3, color_rgba(247, 194, 65, 225));
    }

    draw_text_center(app, app->fontSmall, gate->label != NULL ? gate->label : "TIMED",
                     (int)(gateRect.x + gateRect.w * 0.5), (int)(gateRect.y + gateRect.h * 0.5 - 9.0),
                     color_rgba(255, 226, 181, 255));
    fill_circle(app, (int)(gateRect.x + gateRect.w * 0.5), (int)(gateRect.y - 14.0), 8,
                marker > 0.55 ? color_rgba(83, 230, 116, 255) : color_rgba(225, 61, 54, 255));
    fill_circle(app, (int)(gateRect.x + gateRect.w * 0.5), (int)(gateRect.y + gateRect.h + 14.0), 8,
                marker < -0.55 ? color_rgba(83, 230, 116, 255) : color_rgba(225, 61, 54, 255));
}
'''
text = re.sub(r'static void drawMovingGate\(App \*app, const MovingGate \*gate, FRect gateRect, double time\) \{.*?\n\}\n\nstatic void drawMagnetHazard',
              new_gate + '\n\nstatic void drawMagnetHazard', text, flags=re.S)

# 5) Replace unstable-zone visuals with border strips only. No full-height diagonal scratches.
new_unstable = r'''static void drawUnstableZones(App *app, const Level *level) {
    int i;
    for (i = 0; i < level->unstableZoneCount; ++i) {
        const UnstableZone *zone = &level->unstableZones[i];
        int stripe;
        FRect topStrip = {zone->rect.x, zone->rect.y, zone->rect.w, 16.0};
        FRect bottomStrip = {zone->rect.x, zone->rect.y + zone->rect.h - 16.0, zone->rect.w, 16.0};
        FRect labelPanel = {zone->rect.x + 8.0, zone->rect.y + zone->rect.h * 0.5 - 22.0,
                            zone->rect.w - 16.0, 44.0};

        outline_rect(app, zone->rect, color_rgba(245, 202, 56, 110));
        fill_rect(app, topStrip, color_rgba(245, 202, 56, 54));
        fill_rect(app, bottomStrip, color_rgba(245, 202, 56, 54));
        for (stripe = (int)zone->rect.x - 24; stripe < (int)(zone->rect.x + zone->rect.w + 24); stripe += 34) {
            draw_thick_line(app, (double)stripe, topStrip.y + topStrip.h,
                            (double)stripe + 20.0, topStrip.y, 2, color_rgba(48, 43, 31, 130));
            draw_thick_line(app, (double)stripe, bottomStrip.y + bottomStrip.h,
                            (double)stripe + 20.0, bottomStrip.y, 2, color_rgba(48, 43, 31, 130));
        }
        fill_rect(app, labelPanel, color_rgba(255, 225, 82, 34));
        outline_rect(app, labelPanel, color_rgba(92, 69, 25, 95));
        draw_text_center(app, app->fontSmall, "NO-STOP ZONE",
                         (int)(labelPanel.x + labelPanel.w * 0.5),
                         (int)(labelPanel.y + 5.0), color_rgba(64, 49, 14, 225));
        draw_text_center(app, app->fontSmall, "KEEP MOVING",
                         (int)(labelPanel.x + labelPanel.w * 0.5),
                         (int)(labelPanel.y + 25.0), color_rgba(64, 49, 14, 225));
    }
}
'''
text = re.sub(r'static void drawUnstableZones\(App \*app, const Level \*level\) \{.*?\n\}\n\nstatic void drawStarCollectible',
              new_unstable + '\n\nstatic void drawStarCollectible', text, flags=re.S)

# 6) Tune current campaign hazard placements to avoid weird overlaps in screenshots.
# These replacements are intentionally narrow and match the timing-hazard patch values.
repls = {
    '(MagnetHazard){610.0, 445.0, 190.0, 520.0, 2.4, 0.0, (FRect){570.0, 402.0, 80.0, 126.0}}':
    '(MagnetHazard){610.0, 450.0, 170.0, 420.0, 2.4, 0.0, (FRect){580.0, 438.0, 60.0, 92.0}}',
    '(MagnetHazard){610.0, 435.0, 185.0, 460.0, 2.2, 0.3, (FRect){572.0, 392.0, 78.0, 126.0}}':
    '(MagnetHazard){610.0, 448.0, 165.0, 390.0, 2.2, 0.3, (FRect){582.0, 438.0, 56.0, 90.0}}',
    '(MagnetHazard){590.0, 430.0, 180.0, 450.0, 2.4, 0.0, (FRect){552.0, 388.0, 78.0, 126.0}}':
    '(MagnetHazard){590.0, 448.0, 165.0, 390.0, 2.4, 0.0, (FRect){562.0, 438.0, 56.0, 90.0}}',
    '(SweeperHazard){760.0, 455.0, 165.0, 18.0, 1.45, 0.0}':
    '(SweeperHazard){760.0, 455.0, 145.0, 15.0, 1.35, 0.0}',
    '(SweeperHazard){650.0, 455.0, 165.0, 18.0, 1.38, 0.6}':
    '(SweeperHazard){650.0, 455.0, 145.0, 15.0, 1.28, 0.6}',
    '(SweeperHazard){790.0, 455.0, 170.0, 18.0, 1.55, 0.2}':
    '(SweeperHazard){790.0, 455.0, 150.0, 15.0, 1.42, 0.2}',
}
for old, new in repls.items():
    text = text.replace(old, new)

# 7) README note.
if README.exists():
    readme = README.read_text(encoding='utf-8')
    note = '''

## Hazard visual bugfix pass

This build replaces the buggy magnet field, sweeper, moving-gate, and unstable-zone visuals with clean SDL primitive drawings. The mechanics are preserved, but old large animated/texture effects are no longer used for these hazards in normal rendering. Magnets now appear as small machines with subtle field rings, sweepers appear as clear rotating red arms, moving gates show a faint movement path, and no-stop zones use only border strips and a compact label.
'''
    if 'Hazard visual bugfix pass' not in readme:
        README.write_text(readme.rstrip() + note, encoding='utf-8')

if text == original:
    print('Warning: no crane_game.c changes detected. The file may already be patched or has unexpected structure.')
else:
    GAME.write_text(text, encoding='utf-8')
    print('Applied hazard visual bugfix patch to crane_game.c')

print('Next: build_crane_game.bat')
