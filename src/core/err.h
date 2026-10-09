#ifndef LOCKIN_CORE_ERR_H
#define LOCKIN_CORE_ERR_H

#include <stdbool.h>

/* Stable codes: they appear in the log as E_IO, E_CONFIG and so on. */
enum err_code {
    ERR_NONE,
    ERR_IO,
    ERR_CONFIG,
    ERR_INPUT,
    ERR_PRIVILEGE,
    ERR_BLOCK,
    ERR_DISPLAY,
};

/* The one error type. `text` is written for the person at the keyboard:
 * what failed, on what, and what to do about it. */
struct error {
    enum err_code code;
    char text[320];
};

/* Records a failure. Always returns false so callers can `return err_set(...)`. */
bool err_set(struct error *err, enum err_code code, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* Returns the log name of a code, for example "E_IO". */
const char *err_name(enum err_code code);

#endif
