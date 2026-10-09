#include "app/app.h"

#include "sys/sys.h"

#include <stdio.h>

static void describe_slot(const struct app *app, char *out, size_t cap) {
    char planned[6], now[6];
    int late = slot_latency(app->slot, &app->now);
    clock_format(slot_clock(app->slot), planned);
    clock_format(moment_clock(&app->now), now);
    if (late == 0) {
        snprintf(out, cap, "%s slot, planned for %s. It is %s: on time.", slot_name(app->slot.id),
                 planned, now);
    } else {
        snprintf(out, cap, "%s slot, planned for %s. It is %s: %d min late.",
                 slot_name(app->slot.id), planned, now, late);
    }
}

static void draw_woop(struct app *app) {
    struct ui *ui = &app->ui;
    struct woop *w = &app->state.woop;
    struct {
        const char *label;
        char *text;
        size_t cap;
    } rows[] = {
        {"Wish", w->wish, sizeof w->wish},
        {"Best outcome", w->outcome, sizeof w->outcome},
        {"Obstacle in me", w->obstacle, sizeof w->obstacle},
        {"If... then I will", w->plan, sizeof w->plan},
    };
    ui_group(ui, (struct rect){12, 66, 556, 116}, "WOOP, 60 seconds");
    for (int i = 0; i < 4; i++) {
        int y = 82 + i * 24;
        ui_label(ui, (struct point){24, y + 3}, rows[i].label);
        ui_field(ui, (struct rect){130, y, 426, 21}, rows[i].text, rows[i].cap);
    }
}

void screen_start(struct app *app) {
    struct ui *ui = &app->ui;
    char subtitle[120];
    /* Not the default button: Enter in a text box must never start a lock
     * that only a reboot ends. */
    enum button_flags lock = sys_can_block() ? BUTTON_PLAIN : BUTTON_DISABLED;
    describe_slot(app, subtitle, sizeof subtitle);
    ui_header(ui, "Start session", subtitle);
    draw_woop(app);
    panel_experiment(app, panel_card(app, panel_next(app, 192)));
    if (ui_button(ui, (struct rect){12, APP_BUTTON_Y, 84, APP_BUTTON_H}, "Stats...", 0)) {
        app_show(app, SCREEN_STATS);
    }
    if (ui_button(ui, (struct rect){104, APP_BUTTON_Y, 100, APP_BUTTON_H}, "Missed slot", 0)) {
        action_missed(app);
    }
    if (ui_button(ui, (struct rect){468, APP_BUTTON_Y, 100, APP_BUTTON_H}, "Lock in", lock)) {
        action_lock(app);
    }
}
