#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#ifdef USE_SDL_IMAGE
#include <SDL2/SDL_image.h>
#endif
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define LOGICAL_W 1280
#define LOGICAL_H 720
#define HUD_H 92
#define BOTTOM_BAR_Y 660
#define RAIL_Y 124
#define PIVOT_Y 164
#define RAIL_MIN_X 88.0
#define RAIL_MAX_X 1192.0
#define LEVEL_COUNT 6
#define STAR_COUNT 3
#define CAMPAIGN_STAR_TOTAL (LEVEL_COUNT * STAR_COUNT)
#define MAX_STATIC_OBSTACLES 12
#define MAX_UNSTABLE_ZONES 2
#define MAX_WIND_ZONES 2
#define MAX_MOVING_GATES 2
#define MAX_MAGNETS 2
#define MAX_LASERS 2
#define MAX_BRIDGES 2
#define MAX_CRUSHERS 2
#define MAX_HAMMERS 2
#define MAX_SWEEPERS 2
#define ANIM_MAX_FRAMES 8
#ifndef DEBUG_COLLIDERS
#define DEBUG_COLLIDERS 0
#endif
#ifndef DEBUG_PERFORMANCE
#define DEBUG_PERFORMANCE 0
#endif

#define FIXED_DT (1.0 / 120.0)
#define GRAVITY 900.0
#define TROLLEY_MASS 130.0
#define MOTOR_FORCE 90000.0
#define TROLLEY_DAMPING 175.0
#define STABILIZER_TROLLEY_DAMPING 620.0
#define BASE_SWING_DAMPING 0.23
#define STABILIZER_SWING_DAMPING 2.55
#define CABLE_RATE 145.0
#define MIN_CABLE_LENGTH 145.0
#define MAX_CABLE_LENGTH 470.0
#define WIN_STABLE_SECONDS 3.0
#define CARGO_W 54.0
#define CARGO_H 44.0
#define MAX_SWING_RADIANS 1.35

typedef enum {
    AXIS_NONE,
    AXIS_X,
    AXIS_Y
} MotionAxis;

typedef enum {
    STATE_MAIN_MENU,
    STATE_HOW_TO_PLAY,
    STATE_LEVEL_SELECT,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_WIN,
    STATE_LOSE
} GameState;

typedef enum {
    GRAPHICS_SIMPLE_FAST,
    GRAPHICS_POLISHED,
    GRAPHICS_FULL_ANIMATED
} GraphicsQuality;

typedef enum {
    LOSS_NONE,
    LOSS_TIMEOUT,
    LOSS_COLLISION,
    LOSS_SWING,
    LOSS_UNSTABLE
} LossReason;

typedef struct {
    double x;
    double y;
    double w;
    double h;
} FRect;

typedef struct {
    SDL_Texture *frames[ANIM_MAX_FRAMES];
    int frameCount;
    double fps;
    double time;
    int currentFrame;
    int loop;
} AnimatedSprite;

typedef struct {
    AnimatedSprite magnetTower;
    AnimatedSprite magneticField;
    AnimatedSprite turbineFan;
    AnimatedSprite windGust;
    AnimatedSprite crusher;
    AnimatedSprite extendableBridge;
    AnimatedSprite laserGate;
    AnimatedSprite sweeperArm;
    AnimatedSprite swingHammer;
    AnimatedSprite movingDock;
} AnimationAssets;

typedef struct {
    double x;
    double velocity;
    double acceleration;
} Trolley;

typedef struct {
    double theta;
    double omega;
    double length;
    double x;
    double y;
    double prevX;
    double prevY;
    double speed;
} Cargo;

typedef struct {
    FRect rect;
    double strength;
    int direction;
} WindZone;

typedef struct {
    FRect baseRect;
    MotionAxis axis;
    double amplitude;
    double omega;
    double phase;
    const char *label;
} MovingGate;

typedef enum {
    CARGO_NORMAL,
    CARGO_HEAVY,
    CARGO_FRAGILE
} CargoKind;

typedef enum {
    PENDULUM_SINGLE,
    PENDULUM_DOUBLE,
    PENDULUM_TRIPLE
} PendulumMode;

typedef struct {
    double x;
    double y;
    double prevX;
    double prevY;
} ChainMass;

typedef struct {
    PendulumMode mode;
    int massCount;
    ChainMass masses[3];
    double totalLength;
    double segmentLength[3];
} MultiPendulum;

typedef struct {
    double x;
    double y;
    double radius;
    int collected;
} StarCollectible;

typedef struct {
    FRect rect;
    double timeLimit;
} UnstableZone;

typedef struct {
    double x;
    double y;
    double radius;
    double strength;
    double omega;
    double phase;
    FRect body;
} MagnetHazard;

typedef struct {
    FRect frame;
    FRect beam;
    double period;
    double activeFraction;
    double phase;
} LaserHazard;

typedef struct {
    FRect baseRect;
    double minWidth;
    double maxWidth;
    double period;
    double phase;
    int extendRight;
} BridgeHazard;

typedef struct {
    FRect guideRect;
    FRect headRect;
    double travel;
    double period;
    double phase;
} CrusherHazard;

typedef struct {
    double pivotX;
    double pivotY;
    double length;
    double amplitude;
    double omega;
    double phase;
    double bobRadius;
} HammerHazard;

typedef struct {
    double pivotX;
    double pivotY;
    double length;
    double width;
    double omega;
    double phase;
} SweeperHazard;

typedef struct {
    const char *name;
    const char *objective;
    const char *hint;
    double timeLimit;
    double startTrolleyX;
    double startCableLength;
    double startTheta;
    FRect targetBase;
    MotionAxis targetAxis;
    double targetAmplitude;
    double targetOmega;
    double targetPhase;
    double stableSpeedLimit;
    double stableAngleLimit;
    double stableOmegaLimit;
    double safeChainMotion;
    double requiredHoldTime;
    double lowTargetRatio;
    CargoKind cargoKind;
    PendulumMode pendulumMode;
    double windScale;
    double magnetScale;
    double dampingScale;
    StarCollectible stars[STAR_COUNT];
    UnstableZone unstableZones[MAX_UNSTABLE_ZONES];
    int unstableZoneCount;
    FRect obstacles[MAX_STATIC_OBSTACLES];
    int obstacleCount;
    WindZone windZones[MAX_WIND_ZONES];
    int windZoneCount;
    MovingGate gates[MAX_MOVING_GATES];
    int gateCount;
    MagnetHazard magnets[MAX_MAGNETS];
    int magnetCount;
    LaserHazard lasers[MAX_LASERS];
    int laserCount;
    BridgeHazard bridges[MAX_BRIDGES];
    int bridgeCount;
    CrusherHazard crushers[MAX_CRUSHERS];
    int crusherCount;
    HammerHazard hammers[MAX_HAMMERS];
    int hammerCount;
    SweeperHazard sweepers[MAX_SWEEPERS];
    int sweeperCount;
} Level;

typedef struct {
    SDL_Texture *craneTrolley;
    SDL_Texture *hookAssembly;
    SDL_Texture *cargoCrate;
    SDL_Texture *deliveryDock;
    SDL_Texture *hazardBarrier;
    SDL_Texture *steelBeam;
    SDL_Texture *industrialFan;
    SDL_Texture *movingGate;
    SDL_Texture *movingDock;
    SDL_Texture *yardBackground;
    SDL_Texture *windArrow;
    SDL_Texture *windZoneOverlay;
    SDL_Texture *menuButtonNormal;
    SDL_Texture *menuButtonHover;
    SDL_Texture *menuButtonSelected;
    SDL_Texture *menuPanel;
    SDL_Texture *progressBarFrame;
    SDL_Texture *magnetTower;
    SDL_Texture *swingHammerRig;
    SDL_Texture *twinTurbineFan;
    SDL_Texture *movingDockHeavy;
    SDL_Texture *heavyHazardContainer;
    SDL_Texture *precisionFragileCargo;
    SDL_Texture *hydraulicCrusher;
    SDL_Texture *extendableBridge;
    SDL_Texture *laserSecurityGate;
    SDL_Texture *rotatingSweeperArm;
    AnimationAssets anim;
    int imageEnabled;
    int loadedCount;
} Assets;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *fontSmall;
    TTF_Font *font;
    TTF_Font *fontLarge;
    Assets assets;
    int windowedX;
    int windowedY;
    int windowedW;
    int windowedH;
    GraphicsQuality graphicsQuality;
    double smoothedFps;
} App;

typedef struct {
    GameState state;
    int running;
    int fullscreen;
    int selectedLevel;
    int menuIndex;
    int levelIndex;
    int pauseIndex;
    int winIndex;
    int loseIndex;
    int motorInput;
    int cableInput;
    int stabilizerHeld;
    int cargoTooFast;
    int cargoTooHigh;
    int cargoTooMuchSwing;
    int cargoInDeliveryZone;
    int deliveryValid;
    int cargoInWind;
    int cargoInMagnet;
    int chainTooMuchMotion;
    int unstableZoneActive;
    int unstableZoneTooLong;
    int lastStarsCollected;
    int bestStars[LEVEL_COUNT];
    double mouseX;
    double mouseY;
    int mouseValid;
    double timeLeft;
    double levelElapsed;
    double stableTimer;
    double chainMotion;
    double unstableZoneTimer;
    double starMessageTimer;
    double lastCompletionTime;
    LossReason lossReason;
    Trolley trolley;
    Cargo cargo;
    MultiPendulum chain;
    Level levels[LEVEL_COUNT];
} Game;

static SDL_Texture *load_asset_texture(App *app, const char *relativePath) {
#ifdef USE_SDL_IMAGE
    char fullPath[512];
    SDL_Texture *texture = IMG_LoadTexture(app->renderer, relativePath);
    if (texture != NULL) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        app->assets.loadedCount++;
        return texture;
    }
    snprintf(fullPath, sizeof(fullPath), "crane_operator_asset_pack_v11_polished/%s", relativePath);
    texture = IMG_LoadTexture(app->renderer, fullPath);
    if (texture != NULL) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        app->assets.loadedCount++;
        return texture;
    }
    snprintf(fullPath, sizeof(fullPath), "crane_operator_monster_asset_pack_v12/%s", relativePath);
    texture = IMG_LoadTexture(app->renderer, fullPath);
    if (texture != NULL) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        app->assets.loadedCount++;
        return texture;
    }
    printf("Asset missing or failed to load: %s\n", relativePath);
#else
    (void)app;
    (void)relativePath;
#endif
    return NULL;
}

static void clear_animation(AnimatedSprite *sprite) {
    int i;
    for (i = 0; i < ANIM_MAX_FRAMES; ++i) {
        if (sprite->frames[i] != NULL) {
            SDL_DestroyTexture(sprite->frames[i]);
            sprite->frames[i] = NULL;
        }
    }
    sprite->frameCount = 0;
    sprite->fps = 0.0;
    sprite->time = 0.0;
    sprite->currentFrame = 0;
    sprite->loop = 0;
}

static void load_animation(App *app, AnimatedSprite *sprite, const char *folder, const char *basename,
                           int frameCount, double fps, int loop) {
#ifdef USE_SDL_IMAGE
    char path[512];
    char fallbackPath[512];
    int i;
    int loadedHere = 0;

    memset(sprite, 0, sizeof(*sprite));
    if (!app->assets.imageEnabled) {
        return;
    }
    if (frameCount > ANIM_MAX_FRAMES) {
        frameCount = ANIM_MAX_FRAMES;
    }
    for (i = 0; i < frameCount; ++i) {
        snprintf(path, sizeof(path), "crane_operator_ready_import_pack_v15_fixed_alpha/assets/frames_clean/%s/%s_%02d.png",
                 folder, basename, i);
        sprite->frames[i] = IMG_LoadTexture(app->renderer, path);
        if (sprite->frames[i] == NULL) {
            snprintf(fallbackPath, sizeof(fallbackPath),
                     "crane_operator_ready_import_pack_v15_fixed_alpha/crane_operator_ready_import_pack_v15_fixed_alpha/assets/frames_clean/%s/%s_%02d.png",
                     folder, basename, i);
            sprite->frames[i] = IMG_LoadTexture(app->renderer, fallbackPath);
            if (sprite->frames[i] == NULL) {
                printf("Animation frame missing or failed to load: %s or %s (%s)\n",
                       path, fallbackPath, IMG_GetError());
                clear_animation(sprite);
                app->assets.loadedCount -= loadedHere;
                if (app->assets.loadedCount < 0) {
                    app->assets.loadedCount = 0;
                }
                return;
            }
        }
        SDL_SetTextureBlendMode(sprite->frames[i], SDL_BLENDMODE_BLEND);
        app->assets.loadedCount++;
        loadedHere++;
    }
    sprite->frameCount = frameCount;
    sprite->fps = fps;
    sprite->time = 0.0;
    sprite->currentFrame = 0;
    sprite->loop = loop;
    printf("Animation loaded: %s (%d frames @ %.0f fps)\n", basename, sprite->frameCount, sprite->fps);
#else
    (void)app;
    (void)sprite;
    (void)folder;
    (void)basename;
    (void)frameCount;
    (void)fps;
    (void)loop;
#endif
}

static void load_assets(App *app) {
    memset(&app->assets, 0, sizeof(app->assets));
#ifdef USE_SDL_IMAGE
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        printf("SDL2_image PNG init failed: %s\n", IMG_GetError());
        return;
    }
    app->assets.imageEnabled = 1;
    app->assets.craneTrolley = load_asset_texture(app, "assets/world/crane_trolley.png");
    app->assets.hookAssembly = load_asset_texture(app, "assets/world/hook_assembly.png");
    app->assets.cargoCrate = load_asset_texture(app, "assets/world/cargo_crate.png");
    app->assets.deliveryDock = load_asset_texture(app, "assets/world/delivery_dock.png");
    app->assets.hazardBarrier = load_asset_texture(app, "assets/world/hazard_barrier.png");
    app->assets.steelBeam = load_asset_texture(app, "assets/world/steel_beam.png");
    app->assets.industrialFan = load_asset_texture(app, "assets/world/industrial_fan.png");
    app->assets.movingGate = load_asset_texture(app, "assets/world/moving_gate.png");
    app->assets.movingDock = load_asset_texture(app, "assets/world/moving_dock.png");
    app->assets.yardBackground = load_asset_texture(app, "assets/backgrounds/yard_background_1280x720.png");
    app->assets.windArrow = load_asset_texture(app, "assets/effects/wind_arrow.png");
    app->assets.windZoneOverlay = load_asset_texture(app, "assets/effects/wind_zone_overlay.png");
    app->assets.menuButtonNormal = load_asset_texture(app, "assets/ui/menu_button_normal.png");
    app->assets.menuButtonHover = load_asset_texture(app, "assets/ui/menu_button_hover.png");
    app->assets.menuButtonSelected = load_asset_texture(app, "assets/ui/menu_button_selected.png");
    app->assets.menuPanel = load_asset_texture(app, "assets/ui/menu_panel.png");
    app->assets.progressBarFrame = load_asset_texture(app, "assets/ui/progress_bar_frame.png");
    app->assets.magnetTower = load_asset_texture(app, "assets/01_magnet_tower.png");
    app->assets.swingHammerRig = load_asset_texture(app, "assets/02_swing_hammer_rig.png");
    app->assets.twinTurbineFan = load_asset_texture(app, "assets/03_twin_turbine_fan.png");
    app->assets.movingDockHeavy = load_asset_texture(app, "assets/04_moving_dock_heavy.png");
    app->assets.heavyHazardContainer = load_asset_texture(app, "assets/05_heavy_hazard_container.png");
    app->assets.precisionFragileCargo = load_asset_texture(app, "assets/06_precision_fragile_cargo.png");
    app->assets.hydraulicCrusher = load_asset_texture(app, "assets/07_hydraulic_crusher.png");
    app->assets.extendableBridge = load_asset_texture(app, "assets/08_extendable_bridge_platform.png");
    app->assets.laserSecurityGate = load_asset_texture(app, "assets/09_laser_security_gate.png");
    app->assets.rotatingSweeperArm = load_asset_texture(app, "assets/10_rotating_sweeper_arm.png");
    load_animation(app, &app->assets.anim.magnetTower, "magnet_tower", "magnet_tower", 8, 12.0, 1);
    load_animation(app, &app->assets.anim.magneticField, "magnetic_field_effect", "magnetic_field_effect", 8, 18.0, 1);
    load_animation(app, &app->assets.anim.turbineFan, "twin_turbine_fan", "twin_turbine_fan", 8, 18.0, 1);
    load_animation(app, &app->assets.anim.windGust, "wind_gust_effect", "wind_gust_effect", 8, 18.0, 1);
    load_animation(app, &app->assets.anim.crusher, "hydraulic_crusher", "hydraulic_crusher", 8, 12.0, 1);
    load_animation(app, &app->assets.anim.extendableBridge, "extendable_bridge", "extendable_bridge", 8, 12.0, 1);
    load_animation(app, &app->assets.anim.laserGate, "laser_gate", "laser_gate", 8, 16.0, 1);
    load_animation(app, &app->assets.anim.sweeperArm, "rotating_sweeper_arm", "rotating_sweeper_arm", 8, 10.0, 1);
    load_animation(app, &app->assets.anim.swingHammer, "swing_hammer_rig", "swing_hammer_rig", 8, 10.0, 1);
    load_animation(app, &app->assets.anim.movingDock, "moving_dock", "moving_dock", 8, 10.0, 1);
    printf("SDL2_image enabled; loaded %d asset textures.\n", app->assets.loadedCount);
#endif
}

static void destroy_texture(SDL_Texture **texture) {
    if (*texture != NULL) {
        SDL_DestroyTexture(*texture);
        *texture = NULL;
    }
}

