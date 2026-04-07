#include "renderer.h"
#include <string.h>

/*=============================================================================
 * FRAMEBUFFER
 * We use Mode FB (framebuffer) on the bottom screen for direct pixel access.
 *===========================================================================*/
static u16 *g_fb = NULL;

u16* renderer_get_fb(void) {
    return g_fb;
}

/*=============================================================================
 * TINY 5x7 BITMAP FONT (ASCII 32-95)
 * Each char is 5 columns, 7 rows. Stored column-major as bytes.
 *===========================================================================*/
static const u8 font5x7[] = {
    /* Space (32) */ 0x00,0x00,0x00,0x00,0x00,
    /* !     (33) */ 0x00,0x00,0x5F,0x00,0x00,
    /* "     (34) */ 0x00,0x07,0x00,0x07,0x00,
    /* #     (35) */ 0x14,0x7F,0x14,0x7F,0x14,
    /* $     (36) */ 0x24,0x2A,0x7F,0x2A,0x12,
    /* %     (37) */ 0x23,0x13,0x08,0x64,0x62,
    /* &     (38) */ 0x36,0x49,0x55,0x22,0x50,
    /* '     (39) */ 0x00,0x05,0x03,0x00,0x00,
    /* (     (40) */ 0x00,0x1C,0x22,0x41,0x00,
    /* )     (41) */ 0x00,0x41,0x22,0x1C,0x00,
    /* *     (42) */ 0x14,0x08,0x3E,0x08,0x14,
    /* +     (43) */ 0x08,0x08,0x3E,0x08,0x08,
    /* ,     (44) */ 0x00,0x50,0x30,0x00,0x00,
    /* -     (45) */ 0x08,0x08,0x08,0x08,0x08,
    /* .     (46) */ 0x00,0x60,0x60,0x00,0x00,
    /* /     (47) */ 0x20,0x10,0x08,0x04,0x02,
    /* 0     (48) */ 0x3E,0x51,0x49,0x45,0x3E,
    /* 1     (49) */ 0x00,0x42,0x7F,0x40,0x00,
    /* 2     (50) */ 0x42,0x61,0x51,0x49,0x46,
    /* 3     (51) */ 0x21,0x41,0x45,0x4B,0x31,
    /* 4     (52) */ 0x18,0x14,0x12,0x7F,0x10,
    /* 5     (53) */ 0x27,0x45,0x45,0x45,0x39,
    /* 6     (54) */ 0x3C,0x4A,0x49,0x49,0x30,
    /* 7     (55) */ 0x01,0x71,0x09,0x05,0x03,
    /* 8     (56) */ 0x36,0x49,0x49,0x49,0x36,
    /* 9     (57) */ 0x06,0x49,0x49,0x29,0x1E,
    /* :     (58) */ 0x00,0x36,0x36,0x00,0x00,
    /* ;     (59) */ 0x00,0x56,0x36,0x00,0x00,
    /* <     (60) */ 0x08,0x14,0x22,0x41,0x00,
    /* =     (61) */ 0x14,0x14,0x14,0x14,0x14,
    /* >     (62) */ 0x00,0x41,0x22,0x14,0x08,
    /* ?     (63) */ 0x02,0x01,0x51,0x09,0x06,
    /* @     (64) */ 0x3E,0x41,0x5D,0x55,0x1E,
    /* A     (65) */ 0x7E,0x11,0x11,0x11,0x7E,
    /* B     (66) */ 0x7F,0x49,0x49,0x49,0x36,
    /* C     (67) */ 0x3E,0x41,0x41,0x41,0x22,
    /* D     (68) */ 0x7F,0x41,0x41,0x22,0x1C,
    /* E     (69) */ 0x7F,0x49,0x49,0x49,0x41,
    /* F     (70) */ 0x7F,0x09,0x09,0x09,0x01,
    /* G     (71) */ 0x3E,0x41,0x49,0x49,0x7A,
    /* H     (72) */ 0x7F,0x08,0x08,0x08,0x7F,
    /* I     (73) */ 0x00,0x41,0x7F,0x41,0x00,
    /* J     (74) */ 0x20,0x40,0x41,0x3F,0x01,
    /* K     (75) */ 0x7F,0x08,0x14,0x22,0x41,
    /* L     (76) */ 0x7F,0x40,0x40,0x40,0x40,
    /* M     (77) */ 0x7F,0x02,0x0C,0x02,0x7F,
    /* N     (78) */ 0x7F,0x04,0x08,0x10,0x7F,
    /* O     (79) */ 0x3E,0x41,0x41,0x41,0x3E,
    /* P     (80) */ 0x7F,0x09,0x09,0x09,0x06,
    /* Q     (81) */ 0x3E,0x41,0x51,0x21,0x5E,
    /* R     (82) */ 0x7F,0x09,0x19,0x29,0x46,
    /* S     (83) */ 0x46,0x49,0x49,0x49,0x31,
    /* T     (84) */ 0x01,0x01,0x7F,0x01,0x01,
    /* U     (85) */ 0x3F,0x40,0x40,0x40,0x3F,
    /* V     (86) */ 0x1F,0x20,0x40,0x20,0x1F,
    /* W     (87) */ 0x3F,0x40,0x38,0x40,0x3F,
    /* X     (88) */ 0x63,0x14,0x08,0x14,0x63,
    /* Y     (89) */ 0x07,0x08,0x70,0x08,0x07,
    /* Z     (90) */ 0x61,0x51,0x49,0x45,0x43,
    /* [     (91) */ 0x00,0x7F,0x41,0x41,0x00,
    /* \     (92) */ 0x02,0x04,0x08,0x10,0x20,
    /* ]     (93) */ 0x00,0x41,0x41,0x7F,0x00,
    /* ^     (94) */ 0x04,0x02,0x01,0x02,0x04,
    /* _     (95) */ 0x40,0x40,0x40,0x40,0x40,
};

