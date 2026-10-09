#include "ui/canvas.h"

#include "core/text.h"

#include <stddef.h>
#include <string.h>

bool rect_holds(struct rect r, int x, int y) {
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

struct rect rect_inset(struct rect r, int by) {
    return (struct rect){r.x + by, r.y + by, r.w - 2 * by, r.h - 2 * by};
}

void canvas_fill(struct canvas *c, struct rect r, uint32_t color) {
    int left = r.x < 0 ? 0 : r.x;
    int top = r.y < 0 ? 0 : r.y;
    int right = r.x + r.w > c->width ? c->width : r.x + r.w;
    int bottom = r.y + r.h > c->height ? c->height : r.y + r.h;
    for (int y = top; y < bottom; y++) {
        for (int x = left; x < right; x++) {
            c->pixels[y * c->width + x] = color;
        }
    }
}

void canvas_edge(struct canvas *c, struct rect r, uint32_t lit, uint32_t shade) {
    canvas_fill(c, (struct rect){r.x, r.y, r.w, 1}, lit);
    canvas_fill(c, (struct rect){r.x, r.y, 1, r.h}, lit);
    canvas_fill(c, (struct rect){r.x, r.y + r.h - 1, r.w, 1}, shade);
    canvas_fill(c, (struct rect){r.x + r.w - 1, r.y, 1, r.h}, shade);
}

void canvas_dotted(struct canvas *c, struct rect r, uint32_t color) {
    for (int x = r.x; x < r.x + r.w; x += 2) {
        canvas_fill(c, (struct rect){x, r.y, 1, 1}, color);
        canvas_fill(c, (struct rect){x, r.y + r.h - 1, 1, 1}, color);
    }
    for (int y = r.y; y < r.y + r.h; y += 2) {
        canvas_fill(c, (struct rect){r.x, y, 1, 1}, color);
        canvas_fill(c, (struct rect){r.x + r.w - 1, y, 1, 1}, color);
    }
}

static const struct glyph *glyph_for(const struct font *font, char ch) {
    int code = ch < FONT_FIRST || ch > FONT_LAST ? '?' : ch;
    return &font->glyphs[code - FONT_FIRST];
}

static void draw_glyph(struct canvas *c, struct point pen, const struct text_style *style,
                       const struct glyph *g) {
    int stride = (g->width + 7) / 8;
    const uint8_t *bits = style->font->bits + g->offset;
    int zoom = style->zoom;
    for (int row = 0; row < g->height; row++) {
        for (int col = 0; col < g->width; col++) {
            if ((bits[row * stride + col / 8] & (0x80 >> (col % 8))) == 0) {
                continue;
            }
            int x = pen.x + (g->left + col) * zoom;
            int y = pen.y + (style->font->ascent - g->top + row) * zoom;
            canvas_fill(c, (struct rect){x, y, zoom, zoom}, style->color);
        }
    }
}

int canvas_text(struct canvas *c, struct point at, const struct text_style *style, const char *s) {
    struct point pen = at;
    while (*s != '\0') {
        const struct glyph *g = glyph_for(style->font, text_next_ascii(&s));
        draw_glyph(c, pen, style, g);
        pen.x += g->advance * style->zoom;
    }
    return pen.x - at.x;
}

int canvas_text_width(const struct text_style *style, const char *s) {
    int width = 0;
    while (*s != '\0') {
        width += glyph_for(style->font, text_next_ascii(&s))->advance * style->zoom;
    }
    return width;
}

int canvas_line_height(const struct text_style *style) {
    return (style->font->ascent + style->font->descent + 2) * style->zoom;
}

/* Length in bytes of the longest prefix of `s` that fits `width`, cut at a
 * space when one is available. Always at least one byte. */
static size_t fitting_prefix(const struct text_style *style, const char *s, int width) {
    char line[256];
    size_t best = 0;
    size_t len;
    for (len = 1; len < sizeof line && s[len - 1] != '\0'; len++) {
        memcpy(line, s, len);
        line[len] = '\0';
        if (canvas_text_width(style, line) > width) {
            return best > 0 ? best : len - 1 + (len == 1);
        }
        if (s[len] == ' ' || s[len] == '\0') {
            best = len;
        }
    }
    return best > 0 ? best : len - 1;
}

int canvas_text_wrapped(struct canvas *c, struct rect box, const struct text_style *style,
                        const char *s) {
    int line_height = canvas_line_height(style);
    int y = box.y;
    while (*s != '\0' && y + line_height <= box.y + box.h) {
        char line[256];
        size_t len = fitting_prefix(style, s, box.w);
        memcpy(line, s, len);
        line[len] = '\0';
        canvas_text(c, (struct point){box.x, y}, style, line);
        s += len;
        while (*s == ' ') {
            s++;
        }
        y += line_height;
    }
    return y - box.y;
}

static bool palette_color(const uint32_t *palette, char key, uint32_t *color) {
    for (const uint32_t *entry = palette; entry[0] != 0; entry += 2) {
        if (entry[0] == (uint32_t)key) {
            *color = entry[1];
            return true;
        }
    }
    return false;
}

void canvas_picture(struct canvas *c, struct point at, const char *const *rows,
                    const uint32_t *palette) {
    for (int y = 0; rows[y] != NULL; y++) {
        for (int x = 0; rows[y][x] != '\0'; x++) {
            uint32_t color;
            if (palette_color(palette, rows[y][x], &color)) {
                canvas_fill(c, (struct rect){at.x + x, at.y + y, 1, 1}, color);
            }
        }
    }
}
