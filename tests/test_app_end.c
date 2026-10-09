#include "app_driver.h"
#include "suites.h"
#include "test.h"

#include <string.h>

static void saving_an_empty_form_lists_every_problem(void) {
    driver_open();
    driver_lock();
    driver_click(RIGHT_BUTTON_X - 10, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_END);
    driver_fill(210, 154, "\b-5");
    driver_fill(210, 180, "9999");
    driver_fill(320, 180, "25:00");
    driver_fill(210, 206, "late");
    driver_fill(320, 206, "7");
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_END);
    CHECK_HAS(app.form.problems, "Next step is empty");
    CHECK_HAS(app.form.problems, "Exercise must be");
    CHECK_HAS(app.form.problems, "Caffeine must be");
    CHECK_HAS(app.form.problems, "Caffeine time must");
    CHECK_HAS(app.form.problems, "Bedtime must");
    CHECK_HAS(app.form.problems, "Wake time must");
    CHECK_HAS(app.form.problems, "Pick how strong");
    CHECK(fake_file("/data/sessions.csv") == NULL);
}

static void a_valid_form_writes_next_md_and_one_log_row(void) {
    driver_open();
    driver_lock();
    driver_click(URGE_X, URGE_Y);
    fake_advance(74 * 60);
    driver_fill_end_form();
    driver_click(195 + 92, 127);
    driver_click(196, 231);
    driver_fill(210, 154, "\b15");
    driver_fill(210, 180, "\b100");
    driver_fill(320, 180, "10:30");
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_DONE);
    CHECK_STR(fake_file("/data/NEXT.md"), "src/kilo.c:212 write editorScroll\n");
    CHECK_HAS(fake_file("/data/sessions.csv"), record_header);
    CHECK_HAS(fake_file("/data/sessions.csv"),
              "2026-10-12,PM,done,23:00,23:04,4,74,1,3,pink,15,100,10:30,02:30,09:45,1,0,1,N,"
              "\"rows draw, 1 commit\"\n");
    CHECK_HAS(fake_file("/state/state"), "bedtime = 02:30\nwaketime = 09:45\n");
    CHECK_INT(app.row_count, 1);
    CHECK_INT(app.last.focused, 74);
    CHECK_STR(app.next_step, "src/kilo.c:212 write editorScroll");
}

static void a_failed_save_keeps_the_form_open(void) {
    driver_open();
    driver_lock();
    driver_fill_end_form();
    fake.fail_write = "NEXT.md";
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_END);
    CHECK_HAS(app.notice, "Cannot save /data/NEXT.md");
    fake.fail_write = "";
    fake.fail_append = "sessions.csv";
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_END);
    CHECK_INT(app.row_count, 0);
    fake.fail_append = "";
    fake.fail_write = "/state/state";
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_DONE);
    CHECK_INT(app.row_count, 1);
}

static void back_and_escape_return_to_the_session(void) {
    driver_open();
    driver_lock();
    driver_click(RIGHT_BUTTON_X - 10, BOTTOM_Y);
    driver_click(BACK_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_SESSION);
    driver_click(RIGHT_BUTTON_X - 10, BOTTOM_Y);
    driver_key(KEY_ESCAPE);
    CHECK_INT(app.screen, SCREEN_SESSION);
}

static void a_missed_slot_is_logged_and_the_window_can_close(void) {
    driver_open();
    driver_click(MISSED_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_MISSED);
    CHECK_HAS(fake_file("/data/sessions.csv"), "2026-10-12,PM,missed,23:00,,0,0,");
    CHECK_INT(app.row_count, 1);
    driver_click(RIGHT_BUTTON_X - 10, BOTTOM_Y);
    CHECK(app.quit);

    driver_open();
    fake.fail_append = "sessions.csv";
    driver_click(MISSED_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_START);
    CHECK_HAS(app.notice, "Cannot add to");
}

static void a_second_session_reuses_the_block_without_stacking_it(void) {
    char once[70000];
    driver_open();
    driver_lock();
    text_copy(once, sizeof once, fake.blocked_hosts);
    driver_fill_end_form();
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    driver_click(RIGHT_BUTTON_X - 10, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_START);
    fake_advance(12 * 60 * 60);
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_SESSION);
    CHECK_INT(app.session.slot.id, SLOT_AM);
    CHECK_STR(fake.blocked_hosts, once);
    driver_fill_end_form();
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.row_count, 2);
}

