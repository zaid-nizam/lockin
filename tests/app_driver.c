#include "app_driver.h"

#include <string.h>

struct app app;

void driver_reopen(void) {
    struct config config = app.config;
    memset(&app, 0, sizeof app);
    app.config = config;
    app_open(&app);
    app_tick(&app, false);
    app_frame(&app);
}

void driver_open(void) {
    fake_reset();
    config_defaults(&app.config);
    driver_reopen();
}

static void step(const struct pf_event *event) {
    app_event(&app, event);
    app_tick(&app, false);
    app_frame(&app);
}

void driver_click(int x, int y) {
    step(&(struct pf_event){PF_MOVE, x, y, 0});
    step(&(struct pf_event){PF_PRESS, x, y, 0});
    step(&(struct pf_event){PF_RELEASE, x, y, 0});
}

void driver_key(int key) {
    step(&(struct pf_event){PF_KEY, 0, 0, key});
}

void driver_type(const char *text) {
    for (const char *c = text; *c != '\0'; c++) {
        driver_key(*c);
    }
}

void driver_fill(int x, int y, const char *text) {
    driver_click(x, y);
    driver_type(text);
}

void driver_lock(void) {
    driver_fill(WOOP_X, WOOP_Y, "Finish chapter 3");
    driver_fill(WOOP_X, WOOP_Y + 2 * WOOP_STEP, "Discord");
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
}

void driver_fill_end_form(void) {
    driver_click(RIGHT_BUTTON_X - 10, BOTTOM_Y);
    driver_fill(300, 74, "src/kilo.c:212 write editorScroll");
    driver_fill(300, 100, "rows draw, 1 commit");
    driver_click(196 + 2 * 40, 257);
    driver_fill(210, 206, "02:30");
    driver_fill(320, 206, "09:45");
}

uint32_t driver_picture(void) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < APP_WIDTH * APP_HEIGHT; i++) {
        hash = (hash ^ app.pixels[i]) * 16777619u;
    }
    return hash;
}

bool driver_shows(struct rect r, uint32_t color) {
    for (int y = r.y; y < r.y + r.h; y++) {
        for (int x = r.x; x < r.x + r.w; x++) {
            if (app.pixels[y * APP_WIDTH + x] == color) {
                return true;
            }
        }
    }
    return false;
}
