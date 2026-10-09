#include "app/app.h"

#include "sys/sys.h"

#include <stdio.h>
#include <string.h>

void app_plan_slot(struct app *app) {
    int64_t first_day;
    sys_now(&app->now);
    app->slot = slot_nearest(&app->now, app->config.slot_am, app->config.slot_pm);
    first_day = app->row_count > 0 ? app->rows[0].day : slot_day(app->slot);
    app->week = experiment_week(first_day, slot_day(app->slot));
    app->variable = experiment_variable(app->week);
    app->coin = experiment_coin(app->state.seed, app->slot, app->variable);
}

void app_open(struct app *app) {
    app->ui.canvas = (struct canvas){app->pixels, APP_WIDTH, APP_HEIGHT};
    app->ui.caret = true;
    store_state_load(&app->state);
    app->row_count = store_rows_load(app->rows, APP_ROWS);
    store_next_load(&app->config, app->next_step);
    app->blocked = sys_blocked();
    app_plan_slot(app);
    if (!sys_can_block()) {
        text_copy(app->notice, sizeof app->notice,
                  "Not running as root, so sites cannot be blocked. Start with: sudo lockin");
    }
    app_show(app, SCREEN_START);
}

void app_show(struct app *app, enum screen screen) {
    if (screen == SCREEN_STATS) {
        app->stats_return = app->screen;
    }
    app->screen = screen;
    ui_reset_focus(&app->ui);
}

void app_event(struct app *app, const struct pf_event *event) {
    if (event->kind == PF_CLOSE) {
        action_close(app);
    }
    ui_input(&app->ui, event);
}

void app_tick(struct app *app, bool blink) {
    if (app->screen == SCREEN_START) {
        /* The window may sit on this screen for hours: keep the slot current. */
        app_plan_slot(app);
    }
    sys_now(&app->now);
    if (blink) {
        app->ui.caret = !app->ui.caret;
    }
    if (app->blocked && app->now.epoch - app->swept_at >= APP_SWEEP_SECONDS) {
        app->swept_at = app->now.epoch;
        app->closed_programs += sys_close_programs(&app->config);
    }
}

static void draw_status(struct app *app) {
    char clock[6], right[40];
    const char *left = app->blocked ? "Sites blocked until shutdown" : "Sites not blocked yet";
    clock_format(moment_clock(&app->now), clock);
    snprintf(right, sizeof right, "%s   %d logged", clock, app->row_count);
    ui_status(&app->ui, app->notice[0] != '\0' ? app->notice : left, right);
}

void app_frame(struct app *app) {
    static void (*const screens[])(struct app *) = {
        [SCREEN_START] = screen_start,   [SCREEN_SESSION] = screen_session,
        [SCREEN_END] = screen_end,       [SCREEN_DONE] = screen_done,
        [SCREEN_MISSED] = screen_missed, [SCREEN_STATS] = screen_stats,
    };
    /* A click changes what should be on screen (a counter, the screen
     * itself) part-way through drawing. Draw again so the picture matches. */
    int passes = app->ui.released || app->ui.key_count > 0 ? 2 : 1;
    for (int pass = 0; pass < passes; pass++) {
        ui_begin(&app->ui);
        screens[app->screen](app);
        draw_status(app);
        ui_end(&app->ui);
    }
}
