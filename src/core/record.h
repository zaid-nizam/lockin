#ifndef LOCKIN_CORE_RECORD_H
#define LOCKIN_CORE_RECORD_H

#include "core/slot.h"
#include "core/text.h"

enum sound { SOUND_SILENCE, SOUND_PINK, SOUND_INSTRUMENTAL, SOUND_LYRICS, SOUND_COUNT };

enum { RECORD_FIELDS = 20, RECORD_OUTPUT_MAX = 120 };

/* One row of the session log: one planned slot that was either worked or
 * missed. Clock fields are minutes after midnight, CLOCK_NONE when unknown. */
struct record {
    int64_t day;
    enum slot_id slot;
    bool missed;
    int planned;
    int started;
    int latency;
    int focused;
    int urges;
    int urge_rating;
    enum sound sound;
    int exercise_min;
    int caffeine_mg;
    int caffeine_at;
    int bedtime;
    int waketime;
    bool cowork;
    bool next_present;
    int week;
    char coin;
    char output[RECORD_OUTPUT_MAX + 1];
};

/* The CSV header line, ending in a newline. */
extern const char record_header[];

/* Appends the record as one CSV line. */
void record_write(const struct record *r, struct sbuf *out);

/* Parses one CSV line. Returns false when the line is not a valid record,
 * which includes the header line. */
bool record_read(char *line, struct record *r);

/* "silence", "pink", "instrumental" or "lyrics". */
const char *sound_name(enum sound sound);

#endif
