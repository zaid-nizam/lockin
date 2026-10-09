#include "app/app.h"

#include "sys/sys.h"

#include <stdio.h>

/* Says when the next slot is and what the experiment asks for it, so an
 * instruction like "walk first" is known before the slot starts. */
static void draw_next_slot(struct app *app, struct slot after, int top) {
    struct ui *ui = &app->ui;
    char line[80], planned[6];
    struct slot next = slot_after(after, app->config.slot_am, app->config.slot_pm);
    int week = experiment_week(app->rows[0].day, slot_day(next));
    enum variable variable = experiment_variable(week);
    clock_format(slot_clock(next), planned);
    snprintf(line, sizeof line, "Your next slot is %s at %s.", slot_name(next.id), planned);
    ui_group(ui, (struct rect){12, top, 556, 62}, "Next slot");
    ui_label(ui, (struct point){24, top + 16}, line);
    ui_label(ui, (struct point){24, top + 36},
             experiment_instruction(variable, experiment_coin(app->state.seed, next, variable)));
}

static void draw_exit_buttons(struct app *app, const char *label) {
    struct ui *ui = &app->ui;
    if (ui_button(ui, (struct rect){12, APP_BUTTON_Y, 84, APP_BUTTON_H}, "Stats...", 0)) {
        app_show(app, SCREEN_STATS);
    }
    if (ui_button(ui, (struct rect){448, APP_BUTTON_Y, 120, APP_BUTTON_H}, label, BUTTON_DEFAULT)) {
        if (app->blocked) {
            app_show(app, SCREEN_START);
        } else {
            action_close(app);
        }
    }
}

void screen_done(struct app *app) {
    struct ui *ui = &app->ui;
    char subtitle[120];
    snprintf(subtitle, sizeof subtitle, "%d focused min, started %d min late, %d urges resisted.",
             app->last.focused, app->last.latency, app->last.urges);
    ui_header(ui, "Session logged", subtitle);
    ui_group(ui, (struct rect){12, 66, 556, 96}, "Now");
    ui_text(ui, (struct rect){24, 82, 532, 76},
            "NEXT.md is written. Sites stay blocked until you shut down. Do something that "
            "needs no thinking for a while: an unfinished task keeps part of your attention, and "
            "the note you just wrote is what lets you put it down.");
    draw_next_slot(app, app->session.slot, 174);
    draw_exit_buttons(app, "New session");
}

void screen_missed(struct app *app) {
    struct ui *ui = &app->ui;
    ui_header(ui, "Slot missed", "Missed it. That is normal, and it is logged.");
    ui_group(ui, (struct rect){12, 66, 556, 96}, "What to do");
    ui_text(ui, (struct rect){24, 82, 532, 76},
            "Do not try again now. Starting at random times trains the failure; starting at "
            "the next slot trains the cue. Being hard on yourself makes the next start harder, "
            "so skip that part.");
    draw_next_slot(app, app->slot, 174);
    draw_exit_buttons(app, app->blocked ? "Back" : "Close");
}
