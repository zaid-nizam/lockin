#include "app/app.h"

#include "core/stats.h"

#include <stdio.h>

enum { TABLE_X = 12, TABLE_Y = 66, ROW_H = 18 };

static const int columns[] = {0, 96, 216, 300, 428, 556};
static const char *const headings[] = {"Tested", "Condition", "Sessions", "Start late, median",
                                       "Focus, median"};
static const char *const tested[] = {
    [VAR_SOUND] = "Sound", [VAR_EXERCISE] = "Exercise", [VAR_COWORK] = "Co-work"};

static void draw_cells(struct app *app, int y, const char *const cells[5]) {
    for (int i = 0; i < 5; i++) {
        ui_label(&app->ui, (struct point){TABLE_X + columns[i] + 6, y}, cells[i]);
    }
}

static void draw_arm(struct app *app, int y, enum variable variable, int which) {
    struct comparison c = stats_compare(app->rows, app->row_count, variable);
    const struct arm *arm = which == 0 ? &c.a : &c.b;
    char sessions[12], latency[16] = "-", focus[16] = "-";
    snprintf(sessions, sizeof sessions, "%d", arm->slots);
    if (arm->slots > 0) {
        snprintf(latency, sizeof latency, "%d min", arm->latency_median);
        snprintf(focus, sizeof focus, "%d min", arm->focus_median);
    }
    const char *const cells[5] = {which == 0 ? tested[variable] : "",
                                  stats_condition(variable, which), sessions, latency, focus};
    draw_cells(app, y, cells);
}

static void verdict_line(const struct app *app, enum variable variable, char *out, size_t cap) {
    struct comparison c = stats_compare(app->rows, app->row_count, variable);
    switch (c.verdict) {
    case VERDICT_KEEP_A:
    case VERDICT_KEEP_B:
        snprintf(out, cap, "%s: keep \"%s\". It wins by %d%% or more.", tested[variable],
                 stats_condition(variable, c.verdict == VERDICT_KEEP_B), STATS_MARGIN_PERCENT);
        break;
    case VERDICT_NO_DIFFERENCE:
        snprintf(out, cap, "%s: no clear difference. Under %d%% is probably noise.",
                 tested[variable], STATS_MARGIN_PERCENT);
        break;
    default:
        snprintf(out, cap, "%s: not enough yet, %d of %d sessions (%d needed per condition).",
                 tested[variable], c.a.slots + c.b.slots, STATS_MIN_SLOTS, STATS_MIN_PER_ARM);
        break;
    }
}

static void draw_table(struct app *app) {
    struct ui *ui = &app->ui;
    struct rect body = {TABLE_X, TABLE_Y + 20, 556, 6 * ROW_H + 6};
    for (int i = 0; i < 5; i++) {
        ui_column(ui, (struct rect){TABLE_X + columns[i], TABLE_Y, columns[i + 1] - columns[i], 20},
                  headings[i]);
    }
    canvas_fill(&ui->canvas, body, COLOR_WINDOW);
    canvas_edge(&ui->canvas, body, COLOR_SHADOW, COLOR_HIGHLIGHT);
    for (int row = 0; row < 6; row++) {
        draw_arm(app, body.y + 4 + row * ROW_H, (enum variable)(VAR_SOUND + row / 2), row % 2);
    }
    for (int i = 0; i < 3; i++) {
        char line[120];
        verdict_line(app, (enum variable)(VAR_SOUND + i), line, sizeof line);
        ui_label(ui, (struct point){TABLE_X + 4, body.y + body.h + 8 + i * ROW_H}, line);
    }
}

static void draw_targets(struct app *app) {
    struct ui *ui = &app->ui;
    struct targets t = stats_targets(app->rows, app->row_count, slot_day(app->slot));
    char line[120];
    int percent = t.slots > 0 ? t.on_time * 100 / t.slots : 0;
    ui_group(ui, (struct rect){12, 268, 556, 84}, "Month one targets, last 30 days");
    snprintf(line, sizeof line, "Started within %d min: %d of %d slots, %d%%. Target 70%%.",
             STATS_ON_TIME_MINUTES, t.on_time, t.slots, percent);
    ui_label(ui, (struct point){24, 284}, line);
    snprintf(line, sizeof line, "Sessions of %d min or longer: %d. Target 10.",
             STATS_LONG_SESSION_MINUTES, t.long_sessions);
    ui_label(ui, (struct point){24, 304}, line);
    if (t.bedtime_spread == STATS_NO_DATA) {
        snprintf(line, sizeof line,
                 "Bedtime spread: log two bedtimes to see it. Target under 2 h.");
    } else {
        snprintf(line, sizeof line, "Bedtime spread: %d h %02d min. Target under 2 h.",
                 t.bedtime_spread / 60, t.bedtime_spread % 60);
    }
    ui_label(ui, (struct point){24, 324}, line);
}

void screen_stats(struct app *app) {
    struct ui *ui = &app->ui;
    ui_header(ui, "Stats", "Your own numbers decide what stays. Missed slots are left out.");
    draw_table(app);
    draw_targets(app);
    if (app->row_count == 0) {
        ui_group(ui, (struct rect){12, 364, 556, 44}, "Nothing logged yet");
        ui_label(ui, (struct point){24, 380},
                 "Finish one session and this page starts to fill in.");
    }
    if (ui_button(ui, (struct rect){468, APP_BUTTON_Y, 100, APP_BUTTON_H}, "Back",
                  BUTTON_DEFAULT) ||
        ui_key(ui, KEY_ESCAPE)) {
        app_show(app, app->stats_return);
    }
}
