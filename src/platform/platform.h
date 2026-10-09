#ifndef LOCKIN_PLATFORM_PLATFORM_H
#define LOCKIN_PLATFORM_PLATFORM_H

/* The window. lockin draws every pixel itself, so all it asks of a platform
 * is: open one fixed-size window, hand over input, and copy a pixel buffer to
 * the screen. One implementation per window system: platform_x11.c today.
 * Porting to Windows or macOS means writing these four functions for it.
 *
 * A process has one window, so the functions take no handle. */

#include "core/err.h"

#include <stdint.h>

enum pf_kind { PF_REDRAW, PF_CLOSE, PF_MOVE, PF_PRESS, PF_RELEASE, PF_KEY };

/* Key codes: printable ASCII stands for itself; these are the rest. */
enum { KEY_BACKSPACE = 8, KEY_TAB = 9, KEY_ENTER = 13, KEY_ESCAPE = 27, KEY_BACKTAB = 256 };

struct pf_event {
    enum pf_kind kind;
    int x;
    int y;
    int key;
};

/* `width` and `height` are in canvas pixels. `scale` is how many screen
 * pixels each one covers, 1 to PF_SCALE_MAX, or 0 to choose from the screen
 * size. */
struct pf_window {
    const char *title;
    int width;
    int height;
    int scale;
};

enum { PF_SCALE_MAX = 3 };

/* Opens the window. On failure `err` says how to fix the display set-up. */
bool pf_open(const struct pf_window *window, struct error *err);

/* Closes the window and frees what pf_open took. */
void pf_close(void);

/* Waits up to `timeout_ms` for input. Returns false when the time ran out.
 * Mouse positions are in canvas pixels. */
bool pf_wait(int timeout_ms, struct pf_event *event);

/* Copies a full canvas (width * height pixels, 0xRRGGBB) to the window. */
void pf_show(const uint32_t *pixels);

#endif
