#include "app/app.h"

#include <stdlib.h>
#include <string.h>

void form_reset(struct form *form, const struct app *app) {
    memset(form, 0, sizeof *form);
    form->sound = app->variable == VAR_SOUND && app->coin == COIN_B ? SOUND_PINK : SOUND_SILENCE;
    form->cowork = app->variable == VAR_COWORK && app->coin == COIN_B;
    text_copy(form->exercise, sizeof form->exercise, "0");
    text_copy(form->caffeine, sizeof form->caffeine, "0");
    text_copy(form->bedtime, sizeof form->bedtime, app->state.bedtime);
    text_copy(form->waketime, sizeof form->waketime, app->state.waketime);
}

/* A whole number from 0 to `max`. An empty box counts as 0. */
static bool number(const char *typed, int max, int *value) {
    char *end;
    long parsed = strtol(typed, &end, 10);
    *value = (int)parsed;
    return *end == '\0' && parsed >= 0 && parsed <= max;
}

/* A time of day, or nothing. */
static bool optional_clock(const char *typed, int *minutes) {
    *minutes = CLOCK_NONE;
    return typed[0] == '\0' || clock_parse(typed, minutes);
}

bool form_check(struct form *form, struct record *row) {
    struct sbuf problems = sbuf_on(form->problems, sizeof form->problems);
    if (text_trim(form->next)[0] == '\0') {
        sbuf_put(&problems, "Next step is empty. Write the file, line and first thing to type.\n");
    }
    if (!number(form->exercise, 600, &row->exercise_min)) {
        sbuf_put(&problems, "Exercise must be whole minutes from 0 to 600.\n");
    }
    if (!number(form->caffeine, 2000, &row->caffeine_mg)) {
        sbuf_put(&problems, "Caffeine must be whole milligrams from 0 to 2000.\n");
    }
    if (!optional_clock(form->caffeine_at, &row->caffeine_at)) {
        sbuf_put(&problems, "Caffeine time must look like 10:30, or be left empty.\n");
    }
    if (!optional_clock(form->bedtime, &row->bedtime)) {
        sbuf_put(&problems, "Bedtime must look like 02:30, or be left empty.\n");
    }
    if (!optional_clock(form->waketime, &row->waketime)) {
        sbuf_put(&problems, "Wake time must look like 09:45, or be left empty.\n");
    }
    if (form->urge_rating < 1) {
        sbuf_put(&problems, "Pick how strong the urge to switch was, 1 to 5.\n");
    }
    row->sound = (enum sound)form->sound;
    row->cowork = form->cowork;
    row->urge_rating = form->urge_rating;
    text_copy(row->output, sizeof row->output, text_trim(form->output));
    return problems.len == 0;
}