static void after_missing_while_locked_the_button_goes_back_to_start(void) {
    driver_open();
    fake.blocked = true;
    driver_reopen();
    CHECK(app.blocked);
    driver_click(MISSED_X, BOTTOM_Y);
    driver_click(RIGHT_BUTTON_X - 10, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_START);
    CHECK(!app.quit);
}

static void stats_returns_to_where_it_was_opened(void) {
    driver_open();
    driver_click(STATS_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_STATS);
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_START);
    driver_click(MISSED_X, BOTTOM_Y);
    driver_click(STATS_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_STATS);
    driver_key(KEY_ESCAPE);
    CHECK_INT(app.screen, SCREEN_MISSED);
}

static void a_restart_picks_up_the_log_the_week_and_the_next_step(void) {
    fake_reset();
    fake_file_set(
        "/data/sessions.csv",
        "2026-10-04,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,1,N,\n");
    fake_file_set("/data/NEXT.md", "src/cpu.c:212 ADC flag\n");
    fake_file_set("/state/state", "seed = 7\nbedtime = 02:30\n");
    config_defaults(&app.config);
    driver_reopen();
    CHECK_INT(app.row_count, 1);
    CHECK_INT(app.week, 2);
    CHECK_INT(app.variable, VAR_SOUND);
    CHECK(app.coin == COIN_A || app.coin == COIN_B);
    CHECK_STR(app.next_step, "src/cpu.c:212 ADC flag");
    driver_lock();
    CHECK(app.session.next_present);
    CHECK_INT(app.form.sound, app.coin == COIN_B ? SOUND_PINK : SOUND_SILENCE);
    CHECK_STR(app.form.bedtime, "02:30");
}

static void the_form_starts_from_what_the_coin_asked_for(void) {
    struct form form;
    driver_open();
    app.variable = VAR_SOUND;
    app.coin = COIN_B;
    form_reset(&form, &app);
    CHECK_INT(form.sound, SOUND_PINK);
    CHECK(!form.cowork);
    app.variable = VAR_COWORK;
    form_reset(&form, &app);
    CHECK_INT(form.sound, SOUND_SILENCE);
    CHECK(form.cowork);
    app.coin = COIN_A;
    form_reset(&form, &app);
    CHECK(!form.cowork);
    app.variable = VAR_SOUND;
    form_reset(&form, &app);
    CHECK_INT(form.sound, SOUND_SILENCE);
}

static void a_full_log_in_memory_drops_its_oldest_row(void) {
    driver_open();
    app.row_count = APP_ROWS;
    app.rows[0].focused = 111;
    app.rows[1].focused = 222;
    driver_click(MISSED_X, BOTTOM_Y);
    CHECK_INT(app.row_count, APP_ROWS);
    CHECK_INT(app.rows[0].focused, 222);
    CHECK(app.rows[APP_ROWS - 1].missed);
}

static void actions_ignore_being_called_on_the_wrong_screen(void) {
    driver_open();
    action_save(&app);
    CHECK_INT(app.screen, SCREEN_START);
    driver_lock();
    action_lock(&app);
    action_missed(&app);
    CHECK_INT(app.screen, SCREEN_SESSION);
    CHECK_INT(app.row_count, 0);
}

void suite_app_end(void) {
    RUN(saving_an_empty_form_lists_every_problem);
    RUN(a_valid_form_writes_next_md_and_one_log_row);
    RUN(a_failed_save_keeps_the_form_open);
    RUN(back_and_escape_return_to_the_session);
    RUN(a_missed_slot_is_logged_and_the_window_can_close);
    RUN(a_second_session_reuses_the_block_without_stacking_it);
    RUN(after_missing_while_locked_the_button_goes_back_to_start);
    RUN(stats_returns_to_where_it_was_opened);
    RUN(a_restart_picks_up_the_log_the_week_and_the_next_step);
    RUN(the_form_starts_from_what_the_coin_asked_for);
    RUN(a_full_log_in_memory_drops_its_oldest_row);
    RUN(actions_ignore_being_called_on_the_wrong_screen);
}
