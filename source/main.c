/*=============================================================================
 * PAPER PHYSICS: UN-STRESSED
 * A ragdoll physics sandbox for Nintendo DS
 *
 * Main entry point — initializes hardware, runs game loop, handles input.
 *===========================================================================*/

#include <nds.h>
#include <stdio.h>
#include <string.h>

#include "physics.h"
#include "ragdoll.h"
#include "renderer.h"
#include "tools.h"
#include "game.h"
#include "sound.h"

/*=============================================================================
 * GLOBALS
 *===========================================================================*/
static PhysicsWorld g_world;
static Ragdoll      g_ragdoll;
static ToolState    g_tools;
static GameState    g_game;

/*=============================================================================
 * MICROPHONE HANDLING
 * Read mic amplitude and convert to wind force.
 *===========================================================================*/
#define MIC_BUFFER_SIZE 64
static u8 g_mic_buffer[MIC_BUFFER_SIZE];

static int get_mic_volume(void) {
    /* Check for emulator / no-mic: if all samples are identical, return 0 */
    bool all_same = true;
    u8 first = g_mic_buffer[0];
    for (int i = 1; i < MIC_BUFFER_SIZE; i++) {
        if (g_mic_buffer[i] != first) {
            all_same = false;
            break;
        }
    }
    if (all_same) return 0; /* No real mic input */

    /* Calculate peak amplitude relative to center (128 = silence for 8-bit unsigned) */
    int peak = 0;
    for (int i = 0; i < MIC_BUFFER_SIZE; i++) {
        int sample = (int)g_mic_buffer[i] - 128;
        if (sample < 0) sample = -sample;
        if (sample > peak) peak = sample;
    }
    return peak;
}

/*=============================================================================
 * CLEAR ALL USER OBJECTS (keep ragdoll)
 *===========================================================================*/
static void clear_user_objects(void) {
    /* Remove particles that don't belong to the ragdoll */
    for (int i = 0; i < g_world.particle_count; i++) {
        if (!ragdoll_owns_particle(&g_ragdoll, i)) {
            physics_remove_particle(&g_world, i);
        }
    }

    /* Remove non-ragdoll constraints */
    for (int i = 0; i < g_world.constraint_count; i++) {
        Constraint *c = &g_world.constraints[i];
        if (!c->active) continue;

        bool is_ragdoll = false;
        for (int j = 0; j < RAGDOLL_BONES; j++) {
            if (g_ragdoll.bone_indices[j] == i) {
                is_ragdoll = true;
                break;
            }
        }
        if (!is_ragdoll) {
            physics_remove_constraint(&g_world, i);
        }
    }
}

/*=============================================================================
 * SPLASH SCREEN
 *===========================================================================*/
