#include "core/experiment.h"
#include "core/record.h"
#include "core/session.h"
#include "suites.h"
#include "test.h"

#include <string.h>

enum { AM = 11 * 60, PM = 23 * 60 };

static struct moment at(int day, int hour, int minute) {
    struct moment m = {2026, 10, day, hour, minute, 0, 0};
    m.epoch = moment_stamp(&m) * 60;
    return m;
}

static void picks_the_nearest_slot_including_after_midnight(void) {
    struct moment late_morning = at(12, 11, 20);
    struct moment early = at(12, 10, 40);
    struct moment past_midnight = at(13, 0, 30);
    struct moment evening = at(12, 20, 0);
    struct slot slot = slot_nearest(&late_morning, AM, PM);
    CHECK_INT(slot.id, SLOT_AM);
    CHECK_INT(slot_latency(slot, &late_morning), 20);
    CHECK_INT(slot_clock(slot), AM);

    slot = slot_nearest(&early, AM, PM);
    CHECK_INT(slot.id, SLOT_AM);
    CHECK_INT(slot_latency(slot, &early), 0);

    slot = slot_nearest(&past_midnight, AM, PM);
    CHECK_INT(slot.id, SLOT_PM);
    CHECK_INT(slot_latency(slot, &past_midnight), 90);
    CHECK_INT(slot_day(slot), civil_days(2026, 10, 12));

    slot = slot_nearest(&evening, AM, PM);
    CHECK_INT(slot.id, SLOT_PM);
    CHECK_STR(slot_name(slot.id), "PM");
    CHECK_STR(slot_name(SLOT_AM), "AM");
}

static void next_slot_follows_in_order_across_days(void) {
    struct moment now = at(12, 11, 0);
    struct slot morning = slot_nearest(&now, AM, PM);
    struct slot evening = slot_after(morning, AM, PM);
    struct slot tomorrow = slot_after(evening, AM, PM);
    CHECK_INT(evening.id, SLOT_PM);
    CHECK_INT(slot_day(evening), slot_day(morning));
    CHECK_INT(tomorrow.id, SLOT_AM);
    CHECK_INT(slot_day(tomorrow), slot_day(morning) + 1);
}

static struct record sample(void) {
    struct record r;
    memset(&r, 0, sizeof r);
    r.day = civil_days(2026, 10, 12);
    r.slot = SLOT_PM;
    r.planned = PM;
    r.started = PM + 4;
    r.latency = 4;
    r.focused = 74;
    r.urges = 2;
    r.urge_rating = 3;
    r.sound = SOUND_PINK;
    r.exercise_min = 15;
    r.caffeine_mg = 100;
    r.caffeine_at = 10 * 60 + 30;
    r.bedtime = 2 * 60 + 30;
    r.waketime = 9 * 60 + 45;
    r.cowork = true;
    r.next_present = true;
    r.week = 2;
    r.coin = COIN_B;
    text_copy(r.output, sizeof r.output, "1 commit, 2 tests");
    return r;
}

static void a_row_survives_writing_and_reading(void) {
    struct record in = sample(), out;
    char line[512];
    struct sbuf sb = sbuf_on(line, sizeof line);
    record_write(&in, &sb);
    CHECK_STR(line, "2026-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,"
                    "\"1 commit, 2 tests\"\n");
    CHECK(record_read(line, &out));
    CHECK_INT(out.day, in.day);
    CHECK_INT(out.slot, SLOT_PM);
    CHECK(!out.missed);
    CHECK_INT(out.started, in.started);
    CHECK_INT(out.focused, 74);
    CHECK_INT(out.sound, SOUND_PINK);
    CHECK_INT(out.caffeine_at, in.caffeine_at);
    CHECK(out.cowork && out.next_present);
    CHECK_INT(out.coin, COIN_B);
    CHECK_STR(out.output, "1 commit, 2 tests");
}

static void a_missed_row_has_empty_times(void) {
    struct record in, out;
    char line[512];
    struct moment now = at(12, 23, 30);
    struct sbuf sb = sbuf_on(line, sizeof line);
    session_missed(slot_nearest(&now, AM, PM), 1, &in);
    record_write(&in, &sb);
    CHECK_STR(line, "2026-10-12,PM,missed,23:00,,0,0,0,0,silence,0,0,,,,0,0,1,N,\n");
    CHECK(record_read(line, &out));
    CHECK(out.missed);
    CHECK_INT(out.started, CLOCK_NONE);
    CHECK_INT(out.bedtime, CLOCK_NONE);
}

static const char *const damaged_rows[] = {
    "2026-10-12,PM,done",
    "2026-10,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "-5-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "99999-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-00-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-00,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-32,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "x,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,maybe,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,9,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,x,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,x,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,x,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,x,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,2,3,pink,601,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,2001,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,x,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,2,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,2,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,100001,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-13-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12x,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,XX,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,-4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,2,9,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,2,3,jazz,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,25:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,7x,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,B,x",
    "2026-10-12,PM,done,23:00,23:04,4,74,2,3,pink,15,100,10:30,02:30,09:45,1,1,2,Z,x",
};

static void damaged_rows_are_rejected(void) {
    char header[512];
    struct record unused;
    text_copy(header, sizeof header, record_header);
    CHECK(!record_read(header, &unused));
    for (size_t i = 0; i < sizeof damaged_rows / sizeof damaged_rows[0]; i++) {
        struct record out;
        char line[512];
        text_copy(line, sizeof line, damaged_rows[i]);
        CHECK(!record_read(line, &out));
    }
    CHECK_STR(sound_name(SOUND_LYRICS), "lyrics");
    CHECK(strncmp(record_header, "date,slot,status", 16) == 0);
}

