#include "app/store.h"
#include "fake_sys.h"
#include "suites.h"
#include "test.h"

#include <string.h>

static const char good_row[] =
    "2026-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,ok\n";

static void first_run_writes_a_commented_settings_file(void) {
    struct config c;
    struct error err;
    char text[256];
    struct sbuf problems = sbuf_on(text, sizeof text);
    fake_reset();
    CHECK(store_config(&c, &problems, &err));
    CHECK_HAS(fake_file("/cfg/lockin.conf"), "slot_am = 11:00");
    CHECK_INT(c.slot_pm, 23 * 60);
}

static void settings_file_is_applied_or_refused_with_reasons(void) {
    struct config c;
    struct error err;
    char text[512];
    struct sbuf problems = sbuf_on(text, sizeof text);
    fake_reset();
    fake_file_set("/cfg/lockin.conf", "slot_pm = 22:00\n");
    CHECK(store_config(&c, &problems, &err));
    CHECK_INT(c.slot_pm, 22 * 60);
    fake_file_set("/cfg/lockin.conf", "slot_pm = late\nwat = 1\n");
    CHECK(!store_config(&c, &problems, &err));
    CHECK_INT(err.code, ERR_CONFIG);
    CHECK_HAS(err.text, "/cfg/lockin.conf");
    CHECK_HAS(text, "line 1: slot_pm");
    CHECK_HAS(text, "line 2: unknown setting");
}

static void unreadable_settings_stop_the_start(void) {
    struct config c;
    struct error err;
    char text[64];
    struct sbuf problems = sbuf_on(text, sizeof text);
    fake_reset();
    fake.fail_read = "lockin.conf";
    CHECK(!store_config(&c, &problems, &err));
    CHECK_INT(err.code, ERR_IO);
}

static void state_starts_fresh_then_round_trips(void) {
    struct state saved, loaded;
    struct error err;
    fake_reset();
    store_state_load(&saved);
    CHECK_INT(saved.seed, fake.random | 1);
    CHECK_STR(saved.woop.wish, "");
    text_copy(saved.woop.wish, sizeof saved.woop.wish, "Finish the = parser");
    text_copy(saved.woop.outcome, sizeof saved.woop.outcome, "Tests pass");
    text_copy(saved.woop.obstacle, sizeof saved.woop.obstacle, "Discord");
    text_copy(saved.woop.plan, sizeof saved.woop.plan, "If pull, then Urge");
    text_copy(saved.bedtime, sizeof saved.bedtime, "02:30");
    text_copy(saved.waketime, sizeof saved.waketime, "09:45");
    saved.seed = 18446744073709551557ULL;
    CHECK(store_state_save(&saved, &err));
    store_state_load(&loaded);
    CHECK(loaded.seed == saved.seed);
    CHECK_STR(loaded.woop.wish, "Finish the = parser");
    CHECK_STR(loaded.woop.plan, "If pull, then Urge");
    CHECK_STR(loaded.waketime, "09:45");
}

static void an_unreadable_file_is_logged_and_treated_as_empty(void) {
    struct state state;
    struct record rows[4];
    char next[STORE_NEXT_MAX];
    struct config c;
    fake_reset();
    config_defaults(&c);
    fake_file_set("/state/state", "seed = 5\n");
    fake_file_set("/data/sessions.csv", good_row);
    fake_file_set("/data/NEXT.md", "do it\n");
    fake.fail_read = "/";
    store_state_load(&state);
    CHECK_INT(store_rows_load(rows, 4), 0);
    store_next_load(&c, next);
    CHECK_STR(next, "");
    CHECK(state.seed != 5);
    CHECK_HAS(fake_file("/state/lockin.log"), "ERROR E_IO Cannot read /state/state");
    CHECK_HAS(fake_file("/state/lockin.log"), "sessions.csv");
    CHECK_HAS(fake_file("/state/lockin.log"), "NEXT.md");
}

static void log_lines_carry_time_and_level(void) {
    char path[64];
    struct sbuf sb = sbuf_on(path, sizeof path);
    fake_reset();
    store_log("INFO", "started");
    CHECK_STR(fake_file("/state/lockin.log"), "2026-10-12 23:04:00 INFO started\n");
    store_log_path(&sb);
    CHECK_STR(path, "/state/lockin.log");
    fake.fail_append = "lockin.log";
    store_log("INFO", "still does not crash");
}

