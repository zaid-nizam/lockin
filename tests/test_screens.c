#include "app_driver.h"
#include "suites.h"
#include "test.h"

#include <string.h>

static const struct rect header = {0, 0, APP_WIDTH, UI_HEADER_HEIGHT};
static const struct rect commitment_end = {530, 144, 24, 12};

/* Loads `count` finished sessions under two conditions of one variable. */
static void log_sessions(int count, bool second_arm_better) {
    char text[8192];
    struct sbuf sb = sbuf_on(text, sizeof text);
    for (int i = 0; i < count; i++) {
        bool second = i % 2 == 1;
        int focus = second && second_arm_better ? 90 : 40;
        sbuf_putf(&sb, "2026-10-%02d,AM,done,11:00,11:02,2,%d,0,3,%s,%d,0,,%s,,%d,1,1,N,\n",
                  1 + i % 12, focus, second ? "pink" : "silence", second ? 15 : 0,
                  second ? "02:30" : "23:30", second);
    }
    fake_file_set("/data/sessions.csv", text);
}

static void the_start_screen_changes_with_lateness_and_next_md(void) {
    driver_open();
    uint32_t late = driver_picture();
    fake.now.minute = 0;
    driver_reopen();
    uint32_t on_time = driver_picture();
    fake_file_set("/data/NEXT.md", "src/cpu.c:212 ADC flag\n");
    driver_reopen();
    CHECK(late != on_time);
    CHECK(on_time != driver_picture());
    CHECK(driver_shows(header, COLOR_GOLD));
}

static void the_experiment_panel_follows_the_week(void) {
    uint32_t pictures[3];
    static const char *const first_rows[] = {
        "2026-10-12,PM,missed,23:00,,0,0,0,0,silence,0,0,,,,0,0,1,N,\n",
        "2026-10-04,PM,missed,23:00,,0,0,0,0,silence,0,0,,,,0,0,1,N,\n",
        "2026-08-01,PM,missed,23:00,,0,0,0,0,silence,0,0,,,,0,0,1,N,\n",
    };
    for (int i = 0; i < 3; i++) {
        fake_reset();
        fake_file_set("/data/sessions.csv", first_rows[i]);
        config_defaults(&app.config);
        driver_reopen();
        pictures[i] = driver_picture();
    }
    CHECK_INT(app.variable, VAR_FREE);
    CHECK(pictures[0] != pictures[1] && pictures[1] != pictures[2]);
}

static void the_commitment_bar_fills_over_ten_minutes(void) {
    driver_open();
    driver_lock();
    CHECK(!driver_shows(commitment_end, COLOR_NAVY));
    fake_advance(5 * 60);
    driver_key(KEY_TAB);
    CHECK(!driver_shows(commitment_end, COLOR_NAVY));
    uint32_t counting = driver_picture();
    fake_advance(5 * 60);
    driver_key(KEY_TAB);
    CHECK(driver_shows(commitment_end, COLOR_NAVY));
    CHECK(counting != driver_picture());
}

static void the_session_header_warns_after_the_hard_stop(void) {
    driver_open();
    driver_lock();
    uint32_t before = driver_picture();
    fake_advance(3 * 60 * 60);
    driver_key(KEY_TAB);
    driver_key(KEY_BACKTAB);
    CHECK(before != driver_picture());
}

static void the_end_form_swaps_advice_for_problems(void) {
    static const struct rect advice = {24, 300, 532, 120};
    driver_open();
    driver_lock();
    driver_click(RIGHT_BUTTON_X - 10, BOTTOM_Y);
    uint32_t advice_shown = driver_picture();
    CHECK(driver_shows(advice, COLOR_TEXT));
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK(advice_shown != driver_picture());
}

static void done_and_missed_screens_name_the_next_slot(void) {
    driver_open();
    driver_click(MISSED_X, BOTTOM_Y);
    uint32_t missed = driver_picture();
    driver_open();
    driver_lock();
    driver_fill_end_form();
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_DONE);
    CHECK(missed != driver_picture());
    CHECK(driver_shows((struct rect){24, 180, 400, 50}, COLOR_TEXT));
}

static void stats_shows_an_empty_state_then_fills_in(void) {
    static const struct rect empty_note = {12, 364, 556, 44};
    driver_open();
    driver_click(STATS_X, BOTTOM_Y);
    CHECK(driver_shows(empty_note, COLOR_TEXT));
    uint32_t empty = driver_picture();

    fake_reset();
    log_sessions(6, false);
    config_defaults(&app.config);
    driver_reopen();
    driver_click(STATS_X, BOTTOM_Y);
    CHECK(!driver_shows(empty_note, COLOR_TEXT));
    CHECK(empty != driver_picture());
}

static void stats_words_each_kind_of_verdict(void) {
    uint32_t pictures[3];
    static const int counts[] = {6, 16, 16};
    static const bool better[] = {false, false, true};
    for (int i = 0; i < 3; i++) {
        fake_reset();
        log_sessions(counts[i], better[i]);
        config_defaults(&app.config);
        driver_reopen();
        driver_click(STATS_X, BOTTOM_Y);
        pictures[i] = driver_picture();
    }
    CHECK(pictures[0] != pictures[1] && pictures[1] != pictures[2]);
}

void suite_screens(void) {
    RUN(the_start_screen_changes_with_lateness_and_next_md);
    RUN(the_experiment_panel_follows_the_week);
    RUN(the_commitment_bar_fills_over_ten_minutes);
    RUN(the_session_header_warns_after_the_hard_stop);
    RUN(the_end_form_swaps_advice_for_problems);
    RUN(done_and_missed_screens_name_the_next_slot);
    RUN(stats_shows_an_empty_state_then_fills_in);
    RUN(stats_words_each_kind_of_verdict);
}
