#include "core/record.h"

#include "core/csv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char record_header[] =
    "date,slot,status,planned,started,latency_min,focused_min,urges,urge_rating,sound,"
    "exercise_min,caffeine_mg,caffeine_at,bedtime,waketime,cowork,next_present,week,coin,output\n";

static const char *const sound_names[SOUND_COUNT] = {"silence", "pink", "instrumental", "lyrics"};

const char *sound_name(enum sound sound) {
    return sound_names[sound];
}

static void put_clock(struct sbuf *out, int minutes) {
    char text[6];
    clock_format(minutes, text);
    csv_put(out, text, false);
}

static void put_int(struct sbuf *out, int value) {
    char text[16];
    snprintf(text, sizeof text, "%d", value);
    csv_put(out, text, false);
}

void record_write(const struct record *r, struct sbuf *out) {
    int year, month, day;
    const char coin[2] = {r->coin, '\0'};
    civil_date(r->day, &year, &month, &day);
    sbuf_putf(out, "%04d-%02d-%02d,%s,%s,", year, month, day, slot_name(r->slot),
              r->missed ? "missed" : "done");
    put_clock(out, r->planned);
    put_clock(out, r->started);
    put_int(out, r->latency);
    put_int(out, r->focused);
    put_int(out, r->urges);
    put_int(out, r->urge_rating);
    csv_put(out, sound_name(r->sound), false);
    put_int(out, r->exercise_min);
    put_int(out, r->caffeine_mg);
    put_clock(out, r->caffeine_at);
    put_clock(out, r->bedtime);
    put_clock(out, r->waketime);
    put_int(out, r->cowork);
    put_int(out, r->next_present);
    put_int(out, r->week);
    csv_put(out, coin, false);
    csv_put(out, r->output, true);
}

static bool read_int(const char *field, int max, int *value) {
    char *end;
    long parsed = strtol(field, &end, 10);
    *value = (int)parsed;
    return field[0] != '\0' && *end == '\0' && parsed >= 0 && parsed <= max;
}

static bool read_clock(const char *field, int *minutes) {
    *minutes = CLOCK_NONE;
    return field[0] == '\0' || clock_parse(field, minutes);
}

/* Reads the digits at `*field` up to `stop` and steps past it. */
static bool read_part(const char **field, char stop, int *value) {
    char *end;
    long parsed = strtol(*field, &end, 10);
    bool ok = end != *field && *end == stop && parsed >= 0 && parsed <= 9999;
    *value = (int)parsed;
    *field = *end != '\0' ? end + 1 : end;
    return ok;
}

static bool read_date(const char *field, int64_t *day) {
    int year, month, date;
    if (!read_part(&field, '-', &year) || !read_part(&field, '-', &month) ||
        !read_part(&field, '\0', &date)) {
        return false;
    }
    *day = civil_days(year, month, date);
    return month >= 1 && month <= 12 && date >= 1 && date <= 31;
}

/* Sets `*index` to the position of `field` in `names`; false when absent. */
static bool read_choice(const char *field, const char *const *names, int count, int *index) {
    for (*index = 0; *index < count; (*index)++) {
        if (strcmp(field, names[*index]) == 0) {
            return true;
        }
    }
    return false;
}

static bool read_flags(char **f, struct record *r) {
    static const char *const slots[] = {"AM", "PM"};
    static const char *const statuses[] = {"done", "missed"};
    static const char *const coins[] = {"A", "B", "N"};
    int slot, status, sound, cowork, next_present, coin;
    bool ok = read_choice(f[1], slots, 2, &slot) && read_choice(f[2], statuses, 2, &status) &&
              read_choice(f[9], sound_names, SOUND_COUNT, &sound) && read_int(f[15], 1, &cowork) &&
              read_int(f[16], 1, &next_present) && read_choice(f[18], coins, 3, &coin);
    if (ok) {
        r->slot = (enum slot_id)slot;
        r->missed = status == 1;
        r->sound = (enum sound)sound;
        r->cowork = cowork == 1;
        r->next_present = next_present == 1;
        r->coin = coins[coin][0];
    }
    return ok;
}

static bool read_times(char **f, struct record *r) {
    return read_clock(f[3], &r->planned) && read_clock(f[4], &r->started) &&
           read_clock(f[12], &r->caffeine_at) && read_clock(f[13], &r->bedtime) &&
           read_clock(f[14], &r->waketime);
}

static bool read_counts(char **f, struct record *r) {
    return read_int(f[5], 100000, &r->latency) && read_int(f[6], 100000, &r->focused) &&
           read_int(f[7], 100000, &r->urges) && read_int(f[8], 5, &r->urge_rating) &&
           read_int(f[10], 600, &r->exercise_min) && read_int(f[11], 2000, &r->caffeine_mg) &&
           read_int(f[17], 100000, &r->week);
}

bool record_read(char *line, struct record *r) {
    char *f[RECORD_FIELDS];
    if (csv_split(line, f, RECORD_FIELDS) != RECORD_FIELDS) {
        return false;
    }
    text_copy(r->output, sizeof r->output, f[19]);
    return read_date(f[0], &r->day) && read_flags(f, r) && read_times(f, r) && read_counts(f, r);
}
