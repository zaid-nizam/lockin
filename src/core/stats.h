#ifndef LOCKIN_CORE_STATS_H
#define LOCKIN_CORE_STATS_H

#include "core/experiment.h"
#include "core/record.h"

enum {
    STATS_NO_DATA = -1,
    STATS_MIN_SLOTS = 14,
    STATS_MIN_PER_ARM = 5,
    STATS_MARGIN_PERCENT = 20,
    STATS_WINDOW_DAYS = 30,
    STATS_ON_TIME_MINUTES = 5,
    STATS_LONG_SESSION_MINUTES = 45,
};

/* The sessions logged under one condition. Medians are STATS_NO_DATA when
 * `slots` is 0. */
struct arm {
    int slots;
    int latency_median;
    int focus_median;
};

enum verdict { VERDICT_NEED_DATA, VERDICT_KEEP_A, VERDICT_KEEP_B, VERDICT_NO_DIFFERENCE };

struct comparison {
    struct arm a;
    struct arm b;
    enum verdict verdict;
};

/* Month-one targets over the last STATS_WINDOW_DAYS days. */
struct targets {
    int slots;
    int on_time;
    int long_sessions;
    int bedtime_spread;
};

/* Median of `count` values, which are sorted in place. Even counts give the
 * lower middle value. STATS_NO_DATA when count is 0. */
int stats_median(int *values, int count);

/* Compares the two conditions of one experiment variable. A verdict needs
 * STATS_MIN_SLOTS sessions in total and STATS_MIN_PER_ARM in each condition;
 * a condition wins only when it starts STATS_MARGIN_PERCENT sooner or holds
 * focus STATS_MARGIN_PERCENT longer and the other does not. */
struct comparison stats_compare(const struct record *rows, int count, enum variable variable);

/* Counts progress on the month-one targets. `bedtime_spread` is the gap in
 * minutes between the earliest and latest logged bedtime, STATS_NO_DATA with
 * fewer than two. */
struct targets stats_targets(const struct record *rows, int count, int64_t today);

/* The condition names shown for a variable: [0] is A, [1] is B. */
const char *stats_condition(enum variable variable, int arm);

#endif
