#include "app/app.h"

#include <stdio.h>

enum { PANEL_X = 12, PANEL_W = 556, PANEL_GAP = 10 };

int panel_next(struct app *app, int top) {
    const char *empty = "No NEXT.md yet. You write one when you end this session.";
    ui_group(&app->ui, (struct rect){PANEL_X, top, PANEL_W, 46}, "Next step, from NEXT.md");
    ui_readout(&app->ui, (struct rect){PANEL_X + 12, top + 16, PANEL_W - 24, 21},
               app->next_step[0] != '\0' ? app->next_step : empty);
    return top + 46 + PANEL_GAP;
}

int panel_card(struct app *app, int top) {
    struct ui *ui = &app->ui;
    int y = top + 15;
    for (int i = 0; i < app->config.plan_count; i++) {
        struct rect line = {PANEL_X + 12, y, PANEL_W - 24, 30};
        y += ui_text(ui, line, app->config.plans[i]) + 3;
    }
    ui_group(ui, (struct rect){PANEL_X, top, PANEL_W, y - top + 5}, "If-then card");
    return y + 5 + PANEL_GAP;
}

int panel_experiment(struct app *app, int top) {
    char title[40];
    if (app->week <= 4) {
        snprintf(title, sizeof title, "Experiment, week %d of 4", app->week);
    } else {
        snprintf(title, sizeof title, "Experiment");
    }
    ui_group(&app->ui, (struct rect){PANEL_X, top, PANEL_W, 38}, title);
    ui_label(&app->ui, (struct point){PANEL_X + 12, top + 14},
             experiment_instruction(app->variable, app->coin));
    return top + 38 + PANEL_GAP;
}
