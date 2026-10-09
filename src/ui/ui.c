#include "ui/ui.h"

static void remember_key(struct ui *ui, int key) {
    if (ui->key_count < UI_KEYS) {
        ui->keys[ui->key_count++] = key;
    }
}

void ui_input(struct ui *ui, const struct pf_event *event) {
    switch (event->kind) {
    case PF_MOVE:
    case PF_PRESS:
    case PF_RELEASE:
        ui->mouse_x = event->x;
        ui->mouse_y = event->y;
        ui->pressed = event->kind == PF_PRESS;
        ui->released = event->kind == PF_RELEASE;
        ui->held = event->kind == PF_PRESS || (ui->held && event->kind == PF_MOVE);
        break;
    case PF_KEY:
        remember_key(ui, event->key);
        break;
    default:
        break;
    }
}

bool ui_key(const struct ui *ui, int key) {
    for (int i = 0; i < ui->key_count; i++) {
        if (ui->keys[i] == key) {
            return true;
        }
    }
    return false;
}

void ui_begin(struct ui *ui) {
    int count = ui->drawn_last_frame;
    if (count > 0 && ui_key(ui, KEY_TAB)) {
        ui->focus = (ui->focus + 1) % count;
    }
    if (count > 0 && ui_key(ui, KEY_BACKTAB)) {
        ui->focus = (ui->focus + count - 1) % count;
    }
    ui->drawn = 0;
    ui->button_focused = false;
    canvas_fill(&ui->canvas, (struct rect){0, 0, ui->canvas.width, ui->canvas.height}, COLOR_FACE);
}

void ui_end(struct ui *ui) {
    ui->drawn_last_frame = ui->drawn;
    ui->button_was_focused = ui->button_focused;
    ui->pressed = false;
    ui->released = false;
    ui->key_count = 0;
    if (!ui->held) {
        ui->active = -1;
    }
}

void ui_reset_focus(struct ui *ui) {
    ui->focus = 0;
    ui->active = -1;
    ui->button_was_focused = false;
}
