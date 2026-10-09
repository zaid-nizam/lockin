#include "core/stats.h"

#include <stdlib.h>

enum { ROWS_MAX = 1024 };

static int ascending(const void *a, const void *b) {
    int left = *(const int *)a;
    int right = *(const int *)b;
    return (left > right) - (left < right);
}

int stats_median(int *values, int count) {
    if (count == 0) {
        return STATS_NO_DATA;
    }
    qsort(values, (size_t)count, sizeof values[0], ascending);
    return values[(count - 1) / 2];
}

/* Which condition a row counts under: 0 for A, 1 for B, -1 for neither. */
static int arm_of(const struct record *r, enum variable variable) {
    switch (variable) {
    case VAR_SOUND:
        if (r->sound == SOUND_SILENCE) {
            return 0;
        }
        return r->sound == SOUND_PINK ? 1 : -1;
    case VAR_EXERCISE:
        return r->slot == SLOT_AM ? r->exercise_min > 0 : -1;
    case VAR_COWORK:
        return r->cowork;
    default:
        return -1;
    }
}

static struct arm summarise(const struct record *rows, int count, enum variable variable,
                            int wanted) {
    int latency[ROWS_MAX], focus[ROWS_MAX];
    struct arm arm = {0, STATS_NO_DATA, STATS_NO_DATA};
    for (int i = 0; i < count && arm.slots < ROWS_MAX; i++) {
        if (rows[i].missed || arm_of(&rows[i], variable) != wanted) {
            continue;
        }
        latency[arm.slots] = rows[i].latency;
        focus[arm.slots] = rows[i].focused;
        arm.slots++;
    }
    arm.latency_median = stats_median(latency, arm.slots);
    arm.focus_median = stats_median(focus, arm.slots);
    return arm;
}

/* True when `other` beats `base` by the margin on either outcome. */
static bool beats(const struct arm *base, const struct arm *other) {
    bool sooner =
        base->latency_median > 0 &&
        other->latency_median * 100 <= base->latency_median * (100 - STATS_MARGIN_PERCENT);
    bool longer = other->focus_median > 0 &&
                  other->focus_median * 100 >= base->focus_median * (100 + STATS_MARGIN_PERCENT);
    return sooner || longer;
}

struct comparison stats_compare(const struct record *rows, int count, enum variable variable) {
    struct comparison c = {summarise(rows, count, variable, 0), summarise(rows, count, variable, 1),
                           VERDICT_NEED_DATA};
    if (c.a.slots + c.b.slots < STATS_MIN_SLOTS || c.a.slots < STATS_MIN_PER_ARM ||
        c.b.slots < STATS_MIN_PER_ARM) {
        return c;
    }
    bool b_wins = beats(&c.a, &c.b);
    bool a_wins = beats(&c.b, &c.a);
    if (a_wins == b_wins) {
        c.verdict = VERDICT_NO_DIFFERENCE;
    } else {
        c.verdict = b_wins ? VERDICT_KEEP_B : VERDICT_KEEP_A;
    }
    return c;
}

static bool in_window(const struct record *r, int64_t today) {
    return r->day > today - STATS_WINDOW_DAYS && r->day <= today;
}

/* Bedtimes straddle midnight, so 00:00 to 11:59 counts as late night. */
static int bedtime_spread(const struct record *rows, int count, int64_t today) {
    int earliest = 0, latest = 0, seen = 0;
    for (int i = 0; i < count; i++) {
        int bedtime = rows[i].bedtime;
        if (bedtime == CLOCK_NONE || !in_window(&rows[i], today)) {
            continue;
        }
        if (bedtime < MINUTES_PER_DAY / 2) {
            bedtime += MINUTES_PER_DAY;
        }
        if (seen == 0 || bedtime < earliest) {
            earliest = bedtime;
        }
        if (seen == 0 || bedtime > latest) {
            latest = bedtime;
        }
        seen++;
    }
    return seen >= 2 ? latest - earliest : STATS_NO_DATA;
}

struct targets stats_targets(const struct record *rows, int count, int64_t today) {
    struct targets t = {0, 0, 0, bedtime_spread(rows, count, today)};
    for (int i = 0; i < count; i++) {
        const struct record *r = &rows[i];
        if (!in_window(r, today)) {
            continue;
        }
        t.slots++;
        if (r->missed) {
            continue;
        }
        t.on_time += r->latency < STATS_ON_TIME_MINUTES;
        t.long_sessions += r->focused >= STATS_LONG_SESSION_MINUTES;
    }
    return t;
}

const char *stats_condition(enum variable variable, int arm) {
    static const char *const names[][2] = {
        [VAR_SOUND] = {"silence", "pink noise"},
        [VAR_EXERCISE] = {"no exercise", "exercise first"},
        [VAR_COWORK] = {"alone", "co-work room"},
    };
    return names[variable][arm];
}
