#include "tools.h"
#include "renderer.h"
#include "sound.h"
#include <string.h>

/*=============================================================================
 * TOOL NAMES
 *===========================================================================*/
static const char *tool_names[] = {
    "GRAB", "PENCIL", "PIN", "ELASTIC", "ERASER"
};

const char* tools_get_name(ToolType tool) {
    if (tool < TOOL_COUNT) return tool_names[tool];
    return "???";
}

/*=============================================================================
 * INIT
 *===========================================================================*/
void tools_init(ToolState *ts) {
    memset(ts, 0, sizeof(ToolState));
    ts->current_tool = TOOL_GRAB;
    ts->grabbed_particle = -1;
    ts->elastic_start_particle = -1;
}

/*=============================================================================
 * NEXT / PREVIOUS
 *===========================================================================*/
void tools_next(ToolState *ts) {
    ts->current_tool = (ToolType)((ts->current_tool + 1) % TOOL_COUNT);
    ts->grabbed_particle = -1;
    ts->is_grabbing = false;
    ts->drawing.drawing = false;
    ts->elastic_selecting = false;
}

void tools_prev(ToolState *ts) {
    ts->current_tool = (ToolType)((ts->current_tool + TOOL_COUNT - 1) % TOOL_COUNT);
    ts->grabbed_particle = -1;
    ts->is_grabbing = false;
    ts->drawing.drawing = false;
    ts->elastic_selecting = false;
}

void tools_set(ToolState *ts, ToolType tool) {
    ts->current_tool = tool;
    ts->grabbed_particle = -1;
    ts->is_grabbing = false;
    ts->drawing.drawing = false;
    ts->elastic_selecting = false;
}

/*=============================================================================
 * PROCESS GRAB TOOL
 *===========================================================================*/
static void process_grab(ToolState *ts, PhysicsWorld *world, Ragdoll *doll,
                         int32_t tx, int32_t ty, bool touching,
                         bool just_pressed, bool just_released) {
    (void)doll;

    if (just_pressed) {
        /* Find nearest particle to touch */
        int nearest = physics_find_nearest(world, tx, ty, INT_TO_FP(20));
        if (nearest >= 0) {
            ts->grabbed_particle = nearest;
            ts->is_grabbing = true;
            sound_play(SFX_GRAB);
        }
        /* Reset touch history */
        for (int i = 0; i < 4; i++) {
            ts->touch_history_x[i] = tx;
            ts->touch_history_y[i] = ty;
        }
        ts->touch_history_idx = 0;
    }

    if (touching && ts->is_grabbing && ts->grabbed_particle >= 0) {
        Particle *p = &world->particles[ts->grabbed_particle];
        if (p->active) {
            /* Move particle to touch position */
            p->x = tx;
            p->y = ty;
            /* Also set old position to prevent huge velocity on release */
            p->old_x = ts->touch_history_x[(ts->touch_history_idx + 2) % 4];
            p->old_y = ts->touch_history_y[(ts->touch_history_idx + 2) % 4];
        }

        /* Store touch history for flick detection */
        ts->touch_history_x[ts->touch_history_idx] = tx;
        ts->touch_history_y[ts->touch_history_idx] = ty;
        ts->touch_history_idx = (ts->touch_history_idx + 1) % 4;
    }

    if (just_released && ts->is_grabbing) {
        /* Calculate flick velocity from touch history */
        int newest = (ts->touch_history_idx + 3) % 4;
        int oldest = ts->touch_history_idx;

        int32_t vx = ts->touch_history_x[newest] - ts->touch_history_x[oldest];
        int32_t vy = ts->touch_history_y[newest] - ts->touch_history_y[oldest];

        /* Scale velocity for a satisfying flick */
        vx = FP_MUL(vx, FLOAT_TO_FP(0.5f));
        vy = FP_MUL(vy, FLOAT_TO_FP(0.5f));

        /* Apply flick impulse */
        if (ts->grabbed_particle >= 0) {
            physics_apply_impulse(world, ts->grabbed_particle, vx, vy);
        }

        /* Sound: louder release if flicked hard */
        int32_t speed2 = FP_MUL(vx, vx) + FP_MUL(vy, vy);
        if (speed2 > INT_TO_FP(4)) {
            sound_play(SFX_PAPER_RUSTLE);
        } else {
            sound_play(SFX_RELEASE);
        }

        ts->grabbed_particle = -1;
        ts->is_grabbing = false;
    }
}

/*=============================================================================
 * PROCESS PENCIL TOOL (draw shapes → physics objects)
 *===========================================================================*/
