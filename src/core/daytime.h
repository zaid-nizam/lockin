#ifndef LOCKIN_CORE_DAYTIME_H
#define LOCKIN_CORE_DAYTIME_H

#include <stdbool.h>
#include <stdint.h>

enum { MINUTES_PER_DAY = 1440, CLOCK_NONE = -1 };

/* A point in local time. `epoch` is seconds since 1970 UTC and is only used
 * to measure durations; everything about slots and dates uses the local
 * fields. Daylight saving shifts are not modelled. */
struct moment {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int64_t epoch;
};

/* Parses "HH:MM" (24 hour) into minutes after midnight. */
bool clock_parse(const char *s, int *minutes);

/* Formats minutes after midnight as "HH:MM". CLOCK_NONE gives "". */
void clock_format(int minutes, char out[6]);

/* Days from 1970-01-01 to the given date in the proleptic Gregorian calendar. */
int64_t civil_days(int year, int month, int day);

/* The inverse of civil_days. */
void civil_date(int64_t days, int *year, int *month, int *day);

/* Local minutes since 1970-01-01 00:00, ignoring time zones. */
int64_t moment_stamp(const struct moment *m);

/* Minutes after midnight for a moment. */
int moment_clock(const struct moment *m);

#endif
