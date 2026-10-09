#ifndef LOCKIN_APP_STORE_H
#define LOCKIN_APP_STORE_H

/* The files lockin keeps: settings, remembered answers, the session log, the
 * error log and NEXT.md. Every path is built here and nowhere else. */

#include "core/config.h"
#include "core/err.h"
#include "core/record.h"

#include <stdint.h>

enum { STORE_NEXT_MAX = 200 };

/* The start-of-session answers, remembered so the ritual stays short. */
struct woop {
    char wish[120];
    char outcome[120];
    char obstacle[120];
    char plan[160];
};

/* What survives between runs besides the log. */
struct state {
    uint64_t seed;
    struct woop woop;
    char bedtime[6];
    char waketime[6];
};

/* Appends one line to lockin.log and to stderr: time, level, message. */
void store_log(const char *level, const char *message);

/* Logs an error once, with its code. */
void store_log_error(const struct error *err);

/* Path of the error log, for the crash tracer. */
void store_log_path(struct sbuf *out);

/* Loads lockin.conf over the defaults, writing a commented file on first
 * run. Returns false when the file has problems (listed in `problems`) or
 * cannot be read (described in `err`). */
bool store_config(struct config *config, struct sbuf *problems, struct error *err);

/* Loads the remembered answers. A missing or damaged file gives a fresh
 * state with a new random seed. */
void store_state_load(struct state *state);

bool store_state_save(const struct state *state, struct error *err);

/* Loads up to `max` rows from the end of the session log, oldest first.
 * Lines that are not valid rows are skipped. */
int store_rows_load(struct record *rows, int max);

/* Appends one row to the session log, writing the header first if needed. */
bool store_row_append(const struct record *row, struct error *err);

/* Reads the first non-empty line of NEXT.md into `out`, or leaves it empty. */
void store_next_load(const struct config *config, char out[STORE_NEXT_MAX]);

/* Replaces NEXT.md with one line. */
bool store_next_save(const struct config *config, const char *step, struct error *err);

#endif