static void destroy_assets(App *app) {
    destroy_texture(&app->assets.craneTrolley);
    destroy_texture(&app->assets.hookAssembly);
    destroy_texture(&app->assets.cargoCrate);
    destroy_texture(&app->assets.deliveryDock);
    destroy_texture(&app->assets.hazardBarrier);
    destroy_texture(&app->assets.steelBeam);
    destroy_texture(&app->assets.industrialFan);
    destroy_texture(&app->assets.movingGate);
    destroy_texture(&app->assets.movingDock);
    destroy_texture(&app->assets.yardBackground);
    destroy_texture(&app->assets.windArrow);
    destroy_texture(&app->assets.windZoneOverlay);
    destroy_texture(&app->assets.menuButtonNormal);
    destroy_texture(&app->assets.menuButtonHover);
    destroy_texture(&app->assets.menuButtonSelected);
    destroy_texture(&app->assets.menuPanel);
    destroy_texture(&app->assets.progressBarFrame);
    destroy_texture(&app->assets.magnetTower);
    destroy_texture(&app->assets.swingHammerRig);
    destroy_texture(&app->assets.twinTurbineFan);
    destroy_texture(&app->assets.movingDockHeavy);
    destroy_texture(&app->assets.heavyHazardContainer);
    destroy_texture(&app->assets.precisionFragileCargo);
    destroy_texture(&app->assets.hydraulicCrusher);
    destroy_texture(&app->assets.extendableBridge);
    destroy_texture(&app->assets.laserSecurityGate);
    destroy_texture(&app->assets.rotatingSweeperArm);
    clear_animation(&app->assets.anim.magnetTower);
    clear_animation(&app->assets.anim.magneticField);
    clear_animation(&app->assets.anim.turbineFan);
    clear_animation(&app->assets.anim.windGust);
    clear_animation(&app->assets.anim.crusher);
    clear_animation(&app->assets.anim.extendableBridge);
    clear_animation(&app->assets.anim.laserGate);
    clear_animation(&app->assets.anim.sweeperArm);
    clear_animation(&app->assets.anim.swingHammer);
    clear_animation(&app->assets.anim.movingDock);
#ifdef USE_SDL_IMAGE
    if (app->assets.imageEnabled) {
        IMG_Quit();
    }
#endif
    memset(&app->assets, 0, sizeof(app->assets));
}

static SDL_Color color_rgba(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_Color c = {r, g, b, a};
    return c;
}

static SDL_Rect to_sdl_rect(FRect r) {
    SDL_Rect out;
    out.x = (int)lrint(r.x);
    out.y = (int)lrint(r.y);
    out.w = (int)lrint(r.w);
    out.h = (int)lrint(r.h);
    return out;
}

