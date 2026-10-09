#include "core/csv.h"
#include "core/daytime.h"
#include "core/err.h"
#include "core/kv.h"
#include "core/text.h"
#include "suites.h"
#include "test.h"

#include <string.h>

static void builder_drops_what_does_not_fit_and_says_so(void) {
    char storage[8];
    struct sbuf sb = sbuf_on(storage, sizeof storage);
    sbuf_put(&sb, "abc");
    CHECK(!sb.truncated);
    sbuf_put(&sb, "defghij");
    CHECK_STR(storage, "abcdefg");
    CHECK(sb.truncated);
    sbuf_put(&sb, "more");
    CHECK_STR(storage, "abcdefg");
}

static void builder_formats_and_flags_an_oversized_piece(void) {
    char storage[700], huge[600];
    struct sbuf sb = sbuf_on(storage, sizeof storage);
    sbuf_putf(&sb, "%d-%s", 7, "x");
    CHECK_STR(storage, "7-x");
    memset(huge, 'a', sizeof huge - 1);
    huge[sizeof huge - 1] = '\0';
    sbuf_putf(&sb, "%s", huge);
    CHECK(sb.truncated);
}

static void copy_always_terminates(void) {
    char small[4];
    text_copy(small, sizeof small, "abcdef");
    CHECK_STR(small, "abc");
}

static void trim_removes_space_on_both_sides(void) {
    char padded[] = " \t hello world \n";
    char blank[] = "   ";
    CHECK_STR(text_trim(padded), "hello world");
    CHECK_STR(text_trim(blank), "");
}

static void compares_names_without_case(void) {
    CHECK(text_same_nocase("Discord", "discord"));
    CHECK(!text_same_nocase("Discord", "discordx"));
    CHECK(!text_same_nocase("discordx", "Discord"));
    CHECK(text_same_nocase("", ""));
}

static void non_ascii_reads_as_one_question_mark_per_character(void) {
    const char *s = "a\xE2\x80\x94z";
    CHECK_INT(text_next_ascii(&s), 'a');
    CHECK_INT(text_next_ascii(&s), '?');
    CHECK_INT(text_next_ascii(&s), 'z');
    CHECK_INT(*s, '\0');
}

static void error_keeps_code_and_message_and_returns_false(void) {
    struct error err;
    CHECK(!err_set(&err, ERR_BLOCK, "cannot mount %s", "/etc/hosts"));
    CHECK_INT(err.code, ERR_BLOCK);
    CHECK_STR(err.text, "cannot mount /etc/hosts");
    CHECK_STR(err_name(ERR_BLOCK), "E_BLOCK");
    CHECK_STR(err_name(ERR_NONE), "OK");
}

static void clock_parses_only_real_times(void) {
    int minutes = 0;
    CHECK(clock_parse("00:00", &minutes));
    CHECK_INT(minutes, 0);
    CHECK(clock_parse("23:59", &minutes));
    CHECK_INT(minutes, 1439);
    CHECK(!clock_parse("24:00", &minutes));
    CHECK(!clock_parse("12:60", &minutes));
    CHECK(!clock_parse("1:30", &minutes));
    CHECK(!clock_parse("12-30", &minutes));
    CHECK(!clock_parse("1a:30", &minutes));
    CHECK(!clock_parse("a1:30", &minutes));
    CHECK(!clock_parse("11:3a", &minutes));
    CHECK(!clock_parse("11:a3", &minutes));
    CHECK(!clock_parse("", &minutes));
}

static void clock_formats_and_leaves_unknown_empty(void) {
    char text[6];
    clock_format(9 * 60 + 5, text);
    CHECK_STR(text, "09:05");
    clock_format(CLOCK_NONE, text);
    CHECK_STR(text, "");
    clock_format(MINUTES_PER_DAY, text);
    CHECK_STR(text, "");
}

static void day_numbers_round_trip_across_leap_years_and_eras(void) {
    static const int dates[][3] = {{1970, 1, 1},   {2000, 2, 29},  {2024, 2, 29},
                                   {2026, 10, 12}, {2026, 12, 31}, {2027, 1, 1},
                                   {1969, 12, 31}, {1900, 3, 1},   {-1, 6, 15}};
    CHECK_INT(civil_days(1970, 1, 1), 0);
    CHECK_INT(civil_days(2026, 10, 12), 20738);
    for (int i = 0; i < 9; i++) {
        int year, month, day;
        civil_date(civil_days(dates[i][0], dates[i][1], dates[i][2]), &year, &month, &day);
        CHECK_INT(year, dates[i][0]);
        CHECK_INT(month, dates[i][1]);
        CHECK_INT(day, dates[i][2]);
    }
}

static void csv_quotes_commas_and_doubles_quotes(void) {
    char storage[128], line[128];
    char *fields[4];
    struct sbuf sb = sbuf_on(storage, sizeof storage);
    csv_put(&sb, "plain", false);
    csv_put(&sb, "a,b", false);
    csv_put(&sb, "say \"hi\"", false);
    csv_put(&sb, "two\nlines", true);
    CHECK_STR(storage, "plain,\"a,b\",\"say \"\"hi\"\"\",two lines\n");
    text_copy(line, sizeof line, storage);
    CHECK_INT(csv_split(line, fields, 4), 4);
    CHECK_STR(fields[1], "a,b");
    CHECK_STR(fields[2], "say \"hi\"");
    CHECK_STR(fields[3], "two lines");
}

static void csv_defuses_spreadsheet_formulas(void) {
    char storage[64];
    struct sbuf sb = sbuf_on(storage, sizeof storage);
    csv_put(&sb, "=HYPERLINK(1)", false);
    csv_put(&sb, "@cmd", false);
    csv_put(&sb, "", true);
    CHECK_STR(storage, "'=HYPERLINK(1),'@cmd,\n");
}

static void csv_split_stops_at_the_field_limit(void) {
    char line[] = "a,b,c,d\r\n";
    char *fields[2];
    CHECK_INT(csv_split(line, fields, 2), 2);
    CHECK_STR(fields[0], "a");
    CHECK_STR(fields[1], "b");
}

static void settings_lines_split_and_skip_comments(void) {
    char text[] = "  key = some value  \n# note\n\nno equals\nlast=1";
    char *cursor = text, *key, *value;
    CHECK(kv_split(kv_line(&cursor), &key, &value));
    CHECK_STR(key, "key");
    CHECK_STR(value, "some value");
    CHECK(!kv_split(kv_line(&cursor), &key, &value));
    CHECK(!kv_split(kv_line(&cursor), &key, &value));
    CHECK(!kv_split(kv_line(&cursor), &key, &value));
    CHECK(kv_split(kv_line(&cursor), &key, &value));
    CHECK_STR(value, "1");
    CHECK(kv_line(&cursor) == NULL);
}

void suite_text(void) {
    RUN(builder_drops_what_does_not_fit_and_says_so);
    RUN(builder_formats_and_flags_an_oversized_piece);
    RUN(copy_always_terminates);
    RUN(trim_removes_space_on_both_sides);
    RUN(compares_names_without_case);
    RUN(non_ascii_reads_as_one_question_mark_per_character);
    RUN(error_keeps_code_and_message_and_returns_false);
    RUN(clock_parses_only_real_times);
    RUN(clock_formats_and_leaves_unknown_empty);
    RUN(day_numbers_round_trip_across_leap_years_and_eras);
    RUN(csv_quotes_commas_and_doubles_quotes);
    RUN(csv_defuses_spreadsheet_formulas);
    RUN(csv_split_stops_at_the_field_limit);
    RUN(settings_lines_split_and_skip_comments);
}