static void show_splash(void) {
    /* Draw a simple splash on the bottom screen */
    renderer_clear();
    renderer_draw_notebook();

    /* Title text */
    int cx = SCREEN_WIDTH / 2;
    renderer_draw_string(cx - 54, 50, "PAPER PHYSICS", COLOR_PENCIL);
    renderer_draw_string(cx - 42, 65, "UN-STRESSED", COLOR_PENCIL);

    /* Draw a little doodle ragdoll as logo */
    renderer_draw_circle(cx, 100, 8, COLOR_PENCIL);

    /* Face */
    renderer_put_pixel(cx - 3, 98, COLOR_PENCIL);
    renderer_put_pixel(cx + 3, 98, COLOR_PENCIL);
    /* Smile */
    renderer_put_pixel(cx - 3, 103, COLOR_PENCIL);
    renderer_put_pixel(cx - 2, 104, COLOR_PENCIL);
    renderer_put_pixel(cx - 1, 104, COLOR_PENCIL);
    renderer_put_pixel(cx,     104, COLOR_PENCIL);
    renderer_put_pixel(cx + 1, 104, COLOR_PENCIL);
    renderer_put_pixel(cx + 2, 104, COLOR_PENCIL);
    renderer_put_pixel(cx + 3, 103, COLOR_PENCIL);

    /* Body */
    renderer_draw_line(cx, 108, cx, 130, COLOR_PENCIL);
    /* Arms */
    renderer_draw_line(cx, 115, cx - 15, 122, COLOR_PENCIL);
    renderer_draw_line(cx, 115, cx + 15, 122, COLOR_PENCIL);
    /* Legs */
    renderer_draw_line(cx, 130, cx - 12, 148, COLOR_PENCIL);
    renderer_draw_line(cx, 130, cx + 12, 148, COLOR_PENCIL);

    /* Instruction */
    renderer_draw_string(cx - 54, 165, "TOUCH TO START!", COLOR_MARGIN);

    /* Version */
    renderer_draw_string(4, 182, "V1.0", COLOR_PENCIL_LIGHT);

    /* Wait for touch */
    iprintf("\x1b[2J");
    iprintf("\x1b[5;4H~~ PAPER PHYSICS ~~");
    iprintf("\x1b[6;6HUn-Stressed v1.0");
    iprintf("\x1b[10;2HA ragdoll physics sandbox");
    iprintf("\x1b[11;2Hfor Nintendo DS");
    iprintf("\x1b[14;3HGrab, fling, draw, and");
    iprintf("\x1b[15;3Hexperiment with physics!");
    iprintf("\x1b[18;4HTouch the screen to");
    iprintf("\x1b[19;4Hstart playing...");
    iprintf("\x1b[22;2HBlow into mic for wind!");

    while (pmMainLoop()) {
        swiWaitForVBlank();
        scanKeys();
        if (keysDown() & KEY_TOUCH) break;
        if (keysDown() & KEY_START) break;
        if (keysDown() & KEY_A) break;
    }
}

/*=============================================================================
 * MAIN
 *===========================================================================*/
