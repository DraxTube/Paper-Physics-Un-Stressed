#include "ragdoll.h"
#include "sound.h"
#include <string.h>

/* Bone color — pencil gray on notebook */
#define BONE_COLOR      RGB15(8, 8, 8)
#define HEAD_COLOR      RGB15(10, 10, 10)
#define BODY_COLOR      RGB15(8, 8, 8)

/*=============================================================================
 * CREATE RAGDOLL
 *===========================================================================*/
void ragdoll_create(Ragdoll *doll, PhysicsWorld *world, int px, int py) {
    memset(doll, 0, sizeof(Ragdoll));

    int32_t cx = INT_TO_FP(px);
    int32_t cy = INT_TO_FP(py);
    int32_t mass = FP_ONE;

    /* Create particles for each body part */
    /* Head — top center */
    doll->part_indices[RAGDOLL_HEAD] = physics_add_particle(
        world, cx, cy - INT_TO_FP(30), mass, HEAD_RADIUS, HEAD_COLOR);

    /* Neck — just below head */
    doll->part_indices[RAGDOLL_NECK] = physics_add_particle(
        world, cx, cy - INT_TO_FP(22), mass, LIMB_RADIUS, BODY_COLOR);

    /* Left hand */
    doll->part_indices[RAGDOLL_LHAND] = physics_add_particle(
        world, cx - INT_TO_FP(18), cy - INT_TO_FP(10), mass, LIMB_RADIUS, BODY_COLOR);

    /* Right hand */
    doll->part_indices[RAGDOLL_RHAND] = physics_add_particle(
        world, cx + INT_TO_FP(18), cy - INT_TO_FP(10), mass, LIMB_RADIUS, BODY_COLOR);

    /* Torso — center */
    doll->part_indices[RAGDOLL_TORSO] = physics_add_particle(
        world, cx, cy - INT_TO_FP(8), mass, BODY_RADIUS, BODY_COLOR);

    /* Hip */
    doll->part_indices[RAGDOLL_HIP] = physics_add_particle(
        world, cx, cy + INT_TO_FP(5), mass, BODY_RADIUS, BODY_COLOR);

    /* Left foot */
    doll->part_indices[RAGDOLL_LFOOT] = physics_add_particle(
        world, cx - INT_TO_FP(10), cy + INT_TO_FP(22), mass, LIMB_RADIUS, BODY_COLOR);

    /* Right foot */
    doll->part_indices[RAGDOLL_RFOOT] = physics_add_particle(
        world, cx + INT_TO_FP(10), cy + INT_TO_FP(22), mass, LIMB_RADIUS, BODY_COLOR);

    /* Connect bones with stiff constraints */
    int32_t stiff = FLOAT_TO_FP(0.8f);

    /* Head → Neck */
    doll->bone_indices[0] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_HEAD], doll->part_indices[RAGDOLL_NECK],
        stiff, BONE_COLOR);

    /* Neck → Left Hand (upper arm) */
    doll->bone_indices[1] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_NECK], doll->part_indices[RAGDOLL_LHAND],
        FLOAT_TO_FP(0.5f), BONE_COLOR);

    /* Neck → Right Hand (upper arm) */
    doll->bone_indices[2] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_NECK], doll->part_indices[RAGDOLL_RHAND],
        FLOAT_TO_FP(0.5f), BONE_COLOR);

    /* Neck → Torso */
    doll->bone_indices[3] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_NECK], doll->part_indices[RAGDOLL_TORSO],
        stiff, BONE_COLOR);

    /* Torso → Hip */
    doll->bone_indices[4] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_TORSO], doll->part_indices[RAGDOLL_HIP],
        stiff, BONE_COLOR);

    /* Hip → Left Foot */
    doll->bone_indices[5] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_HIP], doll->part_indices[RAGDOLL_LFOOT],
        FLOAT_TO_FP(0.6f), BONE_COLOR);

    /* Hip → Right Foot */
    doll->bone_indices[6] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_HIP], doll->part_indices[RAGDOLL_RFOOT],
        FLOAT_TO_FP(0.6f), BONE_COLOR);

    /* Structural cross-braces for stability */
    /* Head → Torso (prevents folding) */
    doll->bone_indices[7] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_HEAD], doll->part_indices[RAGDOLL_TORSO],
        FLOAT_TO_FP(0.3f), BONE_COLOR);

    /* Neck → Hip */
    doll->bone_indices[8] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_NECK], doll->part_indices[RAGDOLL_HIP],
        FLOAT_TO_FP(0.3f), BONE_COLOR);

    /* Left Hand → Torso (arm brace) */
    doll->bone_indices[9] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_LHAND], doll->part_indices[RAGDOLL_TORSO],
        FLOAT_TO_FP(0.15f), BONE_COLOR);

    /* Right Hand → Torso (arm brace) */
    doll->bone_indices[10] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_RHAND], doll->part_indices[RAGDOLL_TORSO],
        FLOAT_TO_FP(0.15f), BONE_COLOR);

    /* Left Foot → Right Foot (hip brace) */
    doll->bone_indices[11] = physics_add_constraint(world,
        doll->part_indices[RAGDOLL_LFOOT], doll->part_indices[RAGDOLL_RFOOT],
        FLOAT_TO_FP(0.2f), BONE_COLOR);

    doll->stress_level = 0;
    doll->expression = FACE_HAPPY;
    doll->time_airborne = 0;
    doll->alive = true;
}