static void session_measures_time_and_the_opening_commitment(void) {
    struct session s;
    struct moment start = at(12, 23, 4);
    struct moment later = start;
    session_begin(&s, slot_nearest(&start, AM, PM), &start);
    CHECK_INT(s.latency, 4);
    CHECK_INT(s.started, PM + 4);
    CHECK_INT(session_seconds(&s, &start), 0);
    later.epoch += 300;
    CHECK_INT(session_seconds(&s, &later), 300);
    CHECK_INT(session_commit_permille(&s, &later, 10), 500);
    later.epoch += 3600;
    CHECK_INT(session_commit_permille(&s, &later, 10), 1000);
    later.epoch = start.epoch - 30;
    CHECK_INT(session_seconds(&s, &later), 0);
}

static void hard_stop_warns_for_six_hours_only(void) {
    CHECK(!session_past_hard_stop(1 * 60 + 59, 2 * 60));
    CHECK(session_past_hard_stop(2 * 60, 2 * 60));
    CHECK(session_past_hard_stop(7 * 60 + 59, 2 * 60));
    CHECK(!session_past_hard_stop(8 * 60, 2 * 60));
    CHECK(session_past_hard_stop(1 * 60, 23 * 60));
    CHECK(!session_past_hard_stop(22 * 60, 23 * 60));
}

static void session_fills_its_part_of_the_row(void) {
    struct session s;
    struct record r;
    struct moment start = at(12, 23, 4);
    struct moment end = start;
    session_begin(&s, slot_nearest(&start, AM, PM), &start);
    s.urges = 3;
    s.week = 2;
    s.coin = COIN_A;
    s.next_present = true;
    end.epoch += 74 * 60 + 20;
    session_record(&s, &end, &r);
    CHECK_INT(r.focused, 74);
    CHECK_INT(r.urges, 3);
    CHECK_INT(r.latency, 4);
    CHECK_INT(r.coin, COIN_A);
    CHECK_INT(r.caffeine_at, CLOCK_NONE);
    CHECK(r.next_present && !r.missed);
}

static void experiment_changes_one_thing_per_week(void) {
    CHECK_INT(experiment_week(100, 100), 1);
    CHECK_INT(experiment_week(100, 106), 1);
    CHECK_INT(experiment_week(100, 107), 2);
    CHECK_INT(experiment_week(100, 90), 1);
    CHECK_INT(experiment_variable(1), VAR_BASELINE);
    CHECK_INT(experiment_variable(2), VAR_SOUND);
    CHECK_INT(experiment_variable(3), VAR_EXERCISE);
    CHECK_INT(experiment_variable(4), VAR_COWORK);
    CHECK_INT(experiment_variable(5), VAR_FREE);
    CHECK_INT(experiment_variable(0), VAR_FREE);
}

static void coin_is_fixed_per_slot_and_roughly_fair(void) {
    int heads = 0;
    struct slot slot = {SLOT_AM, 0};
    for (int day = 0; day < 400; day++) {
        slot.stamp = (int64_t)day * MINUTES_PER_DAY + AM;
        char coin = experiment_coin(99, slot, VAR_SOUND);
        CHECK_INT(coin, experiment_coin(99, slot, VAR_SOUND));
        heads += coin == COIN_A;
    }
    CHECK(heads > 160 && heads < 240);
    CHECK_INT(experiment_coin(99, slot, VAR_BASELINE), COIN_NONE);
    CHECK_INT(experiment_coin(99, slot, VAR_FREE), COIN_NONE);
    CHECK(experiment_coin(99, slot, VAR_EXERCISE) != COIN_NONE);
    CHECK(experiment_coin(99, slot, VAR_COWORK) != COIN_NONE);
    slot.id = SLOT_PM;
    CHECK_INT(experiment_coin(99, slot, VAR_EXERCISE), COIN_NONE);
}

static void every_week_has_an_instruction(void) {
    CHECK_HAS(experiment_instruction(VAR_BASELINE, COIN_NONE), "Baseline");
    CHECK_HAS(experiment_instruction(VAR_SOUND, COIN_A), "silence");
    CHECK_HAS(experiment_instruction(VAR_SOUND, COIN_B), "pink noise");
    CHECK_HAS(experiment_instruction(VAR_EXERCISE, COIN_A), "no exercise");
    CHECK_HAS(experiment_instruction(VAR_EXERCISE, COIN_B), "walk");
    CHECK_HAS(experiment_instruction(VAR_EXERCISE, COIN_NONE), "Nothing to do for PM");
    CHECK_HAS(experiment_instruction(VAR_COWORK, COIN_B), "co-work");
    CHECK_HAS(experiment_instruction(VAR_FREE, COIN_NONE), "finished");
}

void suite_records(void) {
    RUN(picks_the_nearest_slot_including_after_midnight);
    RUN(next_slot_follows_in_order_across_days);
    RUN(a_row_survives_writing_and_reading);
    RUN(a_missed_row_has_empty_times);
    RUN(damaged_rows_are_rejected);
    RUN(session_measures_time_and_the_opening_commitment);
    RUN(hard_stop_warns_for_six_hours_only);
    RUN(session_fills_its_part_of_the_row);
    RUN(experiment_changes_one_thing_per_week);
    RUN(coin_is_fixed_per_slot_and_roughly_fair);
    RUN(every_week_has_an_instruction);
}
