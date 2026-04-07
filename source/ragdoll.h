#ifndef RAGDOLL_H
#define RAGDOLL_H

#include "physics.h"

/*=============================================================================
 * RAGDOLL BODY PARTS
 * A stick-figure ragdoll with 8 particles and connecting constraints.
 *
 *            (HEAD)
 *              |
 *            (NECK)
 *           /  |  \
 *     (LHAND)(TORSO)(RHAND)
 *              |
 *            (HIP)
 *           /     \
 *       (LFOOT) (RFOOT)
 *===========================================================================*/

/* Body proportions (fixed-point pixel values) */
#define HEAD_RADIUS     INT_TO_FP(6)
#define BODY_RADIUS     INT_TO_FP(3)
#define LIMB_RADIUS     INT_TO_FP(2)

/* Particle indices within the ragdoll */
#define RAGDOLL_HEAD    0
#define RAGDOLL_NECK    1
#define RAGDOLL_LHAND   2
#define RAGDOLL_RHAND   3
#define RAGDOLL_TORSO   4
#define RAGDOLL_HIP     5
#define RAGDOLL_LFOOT   6
#define RAGDOLL_RFOOT   7
#define RAGDOLL_PARTS   8

/* Constraint indices */
#define RAGDOLL_BONES   12

/* Stress thresholds */
#define STRESS_LOW      INT_TO_FP(5)
#define STRESS_MED      INT_TO_FP(15)
#define STRESS_HIGH     INT_TO_FP(30)
#define STRESS_MAX      INT_TO_FP(100)

/* Face expressions */
typedef enum {
    FACE_HAPPY = 0,
    FACE_NEUTRAL,
    FACE_WORRIED,
    FACE_SCARED,
    FACE_DEAD
} FaceExpression;

typedef struct {
    int             part_indices[RAGDOLL_PARTS];      /* Indices in PhysicsWorld */
    int             bone_indices[RAGDOLL_BONES];      /* Constraint indices */
    int32_t         stress_level;                      /* Accumulated stress */
    FaceExpression  expression;                        /* Current face */
    int32_t         total_velocity;                    /* Speed magnitude */
    int32_t         time_airborne;                     /* Frames with no floor contact */
    bool            alive;                             /* Is the ragdoll active? */
} Ragdoll;

/*=============================================================================
 * FUNCTIONS
 *===========================================================================*/

/* Create a ragdoll at the given center position */
void ragdoll_create(Ragdoll *doll, PhysicsWorld *world, int px, int py);

/* Update ragdoll state (stress, expression, etc.) — call after physics_step */
void ragdoll_update(Ragdoll *doll, PhysicsWorld *world);

/* Reset ragdoll to initial position */
void ragdoll_reset(Ragdoll *doll, PhysicsWorld *world, int px, int py);

/* Get the head position for face drawing */
void ragdoll_get_head_pos(Ragdoll *doll, PhysicsWorld *world, int *x, int *y);

/* Check if a particle index belongs to this ragdoll */
bool ragdoll_owns_particle(Ragdoll *doll, int particle_index);

#endif /* RAGDOLL_H */
