#include "core/daytime.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

bool clock_parse(const char *s, int *minutes) {
    if (strlen(s) != 5 || s[2] != ':') {
        return false;
    }
    if (!isdigit((unsigned char)s[0]) || !isdigit((unsigned char)s[1]) ||
        !isdigit((unsigned char)s[3]) || !isdigit((unsigned char)s[4])) {
        return false;
    }
    int hour = (s[0] - '0') * 10 + (s[1] - '0');
    int minute = (s[3] - '0') * 10 + (s[4] - '0');
    if (hour > 23 || minute > 59) {
        return false;
    }
    *minutes = hour * 60 + minute;
    return true;
}

void clock_format(int minutes, char out[6]) {
    if (minutes < 0 || minutes >= MINUTES_PER_DAY) {
        out[0] = '\0';
        return;
    }
    snprintf(out, 6, "%02d:%02d", minutes / 60, minutes % 60);
}

/* Howard Hinnant's days_from_civil: exact for every Gregorian date. */
int64_t civil_days(int year, int month, int day) {
    int64_t y = month <= 2 ? year - 1 : year;
    int64_t era = (y >= 0 ? y : y - 399) / 400;
    int64_t year_of_era = y - era * 400;
    int64_t day_of_year = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    int64_t day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
    return era * 146097 + day_of_era - 719468;
}

void civil_date(int64_t days, int *year, int *month, int *day) {
    int64_t z = days + 719468;
    int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    int64_t day_of_era = z - era * 146097;
    int64_t year_of_era =
        (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
    int64_t day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
    int64_t shifted_month = (5 * day_of_year + 2) / 153;
    *day = (int)(day_of_year - (153 * shifted_month + 2) / 5 + 1);
    *month = (int)(shifted_month < 10 ? shifted_month + 3 : shifted_month - 9);
    *year = (int)(year_of_era + era * 400 + (*month <= 2 ? 1 : 0));
}

int moment_clock(const struct moment *m) {
    return m->hour * 60 + m->minute;
}

int64_t moment_stamp(const struct moment *m) {
    return civil_days(m->year, m->month, m->day) * MINUTES_PER_DAY + moment_clock(m);
}
