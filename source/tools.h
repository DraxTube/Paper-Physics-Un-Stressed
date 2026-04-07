#ifndef TOOLS_H
#define TOOLS_H

#include "physics.h"
#include "ragdoll.h"

/*=============================================================================
 * TOOL TYPES
 *===========================================================================*/
typedef enum {
    TOOL_GRAB = 0,
    TOOL_PENCIL,
    TOOL_PIN,
    TOOL_ELASTIC,
    TOOL_ERASER,
    TOOL_COUNT
} ToolType;

/*=============================================================================
 * PENCIL DRAWING STATE
 * Tracks points drawn by the stylus to create physics shapes.
 *===========================================================================*/
#define MAX_DRAW_POINTS 32

typedef struct {
    int32_t x[MAX_DRAW_POINTS];
    int32_t y[MAX_DRAW_POINTS];
    int     count;
    bool    drawing;
} DrawingState;

/*=============================================================================
 * TOOL STATE
 *===========================================================================*/
typedef struct {
    ToolType      current_tool;

    /* Grab state */
    int           grabbed_particle;     /* Index of particle being dragged */
    bool          is_grabbing;

    /* Flick detection */
    int32_t       touch_history_x[4];   /* Last 4 touch positions */
    int32_t       touch_history_y[4];
    int           touch_history_idx;
    bool          was_touching;

    /* Pencil / Drawing */
    DrawingState  drawing;

    /* Elastic band */
    int           elastic_start_particle; /* First particle selected */
    bool          elastic_selecting;

    /* Pin tool */
    bool          pin_mode_unpin;         /* If true, unpins instead of pinning */

} ToolState;

/*=============================================================================
 * FUNCTIONS
 *===========================================================================*/

/* Initialize tool state */
void tools_init(ToolState *ts);

/* Switch to the next/previous tool */
void tools_next(ToolState *ts);
void tools_prev(ToolState *ts);

/* Set a specific tool */
void tools_set(ToolState *ts, ToolType tool);

/* Process touch input for the current frame */
void tools_update(ToolState *ts, PhysicsWorld *world, Ragdoll *doll,
                  int touch_x, int touch_y, bool touching, bool just_pressed, bool just_released);

/* Get tool name string */
const char* tools_get_name(ToolType tool);

#endif /* TOOLS_H */
