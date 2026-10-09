#include "core/slot.h"

#include <stdlib.h>

struct slot slot_nearest(const struct moment *now, int am, int pm) {
    int64_t stamp = moment_stamp(now);
    int64_t midnight = stamp - moment_clock(now);
    struct slot best = {SLOT_AM, midnight + am};
    for (int day = -1; day <= 1; day++) {
        for (int id = SLOT_AM; id <= SLOT_PM; id++) {
            int64_t planned = midnight + (int64_t)day * MINUTES_PER_DAY + (id == SLOT_AM ? am : pm);
            if (llabs(planned - stamp) < llabs(best.stamp - stamp)) {
                best = (struct slot){(enum slot_id)id, planned};
            }
        }
    }
    return best;
}

struct slot slot_after(struct slot current, int am, int pm) {
    int64_t midnight = current.stamp - slot_clock(current);
    struct slot best = {current.id, current.stamp + MINUTES_PER_DAY};
    for (int day = 0; day <= 1; day++) {
        for (int id = SLOT_AM; id <= SLOT_PM; id++) {
            int64_t planned = midnight + (int64_t)day * MINUTES_PER_DAY + (id == SLOT_AM ? am : pm);
            if (planned > current.stamp && planned < best.stamp) {
                best = (struct slot){(enum slot_id)id, planned};
            }
        }
    }
    return best;
}

int slot_latency(struct slot slot, const struct moment *now) {
    int64_t late = moment_stamp(now) - slot.stamp;
    return late > 0 ? (int)late : 0;
}

int slot_clock(struct slot slot) {
    return (int)(slot.stamp % MINUTES_PER_DAY);
}

int64_t slot_day(struct slot slot) {
    return slot.stamp / MINUTES_PER_DAY;
}

const char *slot_name(enum slot_id id) {
    return id == SLOT_AM ? "AM" : "PM";
}
