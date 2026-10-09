#include "app/app.h"

#include <stdio.h>

static void draw_timer(struct app *app) {
    struct ui *ui = &app->ui;
    char elapsed[16], urges[40];
    int seconds = session_seconds(&app->session, &app->now);
    snprintf(elapsed, sizeof elapsed, "%02d:%02d:%02d", seconds / 3600, seconds / 60 % 60,
             seconds % 60);
    snprintf(urges, sizeof urges, "Urges noticed and resisted: %d", app->session.urges);
    ui_label_big(ui, (struct point){24, 64}, elapsed);
    ui_label(ui, (struct point){300, 68}, urges);
    if (ui_button(ui, (struct rect){300, 88, 110, APP_BUTTON_H}, "Urge +1", 0)) {
        app->session.urges++;
    }
}

static void draw_commitment(struct app *app) {
    struct ui *ui = &app->ui;
    char title[40], line[100];
    int minutes = app->config.commit_minutes;
    int left = minutes * 60 - session_seconds(&app->session, &app->now);
    snprintf(title, sizeof title, "%d minute commitment", minutes);
    if (left > 0) {
        snprintf(line, sizeof line, "%d:%02d to go. That is all you promised.", left / 60,
                 left % 60);
    } else {
        snprintf(line, sizeof line, "Kept. Stop now with no guilt, or keep going.");
    }
    ui_group(ui, (struct rect){12, 126, 556, 58}, title);
    ui_progress(ui, (struct rect){24, 142, 532, 16},
                session_commit_permille(&app->session, &app->now, minutes));
    ui_label(ui, (struct point){24, 162}, line);
}

void screen_session(struct app *app) {
    struct ui *ui = &app->ui;
    char subtitle[120], started[6], stop[6];
    clock_format(app->session.started, started);
    clock_format(app->config.hard_stop, stop);
    if (session_past_hard_stop(moment_clock(&app->now), app->config.hard_stop)) {
        snprintf(subtitle, sizeof subtitle,
                 "Hard stop %s has passed. End the session and go to sleep.", stop);
    } else {
        snprintf(subtitle, sizeof subtitle, "%s slot, started %s. Hard stop at %s.",
                 slot_name(app->session.slot.id), started, stop);
    }
    ui_header(ui, "Locked in", subtitle);
    /* Drawn first so it, not "Urge +1", has focus when the screen opens. */
    if (ui_button(ui, (struct rect){448, APP_BUTTON_Y, 120, APP_BUTTON_H}, "End session...",
                  BUTTON_DEFAULT)) {
        app_show(app, SCREEN_END);
    }
    draw_timer(app);
    draw_commitment(app);
    panel_experiment(app, panel_card(app, panel_next(app, 194)));
}
