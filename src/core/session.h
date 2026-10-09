#ifndef LOCKIN_CORE_SESSION_H
#define LOCKIN_CORE_SESSION_H

#include "core/record.h"

/* One work session, from the moment the user locks in to the moment the
 * end-of-session form is saved. */
struct session {
    struct slot slot;
    int64_t locked_at;
    int started;
    int latency;
    int urges;
    int week;
    char coin;
    bool next_present;
};

/* Starts the session at `now`. */
void session_begin(struct session *s, struct slot slot, const struct moment *now);

/* Whole seconds since the session began. */
int session_seconds(const struct session *s, const struct moment *now);

/* Progress through the opening commitment, 0 to 1000. */
int session_commit_permille(const struct session *s, const struct moment *now, int commit_minutes);

/* True for the six hours after `hard_stop`, both minutes after midnight. */
bool session_past_hard_stop(int now_clock, int hard_stop);

/* Fills in everything the session itself knows about a log row. The caller
 * adds what the end-of-session form collected. */
void session_record(const struct session *s, const struct moment *now, struct record *r);

/* A log row for a slot that was skipped. */
void session_missed(struct slot slot, int week, struct record *r);

#endif