/*=============================================================================
 * INIT
 *===========================================================================*/
void renderer_init(void) {
    /* Set bottom screen to framebuffer mode */
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    g_fb = VRAM_A;
}

/*=============================================================================
 * PUT PIXEL (bounds-checked)
 *===========================================================================*/
void renderer_put_pixel(int x, int y, u16 color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    g_fb[y * SCREEN_WIDTH + x] = color | BIT(15);
}

/*=============================================================================
 * CLEAR SCREEN
 *===========================================================================*/
void renderer_clear(void) {
    u16 col = COLOR_PAPER | BIT(15);
    /* Fast fill */
    u32 fill = col | ((u32)col << 16);
    u32 *fb32 = (u32*)g_fb;
    for (int i = 0; i < (SCREEN_WIDTH * SCREEN_HEIGHT) / 2; i++) {
        fb32[i] = fill;
    }
}

/*=============================================================================
 * DRAW NOTEBOOK GRID
 *===========================================================================*/
void renderer_draw_notebook(void) {
    /* Horizontal lines (blue, every 16 pixels) */
    for (int row = 16; row < SCREEN_HEIGHT; row += 16) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            renderer_put_pixel(x, row, COLOR_GRID);
        }
    }

    /* Red margin line at x=30 */
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        renderer_put_pixel(30, y, COLOR_MARGIN);
        renderer_put_pixel(31, y, COLOR_MARGIN);
    }

    /* Holes on the left edge (notebook binding) */
    for (int h = 0; h < 3; h++) {
        int cy = 32 + h * 64;
        renderer_draw_circle_filled(2, cy, 4, COLOR_WHITE);
        renderer_draw_circle(2, cy, 4, COLOR_PENCIL_LIGHT);
    }
}

/*=============================================================================
 * BRESENHAM LINE
 *===========================================================================*/
void renderer_draw_line(int x1, int y1, int x2, int y2, u16 color) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int sx = dx > 0 ? 1 : -1;
    int sy = dy > 0 ? 1 : -1;
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;

    int err = dx - dy;

    while (1) {
        renderer_put_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

/*=============================================================================
 * THICK LINE (draws parallel lines for thickness)
 *===========================================================================*/
void renderer_draw_thick_line(int x1, int y1, int x2, int y2, u16 color, int thickness) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int half = thickness / 2;

    /* Draw perpendicular offsets */
    if (dx == 0 && dy == 0) {
        renderer_draw_circle_filled(x1, y1, half, color);
        return;
    }

    /* Simple approach: draw multiple parallel lines */
    bool mostly_vertical = (dy < 0 ? -dy : dy) > (dx < 0 ? -dx : dx);

    for (int i = -half; i <= half; i++) {
        if (mostly_vertical) {
            renderer_draw_line(x1 + i, y1, x2 + i, y2, color);
        } else {
            renderer_draw_line(x1, y1 + i, x2, y2 + i, color);
        }
    }
}

