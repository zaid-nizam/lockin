#include "ui/ui.h"

#include "core/text.h"

#include <string.h>

static const struct text_style regular = {&font_regular, COLOR_TEXT, 1};
static const struct text_style greyed = {&font_regular, COLOR_SHADOW, 1};
static const struct text_style embossed = {&font_regular, COLOR_HIGHLIGHT, 1};

static const char *const tick[] = {
    "      #", "     ##", "#   ###", "## ### ", "#####  ", " ###   ", "  #    ", NULL,
};

static const char *const radio_ring[] = {
    "    ssss    ", "  ssddddss  ", " sddwwwwddw ", " sdwwwwwwfw ", "sdwwwwwwwwfw",
    "sdwwwwwwwwfw", "sdwwwwwwwwfw", "sdwwwwwwwwfw", " sdwwwwwwfw ", " sffwwwwffw ",
    "  wwffffww  ", "    wwww    ", NULL,
};

static const char *const radio_dot[] = {" ## ", "####", "####", " ## ", NULL};

static const uint32_t ink[] = {
    '#', COLOR_TEXT, 's', COLOR_SHADOW, 'd', COLOR_DARK, 'w', COLOR_WINDOW, 'f', COLOR_FACE, 0,
};

/* What the user did to one focusable control this frame. */
struct touch {
    bool focused;
    bool clicked;
    bool held;
};

/* Registers a control occupying `r`: gives it the next place in the focus
 * order, moves focus to it on a mouse press, and reports a completed click. */
static struct touch touch(struct ui *ui, struct rect r) {
    int id = ui->drawn++;
    bool inside = rect_holds(r, ui->mouse_x, ui->mouse_y);
    if (ui->pressed && inside) {
        ui->focus = id;
        ui->active = id;
    }
    struct touch t = {ui->focus == id, ui->released && ui->active == id && inside,
                      ui->held && ui->active == id && inside};
    return t;
}

static void draw_button(struct ui *ui, struct rect r, bool is_default, bool down) {
    struct canvas *c = &ui->canvas;
    canvas_fill(c, r, COLOR_FACE);
    if (is_default) {
        canvas_edge(c, r, COLOR_TEXT, COLOR_TEXT);
        r = rect_inset(r, 1);
    }
    if (down) {
        canvas_edge(c, r, COLOR_SHADOW, COLOR_SHADOW);
        return;
    }
    canvas_edge(c, r, COLOR_HIGHLIGHT, COLOR_DARK);
    canvas_edge(c, rect_inset(r, 1), COLOR_FACE, COLOR_SHADOW);
}

static void draw_button_label(struct ui *ui, struct rect r, const char *label, bool disabled) {
    struct canvas *c = &ui->canvas;
    struct point at = {r.x + (r.w - canvas_text_width(&regular, label)) / 2,
                       r.y + (r.h - canvas_line_height(&regular)) / 2 + 1};
    if (disabled) {
        canvas_text(c, (struct point){at.x + 1, at.y + 1}, &embossed, label);
        canvas_text(c, at, &greyed, label);
    } else {
        canvas_text(c, at, &regular, label);
    }
}

/* True when the keyboard pressed this button: Space or Enter with focus on
 * it, or Enter for the default button while focus is on a non-button. */
static bool keyed(const struct ui *ui, bool focused, bool is_default) {
    if (focused) {
        return ui_key(ui, ' ') || ui_key(ui, KEY_ENTER);
    }
    return is_default && !ui->button_was_focused && ui_key(ui, KEY_ENTER);
}

bool ui_button(struct ui *ui, struct rect r, const char *label, enum button_flags flags) {
    bool disabled = (flags & BUTTON_DISABLED) != 0;
    bool is_default = (flags & BUTTON_DEFAULT) != 0;
    struct touch t = touch(ui, r);
    bool down = t.held && !disabled;
    struct rect face = down ? (struct rect){r.x + 1, r.y + 1, r.w, r.h} : r;
    draw_button(ui, r, is_default || t.focused, down);
    draw_button_label(ui, face, label, disabled);
    if (t.focused) {
        ui->button_focused = true;
        canvas_dotted(&ui->canvas, rect_inset(r, 4), COLOR_TEXT);
    }
    return !disabled && (t.clicked || keyed(ui, t.focused, is_default));
}