static double clamp_double(double value, double minValue, double maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

static void set_color(SDL_Renderer *renderer, SDL_Color c) {
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}

static void fill_rect(App *app, FRect rect, SDL_Color color) {
    SDL_Rect r = to_sdl_rect(rect);
    set_color(app->renderer, color);
    SDL_RenderFillRect(app->renderer, &r);
}

static void outline_rect(App *app, FRect rect, SDL_Color color) {
    SDL_Rect r = to_sdl_rect(rect);
    set_color(app->renderer, color);
    SDL_RenderDrawRect(app->renderer, &r);
}

static void draw_line(App *app, double x1, double y1, double x2, double y2, SDL_Color color) {
    set_color(app->renderer, color);
    SDL_RenderDrawLine(app->renderer, (int)lrint(x1), (int)lrint(y1), (int)lrint(x2), (int)lrint(y2));
}

static void draw_thick_line(App *app, double x1, double y1, double x2, double y2, int thickness, SDL_Color color) {
    int half = thickness / 2;
    int i;
    for (i = -half; i <= half; ++i) {
        draw_line(app, x1 + i, y1, x2 + i, y2, color);
        draw_line(app, x1, y1 + i, x2, y2 + i, color);
    }
}

static void fill_circle(App *app, int cx, int cy, int radius, SDL_Color color) {
    int dy;
    set_color(app->renderer, color);
    for (dy = -radius; dy <= radius; ++dy) {
        int dx = (int)sqrt((double)(radius * radius - dy * dy));
        SDL_RenderDrawLine(app->renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static void draw_circle_outline(App *app, int cx, int cy, int radius, SDL_Color color) {
    int i;
    set_color(app->renderer, color);
    for (i = 0; i < 360; i += 6) {
        double a1 = (double)i * M_PI / 180.0;
        double a2 = (double)(i + 6) * M_PI / 180.0;
        SDL_RenderDrawLine(app->renderer,
                           cx + (int)lrint(cos(a1) * radius),
                           cy + (int)lrint(sin(a1) * radius),
                           cx + (int)lrint(cos(a2) * radius),
                           cy + (int)lrint(sin(a2) * radius));
    }
}

static TTF_Font *load_font_size(int size) {
    const char *paths[] = {
        "DejaVuSansMono.ttf",
        "Assignment9/DejaVuSansMono.ttf",
        "Assignment10/DejaVuSansMono.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/lucon.ttf",
        NULL
    };
    int i;
    for (i = 0; paths[i] != NULL; ++i) {
        TTF_Font *font = TTF_OpenFont(paths[i], size);
        if (font != NULL) {
            return font;
        }
    }
    return NULL;
}

static void draw_text(App *app, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect dst;
    if (font == NULL || text == NULL || text[0] == '\0') {
        return;
    }
    surface = TTF_RenderUTF8_Blended(font, text, color);
    if (surface == NULL) {
        return;
    }
    texture = SDL_CreateTextureFromSurface(app->renderer, surface);
    if (texture == NULL) {
        SDL_FreeSurface(surface);
        return;
    }
    dst.x = x;
    dst.y = y;
    dst.w = surface->w;
    dst.h = surface->h;
    SDL_RenderCopy(app->renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

static void draw_text_center(App *app, TTF_Font *font, const char *text, int centerX, int y, SDL_Color color) {
    int w = 0;
    int h = 0;
    if (font == NULL || TTF_SizeUTF8(font, text, &w, &h) != 0) {
        return;
    }
    draw_text(app, font, text, centerX - w / 2, y, color);
}

static void draw_panel(App *app, FRect rect, SDL_Color fill, SDL_Color border) {
    fill_rect(app, rect, fill);
    outline_rect(app, rect, border);
}

static void draw_progress_bar(App *app, FRect rect, double fraction, SDL_Color fill, SDL_Color border) {
    SDL_Rect frameRect;
    fraction = clamp_double(fraction, 0.0, 1.0);
    fill_rect(app, rect, color_rgba(14, 24, 28, 220));
    fill_rect(app, (FRect){rect.x + 3.0, rect.y + 3.0, (rect.w - 6.0) * fraction, rect.h - 6.0}, fill);
    if (app->assets.progressBarFrame != NULL) {
        frameRect = to_sdl_rect(rect);
        SDL_RenderCopy(app->renderer, app->assets.progressBarFrame, NULL, &frameRect);
    } else {
        outline_rect(app, rect, border);
    }
}

static int draw_texture_stretched(App *app, SDL_Texture *texture, FRect dst) {
    SDL_Rect out;
    if (texture == NULL) {
        return 0;
    }
    out = to_sdl_rect(dst);
    SDL_RenderCopy(app->renderer, texture, NULL, &out);
    return 1;
}

static int draw_texture_fit_ex(App *app, SDL_Texture *texture, FRect bounds, double angle, SDL_RendererFlip flip) {
    SDL_Rect out;
    int texW = 0;
    int texH = 0;
    double texAspect;
    double dstAspect;
    if (texture == NULL || SDL_QueryTexture(texture, NULL, NULL, &texW, &texH) != 0 || texW <= 0 || texH <= 0) {
        return 0;
    }
    texAspect = (double)texW / (double)texH;
    dstAspect = bounds.w / bounds.h;
    if (dstAspect > texAspect) {
        out.h = (int)lrint(bounds.h);
        out.w = (int)lrint(bounds.h * texAspect);
        out.x = (int)lrint(bounds.x + (bounds.w - (double)out.w) * 0.5);
        out.y = (int)lrint(bounds.y);
    } else {
        out.w = (int)lrint(bounds.w);
        out.h = (int)lrint(bounds.w / texAspect);
        out.x = (int)lrint(bounds.x);
        out.y = (int)lrint(bounds.y + (bounds.h - (double)out.h) * 0.5);
    }
    SDL_RenderCopyEx(app->renderer, texture, NULL, &out, angle, NULL, flip);
    return 1;
}

static int draw_texture_fit(App *app, SDL_Texture *texture, FRect bounds) {
    return draw_texture_fit_ex(app, texture, bounds, 0.0, SDL_FLIP_NONE);
}

static int animated_sprite_ready(const AnimatedSprite *sprite) {
    return sprite != NULL && sprite->frameCount > 0 && sprite->frames[0] != NULL;
}

static int animation_frame_index(const AnimatedSprite *sprite, double time) {
    int frame;
    if (!animated_sprite_ready(sprite)) {
        return -1;
    }
    if (sprite->fps <= 0.0) {
        return 0;
    }
    frame = (int)floor(time * sprite->fps);
    if (sprite->loop) {
        frame %= sprite->frameCount;
        if (frame < 0) {
            frame += sprite->frameCount;
        }
    } else {
        if (frame < 0) frame = 0;
        if (frame >= sprite->frameCount) frame = sprite->frameCount - 1;
    }
    return frame;
}

static int animation_state_frame(const AnimatedSprite *sprite, double fraction) {
    if (!animated_sprite_ready(sprite)) {
        return -1;
    }
    fraction = clamp_double(fraction, 0.0, 1.0);
    return (int)lrint(fraction * (double)(sprite->frameCount - 1));
}

static int animation_phase_frame(const AnimatedSprite *sprite, double radians) {
    double cycle;
    if (!animated_sprite_ready(sprite)) {
        return -1;
    }
    cycle = fmod(radians, 2.0 * M_PI);
    if (cycle < 0.0) {
        cycle += 2.0 * M_PI;
    }
    return (int)floor((cycle / (2.0 * M_PI)) * (double)sprite->frameCount) % sprite->frameCount;
}

static int draw_animation_frame_fit_ex(App *app, const AnimatedSprite *sprite, int frame, FRect bounds,
                                       double angle, SDL_RendererFlip flip) {
    if (!animated_sprite_ready(sprite)) {
        return 0;
    }
    if (frame < 0) frame = 0;
    if (frame >= sprite->frameCount) frame = sprite->frameCount - 1;
    return draw_texture_fit_ex(app, sprite->frames[frame], bounds, angle, flip);
}

static int draw_animation_loop_fit_ex(App *app, const AnimatedSprite *sprite, double time, FRect bounds,
                                      double angle, SDL_RendererFlip flip) {
    (void)time;
    return draw_animation_frame_fit_ex(app, sprite, animated_sprite_ready(sprite) ? sprite->currentFrame : -1,
                                       bounds, angle, flip);
}

static int draw_animation_loop_fit(App *app, const AnimatedSprite *sprite, double time, FRect bounds) {
    return draw_animation_loop_fit_ex(app, sprite, time, bounds, 0.0, SDL_FLIP_NONE);
}

static int draw_animation_frame_stretched_ex(App *app, const AnimatedSprite *sprite, int frame, FRect bounds,
                                             double angle, SDL_RendererFlip flip) {
    SDL_Rect out;
    if (!animated_sprite_ready(sprite)) {
        return 0;
    }
    if (frame < 0) frame = 0;
    if (frame >= sprite->frameCount) frame = sprite->frameCount - 1;
    out = to_sdl_rect(bounds);
    SDL_RenderCopyEx(app->renderer, sprite->frames[frame], NULL, &out, angle, NULL, flip);
    return 1;
}

static int draw_animation_loop_stretched_ex(App *app, const AnimatedSprite *sprite, double time, FRect bounds,
                                            double angle, SDL_RendererFlip flip) {
    (void)time;
    return draw_animation_frame_stretched_ex(app, sprite, animated_sprite_ready(sprite) ? sprite->currentFrame : -1,
                                            bounds, angle, flip);
}

static int draw_animation_loop_stretched(App *app, const AnimatedSprite *sprite, double time, FRect bounds) {
    return draw_animation_loop_stretched_ex(app, sprite, time, bounds, 0.0, SDL_FLIP_NONE);
}

static void update_animation_clock(AnimatedSprite *sprite, double dt) {
    int frame;
    if (!animated_sprite_ready(sprite) || dt <= 0.0) {
        return;
    }
    sprite->time += dt;
    if (sprite->time > 3600.0 && sprite->fps > 0.0) {
        sprite->time = fmod(sprite->time, (double)sprite->frameCount / sprite->fps);
    }
    frame = animation_frame_index(sprite, sprite->time);
    if (frame >= 0) {
        sprite->currentFrame = frame;
    }
}

static void update_animation_assets(Assets *assets, double dt) {
    update_animation_clock(&assets->anim.magnetTower, dt);
    update_animation_clock(&assets->anim.magneticField, dt);
    update_animation_clock(&assets->anim.turbineFan, dt);
    update_animation_clock(&assets->anim.windGust, dt);
    update_animation_clock(&assets->anim.crusher, dt);
    update_animation_clock(&assets->anim.extendableBridge, dt);
    update_animation_clock(&assets->anim.laserGate, dt);
    update_animation_clock(&assets->anim.sweeperArm, dt);
    update_animation_clock(&assets->anim.swingHammer, dt);
    update_animation_clock(&assets->anim.movingDock, dt);
}

static const char *graphics_quality_name(GraphicsQuality quality) {
    switch (quality) {
        case GRAPHICS_SIMPLE_FAST: return "Simple Fast";
        case GRAPHICS_FULL_ANIMATED: return "Full Animated";
        case GRAPHICS_POLISHED:
        default: return "Polished";
    }
}

static void cycle_graphics_quality(App *app) {
    if (app->graphicsQuality == GRAPHICS_SIMPLE_FAST) {
        app->graphicsQuality = GRAPHICS_POLISHED;
    } else if (app->graphicsQuality == GRAPHICS_POLISHED) {
        app->graphicsQuality = GRAPHICS_FULL_ANIMATED;
    } else {
        app->graphicsQuality = GRAPHICS_SIMPLE_FAST;
    }
    printf("Graphics quality: %s\n", graphics_quality_name(app->graphicsQuality));
}

static void drawMenuPanel(App *app, FRect rect, SDL_Color fill, SDL_Color border) {
    if (!draw_texture_stretched(app, app->assets.menuPanel, rect)) {
        draw_panel(app, rect, fill, border);
    } else {
        outline_rect(app, rect, border);
    }
}

static int frect_intersects(FRect a, FRect b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

static int point_in_frect(double x, double y, FRect r) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

static int mouse_over_rect(const Game *game, FRect r) {
    return game->mouseValid && point_in_frect(game->mouseX, game->mouseY, r);
}

static FRect main_menu_button_rect(int index) {
    return (FRect){430.0, 245.0 + (double)index * 62.0, 420.0, 48.0};
}

static FRect level_select_button_rect(int index) {
    int col = index / 3;
    int row = index % 3;
    return (FRect){135.0 + (double)col * 510.0, 178.0 + (double)row * 88.0, 500.0, 70.0};
}

static FRect level_select_back_rect(void) {
    return (FRect){535.0, 558.0, 210.0, 42.0};
}

static FRect pause_button_rect(int index) {
    return (FRect){430.0, 284.0 + (double)index * 50.0, 420.0, 44.0};
}

static FRect win_button_rect(int index) {
    return (FRect){430.0, 348.0 + (double)index * 50.0, 420.0, 44.0};
}

static FRect lose_button_rect(int index) {
    return (FRect){430.0, 338.0 + (double)index * 56.0, 420.0, 48.0};
}

static FRect how_to_play_back_rect(void) {
    return (FRect){535.0, 548.0, 210.0, 44.0};
}

static FRect cargo_rect(const Cargo *cargo) {
    FRect r;
    r.x = cargo->x - CARGO_W * 0.5;
    r.y = cargo->y - CARGO_H * 0.5;
    r.w = CARGO_W;
    r.h = CARGO_H;
    return r;
}

static FRect moved_rect(FRect base, MotionAxis axis, double amplitude, double omega, double phase, double time) {
    double offset = amplitude * sin(omega * time + phase);
    if (axis == AXIS_X) {
        base.x += offset;
    } else if (axis == AXIS_Y) {
        base.y += offset;
    }
    return base;
}

static FRect current_target_rect_for_level(const Level *level, double time) {
    return moved_rect(level->targetBase, level->targetAxis, level->targetAmplitude,
                      level->targetOmega, level->targetPhase, time);
}

static FRect current_target_rect(const Game *game) {
    const Level *level = &game->levels[game->selectedLevel];
    return current_target_rect_for_level(level, game->levelElapsed);
}

static FRect current_gate_rect(const MovingGate *gate, double time) {
    return moved_rect(gate->baseRect, gate->axis, gate->amplitude, gate->omega, gate->phase, time);
}

static double cycle01(double time, double period, double phase) {
    double t;
    if (period <= 0.0) {
        return 0.0;
    }
    t = fmod(time + phase, period);
    if (t < 0.0) t += period;
    return t / period;
}

static int laser_is_active(const LaserHazard *laser, double time) {
    return cycle01(time, laser->period, laser->phase) < laser->activeFraction;
}

static void laser_gate_cap_rects(const LaserHazard *laser, FRect *top, FRect *bottom) {
    double capH = laser->frame.h * 0.14;
    if (capH < 26.0) capH = 26.0;
    if (capH > 42.0) capH = 42.0;
    *top = (FRect){laser->frame.x + 8.0, laser->frame.y, laser->frame.w - 16.0, capH};
    *bottom = (FRect){laser->frame.x + 8.0, laser->frame.y + laser->frame.h - capH, laser->frame.w - 16.0, capH};
}

static double magnet_pulse(const MagnetHazard *magnet, double time) {
    return 0.35 + 0.65 * (0.5 + 0.5 * sin(magnet->omega * time + magnet->phase));
}

static FRect current_bridge_rect(const BridgeHazard *bridge, double time) {
    double phase = 0.5 + 0.5 * sin((2.0 * M_PI / bridge->period) * time + bridge->phase);
    double width = bridge->minWidth + (bridge->maxWidth - bridge->minWidth) * phase;
    FRect r = bridge->baseRect;
    r.w = width;
    if (!bridge->extendRight) {
        r.x = bridge->baseRect.x + bridge->maxWidth - width;
    }
    return r;
}

static FRect current_crusher_head_rect(const CrusherHazard *crusher, double time) {
    double phase = 0.5 + 0.5 * sin((2.0 * M_PI / crusher->period) * time + crusher->phase);
    FRect r = crusher->headRect;
    r.y += crusher->travel * phase;
    return r;
}

static double current_hammer_angle(const HammerHazard *hammer, double time) {
    return hammer->amplitude * sin(hammer->omega * time + hammer->phase);
}

static double current_sweeper_angle(const SweeperHazard *sweeper, double time) {
    return sweeper->omega * time + sweeper->phase;
}

static FRect cargo_hit_rect(const Game *game) {
    FRect r;
    double w = CARGO_W;
    double h = CARGO_H;
    const Level *level = &game->levels[game->selectedLevel];
    if (level->cargoKind == CARGO_HEAVY) {
        w = 66.0;
        h = 52.0;
    } else if (level->cargoKind == CARGO_FRAGILE) {
        w = 48.0;
        h = 42.0;
    }
    r.x = game->cargo.x - w * 0.5;
    r.y = game->cargo.y - h * 0.5;
    r.w = w;
    r.h = h;
    return r;
}

static double point_segment_distance(double px, double py, double ax, double ay, double bx, double by) {
    double vx = bx - ax;
    double vy = by - ay;
    double wx = px - ax;
    double wy = py - ay;
    double len2 = vx * vx + vy * vy;
    double t = 0.0;
    double cx;
    double cy;
    if (len2 > 0.0001) {
        t = (wx * vx + wy * vy) / len2;
        t = clamp_double(t, 0.0, 1.0);
    }
    cx = ax + t * vx;
    cy = ay + t * vy;
    return sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
}

static int cargo_hits_segment(FRect cargo, double ax, double ay, double bx, double by, double radius) {
    double cx = cargo.x + cargo.w * 0.5;
    double cy = cargo.y + cargo.h * 0.5;
    double cargoRadius = 0.42 * (cargo.w > cargo.h ? cargo.w : cargo.h);
    return point_segment_distance(cx, cy, ax, ay, bx, by) <= radius + cargoRadius;
}

static void add_obstacle(Level *level, FRect obstacle) {
    if (level->obstacleCount < MAX_STATIC_OBSTACLES) {
        level->obstacles[level->obstacleCount++] = obstacle;
    }
}

static void add_bay_walls(Level *level, double wallTop, double wallHeight) {
    FRect target = level->targetBase;
    add_obstacle(level, (FRect){target.x - 30.0, wallTop, 22.0, wallHeight});
    add_obstacle(level, (FRect){target.x + target.w + 8.0, wallTop, 22.0, wallHeight});
}

static void set_star(Level *level, int index, double x, double y, double radius) {
    if (index >= 0 && index < STAR_COUNT) {
        level->stars[index].x = x;
        level->stars[index].y = y;
        level->stars[index].radius = radius;
        level->stars[index].collected = 0;
    }
}

static void add_unstable_zone(Level *level, FRect rect, double timeLimit) {
    if (level->unstableZoneCount < MAX_UNSTABLE_ZONES) {
        level->unstableZones[level->unstableZoneCount].rect = rect;
        level->unstableZones[level->unstableZoneCount].timeLimit = timeLimit;
        level->unstableZoneCount++;
    }
}

static void initialize_levels(Game *game) {
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

    l = &game->levels[0];
    l->name = "Level 1: Intro Crane Test";
    l->objective = "Single pendulum: collect stars, then pocket the cargo.";
    l->hint = "Medium bay, one overhead beam, and three optional stars teach the scoring route.";
    l->timeLimit = 110.0;
    l->startTrolleyX = 165.0;
    l->startCableLength = 270.0;
    l->startTheta = 0.03;
    l->targetBase = (FRect){970.0, 552.0, 170.0, 82.0};
    l->stableSpeedLimit = 75.0;
    l->stableAngleLimit = 0.14;
    l->stableOmegaLimit = 0.42;
    l->lowTargetRatio = 0.50;
    add_obstacle(l, (FRect){820.0, 360.0, 235.0, 36.0});
    add_bay_walls(l, 516.0, 116.0);
    set_star(l, 0, 505.0, 370.0, 25.0);
    set_star(l, 1, 890.0, 585.0, 25.0);
    set_star(l, 2, 930.0, 326.0, 25.0);

    l = &game->levels[1];
    l->name = "Level 2: Single Pendulum Gate Trial";
    l->objective = "Single pendulum: low, lift, low, then bay.";
    l->hint = "One cable height cannot solve this. Change length through each gate.";
    l->timeLimit = 100.0;
    l->startTrolleyX = 160.0;
    l->startCableLength = 275.0;
    l->startTheta = -0.035;
    l->targetBase = (FRect){1035.0, 552.0, 115.0, 82.0};
    l->stableSpeedLimit = 50.0;
    l->stableAngleLimit = 0.087;
    l->stableOmegaLimit = 0.28;
    l->lowTargetRatio = 0.56;
    l->cargoKind = CARGO_FRAGILE;
    add_obstacle(l, (FRect){260.0, 330.0, 300.0, 36.0});
    add_obstacle(l, (FRect){560.0, 500.0, 118.0, 80.0});
    add_obstacle(l, (FRect){730.0, 342.0, 265.0, 36.0});
    add_obstacle(l, (FRect){928.0, 492.0, 58.0, 42.0});
    add_bay_walls(l, 514.0, 118.0);
    set_star(l, 0, 410.0, 410.0, 24.0);
    set_star(l, 1, 620.0, 455.0, 24.0);
    set_star(l, 2, 955.0, 505.0, 24.0);

    l = &game->levels[2];
    l->name = "Level 3: Double Pendulum Stabilization";
    l->objective = "Double pendulum: calm the whole chain.";
    l->hint = "Cargo speed alone is not enough. The upper link must settle too.";
    l->timeLimit = 120.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 320.0;
    l->startTheta = 0.02;
    l->targetBase = (FRect){995.0, 552.0, 145.0, 82.0};
    l->stableSpeedLimit = 45.0;
    l->stableAngleLimit = 0.18;
    l->stableOmegaLimit = 0.62;
    l->safeChainMotion = 180.0;
    l->lowTargetRatio = 0.53;
    l->pendulumMode = PENDULUM_DOUBLE;
    l->dampingScale = 1.08;
    add_obstacle(l, (FRect){500.0, 354.0, 220.0, 34.0});
    add_obstacle(l, (FRect){805.0, 350.0, 170.0, 34.0});
    add_obstacle(l, (FRect){820.0, 500.0, 88.0, 82.0});
    add_bay_walls(l, 514.0, 118.0);
    set_star(l, 0, 485.0, 330.0, 24.0);
    set_star(l, 1, 680.0, 505.0, 24.0);
    set_star(l, 2, 910.0, 440.0, 24.0);

    l = &game->levels[3];
    l->name = "Level 4: Double Pendulum Needle Corridor";
    l->objective = "Double pendulum: thread the no-stop corridor.";
    l->hint = "A narrow S-route and unstable zone punish creeping and swing.";
    l->timeLimit = 130.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 335.0;
    l->startTheta = -0.025;
    l->targetBase = (FRect){1060.0, 552.0, 95.0, 82.0};
    l->stableSpeedLimit = 32.0;
    l->stableAngleLimit = 0.13;
    l->stableOmegaLimit = 0.45;
    l->safeChainMotion = 110.0;
    l->requiredHoldTime = 3.5;
    l->lowTargetRatio = 0.58;
    l->cargoKind = CARGO_FRAGILE;
    l->pendulumMode = PENDULUM_DOUBLE;
    l->dampingScale = 1.02;
    add_obstacle(l, (FRect){250.0, 328.0, 260.0, 36.0});
    add_obstacle(l, (FRect){420.0, 496.0, 90.0, 84.0});
    add_obstacle(l, (FRect){540.0, 318.0, 240.0, 36.0});
    add_obstacle(l, (FRect){710.0, 470.0, 85.0, 102.0});
    add_obstacle(l, (FRect){835.0, 358.0, 160.0, 36.0});
    add_bay_walls(l, 512.0, 120.0);
    add_unstable_zone(l, (FRect){585.0, 126.0, 185.0, 526.0}, 4.0);
    set_star(l, 0, 330.0, 420.0, 23.0);
    set_star(l, 1, 660.0, 452.0, 23.0);
    set_star(l, 2, 910.0, 500.0, 23.0);

    l = &game->levels[4];
    l->name = "Level 5: Triple Pendulum Height Puzzle";
    l->objective = "Triple pendulum: low, lift, low, calm.";
    l->hint = "Plan cable changes early. Sudden height changes wake every link.";
    l->timeLimit = 150.0;
    l->startTrolleyX = 150.0;
    l->startCableLength = 360.0;
    l->startTheta = 0.018;
    l->targetBase = (FRect){1020.0, 552.0, 120.0, 82.0};
    l->stableSpeedLimit = 28.0;
    l->stableAngleLimit = 0.17;
    l->stableOmegaLimit = 0.58;
    l->safeChainMotion = 95.0;
    l->requiredHoldTime = 3.5;
    l->lowTargetRatio = 0.58;
    l->pendulumMode = PENDULUM_TRIPLE;
    l->dampingScale = 1.10;
    add_obstacle(l, (FRect){245.0, 335.0, 300.0, 36.0});
    add_obstacle(l, (FRect){575.0, 500.0, 125.0, 84.0});
    add_obstacle(l, (FRect){730.0, 340.0, 255.0, 36.0});
    add_obstacle(l, (FRect){905.0, 502.0, 66.0, 82.0});
    add_bay_walls(l, 512.0, 120.0);
    set_star(l, 0, 380.0, 420.0, 23.0);
    set_star(l, 1, 640.0, 452.0, 23.0);
    set_star(l, 2, 952.0, 430.0, 23.0);

    l = &game->levels[5];
    l->name = "Level 6: Triple Pendulum Final Exam";
    l->objective = "Triple pendulum: boss route and fragile bay.";
    l->hint = "Low ceiling, lift block, unstable bridge, slot, and strict chain calm.";
    l->timeLimit = 165.0;
    l->startTrolleyX = 155.0;
    l->startCableLength = 375.0;
    l->startTheta = -0.018;
    l->targetBase = (FRect){1070.0, 552.0, 90.0, 82.0};
    l->stableSpeedLimit = 22.0;
    l->stableAngleLimit = 0.10;
    l->stableOmegaLimit = 0.36;
    l->safeChainMotion = 65.0;
    l->requiredHoldTime = 4.0;
    l->lowTargetRatio = 0.60;
    l->cargoKind = CARGO_FRAGILE;
    l->pendulumMode = PENDULUM_TRIPLE;
    l->dampingScale = 1.08;
    add_obstacle(l, (FRect){220.0, 326.0, 310.0, 36.0});
    add_obstacle(l, (FRect){548.0, 500.0, 120.0, 86.0});
    add_obstacle(l, (FRect){745.0, 310.0, 255.0, 36.0});
    add_obstacle(l, (FRect){800.0, 500.0, 90.0, 88.0});
    add_obstacle(l, (FRect){900.0, 362.0, 130.0, 34.0});
    add_obstacle(l, (FRect){960.0, 496.0, 60.0, 86.0});
    add_bay_walls(l, 512.0, 120.0);
    add_unstable_zone(l, (FRect){665.0, 126.0, 170.0, 526.0}, 4.0);
    set_star(l, 0, 355.0, 414.0, 22.0);
    set_star(l, 1, 735.0, 454.0, 22.0);
    set_star(l, 2, 1010.0, 455.0, 22.0);
}

static void update_cargo_position(Game *game) {
    game->cargo.x = game->trolley.x + game->cargo.length * sin(game->cargo.theta);
    game->cargo.y = PIVOT_Y + game->cargo.length * cos(game->cargo.theta);
}

static const char *pendulum_mode_name(PendulumMode mode) {
    if (mode == PENDULUM_DOUBLE) return "Double";
    if (mode == PENDULUM_TRIPLE) return "Triple";
    return "Single";
}

static int pendulum_mass_count(PendulumMode mode) {
    if (mode == PENDULUM_TRIPLE) return 3;
    if (mode == PENDULUM_DOUBLE) return 2;
    return 1;
}

static double level_hold_time(const Level *level) {
    return level->requiredHoldTime > 0.0 ? level->requiredHoldTime : WIN_STABLE_SECONDS;
}

static int count_collected_stars(const Level *level) {
    int i;
    int count = 0;
    for (i = 0; i < STAR_COUNT; ++i) {
        if (level->stars[i].collected) {
            count++;
        }
    }
    return count;
}

static int campaign_best_star_total(const Game *game) {
    int i;
    int total = 0;
    for (i = 0; i < LEVEL_COUNT; ++i) {
        total += game->bestStars[i];
    }
    return total;
}

static const char *operator_rank_for_stars(int stars) {
    if (stars >= 3) return "Gold Operator";
    if (stars == 2) return "Silver Operator";
    if (stars == 1) return "Bronze Operator";
    return "Delivery Complete";
}

static const char *campaign_rank_for_stars(int stars) {
    if (stars >= 16) return "Master Crane Operator";
    if (stars >= 11) return "Expert Operator";
    if (stars >= 6) return "Licensed Operator";
    return "Trainee";
}

static void sync_chain_lengths(Game *game) {
    int i;
    int count = game->chain.massCount;
    double segment;

    if (count < 1) count = 1;
    if (count > 3) count = 3;
    game->chain.massCount = count;
    game->chain.totalLength = game->cargo.length;
    segment = game->cargo.length / (double)count;

    for (i = 0; i < 3; ++i) {
        game->chain.segmentLength[i] = i < count ? segment : 0.0;
    }
}

static void update_cargo_from_chain(Game *game) {
    int last = game->chain.massCount - 1;
    if (last < 0) last = 0;
    if (last > 2) last = 2;
    game->cargo.x = game->chain.masses[last].x;
    game->cargo.y = game->chain.masses[last].y;
}

static void initialize_multi_pendulum(Game *game, const Level *level) {
    int i;
    double angle = level->startTheta;
    double baseX = game->trolley.x;
    double baseY = PIVOT_Y;

    game->chain.mode = level->pendulumMode;
    game->chain.massCount = pendulum_mass_count(level->pendulumMode);
    sync_chain_lengths(game);

    if (level->pendulumMode == PENDULUM_SINGLE) {
        update_cargo_position(game);
        game->chain.masses[0].x = game->cargo.x;
        game->chain.masses[0].y = game->cargo.y;
        game->chain.masses[0].prevX = game->cargo.x;
        game->chain.masses[0].prevY = game->cargo.y;
        game->cargo.prevX = game->cargo.x;
        game->cargo.prevY = game->cargo.y;
        game->cargo.speed = 0.0;
        return;
    }

    for (i = 0; i < game->chain.massCount; ++i) {
        double length = game->chain.segmentLength[i];
        ChainMass *mass = &game->chain.masses[i];
        mass->x = baseX + length * sin(angle);
        mass->y = baseY + length * cos(angle);
        mass->prevX = mass->x;
        mass->prevY = mass->y;
        baseX = mass->x;
        baseY = mass->y;
    }

    update_cargo_from_chain(game);
    game->cargo.prevX = game->cargo.x;
    game->cargo.prevY = game->cargo.y;
    game->cargo.speed = 0.0;
}

static void reset_level(Game *game, int levelIndex) {
    Level *level;
    int i;
    if (levelIndex < 0) {
        levelIndex = 0;
    }
    if (levelIndex >= LEVEL_COUNT) {
        levelIndex = LEVEL_COUNT - 1;
    }
    game->selectedLevel = levelIndex;
    game->levelIndex = levelIndex;
    level = &game->levels[game->selectedLevel];
    game->state = STATE_PLAYING;
    game->timeLeft = level->timeLimit;
    game->levelElapsed = 0.0;
    game->stableTimer = 0.0;
    game->cargoTooFast = 0;
    game->cargoTooHigh = 0;
    game->cargoTooMuchSwing = 0;
    game->cargoInDeliveryZone = 0;
    game->deliveryValid = 0;
    game->cargoInWind = 0;
    game->cargoInMagnet = 0;
    game->chainTooMuchMotion = 0;
    game->unstableZoneActive = 0;
    game->unstableZoneTooLong = 0;
    game->chainMotion = 0.0;
    game->unstableZoneTimer = 0.0;
    game->starMessageTimer = 0.0;
    game->lastStarsCollected = 0;
    game->lastCompletionTime = 0.0;
    game->lossReason = LOSS_NONE;
    for (i = 0; i < STAR_COUNT; ++i) {
        level->stars[i].collected = 0;
    }
    game->trolley.x = level->startTrolleyX;
    game->trolley.velocity = 0.0;
    game->trolley.acceleration = 0.0;
    game->cargo.theta = level->startTheta;
    game->cargo.omega = 0.0;
    game->cargo.length = level->startCableLength;
    initialize_multi_pendulum(game, level);
}

static void initialize_game(Game *game) {
    memset(game, 0, sizeof(*game));
    game->running = 1;
    game->state = STATE_MAIN_MENU;
    initialize_levels(game);
    reset_level(game, 0);
    game->state = STATE_MAIN_MENU;
}

static void toggle_fullscreen(App *app, Game *game) {
    if (!game->fullscreen) {
        SDL_GetWindowPosition(app->window, &app->windowedX, &app->windowedY);
        SDL_GetWindowSize(app->window, &app->windowedW, &app->windowedH);
        if (SDL_SetWindowFullscreen(app->window, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0) {
            game->fullscreen = 1;
        }
    } else {
        if (SDL_SetWindowFullscreen(app->window, 0) == 0) {
            game->fullscreen = 0;
            SDL_SetWindowSize(app->window, app->windowedW, app->windowedH);
            SDL_SetWindowPosition(app->window, app->windowedX, app->windowedY);
        }
    }
    SDL_RenderSetLogicalSize(app->renderer, LOGICAL_W, LOGICAL_H);
}

static void move_menu_index(int *index, int count, int delta) {
    *index += delta;
    if (*index < 0) {
        *index = count - 1;
    }
    if (*index >= count) {
        *index = 0;
    }
}

static void update_gameplay_input(Game *game) {
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    game->motorInput = 0;
    game->cableInput = 0;
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
        game->motorInput -= 1;
    }
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
        game->motorInput += 1;
    }
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
        game->cableInput -= 1;
    }
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
        game->cableInput += 1;
    }
    game->stabilizerHeld = keys[SDL_SCANCODE_SPACE] ? 1 : 0;
}

static int cargo_hits_any_obstacle(const Game *game) {
    const Level *level = &game->levels[game->selectedLevel];
    FRect c = cargo_hit_rect(game);
    int i;
    for (i = 0; i < level->obstacleCount; ++i) {
        if (frect_intersects(c, level->obstacles[i])) {
            return 1;
        }
    }
    for (i = 0; i < level->gateCount; ++i) {
        FRect gate = current_gate_rect(&level->gates[i], game->levelElapsed);
        if (frect_intersects(c, gate)) {
            return 1;
        }
    }
    for (i = 0; i < level->magnetCount; ++i) {
        if (frect_intersects(c, level->magnets[i].body)) {
            return 1;
        }
    }
    for (i = 0; i < level->laserCount; ++i) {
        FRect capTop;
        FRect capBottom;
        laser_gate_cap_rects(&level->lasers[i], &capTop, &capBottom);
        if (frect_intersects(c, capTop) || frect_intersects(c, capBottom)) {
            return 1;
        }
        if (laser_is_active(&level->lasers[i], game->levelElapsed) && frect_intersects(c, level->lasers[i].beam)) {
            return 1;
        }
    }
    for (i = 0; i < level->bridgeCount; ++i) {
        FRect bridge = current_bridge_rect(&level->bridges[i], game->levelElapsed);
        if (frect_intersects(c, bridge)) {
            return 1;
        }
    }
    for (i = 0; i < level->crusherCount; ++i) {
        FRect head = current_crusher_head_rect(&level->crushers[i], game->levelElapsed);
        if (frect_intersects(c, head)) {
            return 1;
        }
    }
    for (i = 0; i < level->hammerCount; ++i) {
        const HammerHazard *h = &level->hammers[i];
        double a = current_hammer_angle(h, game->levelElapsed);
        double bx = h->pivotX + h->length * sin(a);
        double by = h->pivotY + h->length * cos(a);
        if (cargo_hits_segment(c, h->pivotX, h->pivotY, bx, by, h->bobRadius * 0.46)) {
            return 1;
        }
    }
    for (i = 0; i < level->sweeperCount; ++i) {
        const SweeperHazard *s = &level->sweepers[i];
        double a = current_sweeper_angle(s, game->levelElapsed);
        double ax = s->pivotX - cos(a) * s->length * 0.5;
        double ay = s->pivotY - sin(a) * s->length * 0.5;
        double bx = s->pivotX + cos(a) * s->length * 0.5;
        double by = s->pivotY + sin(a) * s->length * 0.5;
        if (cargo_hits_segment(c, ax, ay, bx, by, s->width * 0.5)) {
            return 1;
        }
    }
    if (c.y < HUD_H) {
        return 1;
    }
    return 0;
}

static void update_star_collection(Game *game) {
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

static void update_unstable_zones(Game *game, double dt) {
    const Level *level = &game->levels[game->selectedLevel];
    int i;
    int active = 0;
    double limit = 0.0;

    for (i = 0; i < level->unstableZoneCount; ++i) {
        const UnstableZone *zone = &level->unstableZones[i];
        if (point_in_frect(game->cargo.x, game->cargo.y, zone->rect)
            || point_in_frect(game->trolley.x, PIVOT_Y, zone->rect)) {
            active = 1;
            limit = zone->timeLimit;
            break;
        }
    }

    game->unstableZoneActive = active;
    game->unstableZoneTooLong = 0;
    if (!active) {
        game->unstableZoneTimer = 0.0;
        return;
    }

    game->unstableZoneTimer += dt;
    if (limit > 0.0 && game->unstableZoneTimer >= limit) {
        game->unstableZoneTooLong = 1;
        game->state = STATE_LOSE;
        game->lossReason = LOSS_UNSTABLE;
        game->loseIndex = 0;
    }
}

static void update_stability_and_win(Game *game, double dt) {
    const Level *level = &game->levels[game->selectedLevel];
    FRect target = current_target_rect(game);
    double holdTime = level_hold_time(level);
    double angleAbs = fabs(game->cargo.theta);
    double omegaAbs = fabs(game->cargo.omega);
    int insideX = game->cargo.x >= target.x + 18.0 && game->cargo.x <= target.x + target.w - 18.0;
    int insideY = game->cargo.y >= target.y + 20.0 && game->cargo.y <= target.y + target.h - 10.0;
    int lowEnough = game->cargo.y >= target.y + target.h * level->lowTargetRatio;
    int insideTarget = insideX && insideY && lowEnough;
    int speedOk = game->cargo.speed < level->stableSpeedLimit;
    int singleSwingOk = angleAbs < level->stableAngleLimit && omegaAbs < level->stableOmegaLimit;
    int chainOk = level->pendulumMode == PENDULUM_SINGLE
                || level->safeChainMotion <= 0.0
                || game->chainMotion < level->safeChainMotion;
    int stable = speedOk && (level->pendulumMode == PENDULUM_SINGLE ? singleSwingOk : chainOk);

    game->cargoInDeliveryZone = insideX && insideY;
    game->cargoTooHigh = insideX && insideY && !lowEnough;
    game->cargoTooFast = insideTarget && !speedOk;
    game->chainTooMuchMotion = insideTarget && level->pendulumMode != PENDULUM_SINGLE && !chainOk;
    game->cargoTooMuchSwing = insideTarget && level->pendulumMode == PENDULUM_SINGLE && !singleSwingOk;
    game->deliveryValid = insideTarget && stable;

    if (game->deliveryValid) {
        game->stableTimer += dt;
    } else {
        game->stableTimer = 0.0;
    }

    if (game->stableTimer >= holdTime) {
        int stars = count_collected_stars(level);
        game->lastStarsCollected = stars;
        game->lastCompletionTime = level->timeLimit - game->timeLeft;
        if (stars > game->bestStars[game->selectedLevel]) {
            game->bestStars[game->selectedLevel] = stars;
        }
        game->state = STATE_WIN;
        game->winIndex = 0;
    }
}

static double wind_acceleration_for_cargo(Game *game) {
    const Level *level = &game->levels[game->selectedLevel];
    double windAcceleration = 0.0;
    int i;
    game->cargoInWind = 0;

    for (i = 0; i < level->windZoneCount; ++i) {
        WindZone zone = level->windZones[i];
        if (point_in_frect(game->cargo.x, game->cargo.y, zone.rect)) {
            double heightFactor = (zone.rect.y + zone.rect.h - game->cargo.y) / zone.rect.h;
            heightFactor = clamp_double(heightFactor, 0.0, 1.0);
            windAcceleration += (double)zone.direction * zone.strength * level->windScale * (0.25 + 0.90 * heightFactor);
            game->cargoInWind = 1;
        }
    }

    return windAcceleration;
}

static double magnet_acceleration_for_cargo(Game *game) {
    const Level *level = &game->levels[game->selectedLevel];
    double accel = 0.0;
    int i;
    game->cargoInMagnet = 0;
    for (i = 0; i < level->magnetCount; ++i) {
        const MagnetHazard *m = &level->magnets[i];
        double dx = m->x - game->cargo.x;
        double dy = m->y - game->cargo.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < m->radius && dist > 1.0) {
            double falloff = 1.0 - dist / m->radius;
            accel += (dx / dist) * m->strength * level->magnetScale * falloff * magnet_pulse(m, game->levelElapsed);
            game->cargoInMagnet = 1;
        }
    }
    return accel;
}

static void enforce_anchor_constraint(ChainMass *mass, double anchorX, double anchorY, double desiredLength) {
    double dx = mass->x - anchorX;
    double dy = mass->y - anchorY;
    double dist = sqrt(dx * dx + dy * dy);
    double scale;

    if (dist < 0.0001) {
        mass->x = anchorX;
        mass->y = anchorY + desiredLength;
        return;
    }

    scale = desiredLength / dist;
    mass->x = anchorX + dx * scale;
    mass->y = anchorY + dy * scale;
}

static void enforce_pair_constraint(ChainMass *a, ChainMass *b, double desiredLength) {
    double dx = b->x - a->x;
    double dy = b->y - a->y;
    double dist = sqrt(dx * dx + dy * dy);
    double error;
    double correctionX;
    double correctionY;

    if (dist < 0.0001) {
        b->y = a->y + desiredLength;
        return;
    }

    error = (dist - desiredLength) / dist;
    correctionX = dx * 0.5 * error;
    correctionY = dy * 0.5 * error;
    a->x += correctionX;
    a->y += correctionY;
    b->x -= correctionX;
    b->y -= correctionY;
}

static void update_multi_pendulum(Game *game, const Level *level, double dt) {
    int i;
    int iteration;
    int iterations = level->pendulumMode == PENDULUM_TRIPLE ? 14 : 11;
    double oldCargoX = game->cargo.x;
    double oldCargoY = game->cargo.y;
    double oldTheta = game->cargo.theta;
    double dampingRate = (game->stabilizerHeld ? 5.3 : 0.45) * level->dampingScale;
    double damping = exp(-dampingRate * dt);
    double maxDisplacement = 34.0;

    if (game->chain.mode != level->pendulumMode
        || game->chain.massCount != pendulum_mass_count(level->pendulumMode)) {
        initialize_multi_pendulum(game, level);
    }

    game->cargoInWind = 0;
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

    /*
       Double and triple cargo use a compact Verlet rope solver. The trolley pivot is
       fixed for the first constraint, then each pair of masses shares correction.
    */
    for (iteration = 0; iteration < iterations; ++iteration) {
        enforce_anchor_constraint(&game->chain.masses[0], game->trolley.x, PIVOT_Y,
                                  game->chain.segmentLength[0]);
        for (i = 1; i < game->chain.massCount; ++i) {
            enforce_pair_constraint(&game->chain.masses[i - 1], &game->chain.masses[i],
                                    game->chain.segmentLength[i]);
        }
    }

    update_cargo_from_chain(game);
    game->cargo.speed = sqrt((game->cargo.x - oldCargoX) * (game->cargo.x - oldCargoX)
                           + (game->cargo.y - oldCargoY) * (game->cargo.y - oldCargoY)) / dt;
    game->chainMotion = 0.0;
    for (i = 0; i < game->chain.massCount; ++i) {
        double vx = (game->chain.masses[i].x - game->chain.masses[i].prevX) / dt;
        double vy = (game->chain.masses[i].y - game->chain.masses[i].prevY) / dt;
        game->chainMotion += sqrt(vx * vx + vy * vy);
    }
    game->cargo.prevX = game->cargo.x;
    game->cargo.prevY = game->cargo.y;
    game->cargo.theta = atan2(game->cargo.x - game->trolley.x, game->cargo.y - PIVOT_Y);
    {
        double deltaTheta = game->cargo.theta - oldTheta;
        if (deltaTheta > M_PI) deltaTheta -= 2.0 * M_PI;
        if (deltaTheta < -M_PI) deltaTheta += 2.0 * M_PI;
        game->cargo.omega = deltaTheta / dt;
    }
}

static void update_physics(Game *game, double dt) {
    const Level *level = &game->levels[game->selectedLevel];
    double force;
    double pendulumDamping;
    double thetaDDot;
    double newCargoX;
    double newCargoY;
    double windAccel;
    double magnetAccel;

    if (game->state != STATE_PLAYING) {
        return;
    }

    game->timeLeft -= dt;
    game->levelElapsed += dt;
    if (game->starMessageTimer > 0.0) {
        game->starMessageTimer -= dt;
        if (game->starMessageTimer < 0.0) {
            game->starMessageTimer = 0.0;
        }
    }
    if (game->timeLeft <= 0.0) {
        game->timeLeft = 0.0;
        game->state = STATE_LOSE;
        game->lossReason = LOSS_TIMEOUT;
        game->loseIndex = 0;
        return;
    }

    force = (double)game->motorInput * MOTOR_FORCE;
    force += -TROLLEY_DAMPING * game->trolley.velocity;
    if (game->stabilizerHeld) {
        force += -STABILIZER_TROLLEY_DAMPING * game->trolley.velocity;
    }

    game->trolley.acceleration = force / TROLLEY_MASS;
    game->trolley.velocity += game->trolley.acceleration * dt;
    game->trolley.x += game->trolley.velocity * dt;

    if (game->trolley.x < RAIL_MIN_X) {
        game->trolley.x = RAIL_MIN_X;
        if (game->trolley.velocity < 0.0) game->trolley.velocity = 0.0;
    }
    if (game->trolley.x > RAIL_MAX_X) {
        game->trolley.x = RAIL_MAX_X;
        if (game->trolley.velocity > 0.0) game->trolley.velocity = 0.0;
    }

    game->cargo.length += (double)game->cableInput * CABLE_RATE * dt;
    game->cargo.length = clamp_double(game->cargo.length, MIN_CABLE_LENGTH, MAX_CABLE_LENGTH);

    if (level->pendulumMode != PENDULUM_SINGLE) {
        update_multi_pendulum(game, level, dt);
    } else {
        pendulumDamping = BASE_SWING_DAMPING * level->dampingScale;
        if (game->stabilizerHeld) {
            pendulumDamping += STABILIZER_SWING_DAMPING * level->dampingScale;
        }
        windAccel = wind_acceleration_for_cargo(game);
        magnetAccel = magnet_acceleration_for_cargo(game);

        /*
           Driven pendulum physics:
           theta_ddot = -(g / L) * sin(theta)
                        - (trolley_ax / L) * cos(theta)
                        + ((wind_ax + magnet_ax) / L) * cos(theta)
                        - damping * omega
           Semi-implicit Euler then updates omega first, theta second.
        */
        thetaDDot = -(GRAVITY / game->cargo.length) * sin(game->cargo.theta)
                    - (game->trolley.acceleration / game->cargo.length) * cos(game->cargo.theta)
                    + ((windAccel + magnetAccel) / game->cargo.length) * cos(game->cargo.theta)
                    - pendulumDamping * game->cargo.omega;
        game->cargo.omega += thetaDDot * dt;
        game->cargo.theta += game->cargo.omega * dt;

        newCargoX = game->trolley.x + game->cargo.length * sin(game->cargo.theta);
        newCargoY = PIVOT_Y + game->cargo.length * cos(game->cargo.theta);
        game->cargo.speed = sqrt((newCargoX - game->cargo.prevX) * (newCargoX - game->cargo.prevX)
                               + (newCargoY - game->cargo.prevY) * (newCargoY - game->cargo.prevY)) / dt;
        game->cargo.prevX = newCargoX;
        game->cargo.prevY = newCargoY;
        game->cargo.x = newCargoX;
        game->cargo.y = newCargoY;
        game->chainMotion = game->cargo.speed;
    }

    update_star_collection(game);
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
}

static void handle_menu_confirm(Game *game) {
    if (game->menuIndex == 0) {
        reset_level(game, 0);
    } else if (game->menuIndex == 1) {
        game->state = STATE_HOW_TO_PLAY;
    } else if (game->menuIndex == 2) {
        game->state = STATE_LEVEL_SELECT;
    } else {
        game->running = 0;
    }
}

static void handle_pause_confirm(Game *game) {
    if (game->pauseIndex == 0) {
        game->state = STATE_PLAYING;
    } else if (game->pauseIndex == 1) {
        reset_level(game, game->selectedLevel);
    } else if (game->pauseIndex == 2) {
        game->state = STATE_LEVEL_SELECT;
    } else if (game->pauseIndex == 3) {
        game->state = STATE_MAIN_MENU;
    } else {
        game->running = 0;
    }
}

static void handle_win_confirm(Game *game) {
    if (game->winIndex == 0) {
        if (game->selectedLevel + 1 < LEVEL_COUNT) {
            reset_level(game, game->selectedLevel + 1);
        } else {
            game->state = STATE_LEVEL_SELECT;
        }
    } else if (game->winIndex == 1) {
        reset_level(game, game->selectedLevel);
    } else if (game->winIndex == 2) {
        game->state = STATE_LEVEL_SELECT;
    } else {
        game->state = STATE_MAIN_MENU;
    }
}

static void handle_lose_confirm(Game *game) {
    if (game->loseIndex == 0) {
        reset_level(game, game->selectedLevel);
    } else if (game->loseIndex == 1) {
        game->state = STATE_LEVEL_SELECT;
    } else {
        game->state = STATE_MAIN_MENU;
    }
}

static int getLogicalMousePosition(App *app, int windowX, int windowY, double *logicalX, double *logicalY) {
    int windowW = 0;
    int windowH = 0;
    int outputW = 0;
    int outputH = 0;
    int logicalW = 0;
    int logicalH = 0;
    SDL_Rect viewport;
    float apiX = 0.0f;
    float apiY = 0.0f;
    double outputX;
    double outputY;

    SDL_RenderWindowToLogical(app->renderer, windowX, windowY, &apiX, &apiY);
    *logicalX = (double)apiX;
    *logicalY = (double)apiY;

    SDL_GetWindowSize(app->window, &windowW, &windowH);
    SDL_GetRendererOutputSize(app->renderer, &outputW, &outputH);
    SDL_RenderGetLogicalSize(app->renderer, &logicalW, &logicalH);
    SDL_RenderGetViewport(app->renderer, &viewport);

    if (windowW <= 0 || windowH <= 0 || outputW <= 0 || outputH <= 0
        || logicalW <= 0 || logicalH <= 0 || viewport.w <= 0 || viewport.h <= 0) {
        return *logicalX >= 0.0 && *logicalY >= 0.0
            && *logicalX <= (double)LOGICAL_W && *logicalY <= (double)LOGICAL_H;
    }

    outputX = (double)windowX * (double)outputW / (double)windowW;
    outputY = (double)windowY * (double)outputH / (double)windowH;

    *logicalX = (outputX - (double)viewport.x) * (double)logicalW / (double)viewport.w;
    *logicalY = (outputY - (double)viewport.y) * (double)logicalH / (double)viewport.h;

    return outputX >= (double)viewport.x && outputX <= (double)(viewport.x + viewport.w)
        && outputY >= (double)viewport.y && outputY <= (double)(viewport.y + viewport.h)
        && *logicalX >= 0.0 && *logicalX <= (double)logicalW
        && *logicalY >= 0.0 && *logicalY <= (double)logicalH;
}

static void update_mouse_position(App *app, Game *game, int windowX, int windowY) {
    double logicalX = 0.0;
    double logicalY = 0.0;
    game->mouseValid = getLogicalMousePosition(app, windowX, windowY, &logicalX, &logicalY);
    game->mouseX = logicalX;
    game->mouseY = logicalY;
}

static int hovered_button_index(const Game *game, FRect (*rect_fn)(int), int count) {
    int i;
    for (i = 0; i < count; ++i) {
        if (mouse_over_rect(game, rect_fn(i))) {
            return i;
        }
    }
    return -1;
}

static void update_mouse_hover(Game *game) {
    int index;
    if (game->state == STATE_MAIN_MENU) {
        index = hovered_button_index(game, main_menu_button_rect, 4);
        if (index >= 0) game->menuIndex = index;
    } else if (game->state == STATE_LEVEL_SELECT) {
        index = hovered_button_index(game, level_select_button_rect, LEVEL_COUNT);
        if (index >= 0) game->levelIndex = index;
    } else if (game->state == STATE_PAUSED) {
        index = hovered_button_index(game, pause_button_rect, 5);
        if (index >= 0) game->pauseIndex = index;
    } else if (game->state == STATE_WIN) {
        index = hovered_button_index(game, win_button_rect, 4);
        if (index >= 0) game->winIndex = index;
    } else if (game->state == STATE_LOSE) {
        index = hovered_button_index(game, lose_button_rect, 3);
        if (index >= 0) game->loseIndex = index;
    }
}

static void handle_mouse_click(Game *game) {
    int index;
    if (game->state == STATE_MAIN_MENU) {
        index = hovered_button_index(game, main_menu_button_rect, 4);
        if (index >= 0) {
            game->menuIndex = index;
            handle_menu_confirm(game);
        }
    } else if (game->state == STATE_HOW_TO_PLAY) {
        if (mouse_over_rect(game, how_to_play_back_rect())) {
            game->state = STATE_MAIN_MENU;
        }
    } else if (game->state == STATE_LEVEL_SELECT) {
        if (mouse_over_rect(game, level_select_back_rect())) {
            game->state = STATE_MAIN_MENU;
            return;
        }
        index = hovered_button_index(game, level_select_button_rect, LEVEL_COUNT);
        if (index >= 0) {
            game->levelIndex = index;
            reset_level(game, index);
        }
    } else if (game->state == STATE_PAUSED) {
        index = hovered_button_index(game, pause_button_rect, 5);
        if (index >= 0) {
            game->pauseIndex = index;
            handle_pause_confirm(game);
        }
    } else if (game->state == STATE_WIN) {
        index = hovered_button_index(game, win_button_rect, 4);
        if (index >= 0) {
            game->winIndex = index;
            handle_win_confirm(game);
        }
    } else if (game->state == STATE_LOSE) {
        index = hovered_button_index(game, lose_button_rect, 3);
        if (index >= 0) {
            game->loseIndex = index;
            handle_lose_confirm(game);
        }
    }
}

static void handle_key_down(App *app, Game *game, SDL_Keycode key, SDL_Keymod mod) {
    if (key == SDLK_F11 || (key == SDLK_RETURN && (mod & KMOD_ALT))) {
        int mx;
        int my;
        toggle_fullscreen(app, game);
        SDL_GetMouseState(&mx, &my);
        update_mouse_position(app, game, mx, my);
        update_mouse_hover(game);
        return;
    }
    if (key == SDLK_F3) {
        cycle_graphics_quality(app);
        return;
    }

    if (game->state == STATE_MAIN_MENU) {
        if (key == SDLK_UP) move_menu_index(&game->menuIndex, 4, -1);
        else if (key == SDLK_DOWN) move_menu_index(&game->menuIndex, 4, 1);
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) handle_menu_confirm(game);
        else if (key == SDLK_ESCAPE) game->running = 0;
    } else if (game->state == STATE_HOW_TO_PLAY) {
        if (key == SDLK_ESCAPE || key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            game->state = STATE_MAIN_MENU;
        }
    } else if (game->state == STATE_LEVEL_SELECT) {
        if (key == SDLK_UP || key == SDLK_LEFT) move_menu_index(&game->levelIndex, LEVEL_COUNT, -1);
        else if (key == SDLK_DOWN || key == SDLK_RIGHT) move_menu_index(&game->levelIndex, LEVEL_COUNT, 1);
        else if (key >= SDLK_1 && key <= SDLK_6) reset_level(game, key - SDLK_1);
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) reset_level(game, game->levelIndex);
        else if (key == SDLK_ESCAPE) game->state = STATE_MAIN_MENU;
    } else if (game->state == STATE_PLAYING) {
        if (key == SDLK_ESCAPE) {
            game->state = STATE_PAUSED;
            game->pauseIndex = 0;
        } else if (key == SDLK_r) {
            reset_level(game, game->selectedLevel);
        }
    } else if (game->state == STATE_PAUSED) {
        if (key == SDLK_UP) move_menu_index(&game->pauseIndex, 5, -1);
        else if (key == SDLK_DOWN) move_menu_index(&game->pauseIndex, 5, 1);
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) handle_pause_confirm(game);
        else if (key == SDLK_ESCAPE) game->state = STATE_PLAYING;
    } else if (game->state == STATE_WIN) {
        if (key == SDLK_UP) move_menu_index(&game->winIndex, 4, -1);
        else if (key == SDLK_DOWN) move_menu_index(&game->winIndex, 4, 1);
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) handle_win_confirm(game);
        else if (key == SDLK_n) game->winIndex = 0, handle_win_confirm(game);
        else if (key == SDLK_r) reset_level(game, game->selectedLevel);
        else if (key == SDLK_l) game->state = STATE_LEVEL_SELECT;
        else if (key == SDLK_m) game->state = STATE_MAIN_MENU;
        else if (key == SDLK_ESCAPE) game->state = STATE_MAIN_MENU;
    } else if (game->state == STATE_LOSE) {
        if (key == SDLK_UP) move_menu_index(&game->loseIndex, 3, -1);
        else if (key == SDLK_DOWN) move_menu_index(&game->loseIndex, 3, 1);
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) handle_lose_confirm(game);
        else if (key == SDLK_r) reset_level(game, game->selectedLevel);
        else if (key == SDLK_l) game->state = STATE_LEVEL_SELECT;
        else if (key == SDLK_m) game->state = STATE_MAIN_MENU;
        else if (key == SDLK_ESCAPE) game->state = STATE_MAIN_MENU;
    }
}

