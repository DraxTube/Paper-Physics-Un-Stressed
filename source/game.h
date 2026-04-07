#ifndef GAME_H
#define GAME_H

#include "physics.h"
#include "ragdoll.h"
#include "tools.h"

/*=============================================================================
 * GAME MODES
 *===========================================================================*/
typedef enum {
    MODE_SANDBOX = 0,
    MODE_MISSION_AIRBORNE,    /* Keep ragdoll airborne for N seconds */
    MODE_MISSION_TARGET,      /* Hit a target zone */
    MODE_MISSION_BRIDGE,      /* Build a structure to support ragdoll */
    MODE_COUNT
} GameMode;

typedef enum {
    MISSION_INACTIVE = 0,
    MISSION_ACTIVE,
    MISSION_SUCCESS,
    MISSION_FAILED
} MissionStatus;

/*=============================================================================
 * GAME STATE
 *===========================================================================*/
typedef struct {
    GameMode       mode;
    MissionStatus  mission_status;

    /* Mission parameters */
    int32_t        mission_timer;       /* Frames remaining or counted */
    int32_t        mission_goal;        /* Target value (frames, position, etc.) */

    /* Mission: Target */
    int            target_x, target_y;
    int            target_radius;

    /* Score & stats */
    int32_t        total_stress;        /* Cumulative stress for session */
    int            objects_created;
    int            flips_done;

    /* Frame counter */
    int            frame_count;

    /* FPS tracking */
    int            fps;
    int            fps_counter;
    int            fps_timer;

} GameState;

/*=============================================================================
 * FUNCTIONS
 *===========================================================================*/

/* Initialize game state */
void game_init(GameState *gs);

/* Switch game mode */
void game_set_mode(GameState *gs, GameMode mode);

/* Update game logic (call after physics_step) */
void game_update(GameState *gs, PhysicsWorld *world, Ragdoll *doll);

/* Draw mission-specific UI on bottom screen */
void game_draw_mission_elements(GameState *gs);

/* Print top screen HUD */
void game_print_hud(GameState *gs, Ragdoll *doll, ToolState *ts, PhysicsWorld *world);

/* Get mode name */
const char* game_get_mode_name(GameMode mode);

#endif /* GAME_H */
