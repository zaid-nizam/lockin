#ifndef LOCKIN_APP_APP_H
#define LOCKIN_APP_APP_H

/* The program itself: which screen is showing, the session in progress and
 * everything loaded from disk. One struct, passed everywhere, no globals. */

#include "app/store.h"
#include "core/experiment.h"
#include "core/session.h"
#include "ui/ui.h"

#define LOCKIN_VERSION "1.0.0"

enum { APP_WIDTH = 580, APP_HEIGHT = 500, APP_ROWS = 1024, APP_SWEEP_SECONDS = 2 };

enum screen { SCREEN_START, SCREEN_SESSION, SCREEN_END, SCREEN_DONE, SCREEN_MISSED, SCREEN_STATS };

/* The end-of-session form, held as typed until Save checks it. */
struct form {
    char next[STORE_NEXT_MAX];
    char output[RECORD_OUTPUT_MAX + 1];
    int sound;
    char exercise[5];
    char caffeine[6];
    char caffeine_at[6];
    char bedtime[6];
    char waketime[6];
    bool cowork;
    int urge_rating;
    char problems[480];
};

struct app {
    struct ui ui;
    struct config config;
    struct state state;
    struct record rows[APP_ROWS];
    int row_count;
    enum screen screen;
    enum screen stats_return;
    struct moment now;
    struct slot slot;
    int week;
    enum variable variable;
    char coin;
    struct session session;
    struct form form;
    struct record last;
    char next_step[STORE_NEXT_MAX];
    char notice[200];
    int64_t swept_at;
    int closed_programs;
    bool blocked;
    bool quit;
    uint32_t pixels[APP_WIDTH * APP_HEIGHT];
};

/* Loads state and the log and puts the app on the start screen. The settings
 * in `app->config` must already be loaded. */
void app_open(struct app *app);

/* Handles one input event. */
void app_event(struct app *app, const struct pf_event *event);

/* Advances time: refreshes the clock and the current slot, blinks the caret
 * when `blink`, and while sites are blocked closes the listed programs every
 * few seconds. Call it before every app_frame. */
void app_tick(struct app *app, bool blink);

/* Draws the current screen into `app->pixels` and acts on what was clicked. */
void app_frame(struct app *app);

/* Switches screens and puts focus on the new screen's first control. */
void app_show(struct app *app, enum screen screen);

/* Shows an error in the status bar and writes it to the log. */
void app_fail(struct app *app, const struct error *err);

/* Works out the slot, week and coin for the current time. */
void app_plan_slot(struct app *app);

/* What the buttons do. Each is safe to call twice. */
void action_lock(struct app *app);
void action_missed(struct app *app);
void action_save(struct app *app);
void action_close(struct app *app);

/* Checks the form and fills `row`. Every problem goes into `form->problems`,
 * one per line; returns true when there are none. */
bool form_check(struct form *form, struct record *row);

/* Clears the form and fills in what is already known. */
void form_reset(struct form *form, const struct app *app);

void screen_start(struct app *app);
void screen_session(struct app *app);
void screen_end(struct app *app);
void screen_done(struct app *app);
void screen_missed(struct app *app);
void screen_stats(struct app *app);

/* Panels shown on more than one screen. Each is drawn with its top edge at
 * `top` and returns where the next panel should start. */
int panel_next(struct app *app, int top);
int panel_card(struct app *app, int top);
int panel_experiment(struct app *app, int top);

/* Rows of buttons along the bottom share one line. */
enum { APP_BUTTON_Y = 440, APP_BUTTON_H = 26 };

#endif
