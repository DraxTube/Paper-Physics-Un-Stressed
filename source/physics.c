#include "physics.h"
#include <string.h>

/*=============================================================================
 * FIXED-POINT SQUARE ROOT
 * Uses iterative Newton's method. Good enough for constraint solving.
 *===========================================================================*/
int32_t fp_sqrt(int32_t x) {
    if (x <= 0) return 0;

    /* Initial guess: shift right by half the fractional bits */
    int32_t guess = x;
    int32_t last;
    int iters = 0;

    /* Scale up for better precision */
    if (guess < INT_TO_FP(1)) {
        guess = INT_TO_FP(1);
    }

    do {
        last = guess;
        /* Newton iteration: guess = (guess + x/guess) / 2 */
        if (guess == 0) break;
        guess = (guess + FP_DIV(x, guess)) >> 1;
        iters++;
    } while (iters < 8 && (guess != last));

    return guess;
}

/*=============================================================================
 * DISTANCE SQUARED (avoid sqrt when possible)
 *===========================================================================*/
static int32_t distance_sq(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;
    return FP_MUL(dx, dx) + FP_MUL(dy, dy);
}

/*=============================================================================
 * INITIALIZATION
 *===========================================================================*/
void physics_init(PhysicsWorld *world) {
    memset(world, 0, sizeof(PhysicsWorld));
    world->gravity_x = 0;
    world->gravity_y = FLOAT_TO_FP(0.35f);  /* Gravity pointing down */
    world->gravity_flipped = false;
}

/*=============================================================================
 * ADD PARTICLE
 *===========================================================================*/
int physics_add_particle(PhysicsWorld *world, int32_t x, int32_t y,
                         int32_t mass, int32_t radius, u16 color) {
    /* Find an empty slot */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!world->particles[i].active) {
            Particle *p = &world->particles[i];
            p->x = x;
            p->y = y;
            p->old_x = x;
            p->old_y = y;
            p->acc_x = 0;
            p->acc_y = 0;
            p->mass = mass;
            p->radius = radius;
            p->pinned = false;
            p->active = true;
            p->color = color;

            if (i >= world->particle_count)
                world->particle_count = i + 1;

            return i;
        }
    }
    return -1; /* Full */
}

/*=============================================================================
 * ADD DISTANCE CONSTRAINT
 *===========================================================================*/
int physics_add_constraint(PhysicsWorld *world, int p1, int p2,
                           int32_t stiffness, u16 color) {
    if (p1 < 0 || p2 < 0) return -1;

    for (int i = 0; i < MAX_CONSTRAINTS; i++) {
        if (!world->constraints[i].active) {
            Constraint *c = &world->constraints[i];
            c->p1 = p1;
            c->p2 = p2;
            c->stiffness = stiffness;
            c->is_elastic = false;
            c->active = true;
            c->color = color;

            /* Calculate rest length from current particle positions */
            Particle *a = &world->particles[p1];
            Particle *b = &world->particles[p2];
            int32_t dx = b->x - a->x;
            int32_t dy = b->y - a->y;
            int32_t dist2 = FP_MUL(dx, dx) + FP_MUL(dy, dy);
            c->rest_length = fp_sqrt(dist2);

            if (i >= world->constraint_count)
                world->constraint_count = i + 1;

            return i;
        }
    }
    return -1;
}

/*=============================================================================
 * ADD ELASTIC CONSTRAINT (rubber band)
 *===========================================================================*/
int physics_add_elastic(PhysicsWorld *world, int p1, int p2, u16 color) {
    int idx = physics_add_constraint(world, p1, p2, FLOAT_TO_FP(0.1f), color);
    if (idx >= 0) {
        world->constraints[idx].is_elastic = true;
    }
    return idx;
}

/*=============================================================================
 * VERLET INTEGRATION STEP
 *===========================================================================*/
static void integrate(PhysicsWorld *world) {
    for (int i = 0; i < world->particle_count; i++) {
        Particle *p = &world->particles[i];
        if (!p->active || p->pinned) continue;

        /* Velocity = current - old (implicit in Verlet) */
        int32_t vx = FP_MUL(p->x - p->old_x, FRICTION);
        int32_t vy = FP_MUL(p->y - p->old_y, FRICTION);

        /* Save old position */
        p->old_x = p->x;
        p->old_y = p->y;

        /* Apply gravity */
        p->acc_x += world->gravity_x;
        p->acc_y += world->gravity_y;

        /* Verlet: new_pos = pos + velocity + acc */
        p->x += vx + p->acc_x;
        p->y += vy + p->acc_y;

        /* Reset acceleration */
        p->acc_x = 0;
        p->acc_y = 0;
    }
}

/*=============================================================================
 * CONSTRAINT SOLVER
 *===========================================================================*/
