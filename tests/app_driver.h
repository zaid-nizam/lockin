#ifndef LOCKIN_TESTS_APP_DRIVER_H
#define LOCKIN_TESTS_APP_DRIVER_H

/* Drives the whole app the way a user does: clicks at window coordinates and
 * key presses, one frame per event, against the fake system. */

#include "app/app.h"
#include "fake_sys.h"

extern struct app app;

/* Places of the controls the tests click, in window pixels. */
enum {
    BOTTOM_Y = 452,
    STATS_X = 50,
    MISSED_X = 150,
    RIGHT_BUTTON_X = 518,
    BACK_X = 417,
    URGE_X = 350,
    URGE_Y = 100,
    WOOP_X = 300,
    WOOP_Y = 92,
    WOOP_STEP = 24,
};

/* Resets the fake system and opens the app with default settings. */
void driver_open(void);

/* Opens the app again without resetting the fake, as a restart would. */
void driver_reopen(void);

void driver_click(int x, int y);
void driver_key(int key);
void driver_type(const char *text);

/* Clicks into a text box and types. */
void driver_fill(int x, int y, const char *text);

/* Fills the start screen and locks in. */
void driver_lock(void);

/* From the session screen: opens the end form and fills it validly. */
void driver_fill_end_form(void);

/* A checksum of what is on screen, to tell two pictures apart. */
uint32_t driver_picture(void);

/* True when the rectangle shows at least one pixel of `color`. */
bool driver_shows(struct rect r, uint32_t color);

#endif