/*=============================================================================
 * UPDATE RAGDOLL STATE
 *===========================================================================*/
void ragdoll_update(Ragdoll *doll, PhysicsWorld *world) {
    if (!doll->alive) return;

    /* Calculate stress from constraint violations and velocity */
    int32_t stress = 0;
    int32_t total_vel = 0;

    for (int i = 0; i < RAGDOLL_PARTS; i++) {
        int idx = doll->part_indices[i];
        Particle *p = &world->particles[idx];
        if (!p->active) continue;

        int32_t vx = p->x - p->old_x;
        int32_t vy = p->y - p->old_y;
        int32_t vel = FP_MUL(vx, vx) + FP_MUL(vy, vy);
        total_vel += vel;

        /* Wall collision adds stress */
        int32_t py_int = FP_TO_INT(p->y);
        if (py_int >= SCREEN_HEIGHT - 5 || py_int <= 5) {
            stress += vel >> 2;
        }
    }

    doll->total_velocity = total_vel;

    /* Stress decays over time */
    doll->stress_level = FP_MUL(doll->stress_level, FLOAT_TO_FP(0.97f));
    doll->stress_level += stress;

    /* Play impact sound when stress spikes (wall hits) — with cooldown */
    {
        static int impact_cooldown = 0;
        if (impact_cooldown > 0) impact_cooldown--;

        if (stress > INT_TO_FP(8) && impact_cooldown == 0) {
            int intensity = FP_TO_INT(stress) * 3;
            if (intensity > 127) intensity = 127;
            if (intensity < 20) intensity = 20;
            sound_play_impact(intensity);
            impact_cooldown = 20; /* Don't play again for 20 frames (~0.33s) */
        }
    }

    /* Clamp stress */
    if (doll->stress_level > STRESS_MAX)
        doll->stress_level = STRESS_MAX;
    if (doll->stress_level < 0)
        doll->stress_level = 0;

    /* Update expression based on stress */
    if (doll->stress_level < STRESS_LOW) {
        doll->expression = FACE_HAPPY;
    } else if (doll->stress_level < STRESS_MED) {
        doll->expression = FACE_NEUTRAL;
    } else if (doll->stress_level < STRESS_HIGH) {
        doll->expression = FACE_WORRIED;
    } else {
        doll->expression = FACE_SCARED;
    }

    /* Track airborne time */
    int hip = doll->part_indices[RAGDOLL_HIP];
    int lfoot = doll->part_indices[RAGDOLL_LFOOT];
    int rfoot = doll->part_indices[RAGDOLL_RFOOT];

    bool on_ground = false;
    int floor_y = INT_TO_FP(SCREEN_HEIGHT - 5);

    if (world->particles[hip].y >= floor_y ||
        world->particles[lfoot].y >= floor_y ||
        world->particles[rfoot].y >= floor_y) {
        on_ground = true;
    }

    /* If gravity is flipped, check ceiling instead */
    if (world->gravity_flipped) {
        int ceil_y = INT_TO_FP(5);
        on_ground = false;
        if (world->particles[hip].y <= ceil_y ||
            world->particles[lfoot].y <= ceil_y ||
            world->particles[rfoot].y <= ceil_y) {
            on_ground = true;
        }
    }

    if (on_ground) {
        doll->time_airborne = 0;
    } else {
        doll->time_airborne++;
    }
}

/*=============================================================================
 * RESET RAGDOLL
 *===========================================================================*/
void ragdoll_reset(Ragdoll *doll, PhysicsWorld *world, int px, int py) {
    /* Deactivate old particles and constraints */
    for (int i = 0; i < RAGDOLL_PARTS; i++) {
        physics_remove_particle(world, doll->part_indices[i]);
    }
    for (int i = 0; i < RAGDOLL_BONES; i++) {
        physics_remove_constraint(world, doll->bone_indices[i]);
    }

    /* Recreate */
    ragdoll_create(doll, world, px, py);
}

/*=============================================================================
 * GET HEAD POSITION
 *===========================================================================*/
void ragdoll_get_head_pos(Ragdoll *doll, PhysicsWorld *world, int *x, int *y) {
    int idx = doll->part_indices[RAGDOLL_HEAD];
    *x = FP_TO_INT(world->particles[idx].x);
    *y = FP_TO_INT(world->particles[idx].y);
}

/*=============================================================================
 * CHECK OWNERSHIP
 *===========================================================================*/
bool ragdoll_owns_particle(Ragdoll *doll, int particle_index) {
    for (int i = 0; i < RAGDOLL_PARTS; i++) {
        if (doll->part_indices[i] == particle_index)
            return true;
    }
    return false;
}
