#ifndef LOCKIN_CORE_SLOT_H
#define LOCKIN_CORE_SLOT_H

#include "core/daytime.h"

enum slot_id { SLOT_AM, SLOT_PM };

/* One planned start. `stamp` is the planned time in local minutes since 1970
 * (see moment_stamp), so the slot's date survives a session that starts
 * after midnight. */
struct slot {
    enum slot_id id;
    int64_t stamp;
};

/* Returns the planned start nearest to `now`, looking at yesterday, today
 * and tomorrow. `am` and `pm` are minutes after midnight. */
struct slot slot_nearest(const struct moment *now, int am, int pm);

/* Returns the first planned start after `current`. */
struct slot slot_after(struct slot current, int am, int pm);

/* Minutes the start is late by; 0 when on time or early. */
int slot_latency(struct slot slot, const struct moment *now);

/* Minutes after midnight the slot is planned for. */
int slot_clock(struct slot slot);

/* Day number (see civil_days) the slot belongs to. */
int64_t slot_day(struct slot slot);

/* "AM" or "PM". */
const char *slot_name(enum slot_id id);

#endif