static void handle_events(App *app, Game *game) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            game->running = 0;
        } else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
            handle_key_down(app, game, event.key.keysym.sym, (SDL_Keymod)event.key.keysym.mod);
        } else if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
                || event.window.event == SDL_WINDOWEVENT_RESIZED
                || event.window.event == SDL_WINDOWEVENT_RESTORED
                || event.window.event == SDL_WINDOWEVENT_EXPOSED) {
                int mx;
                int my;
                SDL_RenderSetLogicalSize(app->renderer, LOGICAL_W, LOGICAL_H);
                SDL_GetMouseState(&mx, &my);
                update_mouse_position(app, game, mx, my);
                update_mouse_hover(game);
            }
        } else if (event.type == SDL_MOUSEMOTION) {
            update_mouse_position(app, game, event.motion.x, event.motion.y);
            update_mouse_hover(game);
        } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            update_mouse_position(app, game, event.button.x, event.button.y);
            update_mouse_hover(game);
            handle_mouse_click(game);
        }
    }
}

static void drawBackground(App *app) {
    int x;
    int y;
    set_color(app->renderer, color_rgba(184, 219, 235, 255));
    SDL_RenderClear(app->renderer);

    if (draw_texture_stretched(app, app->assets.yardBackground, (FRect){0, 0, LOGICAL_W, LOGICAL_H})) {
        fill_rect(app, (FRect){0, HUD_H, LOGICAL_W, BOTTOM_BAR_Y - HUD_H}, color_rgba(255, 255, 255, 18));
        fill_rect(app, (FRect){0, BOTTOM_BAR_Y - 4.0, LOGICAL_W, 4.0}, color_rgba(38, 48, 51, 135));
        return;
    }

    fill_rect(app, (FRect){0, HUD_H, LOGICAL_W, BOTTOM_BAR_Y - HUD_H}, color_rgba(197, 224, 235, 255));
    fill_rect(app, (FRect){0, 510, LOGICAL_W, 150}, color_rgba(166, 173, 174, 255));
    fill_rect(app, (FRect){0, 595, LOGICAL_W, 65}, color_rgba(130, 139, 141, 255));

    set_color(app->renderer, color_rgba(120, 158, 175, 42));
    for (x = 0; x <= LOGICAL_W; x += 40) {
        SDL_RenderDrawLine(app->renderer, x, HUD_H, x, BOTTOM_BAR_Y);
    }
    for (y = HUD_H; y <= BOTTOM_BAR_Y; y += 40) {
        SDL_RenderDrawLine(app->renderer, 0, y, LOGICAL_W, y);
    }

    fill_rect(app, (FRect){62, 466, 138, 129}, color_rgba(139, 154, 160, 105));
    fill_rect(app, (FRect){220, 436, 92, 159}, color_rgba(142, 157, 164, 90));
    fill_rect(app, (FRect){1015, 448, 128, 147}, color_rgba(138, 153, 160, 100));
    fill_rect(app, (FRect){335, 548, 150, 34}, color_rgba(139, 114, 85, 92));
    fill_rect(app, (FRect){494, 548, 150, 34}, color_rgba(95, 124, 143, 82));
    fill_rect(app, (FRect){653, 548, 150, 34}, color_rgba(128, 139, 110, 82));
    draw_line(app, 0, 595, LOGICAL_W, 595, color_rgba(96, 105, 107, 170));
}