static void rows_load_skips_the_header_and_junk(void) {
    struct record rows[4];
    char text[1024];
    struct sbuf sb = sbuf_on(text, sizeof text);
    fake_reset();
    CHECK_INT(store_rows_load(rows, 4), 0);
    sbuf_put(&sb, record_header);
    sbuf_put(&sb, good_row);
    sbuf_put(&sb, "this is not a row\n\n");
    sbuf_put(&sb, good_row);
    fake_file_set("/data/sessions.csv", text);
    CHECK_INT(store_rows_load(rows, 4), 2);
    CHECK_INT(rows[1].focused, 74);
}

static void rows_load_keeps_the_newest_when_there_are_too_many(void) {
    struct record rows[3];
    char text[2048];
    struct sbuf sb = sbuf_on(text, sizeof text);
    fake_reset();
    for (int i = 1; i <= 5; i++) {
        sbuf_putf(&sb, "2026-10-12,PM,done,23:00,23:04,4,%d,0,3,pink,0,0,,,,0,0,1,N,\n", i);
    }
    fake_file_set("/data/sessions.csv", text);
    CHECK_INT(store_rows_load(rows, 3), 3);
    CHECK_INT(rows[0].focused, 3);
    CHECK_INT(rows[2].focused, 5);
}

static void appending_writes_the_header_once(void) {
    struct record row;
    struct error err;
    char line[512], expected[1024];
    struct sbuf sb = sbuf_on(expected, sizeof expected);
    fake_reset();
    text_copy(line, sizeof line, good_row);
    CHECK(record_read(line, &row));
    CHECK(store_row_append(&row, &err));
    CHECK(store_row_append(&row, &err));
    sbuf_put(&sb, record_header);
    sbuf_put(&sb, good_row);
    sbuf_put(&sb, good_row);
    CHECK_STR(fake_file("/data/sessions.csv"), expected);
}

static void append_failures_are_reported(void) {
    struct record row;
    struct error err;
    char line[512];
    fake_reset();
    text_copy(line, sizeof line, good_row);
    CHECK(record_read(line, &row));
    fake.fail_read = "sessions.csv";
    CHECK(!store_row_append(&row, &err));
    fake.fail_read = "";
    fake.fail_append = "sessions.csv";
    CHECK(!store_row_append(&row, &err));
    CHECK_INT(err.code, ERR_IO);
}

static void next_step_lives_in_the_project_when_one_is_set(void) {
    struct config c;
    struct error err;
    char next[STORE_NEXT_MAX];
    fake_reset();
    config_defaults(&c);
    store_next_load(&c, next);
    CHECK_STR(next, "");
    CHECK(store_next_save(&c, "src/cpu.c:212 ADC flag", &err));
    CHECK_STR(fake_file("/data/NEXT.md"), "src/cpu.c:212 ADC flag\n");
    text_copy(c.project, sizeof c.project, "/home/z/emu");
    fake_file_set("/home/z/emu/NEXT.md", "\n\n   first real line  \nsecond\n");
    store_next_load(&c, next);
    CHECK_STR(next, "first real line");
    CHECK(store_next_save(&c, "new step", &err));
    CHECK_STR(fake_file("/home/z/emu/NEXT.md"), "new step\n");
}

void suite_store(void) {
    RUN(first_run_writes_a_commented_settings_file);
    RUN(settings_file_is_applied_or_refused_with_reasons);
    RUN(unreadable_settings_stop_the_start);
    RUN(state_starts_fresh_then_round_trips);
    RUN(an_unreadable_file_is_logged_and_treated_as_empty);
    RUN(log_lines_carry_time_and_level);
    RUN(rows_load_skips_the_header_and_junk);
    RUN(rows_load_keeps_the_newest_when_there_are_too_many);
    RUN(appending_writes_the_header_once);
    RUN(append_failures_are_reported);
    RUN(next_step_lives_in_the_project_when_one_is_set);
}
