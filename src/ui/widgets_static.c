#include "ui/ui.h"

#include "core/text.h"

#include <stddef.h>

static const struct text_style regular = {&font_regular, COLOR_TEXT, 1};
static const struct text_style bold = {&font_bold, COLOR_TEXT, 1};
static const struct text_style big = {&font_bold, COLOR_TEXT, 3};

static const char *const padlock[] = {
    "      ######      ",  "    ##wwwwww##    ", "   #ww######ww#   ",
    "  #ww#      #ww#  ",  "  #ws#      #ws#  ", "  #ws#      #ws#  ",
    "  #ws#      #ws#  ",  " ################ ", "#yyyyyyyyyyyyyyyy#",
    "#yggggggggggggggo#",  "#yggggg##gggggggo#", "#ygggg####ggggggo#",
    "#yggggg##gggggggo#",  "#yggggg##gggggggo#", "#yggggggggggggggo#",
    "#yoooooooooooooooo#", " ################ ", NULL,
};

static const uint32_t padlock_colors[] = {
    '#', COLOR_TEXT, 'w', COLOR_WINDOW, 's', COLOR_SHADOW, 'y', 0xFFE680,
    'g', COLOR_GOLD, 'o', 0x9C7600,     0,
};

void ui_label(struct ui *ui, struct point at, const char *text) {
    canvas_text(&ui->canvas, at, &regular, text);
}

void ui_label_bold(struct ui *ui, struct point at, const char *text) {
    canvas_text(&ui->canvas, at, &bold, text);
}

void ui_label_big(struct ui *ui, struct point at, const char *text) {
    canvas_text(&ui->canvas, at, &big, text);
}

int ui_text(struct ui *ui, struct rect box, const char *text) {
    return canvas_text_wrapped(&ui->canvas, box, &regular, text);
}

void ui_group(struct ui *ui, struct rect r, const char *title) {
    struct canvas *c = &ui->canvas;
    canvas_edge(c, r, COLOR_SHADOW, COLOR_HIGHLIGHT);
    canvas_edge(c, rect_inset(r, 1), COLOR_HIGHLIGHT, COLOR_SHADOW);
    int width = canvas_text_width(&regular, title);
    canvas_fill(c, (struct rect){r.x + 7, r.y, width + 6, 2}, COLOR_FACE);
    canvas_text(c, (struct point){r.x + 10, r.y - 7}, &regular, title);
}

void ui_header(struct ui *ui, const char *title, const char *subtitle) {
    struct canvas *c = &ui->canvas;
    canvas_fill(c, (struct rect){0, 0, c->width, UI_HEADER_HEIGHT - 2}, COLOR_WINDOW);
    canvas_fill(c, (struct rect){0, UI_HEADER_HEIGHT - 2, c->width, 1}, COLOR_SHADOW);
    canvas_fill(c, (struct rect){0, UI_HEADER_HEIGHT - 1, c->width, 1}, COLOR_HIGHLIGHT);
    canvas_text(c, (struct point){16, 8}, &bold, title);
    canvas_text(c, (struct point){28, 27}, &regular, subtitle);
    canvas_picture(c, (struct point){c->width - 36, 16}, padlock, padlock_colors);
}

void ui_status(struct ui *ui, const char *left, const char *right) {
    struct canvas *c = &ui->canvas;
    int top = c->height - UI_STATUS_HEIGHT + 2;
    enum { NARROW = 150 };
    int split = c->width - NARROW;
    struct rect wide = {2, top, split - 4, UI_STATUS_HEIGHT - 4};
    struct rect narrow = {split, top, NARROW - 2, UI_STATUS_HEIGHT - 4};
    canvas_edge(c, wide, COLOR_SHADOW, COLOR_HIGHLIGHT);
    canvas_edge(c, narrow, COLOR_SHADOW, COLOR_HIGHLIGHT);
    canvas_text(c, (struct point){wide.x + 5, top + 2}, &regular, left);
    canvas_text(c, (struct point){narrow.x + 5, top + 2}, &regular, right);
}

void ui_progress(struct ui *ui, struct rect r, int permille) {
    struct canvas *c = &ui->canvas;
    enum { BLOCK = 8, GAP = 2 };
    int inner = r.w - 4;
    int filled = inner * permille / 1000;
    canvas_edge(c, r, COLOR_SHADOW, COLOR_HIGHLIGHT);
    for (int x = 0; x + BLOCK <= filled || (permille >= 1000 && x < inner); x += BLOCK + GAP) {
        int width = x + BLOCK > inner ? inner - x : BLOCK;
        canvas_fill(c, (struct rect){r.x + 2 + x, r.y + 2, width, r.h - 4}, COLOR_NAVY);
    }
}

void ui_readout(struct ui *ui, struct rect r, const char *text) {
    struct canvas *c = &ui->canvas;
    canvas_edge(c, r, COLOR_SHADOW, COLOR_HIGHLIGHT);
    canvas_edge(c, rect_inset(r, 1), COLOR_DARK, COLOR_FACE);
    struct rect inner = rect_inset(r, 5);
    inner.y = r.y + (r.h - canvas_line_height(&regular)) / 2 + 1;
    inner.h = canvas_line_height(&regular);
    canvas_text_wrapped(c, inner, &regular, text);
}

void ui_column(struct ui *ui, struct rect r, const char *title) {
    struct canvas *c = &ui->canvas;
    canvas_edge(c, r, COLOR_HIGHLIGHT, COLOR_DARK);
    canvas_edge(c, rect_inset(r, 1), COLOR_FACE, COLOR_SHADOW);
    canvas_text(c, (struct point){r.x + 6, r.y + (r.h - canvas_line_height(&regular)) / 2 + 1},
                &regular, title);
}
