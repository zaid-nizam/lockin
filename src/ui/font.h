#ifndef LOCKIN_UI_FONT_H
#define LOCKIN_UI_FONT_H

#include <stdint.h>

enum { FONT_FIRST = 32, FONT_LAST = 126 };

/* One glyph. `offset` indexes the font's bit array; each row of the glyph is
 * padded to a whole byte. `left` and `top` place the bitmap relative to the
 * pen position on the baseline. */
struct glyph {
    uint16_t offset;
    uint8_t width;
    uint8_t height;
    int8_t left;
    int8_t top;
    uint8_t advance;
};

/* A bitmap font covering ASCII FONT_FIRST..FONT_LAST. */
struct font {
    int ascent;
    int descent;
    const struct glyph *glyphs;
    const uint8_t *bits;
};

extern const struct font font_regular;
extern const struct font font_bold;

#endif