static void draw_hud(App *app, const Game *game) {
    char buffer[160];
    const Level *level = &game->levels[game->selectedLevel];
    double holdTime = level_hold_time(level);
    double stableFraction = game->stableTimer / holdTime;
    SDL_Color progressColor = game->deliveryValid ? color_rgba(89, 226, 116, 255) : color_rgba(172, 64, 57, 255);
    SDL_Color text = color_rgba(236, 245, 246, 255);
    SDL_Color muted = color_rgba(177, 207, 211, 255);
    int stars = count_collected_stars(level);

    draw_panel(app, (FRect){0, 0, LOGICAL_W, HUD_H}, color_rgba(25, 47, 58, 245), color_rgba(65, 96, 110, 255));
    fill_rect(app, (FRect){0, 64, LOGICAL_W, 28}, color_rgba(18, 36, 45, 235));
    draw_text(app, app->font, "Crane Operator", 22, 10, color_rgba(255, 214, 102, 255));
    draw_text(app, app->fontSmall, level->name, 230, 15, text);
    snprintf(buffer, sizeof(buffer), "Time %05.1f", game->timeLeft);
    draw_text(app, app->fontSmall, buffer, 480, 15, text);
    snprintf(buffer, sizeof(buffer), "Mode %s", pendulum_mode_name(level->pendulumMode));
    draw_text(app, app->fontSmall, buffer, 610, 15, text);
    snprintf(buffer, sizeof(buffer), "Cable %03.0f", game->cargo.length);
    draw_text(app, app->fontSmall, buffer, 750, 15, text);
    snprintf(buffer, sizeof(buffer), "Speed %04.0f", game->cargo.speed);
    draw_text(app, app->fontSmall, buffer, 875, 15, text);

    snprintf(buffer, sizeof(buffer), "Hold steady: %.1f / %.1f s", game->stableTimer, holdTime);
    draw_text(app, app->fontSmall, buffer, 1010, 10, game->deliveryValid ? color_rgba(144, 244, 157, 255) : text);
    draw_progress_bar(app, (FRect){1010, 35, 225, 16}, stableFraction, progressColor, color_rgba(92, 128, 123, 255));

    snprintf(buffer, sizeof(buffer), "Objective: %s", level->objective);
    draw_text(app, app->fontSmall, buffer, 24, 70, muted);
    snprintf(buffer, sizeof(buffer), "Stars %d/%d", stars, STAR_COUNT);
    draw_text(app, app->fontSmall, buffer, 610, 70, color_rgba(255, 220, 92, 255));
    if (level->pendulumMode != PENDULUM_SINGLE) {
        snprintf(buffer, sizeof(buffer), "Chain %.0f/%.0f", game->chainMotion, level->safeChainMotion);
        draw_text(app, app->fontSmall, buffer, 710, 70,
                  game->chainTooMuchMotion ? color_rgba(255, 205, 104, 255) : color_rgba(177, 207, 211, 255));
    }
    if (game->unstableZoneActive) {
        double limit = level->unstableZoneCount > 0 ? level->unstableZones[0].timeLimit : 4.0;
        snprintf(buffer, sizeof(buffer), "Keep moving %.1f/%.1f", game->unstableZoneTimer, limit);
        draw_text(app, app->fontSmall, buffer, 1000, 70, color_rgba(255, 224, 88, 255));
    } else if (game->starMessageTimer > 0.0) {
        draw_text(app, app->fontSmall, "Star collected!", 1010, 70, color_rgba(255, 220, 92, 255));
    } else if (game->cargoTooFast) {
        draw_text(app, app->fontSmall, "Too fast", 1030, 70, color_rgba(255, 205, 104, 255));
    } else if (game->chainTooMuchMotion) {
        draw_text(app, app->fontSmall, "Chain still moving", 982, 70, color_rgba(255, 205, 104, 255));
    } else if (game->cargoTooMuchSwing) {
        draw_text(app, app->fontSmall, "Too much swing", 1010, 70, color_rgba(255, 205, 104, 255));
    } else if (game->cargoTooHigh) {
        draw_text(app, app->fontSmall, "Lower cargo", 1025, 70, color_rgba(255, 205, 104, 255));
    } else if (game->cargoInWind) {
        draw_text(app, app->fontSmall, "Wind pushing", 1025, 70, color_rgba(132, 218, 255, 255));
    } else if (game->cargoInMagnet) {
        draw_text(app, app->fontSmall, "Magnet pull", 1025, 70, color_rgba(205, 156, 255, 255));
    } else if (level->targetAxis != AXIS_NONE) {
        draw_text(app, app->fontSmall, "Moving dock", 1025, 70, color_rgba(144, 239, 162, 255));
    } else if (level->pendulumMode == PENDULUM_DOUBLE) {
        draw_text(app, app->fontSmall, "Two links: ease in", 980, 70, color_rgba(144, 219, 239, 255));
    } else if (level->pendulumMode == PENDULUM_TRIPLE) {
        draw_text(app, app->fontSmall, "Three links: tiny moves", 958, 70, color_rgba(144, 219, 239, 255));
    }
#if DEBUG_PERFORMANCE
    snprintf(buffer, sizeof(buffer), "FPS %.0f  Gfx %s", app->smoothedFps, graphics_quality_name(app->graphicsQuality));
    draw_text(app, app->fontSmall, buffer, 1075, 70, color_rgba(220, 235, 238, 255));
#endif
}

static void draw_bottom_controls(App *app) {
    draw_panel(app, (FRect){0, BOTTOM_BAR_Y, LOGICAL_W, LOGICAL_H - BOTTOM_BAR_Y},
               color_rgba(31, 39, 43, 245), color_rgba(83, 94, 100, 255));
    draw_text(app, app->fontSmall,
              "A/D or Left/Right: motor    W/S or Up/Down: cable    Space: stabilizer    R restart    ESC pause    F11 fullscreen    F3 graphics",
              32, 680, color_rgba(232, 237, 232, 255));
}