static void process_pencil(ToolState *ts, PhysicsWorld *world,
                           int32_t tx, int32_t ty, bool touching,
                           bool just_pressed, bool just_released) {
    DrawingState *ds = &ts->drawing;

    if (just_pressed) {
        ds->count = 0;
        ds->drawing = true;
        sound_play(SFX_PENCIL_DRAW);
    }

    if (touching && ds->drawing) {
        /* Add point if far enough from last point */
        if (ds->count < MAX_DRAW_POINTS) {
            bool add = true;
            if (ds->count > 0) {
                int32_t dx = tx - ds->x[ds->count - 1];
                int32_t dy = ty - ds->y[ds->count - 1];
                int32_t dist2 = FP_MUL(dx, dx) + FP_MUL(dy, dy);
                if (dist2 < INT_TO_FP(5) * INT_TO_FP(5)) {
                    add = false; /* Too close to last point */
                }
            }
            if (add) {
                ds->x[ds->count] = tx;
                ds->y[ds->count] = ty;
                ds->count++;
            }
        }

        /* Draw the trail on screen */
        for (int i = 1; i < ds->count; i++) {
            renderer_draw_line(
                FP_TO_INT(ds->x[i-1]), FP_TO_INT(ds->y[i-1]),
                FP_TO_INT(ds->x[i]),   FP_TO_INT(ds->y[i]),
                COLOR_PENCIL_LIGHT);
        }
    }

    if (just_released && ds->drawing && ds->count >= 3) {
        /* Create a physics object from the drawn shape */
        /* Calculate centroid */
        int32_t cx = 0, cy = 0;
        for (int i = 0; i < ds->count; i++) {
            cx += ds->x[i];
            cy += ds->y[i];
        }
        cx /= ds->count;
        cy /= ds->count;

        /* Calculate approximate radius */
        int32_t max_r = 0;
        for (int i = 0; i < ds->count; i++) {
            int32_t dx = ds->x[i] - cx;
            int32_t dy = ds->y[i] - cy;
            int32_t r = FP_MUL(dx, dx) + FP_MUL(dy, dy);
            if (r > max_r) max_r = r;
        }
        int32_t radius = fp_sqrt(max_r);
        if (radius < INT_TO_FP(3)) radius = INT_TO_FP(3);
        if (radius > INT_TO_FP(20)) radius = INT_TO_FP(20);

        /* Spawn a physics particle (ball) at centroid */
        physics_add_particle(world, cx, cy,
                            FP_MUL(radius, FLOAT_TO_FP(0.3f)),
                            radius, RGB15(12, 12, 14));

        sound_play(SFX_PAPER_RUSTLE);

        ds->drawing = false;
        ds->count = 0;
    } else if (just_released) {
        ds->drawing = false;
        ds->count = 0;
    }
}

/*=============================================================================
 * PROCESS PIN TOOL
 *===========================================================================*/
static void process_pin(ToolState *ts, PhysicsWorld *world,
                        int32_t tx, int32_t ty, bool just_pressed) {
    (void)ts;

    if (just_pressed) {
        int nearest = physics_find_nearest(world, tx, ty, INT_TO_FP(15));
        if (nearest >= 0) {
            Particle *p = &world->particles[nearest];
            p->pinned = !p->pinned; /* Toggle pin */
            sound_play(p->pinned ? SFX_PIN_PLACE : SFX_PIN_REMOVE);
        }
    }
}

/*=============================================================================
 * PROCESS ELASTIC BAND TOOL
 *===========================================================================*/
static void process_elastic(ToolState *ts, PhysicsWorld *world,
                            int32_t tx, int32_t ty, bool just_pressed) {
    if (just_pressed) {
        int nearest = physics_find_nearest(world, tx, ty, INT_TO_FP(15));
        if (nearest >= 0) {
            if (!ts->elastic_selecting) {
                /* First tap — select start */
                ts->elastic_start_particle = nearest;
                ts->elastic_selecting = true;
                sound_play(SFX_GRAB);
            } else {
                /* Second tap — create elastic between two particles */
                if (nearest != ts->elastic_start_particle) {
                    physics_add_elastic(world, ts->elastic_start_particle, nearest,
                                       COLOR_ELASTIC);
                    sound_play(SFX_ELASTIC_SNAP);
                }
                ts->elastic_selecting = false;
                ts->elastic_start_particle = -1;
            }
        }
    }
}

/*=============================================================================
 * PROCESS ERASER TOOL
 *===========================================================================*/
static void process_eraser(ToolState *ts, PhysicsWorld *world, Ragdoll *doll,
                           int32_t tx, int32_t ty, bool touching) {
    (void)ts;

    if (touching) {
        /* Draw eraser visual */
        int ex = FP_TO_INT(tx);
        int ey = FP_TO_INT(ty);
        renderer_draw_circle_filled(ex, ey, 8, COLOR_ERASER);
        renderer_draw_circle(ex, ey, 8, COLOR_PENCIL_LIGHT);

        /* Find and remove nearby non-ragdoll particles */
        int nearest = physics_find_nearest(world, tx, ty, INT_TO_FP(10));
        if (nearest >= 0 && !ragdoll_owns_particle(doll, nearest)) {
            physics_remove_particle(world, nearest);
            sound_play(SFX_ERASER);
        }

        /* Unpin ragdoll parts if touching them */
        nearest = physics_find_nearest(world, tx, ty, INT_TO_FP(10));
        if (nearest >= 0 && ragdoll_owns_particle(doll, nearest)) {
            Particle *p = &world->particles[nearest];
            p->pinned = false;
        }
    }
}

/*=============================================================================
 * MAIN UPDATE
 *===========================================================================*/
void tools_update(ToolState *ts, PhysicsWorld *world, Ragdoll *doll,
                  int touch_x, int touch_y, bool touching,
                  bool just_pressed, bool just_released) {
    int32_t tx = INT_TO_FP(touch_x);
    int32_t ty = INT_TO_FP(touch_y);

    /* Don't interact with toolbar area (top 11 pixels) */
    if (touch_y < 12 && just_pressed) return;

    switch (ts->current_tool) {
        case TOOL_GRAB:
            process_grab(ts, world, doll, tx, ty, touching, just_pressed, just_released);
            break;
        case TOOL_PENCIL:
            process_pencil(ts, world, tx, ty, touching, just_pressed, just_released);
            break;
        case TOOL_PIN:
            process_pin(ts, world, tx, ty, just_pressed);
            break;
        case TOOL_ELASTIC:
            process_elastic(ts, world, tx, ty, just_pressed);
            break;
        case TOOL_ERASER:
            process_eraser(ts, world, doll, tx, ty, touching);
            break;
        default:
            break;
    }

    ts->was_touching = touching;
}