int main(void) {
    /*--- Hardware Initialization ---*/

    /* Put main screen (touch) on bottom */
    lcdMainOnBottom();

    /* Initialize bottom screen: framebuffer mode for pixel drawing */
    renderer_init();

    /* Initialize top screen: text console for HUD */
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

    /* Initialize microphone buffer to silence (0x80 = center for unsigned 8-bit) */
    memset(g_mic_buffer, 0x80, MIC_BUFFER_SIZE);

    /* Initialize sound */
    soundEnable();
    /* Mic recording disabled — emulators produce noise that causes physics issues */
    /* soundMicRecord(g_mic_buffer, MIC_BUFFER_SIZE, MicFormat_8Bit, 8000, NULL); */
    sound_init();

    /*--- Show Splash Screen ---*/
    show_splash();

    /*--- Initialize Game Systems ---*/
    physics_init(&g_world);
    ragdoll_create(&g_ragdoll, &g_world, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20);
    tools_init(&g_tools);
    game_init(&g_game);
    sound_music_start();

    /*--- Track previous touch state for edge detection ---*/
    bool prev_touching = false;

    /*=================================================================
     * MAIN GAME LOOP (targeting 60 FPS via VBlank sync)
     *===============================================================*/
    while (pmMainLoop()) {
        /* Wait for VBlank — locks us to 60 FPS */
        swiWaitForVBlank();

        /*--- Read Input ---*/
        scanKeys();
        int keys_down = keysDown();    /* Just pressed this frame */
        int keys_held = keysHeld();    /* Currently held */

        touchPosition touch;
        touchRead(&touch);
        bool touching = (keys_held & KEY_TOUCH) && (touch.px > 0 || touch.py > 0);
        bool just_pressed = touching && !prev_touching;
        bool just_released = !touching && prev_touching;

        /*--- Button Input ---*/

        /* Tool selection */
        if (keys_down & KEY_A) { tools_set(&g_tools, TOOL_PENCIL); sound_play(SFX_MENU_SELECT); }
        if (keys_down & KEY_B) { tools_set(&g_tools, TOOL_PIN); sound_play(SFX_MENU_SELECT); }
        if (keys_down & KEY_X) { tools_set(&g_tools, TOOL_ELASTIC); sound_play(SFX_MENU_SELECT); }
        if (keys_down & KEY_Y) { tools_set(&g_tools, TOOL_ERASER); sound_play(SFX_MENU_SELECT); }

        /* Return to grab when button released */
        if ((keysUp() & (KEY_A | KEY_B | KEY_X | KEY_Y))) {
            tools_set(&g_tools, TOOL_GRAB);
        }

        /* Gravity flip */
        if (keys_down & KEY_L) {
            physics_flip_gravity(&g_world);
            g_game.flips_done++;
            sound_play(SFX_GRAVITY_FLIP);
        }
        if (keys_down & KEY_R) {
            physics_flip_gravity(&g_world);
            g_game.flips_done++;
            sound_play(SFX_GRAVITY_FLIP);
        }

        /* Mode switching */
        if (keys_down & KEY_UP) {
            int m = (g_game.mode + 1) % MODE_COUNT;
            game_set_mode(&g_game, (GameMode)m);
        }
        if (keys_down & KEY_DOWN) {
            int m = (g_game.mode + MODE_COUNT - 1) % MODE_COUNT;
            game_set_mode(&g_game, (GameMode)m);
        }

        /* Reset ragdoll */
        if (keys_down & KEY_START) {
            ragdoll_reset(&g_ragdoll, &g_world,
                         SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20);
            sound_play(SFX_PAPER_RUSTLE);
        }

        /* Clear all user objects */
        if (keys_down & KEY_SELECT) {
            clear_user_objects();
            sound_play(SFX_ERASER);
        }

        /*--- Process Tool Input ---*/
        tools_update(&g_tools, &g_world, &g_ragdoll,
                    touch.px, touch.py, touching, just_pressed, just_released);

        /*--- Microphone Wind (disabled — emulators produce fake mic noise) ---*/
        /* To re-enable on real hardware, uncomment the block below */
        /*
        {
            static int wind_sfx_cooldown = 0;
            if (wind_sfx_cooldown > 0) wind_sfx_cooldown--;
            int mic_vol = get_mic_volume();
            if (mic_vol > 50) {
                int32_t wind_force = INT_TO_FP(mic_vol - 50) >> 5;
                if (g_world.gravity_flipped)
                    physics_apply_wind(&g_world, 0, wind_force);
                else
                    physics_apply_wind(&g_world, 0, -wind_force);
                if (wind_sfx_cooldown == 0) {
                    sound_play(SFX_WIND);
                    wind_sfx_cooldown = 30;
                }
            }
        }
        */

        /*--- Physics Step ---*/
        physics_step(&g_world);

        /*--- Update Game State ---*/
        ragdoll_update(&g_ragdoll, &g_world);
        game_update(&g_game, &g_world, &g_ragdoll);

        /*--- Update Sound/Music ---*/
        sound_update();

        /*--- Render Bottom Screen ---*/
        renderer_clear();
        renderer_draw_notebook();
        renderer_draw_world(&g_world);
        renderer_draw_ragdoll(&g_ragdoll, &g_world);
        game_draw_mission_elements(&g_game);
        renderer_draw_toolbar(g_tools.current_tool);

        /* Draw elastic band preview when selecting second point */
        if (g_tools.elastic_selecting && g_tools.elastic_start_particle >= 0) {
            Particle *start = &g_world.particles[g_tools.elastic_start_particle];
            int sx = FP_TO_INT(start->x);
            int sy = FP_TO_INT(start->y);
            renderer_draw_circle(sx, sy, 5, COLOR_ELASTIC);
            renderer_draw_string(sx - 12, sy - 12, "LINK?", COLOR_ELASTIC);
        }

        /* Draw grab indicator */
        if (g_tools.is_grabbing && g_tools.grabbed_particle >= 0) {
            Particle *gp = &g_world.particles[g_tools.grabbed_particle];
            int gx = FP_TO_INT(gp->x);
            int gy = FP_TO_INT(gp->y);
            renderer_draw_circle(gx, gy, 8, COLOR_TOOL_ACTIVE);
        }

        /*--- Render Top Screen HUD ---*/
        game_print_hud(&g_game, &g_ragdoll, &g_tools, &g_world);

        /*--- Save touch state ---*/
        prev_touching = touching;
    }

    /* Cleanup */
    /* soundMicOff(); */

    return 0;
}