static void drawDeliveryBay(App *app, FRect target, int moving, double time) {
    FRect outer = {target.x - 20.0, target.y - 14.0, target.w + 40.0, target.h + 28.0};
    FRect lipLeft = {outer.x, outer.y, 20.0, outer.h};
    FRect lipRight = {outer.x + outer.w - 20.0, outer.y, 20.0, outer.h};
    FRect lipBottom = {outer.x, outer.y + outer.h - 16.0, outer.w, 16.0};
    SDL_Texture *dockTexture = moving == 2 ? app->assets.movingDockHeavy : moving ? app->assets.movingDock : app->assets.deliveryDock;
    int usedTexture;

    fill_rect(app, (FRect){outer.x + 4.0, outer.y + 8.0, outer.w, outer.h}, color_rgba(0, 0, 0, 75));
    if (moving == 2 && app->graphicsQuality == GRAPHICS_FULL_ANIMATED && animated_sprite_ready(&app->assets.anim.movingDock)) {
        usedTexture = draw_animation_loop_stretched(app, &app->assets.anim.movingDock, time, outer);
    } else if (moving == 2 && app->graphicsQuality != GRAPHICS_SIMPLE_FAST && animated_sprite_ready(&app->assets.anim.movingDock)) {
        usedTexture = draw_animation_frame_stretched_ex(app, &app->assets.anim.movingDock, 0, outer, 0.0, SDL_FLIP_NONE);
    } else {
        usedTexture = draw_texture_stretched(app, dockTexture, outer);
    }
    if (!usedTexture) {
        fill_rect(app, outer, color_rgba(61, 70, 69, 255));
        fill_rect(app, lipLeft, color_rgba(53, 60, 60, 255));
        fill_rect(app, lipRight, color_rgba(53, 60, 60, 255));
        fill_rect(app, lipBottom, color_rgba(43, 48, 48, 255));
    }
    fill_rect(app, target, moving ? color_rgba(28, 126, 80, 105) : color_rgba(35, 147, 77, 115));
    fill_rect(app, (FRect){target.x + 7.0, target.y + 7.0, target.w - 14.0, target.h - 14.0},
              moving ? color_rgba(75, 229, 129, 105) : color_rgba(75, 214, 111, 112));
    fill_rect(app, (FRect){target.x + 14.0, target.y + 14.0, target.w - 28.0, target.h - 28.0},
              color_rgba(95, 247, 137, usedTexture ? 42 : 88));
    outline_rect(app, outer, color_rgba(48, 55, 55, 255));
    draw_line(app, target.x + 22.0, target.y + target.h * 0.52, target.x + target.w * 0.46, target.y + target.h - 20.0, color_rgba(16, 102, 46, 180));
    draw_line(app, target.x + target.w * 0.46, target.y + target.h - 20.0, target.x + target.w - 24.0, target.y + 17.0, color_rgba(16, 102, 46, 180));
    draw_text_center(app, app->fontSmall, moving ? "MOVING DOCK" : "DELIVERY BAY",
                     (int)(target.x + target.w * 0.5), (int)(target.y + 31.0), color_rgba(10, 62, 30, 255));
}

static void drawDeliveryProgressNearTarget(App *app, const Game *game, FRect target) {
    char buffer[64];
    const Level *level = &game->levels[game->selectedLevel];
    double holdTime = level_hold_time(level);
    double fraction = game->stableTimer / holdTime;
    FRect nearTarget = {target.x - 115.0, target.y - 95.0, target.w + 230.0, target.h + 145.0};
    FRect bar = {target.x, target.y - 44.0, target.w, 16.0};
    SDL_Color fill = game->deliveryValid ? color_rgba(86, 229, 111, 255) : color_rgba(199, 67, 58, 255);
    const char *status = "Hold steady";

    if (game->stableTimer <= 0.0 && !point_in_frect(game->cargo.x, game->cargo.y, nearTarget)) {
        return;
    }

    if (game->cargoTooFast) status = "Too fast";
    else if (game->chainTooMuchMotion) status = "Chain moving";
    else if (game->cargoTooMuchSwing) status = "Too much swing";
    else if (game->cargoTooHigh) status = "Lower cargo";
    else if (!game->cargoInDeliveryZone) status = "Enter bay";

    snprintf(buffer, sizeof(buffer), "%s: %.1f / %.1f s", status, game->stableTimer, holdTime);
    draw_text_center(app, app->fontSmall, buffer, (int)(target.x + target.w * 0.5), (int)(target.y - 66.0),
                     game->deliveryValid ? color_rgba(26, 92, 45, 255) : color_rgba(96, 46, 36, 255));
    draw_progress_bar(app, bar, fraction, fill, color_rgba(50, 72, 65, 255));
}

static void drawSteelBeam(App *app, FRect beam, const char *label) {
    int stripeX;
    int important = beam.h > 60.0;
    SDL_Texture *beamTexture = important ? app->assets.hazardBarrier : app->assets.steelBeam;
    int usedTexture;
    SDL_Color fill = important ? color_rgba(146, 67, 61, 255) : color_rgba(96, 106, 114, 255);
    fill_rect(app, (FRect){beam.x + 4.0, beam.y + 5.0, beam.w, beam.h}, color_rgba(0, 0, 0, 72));
    usedTexture = draw_texture_stretched(app, beamTexture, beam);
    if (!usedTexture) {
        fill_rect(app, beam, fill);
        fill_rect(app, (FRect){beam.x, beam.y, beam.w, 7.0}, important ? color_rgba(178, 89, 76, 255) : color_rgba(146, 155, 162, 255));
        fill_rect(app, (FRect){beam.x, beam.y + beam.h - 8.0, beam.w, 8.0}, color_rgba(50, 57, 62, 255));
    }
    outline_rect(app, beam, color_rgba(32, 38, 43, 255));
    for (stripeX = (int)beam.x; stripeX < (int)(beam.x + beam.w); stripeX += 54) {
        draw_thick_line(app, stripeX, beam.y + beam.h - 2.0, stripeX + 28.0, beam.y + beam.h - 18.0, 3, color_rgba(245, 181, 56, 235));
    }
    if (label != NULL && label[0] != '\0' && beam.w > 90.0) {
        draw_text_center(app, app->fontSmall, label, (int)(beam.x + beam.w * 0.5), (int)(beam.y + 13.0), color_rgba(31, 35, 38, 255));
    }
}

static void draw_obstacles(App *app, const Level *level) {
    int i;
    for (i = 0; i < level->obstacleCount; ++i) {
        const char *label = (level->obstacles[i].h <= 48.0) ? "LOW CLEARANCE" : "BARRIER";
        drawSteelBeam(app, level->obstacles[i], label);
    }
}

