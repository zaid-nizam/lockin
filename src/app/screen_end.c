#include "app/app.h"

#include <stdio.h>

enum { LABEL_X = 24, INPUT_X = 190 };

static void draw_text_rows(struct app *app) {
    struct ui *ui = &app->ui;
    struct form *f = &app->form;
    ui_label(ui, (struct point){LABEL_X, 67}, "Next step, for NEXT.md");
    ui_field(ui, (struct rect){INPUT_X, 64, 366, 21}, f->next, sizeof f->next);
    ui_label(ui, (struct point){LABEL_X, 93}, "What got done");
    ui_field(ui, (struct rect){INPUT_X, 90, 366, 21}, f->output, sizeof f->output);
}

static void draw_choices(struct app *app) {
    struct ui *ui = &app->ui;
    struct form *f = &app->form;
    static const char *const sounds[SOUND_COUNT] = {"Silence", "Pink noise", "Instrumental",
                                                    "Lyrics"};
    static const char *const ratings[] = {"1", "2", "3", "4", "5"};
    ui_label(ui, (struct point){LABEL_X, 120}, "Sound while working");
    for (int i = 0; i < SOUND_COUNT; i++) {
        ui_radio(ui, (struct point){INPUT_X + i * 92, 120}, sounds[i],
                 (struct choice){&f->sound, i});
    }
    ui_check(ui, (struct point){INPUT_X, 224}, "Worked in a silent co-work voice room", &f->cowork);
    ui_label(ui, (struct point){LABEL_X, 250}, "Urge to switch, 1 to 5");
    for (int i = 0; i < 5; i++) {
        ui_radio(ui, (struct point){INPUT_X + i * 40, 250}, ratings[i],
                 (struct choice){&f->urge_rating, i + 1});
    }
}

static void draw_number_rows(struct app *app) {
    struct ui *ui = &app->ui;
    struct form *f = &app->form;
    ui_label(ui, (struct point){LABEL_X, 147}, "Exercise before, minutes");
    ui_field(ui, (struct rect){INPUT_X, 144, 48, 21}, f->exercise, sizeof f->exercise);
    ui_label(ui, (struct point){INPUT_X + 56, 147}, "0 for none");
    ui_label(ui, (struct point){LABEL_X, 173}, "Caffeine, mg");
    ui_field(ui, (struct rect){INPUT_X, 170, 48, 21}, f->caffeine, sizeof f->caffeine);
    ui_label(ui, (struct point){INPUT_X + 56, 173}, "taken at");
    ui_field(ui, (struct rect){INPUT_X + 110, 170, 54, 21}, f->caffeine_at, sizeof f->caffeine_at);
    ui_label(ui, (struct point){INPUT_X + 172, 173}, "24 hour clock, like 10:30");
    ui_label(ui, (struct point){LABEL_X, 199}, "Went to bed at");
    ui_field(ui, (struct rect){INPUT_X, 196, 54, 21}, f->bedtime, sizeof f->bedtime);
    ui_label(ui, (struct point){INPUT_X + 62, 199}, "woke at");
    ui_field(ui, (struct rect){INPUT_X + 110, 196, 54, 21}, f->waketime, sizeof f->waketime);
}

static void draw_advice(struct app *app) {
    struct ui *ui = &app->ui;
    struct rect box = {12, 284, 556, 142};
    struct rect inside = {24, 300, 532, 120};
    if (app->form.problems[0] == '\0') {
        ui_group(ui, box, "Why stop here");
        ui_text(ui, inside,
                "Stop before the current sub-step is finished, and write down exactly what to "
                "type first next time. An unfinished task pulls you back to it, and a written "
                "first move makes starting cheap.");
        return;
    }
    ui_group(ui, box, "Fix these, then press Save");
    for (char *line = app->form.problems; *line != '\0';) {
        char one[120];
        int length = 0;
        sscanf(line, "%119[^\n]%n", one, &length);
        inside.y += ui_text(ui, inside, one);
        line += length + (line[length] == '\n');
    }
}

void screen_end(struct app *app) {
    struct ui *ui = &app->ui;
    ui_header(ui, "End session", "Stop mid-task. Write the exact next move, then log the session.");
    draw_text_rows(app);
    draw_choices(app);
    draw_number_rows(app);
    draw_advice(app);
    if (ui_button(ui, (struct rect){372, APP_BUTTON_Y, 90, APP_BUTTON_H}, "< Back", 0) ||
        ui_key(ui, KEY_ESCAPE)) {
        app_show(app, SCREEN_SESSION);
    }
    if (ui_button(ui, (struct rect){468, APP_BUTTON_Y, 100, APP_BUTTON_H}, "Save",
                  BUTTON_DEFAULT)) {
        action_save(app);
    }
}
