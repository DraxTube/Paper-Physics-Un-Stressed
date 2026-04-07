#include "game.h"
#include "renderer.h"
#include "sound.h"
#include <stdio.h>
#include <string.h>

/*=============================================================================
 * MODE NAMES
 *===========================================================================*/
static const char *mode_names[] = {
    "SANDBOX",
    "MISSION: AIRBORNE",
    "MISSION: TARGET",
    "MISSION: BRIDGE"
};

const char* game_get_mode_name(GameMode mode) {
    if (mode < MODE_COUNT) return mode_names[mode];
    return "???";
}

/*=============================================================================
 * INIT
 *===========================================================================*/
void game_init(GameState *gs) {
    memset(gs, 0, sizeof(GameState));
    gs->mode = MODE_SANDBOX;
    gs->mission_status = MISSION_INACTIVE;
    gs->target_x = 230;
    gs->target_y = 25;
    gs->target_radius = 15;
}

/*=============================================================================
 * SET MODE
 *===========================================================================*/
void game_set_mode(GameState *gs, GameMode mode) {
    gs->mode = mode;
    gs->mission_status = MISSION_INACTIVE;
    gs->mission_timer = 0;

    switch (mode) {
        case MODE_MISSION_AIRBORNE:
            gs->mission_goal = 60 * 10; /* 10 seconds at 60fps */
            gs->mission_status = MISSION_ACTIVE;
            sound_play(SFX_MISSION_START);
            break;

        case MODE_MISSION_TARGET:
            gs->mission_goal = 0; /* Just need to hit the target */
            gs->mission_status = MISSION_ACTIVE;
            gs->target_x = 230;
            gs->target_y = 25;
            gs->target_radius = 15;
            sound_play(SFX_MISSION_START);
            break;

        case MODE_MISSION_BRIDGE:
            gs->mission_goal = 60 * 5; /* 5 seconds supported */
            gs->mission_status = MISSION_ACTIVE;
            sound_play(SFX_MISSION_START);
            break;

        default:
            gs->mission_status = MISSION_INACTIVE;
            break;
    }
}

/*=============================================================================
 * UPDATE
 *===========================================================================*/
void game_update(GameState *gs, PhysicsWorld *world, Ragdoll *doll) {
    gs->frame_count++;

    /* FPS counter */
    gs->fps_counter++;
    gs->fps_timer++;
    if (gs->fps_timer >= 60) {
        gs->fps = gs->fps_counter;
        gs->fps_counter = 0;
        gs->fps_timer = 0;
    }

    /* Track total stress */
    gs->total_stress += doll->stress_level >> FP_SHIFT;

    /* Mission logic */
    if (gs->mission_status != MISSION_ACTIVE) return;

    switch (gs->mode) {
        case MODE_MISSION_AIRBORNE:
            /* Count frames where ragdoll is airborne */
            if (doll->time_airborne > 0) {
                gs->mission_timer++;
                if (gs->mission_timer >= gs->mission_goal) {
                    gs->mission_status = MISSION_SUCCESS;
                    sound_play(SFX_MISSION_WIN);
                }
            } else {
                /* Reset timer if ragdoll touches ground */
                gs->mission_timer = 0;
            }
            break;

        case MODE_MISSION_TARGET: {
            /* Check if head is within target zone */
            int hx, hy;
            ragdoll_get_head_pos(doll, world, &hx, &hy);

            int dx = hx - gs->target_x;
            int dy = hy - gs->target_y;
            int dist2 = dx * dx + dy * dy;
            int r = gs->target_radius;

            if (dist2 < r * r) {
                gs->mission_status = MISSION_SUCCESS;
                sound_play(SFX_MISSION_WIN);
            }
            break;
        }

        case MODE_MISSION_BRIDGE:
            /* Check if ragdoll is on a user-created object and not on floor */
            {
                int hip_idx = doll->part_indices[RAGDOLL_HIP];
                Particle *hip = &world->particles[hip_idx];
                int hip_y = FP_TO_INT(hip->y);

                /* If ragdoll is above the floor but has low velocity, count it */
                if (hip_y < SCREEN_HEIGHT - 30 && doll->total_velocity < INT_TO_FP(2)) {
                    gs->mission_timer++;
                    if (gs->mission_timer >= gs->mission_goal) {
                        gs->mission_status = MISSION_SUCCESS;
                        sound_play(SFX_MISSION_WIN);
                    }
                } else if (hip_y >= SCREEN_HEIGHT - 10) {
                    gs->mission_timer = 0;
                }
            }
            break;

        default:
            break;
    }
}

