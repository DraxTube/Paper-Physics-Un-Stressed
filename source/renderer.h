#ifndef RENDERER_H
#define RENDERER_H

#include <nds.h>
#include "physics.h"
#include "ragdoll.h"

/*=============================================================================
 * NOTEBOOK COLORS (RGB555 for DS framebuffer)
 *===========================================================================*/
#define COLOR_PAPER       RGB15(30, 29, 26)    /* Warm cream paper */
#define COLOR_GRID         RGB15(22, 24, 28)    /* Light blue grid lines */
#define COLOR_MARGIN       RGB15(28, 12, 12)    /* Red margin line */
#define COLOR_PENCIL       RGB15(8, 8, 8)       /* Dark pencil */
#define COLOR_PENCIL_LIGHT RGB15(16, 16, 16)    /* Light pencil (cross-braces) */
#define COLOR_PIN          RGB15(28, 5, 5)      /* Red pin */
#define COLOR_ELASTIC      RGB15(5, 20, 28)     /* Blue rubber band */
#define COLOR_ERASER       RGB15(28, 22, 24)    /* Pink eraser trail */
#define COLOR_TARGET       RGB15(28, 18, 2)     /* Yellow/gold target */
#define COLOR_FACE_HAPPY   RGB15(10, 24, 10)    /* Green smiley */
#define COLOR_FACE_SCARED  RGB15(28, 8, 8)      /* Red scared */
#define COLOR_WHITE        RGB15(31, 31, 31)
#define COLOR_BLACK        RGB15(0, 0, 0)
#define COLOR_TOOLBAR_BG   RGB15(6, 6, 8)
#define COLOR_TOOL_ACTIVE  RGB15(28, 22, 5)     /* Gold highlight */

/*=============================================================================
 * FUNCTIONS
 *===========================================================================*/

/* Initialize the bottom screen framebuffer */
void renderer_init(void);

/* Clear the framebuffer with notebook background */
void renderer_clear(void);

/* Draw the notebook grid pattern */
void renderer_draw_notebook(void);

/* Draw all physics objects (particles, constraints) */
void renderer_draw_world(PhysicsWorld *world);

/* Draw the ragdoll with face expression */
void renderer_draw_ragdoll(Ragdoll *doll, PhysicsWorld *world);

/* Draw a line (Bresenham) */
void renderer_draw_line(int x1, int y1, int x2, int y2, u16 color);

/* Draw a thick line (for bones) */
void renderer_draw_thick_line(int x1, int y1, int x2, int y2, u16 color, int thickness);

/* Draw a filled circle */
void renderer_draw_circle_filled(int cx, int cy, int r, u16 color);

/* Draw a circle outline */
void renderer_draw_circle(int cx, int cy, int r, u16 color);

/* Draw a pixel (bounds-checked) */
void renderer_put_pixel(int x, int y, u16 color);

/* Draw the face expression on the head */
void renderer_draw_face(int hx, int hy, int radius, FaceExpression expr);

/* Draw the toolbar at the bottom */
void renderer_draw_toolbar(int active_tool);

/* Draw a target marker for missions */
void renderer_draw_target(int x, int y, int radius);

/* Draw text on the bottom screen (simple bitmap font) */
void renderer_draw_char(int x, int y, char c, u16 color);
void renderer_draw_string(int x, int y, const char *str, u16 color);

/* Get the framebuffer pointer */
u16* renderer_get_fb(void);

#endif /* RENDERER_H */
