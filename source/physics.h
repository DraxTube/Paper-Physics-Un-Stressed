#ifndef PHYSICS_H
#define PHYSICS_H

#include <nds.h>

/*=============================================================================
 * FIXED-POINT MATH
 * Using 12-bit fractional part (20.12 format) for physics calculations.
 * This avoids floating point entirely — critical for NDS performance.
 *===========================================================================*/
#define FP_SHIFT     12
#define FP_ONE       (1 << FP_SHIFT)
#define FP_HALF      (FP_ONE >> 1)

#define INT_TO_FP(x)   ((x) << FP_SHIFT)
#define FP_TO_INT(x)   ((x) >> FP_SHIFT)
#define FLOAT_TO_FP(x) ((int32_t)((x) * FP_ONE))
#define FP_MUL(a, b)   ((int32_t)(((int64_t)(a) * (b)) >> FP_SHIFT))
#define FP_DIV(a, b)   ((int32_t)(((int64_t)(a) << FP_SHIFT) / (b)))

/*=============================================================================
 * CONSTANTS
 *===========================================================================*/
#define MAX_PARTICLES    64
#define MAX_CONSTRAINTS  96
#define SOLVER_ITERATIONS 6

/* Use libnds SCREEN_WIDTH (256) and SCREEN_HEIGHT (192) */
#define WALL_BOUNCE      FLOAT_TO_FP(0.4f)
#define FRICTION         FLOAT_TO_FP(0.998f)

/*=============================================================================
 * DATA STRUCTURES
 *===========================================================================*/
typedef struct {
    int32_t x, y;           /* Current position (fixed-point) */
    int32_t old_x, old_y;   /* Previous position (for Verlet) */
    int32_t acc_x, acc_y;   /* Accumulated acceleration */
    int32_t mass;            /* Mass (fixed-point, 0 = infinite/static) */
    int32_t radius;          /* Collision radius (fixed-point) */
    bool    pinned;          /* If true, position is locked */
    bool    active;          /* Is this particle in use? */
    u16     color;           /* RGB555 draw color */
} Particle;

typedef struct {
    int      p1, p2;         /* Indices into particle array */
    int32_t  rest_length;    /* Target distance (fixed-point) */
    int32_t  stiffness;      /* 0..FP_ONE, how rigid the constraint is */
    bool     active;
    bool     is_elastic;     /* If true, acts like a rubber band */
    u16      color;
} Constraint;

typedef struct {
    Particle    particles[MAX_PARTICLES];
    Constraint  constraints[MAX_CONSTRAINTS];
    int         particle_count;
    int         constraint_count;
    int32_t     gravity_x;
    int32_t     gravity_y;
    bool        gravity_flipped;
} PhysicsWorld;

/*=============================================================================
 * FUNCTIONS
 *===========================================================================*/

/* Initialize the physics world */
void physics_init(PhysicsWorld *world);

/* Add a particle, returns index or -1 if full */
int physics_add_particle(PhysicsWorld *world, int32_t x, int32_t y,
                         int32_t mass, int32_t radius, u16 color);

/* Add a distance constraint between two particles, returns index or -1 */
int physics_add_constraint(PhysicsWorld *world, int p1, int p2,
                           int32_t stiffness, u16 color);

/* Add an elastic (rubber band) constraint */
int physics_add_elastic(PhysicsWorld *world, int p1, int p2, u16 color);

/* Step the simulation by one frame */
void physics_step(PhysicsWorld *world);

/* Flip gravity direction */
void physics_flip_gravity(PhysicsWorld *world);

/* Apply an impulse to a particle */
void physics_apply_impulse(PhysicsWorld *world, int index,
                           int32_t force_x, int32_t force_y);

/* Apply a wind blast to all particles (from microphone) */
void physics_apply_wind(PhysicsWorld *world, int32_t force_x, int32_t force_y);

/* Find the nearest particle to a screen point, returns index or -1 */
int physics_find_nearest(PhysicsWorld *world, int32_t x, int32_t y,
                         int32_t max_dist);

/* Remove a particle (deactivate) */
void physics_remove_particle(PhysicsWorld *world, int index);

/* Remove a constraint (deactivate) */
void physics_remove_constraint(PhysicsWorld *world, int index);

/* Fixed-point square root approximation */
int32_t fp_sqrt(int32_t x);

#endif /* PHYSICS_H */