/*=============================================================================
 * DRAW MISSION ELEMENTS (bottom screen)
 *===========================================================================*/
void game_draw_mission_elements(GameState *gs) {
    if (gs->mode == MODE_MISSION_TARGET && gs->mission_status == MISSION_ACTIVE) {
        renderer_draw_target(gs->target_x, gs->target_y, gs->target_radius);
    }

    /* Draw success/fail message */
    if (gs->mission_status == MISSION_SUCCESS) {
        renderer_draw_string(80, 90, "MISSION COMPLETE!", COLOR_FACE_HAPPY);
    }
}

/*=============================================================================
 * TOP SCREEN HUD
 *===========================================================================*/
void game_print_hud(GameState *gs, Ragdoll *doll, ToolState *ts, PhysicsWorld *world) {
    (void)world;

    /* Clear console & position cursor */
    iprintf("\x1b[2J"); /* Clear screen */

    /* Title */
    iprintf("\x1b[0;2H");
    iprintf("~~ PAPER PHYSICS ~~");
    iprintf("\x1b[1;2H");
    iprintf("   Un-Stressed");

    /* Separator */
    iprintf("\x1b[3;0H");
    iprintf("================================");

    /* Mode */
    iprintf("\x1b[5;1H");
    iprintf("Mode: %s", game_get_mode_name(gs->mode));

    /* Current tool */
    iprintf("\x1b[7;1H");
    iprintf("Tool: %s", tools_get_name(ts->current_tool));

    /* Stress meter */
    iprintf("\x1b[9;1H");
    int stress_bar = FP_TO_INT(doll->stress_level);
    if (stress_bar > 30) stress_bar = 30;
    iprintf("Stress: [");
    for (int i = 0; i < 20; i++) {
        if (i < stress_bar * 20 / 30)
            iprintf("#");
        else
            iprintf(".");
    }
    iprintf("]");

    /* Expression */
    iprintf("\x1b[11;1H");
    const char *faces[] = { ":)", ":|", ":S", "XO", "x_x" };
    iprintf("Face: %s", faces[doll->expression]);

    /* Gravity */
    iprintf("\x1b[13;1H");
    iprintf("Gravity: %s", world->gravity_flipped ? "FLIPPED!" : "Normal");

    /* Mission progress */
    if (gs->mission_status == MISSION_ACTIVE) {
        iprintf("\x1b[15;1H");
        switch (gs->mode) {
            case MODE_MISSION_AIRBORNE:
                iprintf("Airborne: %ld/%ld sec",
                    gs->mission_timer / 60, gs->mission_goal / 60);
                break;
            case MODE_MISSION_TARGET:
                iprintf("Fling ragdoll to target!");
                break;
            case MODE_MISSION_BRIDGE:
                iprintf("Support: %ld/%ld sec",
                    gs->mission_timer / 60, gs->mission_goal / 60);
                break;
            default:
                break;
        }
    } else if (gs->mission_status == MISSION_SUCCESS) {
        iprintf("\x1b[15;1H");
        iprintf("*** MISSION COMPLETE! ***");
    }

    /* FPS */
    iprintf("\x1b[17;1H");
    iprintf("FPS: %d", gs->fps);

    /* Separator */
    iprintf("\x1b[19;0H");
    iprintf("================================");

    /* Controls */
    iprintf("\x1b[20;1H");
    iprintf("A/B/X/Y=Tools L/R=Gravity");
    iprintf("\x1b[21;1H");
    iprintf("START=Reset SEL=Clear");
    iprintf("\x1b[22;1H");
    iprintf("D-PAD Up/Dn=Mode");
}