static void solve_constraints(PhysicsWorld *world) {
    for (int iter = 0; iter < SOLVER_ITERATIONS; iter++) {
        for (int i = 0; i < world->constraint_count; i++) {
            Constraint *c = &world->constraints[i];
            if (!c->active) continue;

            Particle *a = &world->particles[c->p1];
            Particle *b = &world->particles[c->p2];
            if (!a->active || !b->active) {
                c->active = false;
                continue;
            }

            int32_t dx = b->x - a->x;
            int32_t dy = b->y - a->y;
            int32_t dist2 = FP_MUL(dx, dx) + FP_MUL(dy, dy);
            int32_t dist = fp_sqrt(dist2);

            if (dist == 0) dist = FP_ONE;

            /* For elastic constraints, only pull if stretched beyond rest */
            if (c->is_elastic && dist <= c->rest_length) continue;

            /* Difference from rest length */
            int32_t diff = c->rest_length - dist;
            int32_t correction = FP_MUL(FP_DIV(diff, dist), c->stiffness);

            int32_t cx = FP_MUL(dx, correction) >> 1;
            int32_t cy = FP_MUL(dy, correction) >> 1;

            if (!a->pinned) {
                a->x -= cx;
                a->y -= cy;
            }
            if (!b->pinned) {
                b->x += cx;
                b->y += cy;
            }
        }
    }
}

/*=============================================================================
 * WALL COLLISIONS (screen boundaries)
 *===========================================================================*/
static void collide_walls(PhysicsWorld *world) {
    int32_t min_x = INT_TO_FP(2);
    int32_t max_x = INT_TO_FP(SCREEN_WIDTH - 2);
    int32_t min_y = INT_TO_FP(2);
    int32_t max_y = INT_TO_FP(SCREEN_HEIGHT - 2);

    for (int i = 0; i < world->particle_count; i++) {
        Particle *p = &world->particles[i];
        if (!p->active || p->pinned) continue;

        int32_t r = p->radius;

        /* Left wall */
        if (p->x - r < min_x) {
            int32_t vx = p->x - p->old_x;
            p->x = min_x + r;
            p->old_x = p->x + FP_MUL(vx, WALL_BOUNCE);
        }
        /* Right wall */
        if (p->x + r > max_x) {
            int32_t vx = p->x - p->old_x;
            p->x = max_x - r;
            p->old_x = p->x + FP_MUL(vx, WALL_BOUNCE);
        }
        /* Top wall */
        if (p->y - r < min_y) {
            int32_t vy = p->y - p->old_y;
            p->y = min_y + r;
            p->old_y = p->y + FP_MUL(vy, WALL_BOUNCE);
        }
        /* Bottom wall */
        if (p->y + r > max_y) {
            int32_t vy = p->y - p->old_y;
            p->y = max_y - r;
            p->old_y = p->y + FP_MUL(vy, WALL_BOUNCE);
        }
    }
}

/*=============================================================================
 * FULL PHYSICS STEP
 *===========================================================================*/
void physics_step(PhysicsWorld *world) {
    integrate(world);
    solve_constraints(world);
    collide_walls(world);
}

/*=============================================================================
 * FLIP GRAVITY
 *===========================================================================*/
void physics_flip_gravity(PhysicsWorld *world) {
    world->gravity_y = -world->gravity_y;
    world->gravity_x = -world->gravity_x;
    world->gravity_flipped = !world->gravity_flipped;
}

/*=============================================================================
 * APPLY IMPULSE
 *===========================================================================*/
void physics_apply_impulse(PhysicsWorld *world, int index,
                           int32_t force_x, int32_t force_y) {
    if (index < 0 || index >= world->particle_count) return;
    Particle *p = &world->particles[index];
    if (!p->active || p->pinned) return;

    /* Shift old position to create velocity (Verlet trick) */
    p->old_x -= force_x;
    p->old_y -= force_y;
}

/*=============================================================================
 * APPLY WIND (to all particles)
 *===========================================================================*/
void physics_apply_wind(PhysicsWorld *world, int32_t force_x, int32_t force_y) {
    for (int i = 0; i < world->particle_count; i++) {
        Particle *p = &world->particles[i];
        if (!p->active || p->pinned) continue;
        p->acc_x += force_x;
        p->acc_y += force_y;
    }
}

/*=============================================================================
 * FIND NEAREST PARTICLE
 *===========================================================================*/
int physics_find_nearest(PhysicsWorld *world, int32_t x, int32_t y,
                         int32_t max_dist) {
    int best = -1;
    int32_t best_d2 = FP_MUL(max_dist, max_dist);

    for (int i = 0; i < world->particle_count; i++) {
        Particle *p = &world->particles[i];
        if (!p->active) continue;

        int32_t d2 = distance_sq(x, y, p->x, p->y);
        if (d2 < best_d2) {
            best_d2 = d2;
            best = i;
        }
    }
    return best;
}

/*=============================================================================
 * REMOVE PARTICLE
 *===========================================================================*/
void physics_remove_particle(PhysicsWorld *world, int index) {
    if (index < 0 || index >= world->particle_count) return;
    world->particles[index].active = false;

    /* Also remove any constraints referencing this particle */
    for (int i = 0; i < world->constraint_count; i++) {
        Constraint *c = &world->constraints[i];
        if (c->active && (c->p1 == index || c->p2 == index)) {
            c->active = false;
        }
    }
}

/*=============================================================================
 * REMOVE CONSTRAINT
 *===========================================================================*/
void physics_remove_constraint(PhysicsWorld *world, int index) {
    if (index < 0 || index >= world->constraint_count) return;
    world->constraints[index].active = false;
}
