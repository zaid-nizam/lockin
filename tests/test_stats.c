#include "core/stats.h"
#include "suites.h"
#include "test.h"

#include <string.h>

enum { TODAY = 20738 };

static struct record row(enum sound sound, int latency, int focused) {
    struct record r;
    memset(&r, 0, sizeof r);
    r.day = TODAY;
    r.slot = SLOT_AM;
    r.sound = sound;
    r.latency = latency;
    r.focused = focused;
    r.bedtime = CLOCK_NONE;
    return r;
}

static void median_of_nothing_one_and_many(void) {
    int none[1] = {0}, one[] = {7}, odd[] = {9, 1, 5}, even[] = {4, 1, 3, 2};
    CHECK_INT(stats_median(none, 0), STATS_NO_DATA);
    CHECK_INT(stats_median(one, 1), 7);
    CHECK_INT(stats_median(odd, 3), 5);
    CHECK_INT(stats_median(even, 4), 2);
}

static struct record rows[40];

/* Fills `rows` with `a` copies of one session and returns where to go on. */
static int fill(int from, int count, struct record like) {
    for (int i = 0; i < count; i++) {
        rows[from + i] = like;
    }
    return from + count;
}

/* `a` silence sessions then `b` pink-noise sessions. */
#define two_arms(unused, a, like_a, b, like_b) fill(fill(0, (a), (like_a)), (b), (like_b))

static void no_verdict_before_enough_sessions(void) {
    int n = two_arms(rows, 6, row(SOUND_SILENCE, 10, 30), 7, row(SOUND_PINK, 1, 90));
    CHECK_INT(stats_compare(rows, n, VAR_SOUND).verdict, VERDICT_NEED_DATA);
    n = two_arms(rows, 12, row(SOUND_SILENCE, 10, 30), 4, row(SOUND_PINK, 1, 90));
    CHECK_INT(stats_compare(rows, n, VAR_SOUND).verdict, VERDICT_NEED_DATA);
    n = two_arms(rows, 4, row(SOUND_SILENCE, 10, 30), 12, row(SOUND_PINK, 1, 90));
    CHECK_INT(stats_compare(rows, n, VAR_SOUND).verdict, VERDICT_NEED_DATA);
    CHECK_INT(stats_compare(rows, 0, VAR_SOUND).a.focus_median, STATS_NO_DATA);
}

static void a_condition_wins_only_past_the_twenty_percent_margin(void) {
    int n = two_arms(rows, 7, row(SOUND_SILENCE, 10, 50), 7, row(SOUND_PINK, 10, 60));
    struct comparison c = stats_compare(rows, n, VAR_SOUND);
    CHECK_INT(c.verdict, VERDICT_KEEP_B);
    CHECK_INT(c.a.slots, 7);
    CHECK_INT(c.b.focus_median, 60);

    n = two_arms(rows, 7, row(SOUND_SILENCE, 10, 50), 7, row(SOUND_PINK, 10, 59));
    CHECK_INT(stats_compare(rows, n, VAR_SOUND).verdict, VERDICT_NO_DIFFERENCE);

    n = two_arms(rows, 7, row(SOUND_SILENCE, 8, 50), 7, row(SOUND_PINK, 10, 50));
    CHECK_INT(stats_compare(rows, n, VAR_SOUND).verdict, VERDICT_KEEP_A);

    n = two_arms(rows, 7, row(SOUND_SILENCE, 10, 50), 7, row(SOUND_PINK, 8, 50));
    CHECK_INT(stats_compare(rows, n, VAR_SOUND).verdict, VERDICT_KEEP_B);

    /* Each wins on a different outcome: that is not a clear result. */
    n = two_arms(rows, 7, row(SOUND_SILENCE, 5, 50), 7, row(SOUND_PINK, 10, 80));
    CHECK_INT(stats_compare(rows, n, VAR_SOUND).verdict, VERDICT_NO_DIFFERENCE);

    n = two_arms(rows, 7, row(SOUND_SILENCE, 0, 0), 7, row(SOUND_PINK, 0, 0));
    CHECK_INT(stats_compare(rows, n, VAR_SOUND).verdict, VERDICT_NO_DIFFERENCE);
}

static void rows_count_only_under_the_condition_they_belong_to(void) {
    struct record lyrics = row(SOUND_LYRICS, 1, 1), missed = row(SOUND_SILENCE, 0, 0);
    struct record walked = row(SOUND_SILENCE, 2, 40), evening = walked, room = walked;
    walked.exercise_min = 15;
    evening.exercise_min = 15;
    evening.slot = SLOT_PM;
    room.cowork = true;
    missed.missed = true;
    rows[0] = lyrics;
    rows[1] = missed;
    rows[2] = walked;
    rows[3] = evening;
    rows[4] = room;
    struct comparison sound = stats_compare(rows, 5, VAR_SOUND);
    CHECK_INT(sound.a.slots, 3);
    CHECK_INT(sound.b.slots, 0);
    struct comparison exercise = stats_compare(rows, 5, VAR_EXERCISE);
    CHECK_INT(exercise.a.slots, 2);
    CHECK_INT(exercise.b.slots, 1);
    struct comparison cowork = stats_compare(rows, 5, VAR_COWORK);
    CHECK_INT(cowork.a.slots, 3);
    CHECK_INT(cowork.b.slots, 1);
    CHECK_INT(stats_compare(rows, 5, VAR_BASELINE).a.slots, 0);
    CHECK_STR(stats_condition(VAR_SOUND, 1), "pink noise");
    CHECK_STR(stats_condition(VAR_EXERCISE, 0), "no exercise");
    CHECK_STR(stats_condition(VAR_COWORK, 1), "co-work room");
}

static void targets_count_the_last_thirty_days(void) {
    rows[0] = row(SOUND_SILENCE, 4, 45);
    rows[1] = row(SOUND_SILENCE, 5, 44);
    rows[2] = row(SOUND_SILENCE, 0, 0);
    rows[2].missed = true;
    rows[3] = row(SOUND_SILENCE, 0, 90);
    rows[3].day = TODAY - 30;
    rows[4] = row(SOUND_SILENCE, 0, 90);
    rows[4].day = TODAY + 1;
    rows[5] = row(SOUND_SILENCE, 1, 60);
    rows[5].day = TODAY - 29;
    struct targets t = stats_targets(rows, 6, TODAY);
    CHECK_INT(t.slots, 4);
    CHECK_INT(t.on_time, 2);
    CHECK_INT(t.long_sessions, 2);
    CHECK_INT(t.bedtime_spread, STATS_NO_DATA);
}

static void bedtime_spread_treats_after_midnight_as_late(void) {
    for (int i = 0; i < 4; i++) {
        rows[i] = row(SOUND_SILENCE, 0, 0);
    }
    rows[0].bedtime = 23 * 60 + 30;
    rows[1].bedtime = 2 * 60 + 30;
    rows[2].bedtime = 1 * 60;
    rows[3].bedtime = 22 * 60;
    rows[3].day = TODAY - 40;
    CHECK_INT(stats_targets(rows, 4, TODAY).bedtime_spread, 180);
    CHECK_INT(stats_targets(rows, 1, TODAY).bedtime_spread, STATS_NO_DATA);
}

void suite_stats(void) {
    RUN(median_of_nothing_one_and_many);
    RUN(no_verdict_before_enough_sessions);
    RUN(a_condition_wins_only_past_the_twenty_percent_margin);
    RUN(rows_count_only_under_the_condition_they_belong_to);
    RUN(targets_count_the_last_thirty_days);
    RUN(bedtime_spread_treats_after_midnight_as_late);
}
