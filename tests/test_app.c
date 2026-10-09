#include "app_driver.h"
#include "suites.h"
#include "test.h"

#include <string.h>

static void opens_on_the_start_screen_for_the_nearest_slot(void) {
    driver_open();
    CHECK_INT(app.screen, SCREEN_START);
    CHECK_INT(app.slot.id, SLOT_PM);
    CHECK_INT(slot_latency(app.slot, &app.now), 4);
    CHECK_INT(app.week, 1);
    CHECK(!app.blocked);
    CHECK_STR(app.notice, "");
}

static void without_root_the_lock_button_is_dead_and_says_why(void) {
    fake_reset();
    fake.can_block = false;
    config_defaults(&app.config);
    driver_reopen();
    CHECK_HAS(app.notice, "sudo lockin");
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_START);
    CHECK(!fake.blocked);
}

static void locking_blocks_sites_and_starts_the_session(void) {
    driver_open();
    driver_lock();
    CHECK_INT(app.screen, SCREEN_SESSION);
    CHECK(app.blocked && fake.blocked && fake.refused_to_stop);
    CHECK_HAS(fake.blocked_hosts, "# lockin: end of block\n127.0.0.1 localhost\n");
    CHECK_HAS(fake.blocked_hosts, "0.0.0.0 music.youtube.com\n");
    CHECK_HAS(fake_file("/state/state"), "wish = Finish chapter 3\n");
    CHECK_HAS(fake_file("/state/state"), "obstacle = Discord\n");
    CHECK_HAS(fake_file("/state/lockin.log"), "INFO sites blocked until reboot:");
    CHECK_INT(app.session.latency, 4);
    CHECK(fake.close_calls >= 1);
}

static void enter_in_a_text_box_never_locks(void) {
    driver_open();
    driver_fill(WOOP_X, WOOP_Y, "wish");
    driver_key(KEY_ENTER);
    CHECK_INT(app.screen, SCREEN_START);
    CHECK(!fake.blocked);
}

static void a_failed_block_stays_put_and_explains(void) {
    driver_open();
    fake.fail_block = true;
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_START);
    CHECK(!app.blocked && !fake.refused_to_stop);
    CHECK_HAS(app.notice, "Cannot put the block list");
    CHECK_HAS(fake_file("/state/lockin.log"), "ERROR E_BLOCK");

    driver_open();
    fake.fail_read = "/etc/hosts";
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_START);
    CHECK_HAS(app.notice, "Cannot read /etc/hosts");
}

static void an_oversized_hosts_file_is_refused_not_truncated(void) {
    driver_open();
    memset(fake.hosts, '#', sizeof fake.hosts - 1);
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_START);
    CHECK(!fake.blocked);
    CHECK_HAS(app.notice, "larger than 64 KB");
}

static void a_state_save_failure_does_not_stop_the_lock(void) {
    driver_open();
    fake.fail_write = "/state/state";
    driver_click(RIGHT_BUTTON_X, BOTTOM_Y);
    CHECK_INT(app.screen, SCREEN_SESSION);
    CHECK_HAS(fake_file("/state/lockin.log"), "ERROR E_IO Cannot save /state/state");
}

static void the_window_closes_only_before_locking(void) {
    driver_open();
    driver_lock();
    app_event(&app, &(struct pf_event){PF_CLOSE, 0, 0, 0});
    CHECK(!app.quit);
    CHECK_HAS(app.notice, "stays open until you shut down");
    driver_open();
    app_event(&app, &(struct pf_event){PF_CLOSE, 0, 0, 0});
    CHECK(app.quit);
}

static void listed_programs_are_closed_every_two_seconds_while_locked(void) {
    driver_open();
    app_tick(&app, false);
    CHECK_INT(fake.close_calls, 0);
    driver_lock();
    int calls = fake.close_calls;
    fake.programs_to_close = 2;
    fake_advance(1);
    app_tick(&app, false);
    CHECK_INT(fake.close_calls, calls);
    fake_advance(1);
    app_tick(&app, false);
    CHECK_INT(fake.close_calls, calls + 1);
    CHECK_INT(app.closed_programs, 2);
}

static void the_caret_blinks_on_idle_ticks(void) {
    driver_open();
    bool before = app.ui.caret;
    app_tick(&app, true);
    CHECK(app.ui.caret != before);
    app_tick(&app, false);
    CHECK(app.ui.caret != before);
}

static void urges_are_counted(void) {
    driver_open();
    driver_lock();
    driver_click(URGE_X, URGE_Y);
    driver_click(URGE_X, URGE_Y);
    CHECK_INT(app.session.urges, 2);
}

void suite_app(void) {
    RUN(opens_on_the_start_screen_for_the_nearest_slot);
    RUN(without_root_the_lock_button_is_dead_and_says_why);
    RUN(locking_blocks_sites_and_starts_the_session);
    RUN(enter_in_a_text_box_never_locks);
    RUN(a_failed_block_stays_put_and_explains);
    RUN(an_oversized_hosts_file_is_refused_not_truncated);
    RUN(a_state_save_failure_does_not_stop_the_lock);
    RUN(the_window_closes_only_before_locking);
    RUN(listed_programs_are_closed_every_two_seconds_while_locked);
    RUN(the_caret_blinks_on_idle_ticks);
    RUN(urges_are_counted);
}
