#ifndef LOCKIN_UI_CANVAS_H
#define LOCKIN_UI_CANVAS_H

/* A software canvas: every pixel lockin shows is drawn here into plain
 * memory, and the platform layer only copies the result to a window. That
 * keeps drawing identical on every OS and lets tests check exact pixels. */

#include "ui/font.h"

#include <stdbool.h>
#include <stdint.h>

/* Colours of the Windows 2000 "Windows Standard" scheme, as 0xRRGGBB. */
enum {
    COLOR_FACE = 0xD4D0C8,
    COLOR_HIGHLIGHT = 0xFFFFFF,
    COLOR_SHADOW = 0x808080,
    COLOR_DARK = 0x404040,
    COLOR_WINDOW = 0xFFFFFF,
    COLOR_TEXT = 0x000000,
    COLOR_NAVY = 0x0A246A,
    COLOR_GOLD = 0xE8B000,
};

struct point {
    int x;
    int y;
};

struct rect {
    int x;
    int y;
    int w;
    int h;
};

struct canvas {
    uint32_t *pixels;
    int width;
    int height;
};

/* How to draw text. `zoom` multiplies each font pixel; 1 is normal size. */
struct text_style {
    const struct font *font;
    uint32_t color;
    int zoom;
};

/* True when the point lies inside the rectangle. */
bool rect_holds(struct rect r, int x, int y);

/* Returns the rectangle shrunk by `by` pixels on every side. */
struct rect rect_inset(struct rect r, int by);

/* Fills a rectangle. Anything outside the canvas is clipped. */
void canvas_fill(struct canvas *c, struct rect r, uint32_t color);

/* Draws a one pixel border: `lit` on the top and left edges, `shade` on the
 * bottom and right. This is the building block of every 3D edge. */
void canvas_edge(struct canvas *c, struct rect r, uint32_t lit, uint32_t shade);

/* Draws the dotted rectangle that marks keyboard focus. */
void canvas_dotted(struct canvas *c, struct rect r, uint32_t color);

/* Draws one line of text with its top-left corner at `at` and returns its
 * width. Bytes outside ASCII draw as '?'. */
int canvas_text(struct canvas *c, struct point at, const struct text_style *style, const char *s);

/* Width in pixels that canvas_text would use. */
int canvas_text_width(const struct text_style *style, const char *s);

/* Height of one line in this style. */
int canvas_line_height(const struct text_style *style);

/* Draws text inside `box`, breaking at spaces, and returns the height used.
 * Lines that would fall below the box are not drawn. */
int canvas_text_wrapped(struct canvas *c, struct rect box, const struct text_style *style,
                        const char *s);

/* Draws a small picture given as rows of characters. Each character is
 * looked up in `palette` (pairs of character and 0xRRGGBB); characters not
 * in the palette are left transparent. */
void canvas_picture(struct canvas *c, struct point at, const char *const *rows,
                    const uint32_t *palette);

#endif