/*=============================================================================
 * CIRCLE (midpoint algorithm)
 *===========================================================================*/
void renderer_draw_circle(int cx, int cy, int r, u16 color) {
    int x = r, y = 0;
    int err = 1 - r;

    while (x >= y) {
        renderer_put_pixel(cx + x, cy + y, color);
        renderer_put_pixel(cx + y, cy + x, color);
        renderer_put_pixel(cx - y, cy + x, color);
        renderer_put_pixel(cx - x, cy + y, color);
        renderer_put_pixel(cx - x, cy - y, color);
        renderer_put_pixel(cx - y, cy - x, color);
        renderer_put_pixel(cx + y, cy - x, color);
        renderer_put_pixel(cx + x, cy - y, color);

        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

/*=============================================================================
 * FILLED CIRCLE
 *===========================================================================*/
void renderer_draw_circle_filled(int cx, int cy, int r, u16 color) {
    int x = r, y = 0;
    int err = 1 - r;

    while (x >= y) {
        /* Draw horizontal lines across each octant */
        for (int i = cx - x; i <= cx + x; i++) {
            renderer_put_pixel(i, cy + y, color);
            renderer_put_pixel(i, cy - y, color);
        }
        for (int i = cx - y; i <= cx + y; i++) {
            renderer_put_pixel(i, cy + x, color);
            renderer_put_pixel(i, cy - x, color);
        }

        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

/*=============================================================================
 * DRAW CHARACTER (5x7 bitmap font)
 *===========================================================================*/
void renderer_draw_char(int x, int y, char c, u16 color) {
    if (c < 32 || c > 95) return;
    int idx = (c - 32) * 5;

    for (int col = 0; col < 5; col++) {
        u8 column = font5x7[idx + col];
        for (int row = 0; row < 7; row++) {
            if (column & (1 << row)) {
                renderer_put_pixel(x + col, y + row, color);
            }
        }
    }
}

/*=============================================================================
 * DRAW STRING
 *===========================================================================*/
void renderer_draw_string(int x, int y, const char *str, u16 color) {
    while (*str) {
        /* Convert lowercase to uppercase for our limited font */
        char c = *str;
        if (c >= 'a' && c <= 'z') c -= 32;
        renderer_draw_char(x, y, c, color);
        x += 6;
        str++;
    }
}

/*=============================================================================
 * DRAW FACE EXPRESSION
 *===========================================================================*/
void renderer_draw_face(int hx, int hy, int radius, FaceExpression expr) {
    /* Draw head circle */
    renderer_draw_circle_filled(hx, hy, radius, COLOR_PAPER);
    renderer_draw_circle(hx, hy, radius, COLOR_PENCIL);

    /* Eyes (always two dots) */
    int eye_y = hy - 2;
    int eye_lx = hx - 2;
    int eye_rx = hx + 2;

    switch (expr) {
        case FACE_HAPPY:
            /* Round eyes */
            renderer_put_pixel(eye_lx, eye_y, COLOR_PENCIL);
            renderer_put_pixel(eye_rx, eye_y, COLOR_PENCIL);
            /* Smile */
            renderer_put_pixel(hx - 2, hy + 2, COLOR_PENCIL);
            renderer_put_pixel(hx - 1, hy + 3, COLOR_PENCIL);
            renderer_put_pixel(hx, hy + 3, COLOR_PENCIL);
            renderer_put_pixel(hx + 1, hy + 3, COLOR_PENCIL);
            renderer_put_pixel(hx + 2, hy + 2, COLOR_PENCIL);
            break;

        case FACE_NEUTRAL:
            renderer_put_pixel(eye_lx, eye_y, COLOR_PENCIL);
            renderer_put_pixel(eye_rx, eye_y, COLOR_PENCIL);
            /* Flat mouth */
            renderer_draw_line(hx - 2, hy + 2, hx + 2, hy + 2, COLOR_PENCIL);
            break;

        case FACE_WORRIED:
            /* Wide eyes */
            renderer_put_pixel(eye_lx, eye_y, COLOR_PENCIL);
            renderer_put_pixel(eye_lx, eye_y - 1, COLOR_PENCIL);
            renderer_put_pixel(eye_rx, eye_y, COLOR_PENCIL);
            renderer_put_pixel(eye_rx, eye_y - 1, COLOR_PENCIL);
            /* Wavy mouth */
            renderer_put_pixel(hx - 2, hy + 2, COLOR_PENCIL);
            renderer_put_pixel(hx - 1, hy + 3, COLOR_PENCIL);
            renderer_put_pixel(hx, hy + 2, COLOR_PENCIL);
            renderer_put_pixel(hx + 1, hy + 3, COLOR_PENCIL);
            renderer_put_pixel(hx + 2, hy + 2, COLOR_PENCIL);
            break;

        case FACE_SCARED:
            /* X eyes */
            renderer_put_pixel(eye_lx - 1, eye_y - 1, COLOR_FACE_SCARED);
            renderer_put_pixel(eye_lx + 1, eye_y + 1, COLOR_FACE_SCARED);
            renderer_put_pixel(eye_lx + 1, eye_y - 1, COLOR_FACE_SCARED);
            renderer_put_pixel(eye_lx - 1, eye_y + 1, COLOR_FACE_SCARED);
            renderer_put_pixel(eye_rx - 1, eye_y - 1, COLOR_FACE_SCARED);
            renderer_put_pixel(eye_rx + 1, eye_y + 1, COLOR_FACE_SCARED);
            renderer_put_pixel(eye_rx + 1, eye_y - 1, COLOR_FACE_SCARED);
            renderer_put_pixel(eye_rx - 1, eye_y + 1, COLOR_FACE_SCARED);
            /* Open mouth (O) */
            renderer_draw_circle(hx, hy + 3, 2, COLOR_FACE_SCARED);
            break;

        default: /* FACE_DEAD */
            renderer_put_pixel(eye_lx, eye_y, COLOR_PENCIL);
            renderer_put_pixel(eye_rx, eye_y, COLOR_PENCIL);
            renderer_draw_line(hx - 2, hy + 3, hx + 2, hy + 2, COLOR_PENCIL);
            break;
    }
}

/*=============================================================================
 * DRAW RAGDOLL
 *===========================================================================*/
void renderer_draw_ragdoll(Ragdoll *doll, PhysicsWorld *world) {
    if (!doll->alive) return;

    /* Draw visible bones (skip cross-braces, indices 7-11) */
    for (int i = 0; i < 7; i++) {
        int ci = doll->bone_indices[i];
        Constraint *c = &world->constraints[ci];
        if (!c->active) continue;

        Particle *a = &world->particles[c->p1];
        Particle *b = &world->particles[c->p2];

        int x1 = FP_TO_INT(a->x);
        int y1 = FP_TO_INT(a->y);
        int x2 = FP_TO_INT(b->x);
        int y2 = FP_TO_INT(b->y);

        /* Spine is thicker */
        int thickness = (i == 3 || i == 4) ? 3 : 2;
        renderer_draw_thick_line(x1, y1, x2, y2, COLOR_PENCIL, thickness);
    }

    /* Draw joints as small circles */
    for (int i = 1; i < RAGDOLL_PARTS; i++) { /* Skip head, drawn separately */
        int pi = doll->part_indices[i];
        Particle *p = &world->particles[pi];
        if (!p->active) continue;

        int x = FP_TO_INT(p->x);
        int y = FP_TO_INT(p->y);
        int r = FP_TO_INT(p->radius);

        renderer_draw_circle_filled(x, y, r > 0 ? r : 2, COLOR_PENCIL);
    }

    /* Draw hands as small squares (doodle style) */
    for (int h = 0; h < 2; h++) {
        int hi = doll->part_indices[h == 0 ? RAGDOLL_LHAND : RAGDOLL_RHAND];
        Particle *p = &world->particles[hi];
        int hx = FP_TO_INT(p->x);
        int hy = FP_TO_INT(p->y);
        for (int dx = -2; dx <= 2; dx++) {
            for (int dy = -2; dy <= 2; dy++) {
                renderer_put_pixel(hx + dx, hy + dy, COLOR_PENCIL);
            }
        }
    }

    /* Draw feet as small triangles */
    for (int f = 0; f < 2; f++) {
        int fi = doll->part_indices[f == 0 ? RAGDOLL_LFOOT : RAGDOLL_RFOOT];
        Particle *p = &world->particles[fi];
        int fx = FP_TO_INT(p->x);
        int fy = FP_TO_INT(p->y);
        renderer_draw_line(fx - 3, fy + 2, fx + 3, fy + 2, COLOR_PENCIL);
        renderer_draw_line(fx - 3, fy + 2, fx, fy - 2, COLOR_PENCIL);
        renderer_draw_line(fx + 3, fy + 2, fx, fy - 2, COLOR_PENCIL);
    }

    /* Draw face on head */
    int hx, hy;
    ragdoll_get_head_pos(doll, world, &hx, &hy);
    renderer_draw_face(hx, hy, FP_TO_INT(HEAD_RADIUS), doll->expression);

    /* Draw pin indicators */
    for (int i = 0; i < RAGDOLL_PARTS; i++) {
        int pi = doll->part_indices[i];
        Particle *p = &world->particles[pi];
        if (p->pinned) {
            int px = FP_TO_INT(p->x);
            int py = FP_TO_INT(p->y);
            renderer_draw_circle_filled(px, py, 3, COLOR_PIN);
            renderer_draw_circle(px, py, 3, COLOR_PENCIL);
            /* Pin head */
            renderer_draw_circle_filled(px, py - 5, 2, COLOR_PIN);
        }
    }
}

/*=============================================================================
 * DRAW ALL PHYSICS OBJECTS (non-ragdoll constraints and particles)
 *===========================================================================*/
void renderer_draw_world(PhysicsWorld *world) {
    /* Draw constraints */
    for (int i = 0; i < world->constraint_count; i++) {
        Constraint *c = &world->constraints[i];
        if (!c->active) continue;

        Particle *a = &world->particles[c->p1];
        Particle *b = &world->particles[c->p2];

        int x1 = FP_TO_INT(a->x);
        int y1 = FP_TO_INT(a->y);
        int x2 = FP_TO_INT(b->x);
        int y2 = FP_TO_INT(b->y);

        if (c->is_elastic) {
            /* Draw rubber band as dashed line */
            renderer_draw_line(x1, y1, x2, y2, COLOR_ELASTIC);
        } else {
            renderer_draw_line(x1, y1, x2, y2, c->color);
        }
    }

    /* Draw standalone particles (user-drawn objects) */
    for (int i = 0; i < world->particle_count; i++) {
        Particle *p = &world->particles[i];
        if (!p->active) continue;

        int x = FP_TO_INT(p->x);
        int y = FP_TO_INT(p->y);
        int r = FP_TO_INT(p->radius);

        if (r > 3) {
            renderer_draw_circle_filled(x, y, r, p->color);
            renderer_draw_circle(x, y, r, COLOR_PENCIL);
        }
    }
}

/*=============================================================================
 * DRAW TOOLBAR
 *===========================================================================*/
void renderer_draw_toolbar(int active_tool) {
    /* Toolbar background bar at top of screen */
    int bar_y = 0;
    int bar_h = 11;

    for (int y = bar_y; y < bar_y + bar_h; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            renderer_put_pixel(x, y, COLOR_TOOLBAR_BG);
        }
    }

    /* Tool labels */
    const char *tools[] = { "GRAB", "DRAW", "PIN", "BAND", "ERASE" };
    int tool_w = SCREEN_WIDTH / 5;

    for (int i = 0; i < 5; i++) {
        u16 col = (i == active_tool) ? COLOR_TOOL_ACTIVE : COLOR_WHITE;
        int tx = i * tool_w + 4;
        renderer_draw_string(tx, bar_y + 2, tools[i], col);

        /* Active indicator */
        if (i == active_tool) {
            for (int x = i * tool_w; x < (i + 1) * tool_w; x++) {
                renderer_put_pixel(x, bar_y + bar_h - 1, COLOR_TOOL_ACTIVE);
            }
        }

        /* Separator */
        if (i < 4) {
            for (int y = bar_y + 2; y < bar_y + bar_h - 2; y++) {
                renderer_put_pixel((i + 1) * tool_w, y, COLOR_PENCIL_LIGHT);
            }
        }
    }
}

/*=============================================================================
 * DRAW TARGET (for missions)
 *===========================================================================*/
void renderer_draw_target(int x, int y, int radius) {
    renderer_draw_circle(x, y, radius, COLOR_TARGET);
    renderer_draw_circle(x, y, radius / 2, COLOR_TARGET);
    renderer_draw_circle_filled(x, y, 2, COLOR_TARGET);
    /* Crosshair */
    renderer_draw_line(x - radius - 2, y, x + radius + 2, y, COLOR_TARGET);
    renderer_draw_line(x, y - radius - 2, x, y + radius + 2, COLOR_TARGET);
}
