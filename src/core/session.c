#include "core/session.h"

#include "core/experiment.h"

#include <string.h>

void session_begin(struct session *s, struct slot slot, const struct moment *now) {
    s->slot = slot;
    s->locked_at = now->epoch;
    s->started = moment_clock(now);
    s->latency = slot_latency(slot, now);
    s->urges = 0;
}

int session_seconds(const struct session *s, const struct moment *now) {
    int64_t elapsed = now->epoch - s->locked_at;
    return elapsed > 0 ? (int)elapsed : 0;
}

int session_commit_permille(const struct session *s, const struct moment *now, int commit_minutes) {
    int64_t permille = (int64_t)session_seconds(s, now) * 1000 / ((int64_t)commit_minutes * 60);
    return permille > 1000 ? 1000 : (int)permille;
}

bool session_past_hard_stop(int now_clock, int hard_stop) {
    int since = (now_clock - hard_stop + MINUTES_PER_DAY) % MINUTES_PER_DAY;
    return since < 6 * 60;
}

static void record_slot(struct slot slot, int week, struct record *r) {
    memset(r, 0, sizeof *r);
    r->day = slot_day(slot);
    r->slot = slot.id;
    r->planned = slot_clock(slot);
    r->started = CLOCK_NONE;
    r->caffeine_at = CLOCK_NONE;
    r->bedtime = CLOCK_NONE;
    r->waketime = CLOCK_NONE;
    r->week = week;
    r->coin = COIN_NONE;
}

void session_record(const struct session *s, const struct moment *now, struct record *r) {
    record_slot(s->slot, s->week, r);
    r->started = s->started;
    r->latency = s->latency;
    r->focused = session_seconds(s, now) / 60;
    r->urges = s->urges;
    r->next_present = s->next_present;
    r->coin = s->coin;
}

void session_missed(struct slot slot, int week, struct record *r) {
    record_slot(slot, week, r);
    r->missed = true;
}