static void edit(struct ui *ui, char *text, size_t cap) {
    size_t len = strlen(text);
    for (int i = 0; i < ui->key_count; i++) {
        int key = ui->keys[i];
        if (key == KEY_BACKSPACE && len > 0) {
            text[--len] = '\0';
        } else if (key >= ' ' && key <= '~' && len + 1 < cap) {
            text[len++] = (char)key;
            text[len] = '\0';
        }
    }
}

void ui_field(struct ui *ui, struct rect r, char *text, size_t cap) {
    struct canvas *c = &ui->canvas;
    struct touch t = touch(ui, r);
    if (t.focused) {
        edit(ui, text, cap);
    }
    canvas_edge(c, r, COLOR_SHADOW, COLOR_HIGHLIGHT);
    canvas_edge(c, rect_inset(r, 1), COLOR_DARK, COLOR_FACE);
    canvas_fill(c, rect_inset(r, 2), COLOR_WINDOW);
    /* The caret sits at the end, so when the text is too wide show its tail. */
    const char *visible = text;
    while (canvas_text_width(&regular, visible) > r.w - 10) {
        text_next_ascii(&visible);
    }
    struct point at = {r.x + 4, r.y + (r.h - canvas_line_height(&regular)) / 2 + 1};
    int width = canvas_text(c, at, &regular, visible);
    if (t.focused && ui->caret) {
        canvas_fill(c, (struct rect){at.x + width + 1, r.y + 4, 1, r.h - 8}, COLOR_TEXT);
    }
}

/* Draws a label beside a 13 pixel control and returns the area that reacts
 * to the mouse: the control and its label together. */
static struct rect labelled(struct ui *ui, struct point at, const char *label) {
    int width = canvas_text(&ui->canvas, (struct point){at.x + 18, at.y}, &regular, label);
    return (struct rect){at.x, at.y, 18 + width, 14};
}

void ui_check(struct ui *ui, struct point at, const char *label, bool *on) {
    struct canvas *c = &ui->canvas;
    struct rect area = labelled(ui, at, label);
    struct rect box = {at.x, at.y + 1, 13, 13};
    struct touch t = touch(ui, area);
    if (t.clicked || (t.focused && ui_key(ui, ' '))) {
        *on = !*on;
    }
    canvas_edge(c, box, COLOR_SHADOW, COLOR_HIGHLIGHT);
    canvas_edge(c, rect_inset(box, 1), COLOR_DARK, COLOR_FACE);
    canvas_fill(c, rect_inset(box, 2), COLOR_WINDOW);
    if (*on) {
        canvas_picture(c, (struct point){box.x + 3, box.y + 3}, tick, ink);
    }
    if (t.focused) {
        canvas_dotted(c, (struct rect){at.x + 16, at.y, area.w - 15, 15}, COLOR_TEXT);
    }
}

void ui_radio(struct ui *ui, struct point at, const char *label, struct choice choice) {
    struct canvas *c = &ui->canvas;
    struct rect area = labelled(ui, at, label);
    struct touch t = touch(ui, area);
    if (t.clicked || (t.focused && ui_key(ui, ' '))) {
        *choice.value = choice.mine;
    }
    canvas_picture(c, (struct point){at.x, at.y + 1}, radio_ring, ink);
    if (*choice.value == choice.mine) {
        canvas_picture(c, (struct point){at.x + 4, at.y + 5}, radio_dot, ink);
    }
    if (t.focused) {
        canvas_dotted(c, (struct rect){at.x + 16, at.y, area.w - 15, 15}, COLOR_TEXT);
    }
}