static void drawUnstableZones(App *app, const Level *level) {
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

static void drawStarCollectible(App *app, const StarCollectible *star, double time) {
    double pulse = star->collected ? 0.82 : 1.0 + 0.07 * sin(time * 4.2 + star->x * 0.013);
    double outer = star->radius * pulse;
    double inner = outer * 0.45;
    double px[5];
    double py[5];
    int order[] = {0, 2, 4, 1, 3, 0};
    int i;
    SDL_Color fill = star->collected ? color_rgba(113, 102, 61, 80) : color_rgba(255, 219, 74, 255);
    SDL_Color edge = star->collected ? color_rgba(90, 80, 48, 120) : color_rgba(209, 126, 28, 255);

    for (i = 0; i < 5; ++i) {
        double a = -M_PI * 0.5 + (double)i * (2.0 * M_PI / 5.0);
        px[i] = star->x + cos(a) * outer;
        py[i] = star->y + sin(a) * outer;
    }

    fill_circle(app, (int)star->x, (int)star->y, (int)(inner * 1.2), fill);
    for (i = 0; i < 5; ++i) {
        draw_thick_line(app, px[order[i]], py[order[i]], px[order[i + 1]], py[order[i + 1]],
                        star->collected ? 2 : 3, edge);
    }
    for (i = 0; i < 5; ++i) {
        draw_thick_line(app, star->x, star->y, px[i], py[i], star->collected ? 2 : 3, fill);
    }
    fill_circle(app, (int)(star->x - outer * 0.22), (int)(star->y - outer * 0.22),
                star->collected ? 2 : 4, color_rgba(255, 250, 205, star->collected ? 75 : 220));
    if (star->collected) {
        draw_circle_outline(app, (int)star->x, (int)star->y, (int)(outer + 3.0), color_rgba(255, 221, 86, 70));
    }
}

static void drawStars(App *app, const Level *level, double time) {
    int i;
    for (i = 0; i < STAR_COUNT; ++i) {
        drawStarCollectible(app, &level->stars[i], time);
    }
}

static void drawWindZone(App *app, WindZone zone, double time) {
    int dir = zone.direction >= 0 ? 1 : -1;
    FRect fanBase = dir >= 0
                  ? (FRect){zone.rect.x - 122.0, zone.rect.y + zone.rect.h * 0.5 - 47.5, 130.0, 95.0}
                  : (FRect){zone.rect.x + zone.rect.w - 8.0, zone.rect.y + zone.rect.h * 0.5 - 47.5, 130.0, 95.0};
    int usedLargeGust = 0;
    int particleCount = app->graphicsQuality == GRAPHICS_SIMPLE_FAST ? 6 : 10;
    int particle;

    if (app->graphicsQuality == GRAPHICS_FULL_ANIMATED && animated_sprite_ready(&app->assets.anim.windGust)) {
        int gust;
        double span = zone.rect.w + 260.0;
        fill_rect(app, zone.rect, color_rgba(61, 159, 231, 12));
        for (gust = 0; gust < 3; ++gust) {
            double travel = fmod(time * 135.0 + (double)gust * 128.0, span);
            double gx = dir >= 0 ? zone.rect.x - 230.0 + travel : zone.rect.x + zone.rect.w - travel;
            double gy = zone.rect.y + 34.0 + (double)(gust % 3) * ((zone.rect.h - 96.0) / 2.0);
            FRect gustRect = {gx, gy, 230.0, 85.0};
            usedLargeGust |= draw_animation_loop_fit_ex(app, &app->assets.anim.windGust, time, gustRect, 0.0,
                                                        dir >= 0 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
        }
    } else {
        fill_rect(app, zone.rect, color_rgba(61, 159, 231, app->graphicsQuality == GRAPHICS_SIMPLE_FAST ? 8 : 14));
        for (particle = 0; particle < particleCount; ++particle) {
            double lane = (double)(particle % 4) / 3.0;
            double phase = fmod(time * (105.0 + (double)(particle % 3) * 12.0) + (double)particle * 47.0,
                                zone.rect.w + 50.0);
            double x = dir >= 0 ? zone.rect.x - 25.0 + phase : zone.rect.x + zone.rect.w + 25.0 - phase;
            double y = zone.rect.y + 38.0 + lane * (zone.rect.h - 76.0);
            double len = 20.0 + (double)(particle % 3) * 8.0;
            double endX = x + dir * len;
            Uint8 alpha = (Uint8)(95 + (particle % 4) * 25);
            if (x < zone.rect.x + 8.0 || x > zone.rect.x + zone.rect.w - 8.0) {
                continue;
            }
            draw_thick_line(app, x, y, endX, y, 2, color_rgba(77, 173, 238, alpha));
            draw_line(app, endX, y, endX - dir * 8.0, y - 5.0, color_rgba(94, 196, 252, alpha));
            draw_line(app, endX, y, endX - dir * 8.0, y + 5.0, color_rgba(94, 196, 252, alpha));
        }
    }
    outline_rect(app, zone.rect, color_rgba(55, 130, 210, usedLargeGust ? 80 : 115));
    draw_text_center(app, app->fontSmall, "WIND", (int)(zone.rect.x + zone.rect.w * 0.5), (int)(zone.rect.y + 10.0), color_rgba(29, 89, 139, 255));

    if (!((app->graphicsQuality == GRAPHICS_FULL_ANIMATED
           ? draw_animation_loop_fit_ex(app, &app->assets.anim.turbineFan, time, fanBase, 0.0,
                                        dir >= 0 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL)
           : draw_animation_frame_fit_ex(app, &app->assets.anim.turbineFan, 0, fanBase, 0.0,
                                         dir >= 0 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL)))
        && !draw_texture_fit(app, app->assets.industrialFan, (FRect){fanBase.x + 40.0, fanBase.y + 10.0, 42.0, 76.0})) {
        fanBase = (FRect){dir >= 0 ? zone.rect.x - 58.0 : zone.rect.x + zone.rect.w + 16.0,
                          zone.rect.y + zone.rect.h * 0.5 - 38.0, 42.0, 76.0};
        fill_rect(app, fanBase, color_rgba(86, 98, 108, 255));
        outline_rect(app, fanBase, color_rgba(40, 50, 58, 255));
        fill_circle(app, (int)(fanBase.x + fanBase.w * 0.5), (int)(fanBase.y + fanBase.h * 0.5), 23, color_rgba(62, 73, 82, 255));
        draw_thick_line(app, fanBase.x + fanBase.w * 0.5, fanBase.y + fanBase.h * 0.5,
                        fanBase.x + fanBase.w * 0.5 + 20.0 * cos(time * 7.0),
                        fanBase.y + fanBase.h * 0.5 + 20.0 * sin(time * 7.0), 4, color_rgba(166, 202, 222, 255));
        draw_thick_line(app, fanBase.x + fanBase.w * 0.5, fanBase.y + fanBase.h * 0.5,
                        fanBase.x + fanBase.w * 0.5 + 20.0 * cos(time * 7.0 + 2.09),
                        fanBase.y + fanBase.h * 0.5 + 20.0 * sin(time * 7.0 + 2.09), 4, color_rgba(166, 202, 222, 255));
        draw_thick_line(app, fanBase.x + fanBase.w * 0.5, fanBase.y + fanBase.h * 0.5,
                        fanBase.x + fanBase.w * 0.5 + 20.0 * cos(time * 7.0 + 4.18),
                        fanBase.y + fanBase.h * 0.5 + 20.0 * sin(time * 7.0 + 4.18), 4, color_rgba(166, 202, 222, 255));
    }
}

static void drawMovingGate(App *app, const MovingGate *gate, FRect gateRect, double time) {
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

    fill_rect(app, path, color_rgba(222, 91, 73, 26));
    outline_rect(app, path, color_rgba(216, 102, 84, 115));
    draw_line(app, path.x + path.w * 0.5, path.y, path.x + path.w * 0.5, path.y + path.h, color_rgba(222, 91, 73, 105));

    fill_rect(app, (FRect){gateRect.x + 5.0, gateRect.y + 6.0, gateRect.w, gateRect.h}, color_rgba(0, 0, 0, 92));
    if (!draw_texture_stretched(app, app->assets.movingGate, gateRect)) {
        fill_rect(app, gateRect, color_rgba(181, 67, 55, 255));
        fill_rect(app, (FRect){gateRect.x + 6.0, gateRect.y + 6.0, gateRect.w - 12.0, gateRect.h - 12.0}, color_rgba(219, 90, 64, 255));
        fill_rect(app, (FRect){gateRect.x, gateRect.y, gateRect.w, 10.0}, color_rgba(98, 42, 43, 255));
        fill_rect(app, (FRect){gateRect.x, gateRect.y + gateRect.h - 10.0, gateRect.w, 10.0}, color_rgba(98, 42, 43, 255));
    }
    outline_rect(app, gateRect, color_rgba(42, 27, 29, 255));
    for (stripeY = (int)gateRect.y + 18; stripeY < (int)(gateRect.y + gateRect.h - 20.0); stripeY += 42) {
        draw_thick_line(app, gateRect.x + 5.0, stripeY + 18.0, gateRect.x + gateRect.w - 5.0, stripeY, 3, color_rgba(247, 194, 65, 230));
    }
    draw_text_center(app, app->fontSmall, gate->label != NULL ? gate->label : "GATE",
                     (int)(gateRect.x + gateRect.w * 0.5), (int)(gateRect.y + gateRect.h * 0.5 - 9.0), color_rgba(255, 226, 181, 255));
    fill_circle(app, (int)(gateRect.x + gateRect.w * 0.5), (int)(gateRect.y - 14.0), 8,
                marker > 0.55 ? color_rgba(83, 230, 116, 255) : color_rgba(225, 61, 54, 255));
    fill_circle(app, (int)(gateRect.x + gateRect.w * 0.5), (int)(gateRect.y + gateRect.h + 14.0), 8,
                marker < -0.55 ? color_rgba(83, 230, 116, 255) : color_rgba(225, 61, 54, 255));
}

static void drawMagnetHazard(App *app, const MagnetHazard *magnet, double time, double cargoX, double cargoY) {
    double pulse = magnet_pulse(magnet, time);
    double dx = cargoX - magnet->x;
    double dy = cargoY - magnet->y;
    double cargoDist = sqrt(dx * dx + dy * dy);
    int drawField = cargoDist <= magnet->radius * 1.15;
    FRect fieldBounds = {magnet->x - 130.0, magnet->y - 100.0, 260.0, 200.0};
    FRect towerBounds = {magnet->body.x + magnet->body.w * 0.5 - 47.5,
                         magnet->body.y + magnet->body.h - 150.0, 95.0, 150.0};
    int usedField = 0;

    if (drawField && app->graphicsQuality == GRAPHICS_FULL_ANIMATED) {
        usedField = draw_animation_loop_fit(app, &app->assets.anim.magneticField, time + pulse * 0.18, fieldBounds);
    }
    if (drawField && !usedField) {
        int ringA = 40 + (int)(pulse * 18.0);
        int ringB = 66 + (int)(pulse * 18.0);
        fill_circle(app, (int)magnet->x, (int)magnet->y, 24, color_rgba(82, 178, 246, (Uint8)(18 + 20 * pulse)));
        draw_circle_outline(app, (int)magnet->x, (int)magnet->y, ringA, color_rgba(96, 201, 255, (Uint8)(46 + 42 * pulse)));
        if (app->graphicsQuality != GRAPHICS_SIMPLE_FAST) {
            draw_circle_outline(app, (int)magnet->x, (int)magnet->y, ringB, color_rgba(126, 72, 228, (Uint8)(26 + 26 * pulse)));
        }
    } else {
        fill_circle(app, (int)magnet->x, (int)magnet->y, 12, color_rgba(82, 178, 246, 24));
    }
    if (!((app->graphicsQuality == GRAPHICS_FULL_ANIMATED
           ? draw_animation_loop_fit(app, &app->assets.anim.magnetTower, time, towerBounds)
           : draw_animation_frame_fit_ex(app, &app->assets.anim.magnetTower, 0, towerBounds, 0.0, SDL_FLIP_NONE)))
        && !draw_texture_fit(app, app->assets.magnetTower, (FRect){magnet->body.x - 10.0, magnet->body.y - 18.0, magnet->body.w + 20.0, magnet->body.h + 32.0})) {
        drawSteelBeam(app, magnet->body, "MAGNET");
        outline_rect(app, magnet->body, color_rgba(89, 42, 158, 230));
    }
}

static void drawLaserHazard(App *app, const LaserHazard *laser, double time) {
    int active = laser_is_active(laser, time);
    double phase = cycle01(time, laser->period, laser->phase);
    int warning = !active && phase > laser->activeFraction && phase < laser->activeFraction + 0.12;
    int beamAlpha = active ? 130 + (int)(42.0 * (0.5 + 0.5 * sin(time * 48.0))) : warning ? 58 : 0;
    int frame = 0;
    FRect capTop;
    FRect capBottom;
    FRect visual = {laser->frame.x - 18.0, laser->frame.y + 16.0, laser->frame.w + 36.0, laser->frame.h - 32.0};
    FRect preRotate = {visual.x + visual.w * 0.5 - visual.h * 0.5,
                       visual.y + visual.h * 0.5 - visual.w * 0.5,
                       visual.h, visual.w};
    if (app->graphicsQuality == GRAPHICS_FULL_ANIMATED) {
        frame = active
              ? 4 + animation_state_frame(&app->assets.anim.laserGate,
                                          laser->activeFraction > 0.0 ? phase / laser->activeFraction : 0.0) / 2
              : animation_state_frame(&app->assets.anim.laserGate,
                                      laser->activeFraction < 1.0 ? (phase - laser->activeFraction) / (1.0 - laser->activeFraction) : 0.0) / 3;
    }
    int usedAnimation = draw_animation_frame_stretched_ex(app, &app->assets.anim.laserGate, frame,
                                                          preRotate, 90.0, SDL_FLIP_NONE);
    laser_gate_cap_rects(laser, &capTop, &capBottom);
    if (!usedAnimation && !draw_texture_fit(app, app->assets.laserSecurityGate, laser->frame)) {
        drawSteelBeam(app, capTop, "");
        drawSteelBeam(app, capBottom, "");
    }
    if (beamAlpha > 0 || DEBUG_COLLIDERS) {
        fill_rect(app, laser->beam, active ? color_rgba(255, 36, 46, (Uint8)beamAlpha) : color_rgba(255, 196, 68, (Uint8)beamAlpha));
        if (active) {
            fill_rect(app, (FRect){laser->beam.x - 3.0, laser->beam.y, laser->beam.w + 6.0, laser->beam.h},
                      color_rgba(255, 78, 86, 46));
        }
        if (DEBUG_COLLIDERS) {
            outline_rect(app, laser->beam, active ? color_rgba(255, 170, 170, 220) : color_rgba(93, 215, 124, 150));
        }
    }
    fill_circle(app, (int)(laser->frame.x + laser->frame.w * 0.5), (int)(laser->frame.y - 10.0), 7,
                active ? color_rgba(235, 48, 48, 255) : warning ? color_rgba(247, 196, 67, 255) : color_rgba(76, 228, 104, 255));
}

static void drawBridgeHazard(App *app, const BridgeHazard *bridge, double time) {
    FRect full = bridge->baseRect;
    FRect current = current_bridge_rect(bridge, time);
    double span = bridge->maxWidth - bridge->minWidth;
    double fraction = span > 0.0 ? (current.w - bridge->minWidth) / span : 0.0;
    int frame = animation_state_frame(&app->assets.anim.extendableBridge, fraction);
    int usedAnimation;
    FRect visual;
    full.w = bridge->maxWidth;
    visual.w = bridge->maxWidth + 38.0;
    visual.h = 70.0;
    visual.x = bridge->extendRight ? bridge->baseRect.x - 18.0 : bridge->baseRect.x + bridge->maxWidth - visual.w + 18.0;
    visual.y = bridge->baseRect.y - 24.0;
    usedAnimation = draw_animation_frame_stretched_ex(app, &app->assets.anim.extendableBridge, frame, visual, 0.0,
                                                      bridge->extendRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
    if (!usedAnimation) {
        fill_rect(app, full, color_rgba(247, 196, 67, 32));
        outline_rect(app, full, color_rgba(247, 196, 67, 105));
    }
    if (!usedAnimation && !draw_texture_stretched(app, app->assets.extendableBridge, current)) {
        drawSteelBeam(app, current, "");
    }
    if (!usedAnimation || DEBUG_COLLIDERS) {
        outline_rect(app, current, color_rgba(36, 42, 45, usedAnimation ? 90 : 140));
    }
}

static void drawCrusherHazard(App *app, const CrusherHazard *crusher, double time) {
    FRect head = current_crusher_head_rect(crusher, time);
    double danger = (head.y - crusher->headRect.y) / crusher->travel;
    int frame = animation_state_frame(&app->assets.anim.crusher, danger);
    FRect visual = {crusher->guideRect.x + crusher->guideRect.w * 0.5 - 85.0,
                    crusher->guideRect.y + 10.0, 170.0, 230.0};
    int usedAnimation = draw_animation_frame_fit_ex(app, &app->assets.anim.crusher, frame, visual, 0.0, SDL_FLIP_NONE);
    if (!usedAnimation) {
        fill_rect(app, crusher->guideRect, color_rgba(90, 96, 102, 60));
        outline_rect(app, crusher->guideRect, color_rgba(80, 84, 90, 150));
    }
    if (!usedAnimation
        && !draw_texture_fit(app, app->assets.hydraulicCrusher, (FRect){crusher->guideRect.x - 8.0, crusher->guideRect.y - 8.0, crusher->guideRect.w + 16.0, crusher->guideRect.h + 22.0})) {
        drawSteelBeam(app, head, "CRUSH");
    }
    if (usedAnimation && DEBUG_COLLIDERS) {
        fill_rect(app, head, color_rgba(235, 70, 54, danger > 0.62 ? 36 : 18));
        outline_rect(app, head, color_rgba(115, 38, 32, danger > 0.62 ? 135 : 80));
    } else if (!usedAnimation) {
        fill_rect(app, head, color_rgba(183, 58, 52, 110));
        outline_rect(app, head, color_rgba(70, 30, 30, 255));
    }
    fill_circle(app, (int)(crusher->guideRect.x + crusher->guideRect.w - 14.0), (int)(crusher->guideRect.y + 16.0), 7,
                danger > 0.62 ? color_rgba(235, 48, 48, 255) : color_rgba(247, 196, 67, 255));
}

static void drawHammerHazard(App *app, const HammerHazard *hammer, double time) {
    double a = current_hammer_angle(hammer, time);
    double bx = hammer->pivotX + hammer->length * sin(a);
    double by = hammer->pivotY + hammer->length * cos(a);
    int frame = animation_phase_frame(&app->assets.anim.swingHammer, hammer->omega * time + hammer->phase + M_PI * 0.5);
    double visualW = 210.0;
    double visualH = 185.0;
    FRect visual = {hammer->pivotX - visualW * 0.5, hammer->pivotY - 32.0, visualW, visualH};

    if (app->graphicsQuality == GRAPHICS_FULL_ANIMATED
        && draw_animation_frame_fit_ex(app, &app->assets.anim.swingHammer, frame, visual, 0.0, SDL_FLIP_NONE)) {
        return;
    }

    if (draw_animation_frame_fit_ex(app, &app->assets.anim.swingHammer, 2, visual, 0.0, SDL_FLIP_NONE)) {
        draw_thick_line(app, hammer->pivotX, hammer->pivotY, bx, by, 5, color_rgba(49, 52, 55, 235));
        fill_circle(app, (int)bx, (int)by, (int)(hammer->bobRadius * 0.92), color_rgba(88, 76, 65, 232));
        fill_circle(app, (int)bx, (int)by, (int)(hammer->bobRadius * 0.55), color_rgba(42, 42, 41, 230));
        draw_thick_line(app, bx - cos(a) * hammer->bobRadius * 0.85, by + sin(a) * hammer->bobRadius * 0.85,
                        bx + cos(a) * hammer->bobRadius * 0.85, by - sin(a) * hammer->bobRadius * 0.85,
                        5, color_rgba(242, 184, 55, 210));
        return;
    }

    {
        draw_circle_outline(app, (int)hammer->pivotX, (int)hammer->pivotY, (int)hammer->length, color_rgba(185, 73, 64, 70));
        draw_texture_fit(app, app->assets.swingHammerRig, (FRect){hammer->pivotX - 72.0, hammer->pivotY - 42.0, 144.0, 92.0});
        fill_circle(app, (int)hammer->pivotX, (int)hammer->pivotY, 9, color_rgba(35, 39, 43, 255));
        draw_thick_line(app, hammer->pivotX, hammer->pivotY, bx, by, 5, color_rgba(58, 62, 66, 255));
        fill_circle(app, (int)bx, (int)by, (int)hammer->bobRadius, color_rgba(164, 65, 55, 245));
        fill_circle(app, (int)bx, (int)by, (int)(hammer->bobRadius * 0.58), color_rgba(92, 44, 43, 255));
    }
}

static void drawSweeperHazard(App *app, const SweeperHazard *sweeper, double time) {
    double a = current_sweeper_angle(sweeper, time);
    FRect bounds = {sweeper->pivotX - 80.0, sweeper->pivotY - 65.0, 160.0, 130.0};
    int frame = animation_phase_frame(&app->assets.anim.sweeperArm, a);
    int usedVisual = 0;
    if (app->graphicsQuality == GRAPHICS_FULL_ANIMATED) {
        usedVisual = draw_animation_frame_fit_ex(app, &app->assets.anim.sweeperArm, frame, bounds, 0.0, SDL_FLIP_NONE);
    } else {
        usedVisual = draw_texture_fit_ex(app, app->assets.rotatingSweeperArm, bounds, a * 180.0 / M_PI, SDL_FLIP_NONE);
        if (!usedVisual && app->graphicsQuality != GRAPHICS_SIMPLE_FAST) {
            usedVisual = draw_animation_frame_fit_ex(app, &app->assets.anim.sweeperArm, 0, bounds, a * 180.0 / M_PI, SDL_FLIP_NONE);
        }
    }
    if (!usedVisual) {
        draw_circle_outline(app, (int)sweeper->pivotX, (int)sweeper->pivotY, (int)(sweeper->length * 0.5), color_rgba(231, 87, 55, 55));
        draw_thick_line(app,
                        sweeper->pivotX - cos(a) * sweeper->length * 0.5,
                        sweeper->pivotY - sin(a) * sweeper->length * 0.5,
                        sweeper->pivotX + cos(a) * sweeper->length * 0.5,
                        sweeper->pivotY + sin(a) * sweeper->length * 0.5,
                        (int)sweeper->width, color_rgba(204, 78, 53, 230));
    }
    if (!usedVisual || DEBUG_COLLIDERS) {
        fill_circle(app, (int)sweeper->pivotX, (int)sweeper->pivotY, 12, color_rgba(42, 46, 50, 255));
    }
}

static void drawMovingDockTrack(App *app, const Level *level) {
    FRect path = level->targetBase;
    if (level->targetAxis == AXIS_NONE) {
        return;
    }
    if (level->targetAxis == AXIS_X) {
        path.x -= fabs(level->targetAmplitude);
        path.w += fabs(level->targetAmplitude) * 2.0;
    } else if (level->targetAxis == AXIS_Y) {
        path.y -= fabs(level->targetAmplitude);
        path.h += fabs(level->targetAmplitude) * 2.0;
    }
    fill_rect(app, path, color_rgba(53, 213, 118, 32));
    outline_rect(app, path, color_rgba(35, 146, 83, 130));
}

static void draw_crane_rail(App *app) {
    int x;
    fill_rect(app, (FRect){50, RAIL_Y, LOGICAL_W - 100, 14}, color_rgba(67, 77, 84, 255));
    fill_rect(app, (FRect){50, RAIL_Y + 14, LOGICAL_W - 100, 7}, color_rgba(37, 45, 51, 255));
    for (x = 70; x <= LOGICAL_W - 90; x += 56) {
        draw_line(app, x, RAIL_Y, x + 30, RAIL_Y + 21, color_rgba(98, 110, 118, 255));
    }
}

static void drawCargoCrate(App *app, const Cargo *cargo, CargoKind kind) {
    FRect crate = cargo_rect(cargo);
    SDL_Texture *texture = app->assets.cargoCrate;
    FRect sprite = {cargo->x - 34.0, cargo->y - 28.0, 68.0, 56.0};
    if (kind == CARGO_HEAVY) {
        texture = app->assets.heavyHazardContainer;
        sprite = (FRect){cargo->x - 42.0, cargo->y - 34.0, 84.0, 68.0};
    } else if (kind == CARGO_FRAGILE) {
        texture = app->assets.precisionFragileCargo;
        sprite = (FRect){cargo->x - 31.0, cargo->y - 29.0, 62.0, 58.0};
    }
    fill_rect(app, (FRect){crate.x + 8.0, crate.y + crate.h + 5.0, crate.w - 2.0, 9.0}, color_rgba(0, 0, 0, 54));
    if (draw_texture_fit(app, texture, sprite)) {
        outline_rect(app, crate, color_rgba(73, 48, 33, 185));
        return;
    }
    fill_rect(app, crate, color_rgba(147, 84, 42, 255));
    fill_rect(app, (FRect){crate.x + 5.0, crate.y + 5.0, crate.w - 10.0, crate.h - 10.0}, color_rgba(205, 125, 54, 255));
    fill_rect(app, (FRect){crate.x + 7.0, crate.y + 8.0, crate.w - 14.0, 7.0}, color_rgba(225, 148, 69, 255));
    outline_rect(app, crate, color_rgba(73, 48, 33, 255));
    draw_thick_line(app, crate.x + 6.0, crate.y + 6.0, crate.x + crate.w - 6.0, crate.y + crate.h - 6.0, 2, color_rgba(92, 58, 35, 255));
    draw_thick_line(app, crate.x + crate.w - 6.0, crate.y + 6.0, crate.x + 6.0, crate.y + crate.h - 6.0, 2, color_rgba(92, 58, 35, 255));
    draw_line(app, crate.x + crate.w * 0.5, crate.y + 4.0, crate.x + crate.w * 0.5, crate.y + crate.h - 4.0, color_rgba(123, 72, 37, 255));
    fill_rect(app, (FRect){crate.x + 2.0, crate.y + 2.0, 8.0, 8.0}, color_rgba(89, 69, 55, 255));
    fill_rect(app, (FRect){crate.x + crate.w - 10.0, crate.y + 2.0, 8.0, 8.0}, color_rgba(89, 69, 55, 255));
    fill_rect(app, (FRect){crate.x + 2.0, crate.y + crate.h - 10.0, 8.0, 8.0}, color_rgba(89, 69, 55, 255));
    fill_rect(app, (FRect){crate.x + crate.w - 10.0, crate.y + crate.h - 10.0, 8.0, 8.0}, color_rgba(89, 69, 55, 255));
}

static void drawTrolley(App *app, double x) {
    FRect body = {x - 49.0, RAIL_Y + 19.0, 98.0, 38.0};
    FRect sprite = {x - 64.0, RAIL_Y + 0.0, 128.0, 72.0};
    int wheelY = RAIL_Y + 17;
    fill_rect(app, (FRect){body.x + 8.0, body.y + body.h + 4.0, body.w - 16.0, 9.0}, color_rgba(0, 0, 0, 55));
    if (draw_texture_fit(app, app->assets.craneTrolley, sprite)) {
        fill_rect(app, (FRect){x - 8.0, body.y + body.h, 16.0, 13.0}, color_rgba(35, 43, 47, 205));
        return;
    }
    fill_rect(app, body, color_rgba(235, 164, 44, 255));
    fill_rect(app, (FRect){body.x + 8.0, body.y + 7.0, body.w - 16.0, 11.0}, color_rgba(255, 203, 82, 255));
    fill_rect(app, (FRect){body.x + 10.0, body.y + body.h - 10.0, body.w - 20.0, 6.0}, color_rgba(193, 102, 31, 255));
    outline_rect(app, body, color_rgba(79, 56, 35, 255));
    fill_circle(app, (int)(x - 34.0), wheelY, 9, color_rgba(29, 35, 39, 255));
    fill_circle(app, (int)(x - 13.0), wheelY, 8, color_rgba(29, 35, 39, 255));
    fill_circle(app, (int)(x + 13.0), wheelY, 8, color_rgba(29, 35, 39, 255));
    fill_circle(app, (int)(x + 34.0), wheelY, 9, color_rgba(29, 35, 39, 255));
    fill_circle(app, (int)(x - 34.0), wheelY, 4, color_rgba(136, 150, 157, 255));
    fill_circle(app, (int)(x - 13.0), wheelY, 3, color_rgba(136, 150, 157, 255));
    fill_circle(app, (int)(x + 13.0), wheelY, 3, color_rgba(136, 150, 157, 255));
    fill_circle(app, (int)(x + 34.0), wheelY, 4, color_rgba(136, 150, 157, 255));
    fill_rect(app, (FRect){x - 8.0, body.y + body.h, 16.0, 13.0}, color_rgba(62, 70, 74, 255));
}

static void draw_crane(App *app, const Game *game) {
    double x = game->trolley.x;
    const Level *level = &game->levels[game->selectedLevel];
    draw_crane_rail(app);
    drawTrolley(app, x);

    fill_circle(app, (int)x, PIVOT_Y, 8, color_rgba(35, 43, 48, 255));
    if (level->pendulumMode == PENDULUM_SINGLE) {
        draw_thick_line(app, x, PIVOT_Y, game->cargo.x, game->cargo.y - CARGO_H * 0.5, 3, color_rgba(34, 38, 41, 255));
    } else {
        int i;
        double ax = x;
        double ay = PIVOT_Y;
        for (i = 0; i < game->chain.massCount; ++i) {
            double bx = game->chain.masses[i].x;
            double by = game->chain.masses[i].y;
            draw_thick_line(app, ax, ay, bx, by, 3, color_rgba(34, 38, 41, 255));
            if (i < game->chain.massCount - 1) {
                fill_circle(app, (int)bx, (int)by, 8, color_rgba(39, 48, 54, 255));
                fill_circle(app, (int)bx, (int)by, 4, color_rgba(237, 179, 73, 255));
            }
            ax = bx;
            ay = by;
        }
    }
    if (!draw_texture_fit(app, app->assets.hookAssembly,
                          (FRect){game->cargo.x - 18.0, game->cargo.y - CARGO_H * 0.5 - 28.0, 36.0, 42.0})) {
        fill_circle(app, (int)game->cargo.x, (int)(game->cargo.y - CARGO_H * 0.5), 7, color_rgba(35, 43, 48, 255));
    }
    drawCargoCrate(app, &game->cargo, game->levels[game->selectedLevel].cargoKind);
}

static void draw_level_world(App *app, const Game *game) {
    const Level *level = &game->levels[game->selectedLevel];
    FRect target = current_target_rect(game);
    int i;
    drawBackground(app);
    drawMovingDockTrack(app, level);
    drawUnstableZones(app, level);
    drawDeliveryBay(app, target, level->targetAxis != AXIS_NONE ? 1 : 0, game->levelElapsed);
    for (i = 0; i < level->windZoneCount; ++i) {
        drawWindZone(app, level->windZones[i], game->levelElapsed);
    }
    draw_obstacles(app, level);
    drawStars(app, level, game->levelElapsed);
    for (i = 0; i < level->gateCount; ++i) {
        FRect gate = current_gate_rect(&level->gates[i], game->levelElapsed);
        drawMovingGate(app, &level->gates[i], gate, game->levelElapsed);
    }
    for (i = 0; i < level->magnetCount; ++i) {
        drawMagnetHazard(app, &level->magnets[i], game->levelElapsed, game->cargo.x, game->cargo.y);
    }
    for (i = 0; i < level->laserCount; ++i) {
        drawLaserHazard(app, &level->lasers[i], game->levelElapsed);
    }
    for (i = 0; i < level->bridgeCount; ++i) {
        drawBridgeHazard(app, &level->bridges[i], game->levelElapsed);
    }
    for (i = 0; i < level->crusherCount; ++i) {
        drawCrusherHazard(app, &level->crushers[i], game->levelElapsed);
    }
    for (i = 0; i < level->hammerCount; ++i) {
        drawHammerHazard(app, &level->hammers[i], game->levelElapsed);
    }
    for (i = 0; i < level->sweeperCount; ++i) {
        drawSweeperHazard(app, &level->sweepers[i], game->levelElapsed);
    }
    draw_crane(app, game);
    drawDeliveryProgressNearTarget(app, game, target);
    draw_hud(app, game);
    draw_bottom_controls(app);
}

static void drawButton(App *app, const Game *game, FRect rect, const char *label, int selected, int smallText) {
    int hovered = mouse_over_rect(game, rect);
    SDL_Color fill = selected ? color_rgba(239, 178, 61, 255) : color_rgba(39, 63, 74, 238);
    SDL_Color border = selected ? color_rgba(255, 229, 135, 255) : color_rgba(18, 31, 38, 255);
    SDL_Color text = selected ? color_rgba(31, 35, 38, 255) : color_rgba(239, 245, 244, 255);
    TTF_Font *font = smallText ? app->fontSmall : app->font;
    int textY = (int)(rect.y + (smallText ? 8.0 : 12.0));
    SDL_Texture *buttonTexture = selected ? app->assets.menuButtonSelected
                                : hovered ? app->assets.menuButtonHover
                                : app->assets.menuButtonNormal;

    if (hovered && !selected) {
        fill = color_rgba(63, 94, 108, 248);
        border = color_rgba(124, 199, 227, 255);
    }

    fill_rect(app, (FRect){rect.x + 4.0, rect.y + 5.0, rect.w, rect.h}, color_rgba(0, 0, 0, 76));
    if (!draw_texture_stretched(app, buttonTexture, rect)) {
        fill_rect(app, rect, fill);
        fill_rect(app, (FRect){rect.x + 4.0, rect.y + 4.0, rect.w - 8.0, 7.0},
                  selected ? color_rgba(255, 226, 125, 155) : color_rgba(126, 166, 180, 70));
    }
    outline_rect(app, rect, border);
    outline_rect(app, (FRect){rect.x + 2.0, rect.y + 2.0, rect.w - 4.0, rect.h - 4.0},
                 color_rgba(255, 255, 255, selected || hovered ? 72 : 24));
    draw_text_center(app, font, label, (int)(rect.x + rect.w * 0.5), textY, text);
}

static void draw_menu_scene(App *app) {
    Cargo menuCargo;
    drawBackground(app);
    draw_crane_rail(app);
    drawDeliveryBay(app, (FRect){520, 532, 240, 56}, 0, 0.0);
    drawTrolley(app, 650.0);
    draw_thick_line(app, 650, PIVOT_Y, 700, 450, 3, color_rgba(35, 38, 40, 255));
    menuCargo.x = 701.0;
    menuCargo.y = 472.0;
    drawCargoCrate(app, &menuCargo, CARGO_NORMAL);
}

static void draw_main_menu(App *app, const Game *game) {
    const char *options[] = {"Start Game", "How to Play", "Level Select", "Quit"};
    int i;
    draw_menu_scene(app);
    drawMenuPanel(app, (FRect){345, 105, 590, 80}, color_rgba(24, 48, 59, 235), color_rgba(94, 126, 138, 255));
    draw_text_center(app, app->fontLarge, "Crane Operator", LOGICAL_W / 2, 116, color_rgba(255, 213, 101, 255));
    draw_text_center(app, app->font, "Safe Cargo Delivery", LOGICAL_W / 2, 154, color_rgba(232, 241, 241, 255));
    for (i = 0; i < 4; ++i) {
        drawButton(app, game, main_menu_button_rect(i), options[i], game->menuIndex == i, 0);
    }
    draw_text_center(app, app->fontSmall, "Up/Down to choose    Enter to confirm    ESC to quit    F11 fullscreen", LOGICAL_W / 2, 612, color_rgba(31, 43, 47, 255));
}

static void draw_how_to_play(App *app, const Game *game) {
    SDL_Color text = color_rgba(232, 241, 241, 255);
    SDL_Color gold = color_rgba(255, 212, 96, 255);
    draw_menu_scene(app);
    drawMenuPanel(app, (FRect){245, 82, 790, 540}, color_rgba(25, 47, 58, 246), color_rgba(91, 123, 137, 255));
    draw_text_center(app, app->fontLarge, "How to Play", LOGICAL_W / 2, 108, gold);

    draw_text(app, app->fontSmall, "Mission", 305, 160, gold);
    draw_text(app, app->fontSmall, "Deliver the crate into the green bay and keep it stable for 3 seconds.", 330, 184, text);

    draw_text(app, app->fontSmall, "Controls", 305, 226, gold);
    draw_text(app, app->fontSmall, "A/D or Left/Right: move crane", 330, 250, text);
    draw_text(app, app->fontSmall, "W/S or Up/Down: cable length", 330, 272, text);
    draw_text(app, app->fontSmall, "Space: stabilizer brake    R: restart", 330, 294, text);
    draw_text(app, app->fontSmall, "ESC: pause/back    F11: fullscreen", 330, 316, text);
    draw_text(app, app->fontSmall, "Use mouse or keyboard in menus.", 330, 338, text);

    draw_text(app, app->fontSmall, "Physics", 305, 374, gold);
    draw_text(app, app->fontSmall, "The crate behaves like a pendulum. Sudden trolley acceleration", 330, 398, text);
    draw_text(app, app->fontSmall, "creates swing; double/triple levels also check chain motion.", 330, 420, text);

    draw_text(app, app->fontSmall, "Tips", 305, 462, gold);
    draw_text(app, app->fontSmall, "Stars are optional. Yellow unstable zones must be crossed quickly.", 330, 486, text);
    drawButton(app, game, how_to_play_back_rect(), "Back", mouse_over_rect(game, how_to_play_back_rect()), 0);
    draw_text_center(app, app->fontSmall, "Enter or ESC also returns", LOGICAL_W / 2, 596, color_rgba(178, 208, 212, 255));
}

static void draw_level_select(App *app, const Game *game) {
    int i;
    char buffer[48];
    draw_menu_scene(app);
    drawMenuPanel(app, (FRect){105, 80, 1070, 540}, color_rgba(25, 47, 58, 245), color_rgba(91, 123, 137, 255));
    draw_text_center(app, app->fontLarge, "Level Select", LOGICAL_W / 2, 105, color_rgba(255, 212, 96, 255));
    snprintf(buffer, sizeof(buffer), "All 6 unlocked   Best Stars %d/%d", campaign_best_star_total(game), CAMPAIGN_STAR_TOTAL);
    draw_text_center(app, app->fontSmall, buffer, LOGICAL_W / 2, 143, color_rgba(178, 208, 212, 255));
    for (i = 0; i < LEVEL_COUNT; ++i) {
        FRect tile = level_select_button_rect(i);
        SDL_Color text = (game->levelIndex == i) ? color_rgba(35, 39, 41, 255) : color_rgba(236, 244, 244, 255);
        drawButton(app, game, tile, "", game->levelIndex == i, 1);
        draw_text(app, app->fontSmall, game->levels[i].name, (int)tile.x + 20, (int)tile.y + 9, text);
        draw_text(app, app->fontSmall, game->levels[i].objective, (int)tile.x + 20, (int)tile.y + 34, text);
        snprintf(buffer, sizeof(buffer), "Best %d/%d", game->bestStars[i], STAR_COUNT);
        draw_text(app, app->fontSmall, buffer, (int)(tile.x + tile.w - 94.0), (int)tile.y + 9, text);
    }
    drawButton(app, game, level_select_back_rect(), "Back", mouse_over_rect(game, level_select_back_rect()), 0);
    draw_text_center(app, app->fontSmall, "Arrows or 1-6 to choose    Enter to start    ESC for menu", LOGICAL_W / 2, 606, color_rgba(178, 208, 212, 255));
}

static void draw_overlay(App *app) {
    fill_rect(app, (FRect){0, 0, LOGICAL_W, LOGICAL_H}, color_rgba(0, 0, 0, 115));
}

static void draw_pause(App *app, const Game *game) {
    const char *options[] = {"Resume", "Restart Level", "Level Select", "Main Menu", "Quit"};
    int i;
    draw_level_world(app, game);
    draw_overlay(app);
    drawMenuPanel(app, (FRect){410, 184, 460, 390}, color_rgba(25, 47, 58, 250), color_rgba(91, 123, 137, 255));
    draw_text_center(app, app->fontLarge, "Paused", LOGICAL_W / 2, 218, color_rgba(255, 212, 96, 255));
    for (i = 0; i < 5; ++i) {
        drawButton(app, game, pause_button_rect(i), options[i], game->pauseIndex == i, 0);
    }
    draw_text_center(app, app->fontSmall, "ESC resumes", LOGICAL_W / 2, 535, color_rgba(178, 208, 212, 255));
}

static const char *loss_reason_text(LossReason reason) {
    if (reason == LOSS_TIMEOUT) return "Time ran out.";
    if (reason == LOSS_COLLISION) return "Cargo hit a beam or ceiling.";
    if (reason == LOSS_SWING) return "Swing angle became unsafe.";
    if (reason == LOSS_UNSTABLE) return "Stayed too long in an unstable zone.";
    return "Delivery failed.";
}

static void draw_win(App *app, const Game *game) {
    const char *options[] = {
        game->selectedLevel + 1 < LEVEL_COUNT ? "Next Level" : "Campaign Complete",
        "Replay Level",
        "Level Select",
        "Main Menu"
    };
    int i;
    char buffer[96];
    int totalStars = campaign_best_star_total(game);
    draw_level_world(app, game);
    draw_overlay(app);
    drawMenuPanel(app, (FRect){375, 150, 530, 435}, color_rgba(22, 67, 46, 250), color_rgba(111, 212, 132, 255));
    draw_text_center(app, app->fontLarge, "Delivery Successful!", LOGICAL_W / 2, 186, color_rgba(198, 255, 196, 255));
    snprintf(buffer, sizeof(buffer), "Stars collected: %d / %d", game->lastStarsCollected, STAR_COUNT);
    draw_text_center(app, app->font, buffer, LOGICAL_W / 2, 232, color_rgba(255, 229, 122, 255));
    snprintf(buffer, sizeof(buffer), "Rank: %s", operator_rank_for_stars(game->lastStarsCollected));
    draw_text_center(app, app->fontSmall, buffer, LOGICAL_W / 2, 264, color_rgba(236, 247, 236, 255));
    snprintf(buffer, sizeof(buffer), "Completion time: %.1f s", game->lastCompletionTime);
    draw_text_center(app, app->fontSmall, buffer, LOGICAL_W / 2, 286, color_rgba(207, 242, 211, 255));
    if (game->selectedLevel + 1 >= LEVEL_COUNT) {
        snprintf(buffer, sizeof(buffer), "Total Stars: %d / %d    Campaign Rank: %s",
                 totalStars, CAMPAIGN_STAR_TOTAL, campaign_rank_for_stars(totalStars));
        draw_text_center(app, app->fontSmall, buffer, LOGICAL_W / 2, 310, color_rgba(255, 229, 122, 255));
    }
    for (i = 0; i < 4; ++i) {
        drawButton(app, game, win_button_rect(i), options[i], game->winIndex == i, 0);
    }
    draw_text_center(app, app->fontSmall, "Enter/N next    R replay    L level select    M main menu", LOGICAL_W / 2, 552, color_rgba(207, 242, 211, 255));
}

static void draw_lose(App *app, const Game *game) {
    const char *options[] = {"Retry Level", "Level Select", "Main Menu"};
    int i;
    draw_level_world(app, game);
    draw_overlay(app);
    drawMenuPanel(app, (FRect){390, 195, 500, 330}, color_rgba(77, 30, 31, 250), color_rgba(201, 94, 94, 255));
    draw_text_center(app, app->fontLarge, "Level Failed", LOGICAL_W / 2, 240, color_rgba(255, 198, 190, 255));
    draw_text_center(app, app->font, loss_reason_text(game->lossReason), LOGICAL_W / 2, 290, color_rgba(245, 232, 228, 255));
    for (i = 0; i < 3; ++i) {
        drawButton(app, game, lose_button_rect(i), options[i], game->loseIndex == i, 0);
    }
    draw_text_center(app, app->fontSmall, "R retry    L level select    M main menu", LOGICAL_W / 2, 492, color_rgba(255, 219, 213, 255));
}

static void render(App *app, const Game *game) {
    if (game->state == STATE_MAIN_MENU) {
        draw_main_menu(app, game);
    } else if (game->state == STATE_HOW_TO_PLAY) {
        draw_how_to_play(app, game);
    } else if (game->state == STATE_LEVEL_SELECT) {
        draw_level_select(app, game);
    } else if (game->state == STATE_PLAYING) {
        draw_level_world(app, game);
    } else if (game->state == STATE_PAUSED) {
        draw_pause(app, game);
    } else if (game->state == STATE_WIN) {
        draw_win(app, game);
    } else if (game->state == STATE_LOSE) {
        draw_lose(app, game);
    }
    SDL_RenderPresent(app->renderer);
}

static int initialize_app(App *app) {
    memset(app, 0, sizeof(*app));
    app->graphicsQuality = GRAPHICS_SIMPLE_FAST;
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }
    if (TTF_Init() != 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_SetHint("SDL_RENDER_LOGICAL_SIZE_MODE", "letterbox");
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    app->window = SDL_CreateWindow("Crane Operator: Safe Cargo Delivery",
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   LOGICAL_W, LOGICAL_H,
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (app->window == NULL) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    app->windowedX = SDL_WINDOWPOS_CENTERED;
    app->windowedY = SDL_WINDOWPOS_CENTERED;
    app->windowedW = LOGICAL_W;
    app->windowedH = LOGICAL_H;
    SDL_SetWindowMinimumSize(app->window, 960, 540);
    app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (app->renderer == NULL) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(app->window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(app->renderer, LOGICAL_W, LOGICAL_H);
    load_assets(app);

    app->fontSmall = load_font_size(14);
    app->font = load_font_size(20);
    app->fontLarge = load_font_size(34);
    if (app->fontSmall == NULL || app->font == NULL || app->fontLarge == NULL) {
        printf("Could not load DejaVuSansMono.ttf or a Windows monospace fallback.\n");
        return 0;
    }
    return 1;
}

static void cleanup_app(App *app) {
    destroy_assets(app);
    if (app->fontLarge) TTF_CloseFont(app->fontLarge);
    if (app->font) TTF_CloseFont(app->font);
    if (app->fontSmall) TTF_CloseFont(app->fontSmall);
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);
    TTF_Quit();
    SDL_Quit();
}

int main(int argc, char **argv) {
    App app;
    Game game;
    Uint64 previousCounter;
    double perfFrequency;
    double accumulator = 0.0;

    (void)argc;
    (void)argv;

    if (!initialize_app(&app)) {
        cleanup_app(&app);
        return 1;
    }

    initialize_game(&game);
    perfFrequency = (double)SDL_GetPerformanceFrequency();
    previousCounter = SDL_GetPerformanceCounter();

    while (game.running) {
        Uint64 currentCounter = SDL_GetPerformanceCounter();
        double frameTime = (double)(currentCounter - previousCounter) / perfFrequency;
        double stepTime;
        previousCounter = currentCounter;
        if (frameTime > 0.25) {
            frameTime = 0.25;
        }
        stepTime = frameTime > 0.033 ? 0.033 : frameTime;
#if DEBUG_PERFORMANCE
        if (frameTime > 0.0001) {
            double fps = 1.0 / frameTime;
            app.smoothedFps = app.smoothedFps <= 0.0 ? fps : app.smoothedFps * 0.90 + fps * 0.10;
        }
#endif

        if (app.graphicsQuality == GRAPHICS_FULL_ANIMATED) {
            update_animation_assets(&app.assets, stepTime);
        }
        handle_events(&app, &game);
        if (game.state == STATE_PLAYING) {
            update_gameplay_input(&game);
            accumulator += stepTime;
            while (accumulator >= FIXED_DT && game.state == STATE_PLAYING) {
                update_physics(&game, FIXED_DT);
                accumulator -= FIXED_DT;
            }
        } else {
            accumulator = 0.0;
        }

        render(&app, &game);
        SDL_Delay(1);
    }

    cleanup_app(&app);
    return 0;
}
