#ifndef LOCKIN_TESTS_FAKE_SYS_H
#define LOCKIN_TESTS_FAKE_SYS_H

/* An in-memory stand-in for sys_linux*.c: files live in an array, the clock
 * is set by the test, and any call can be told to fail. */

#include "sys/sys.h"

struct fake_sys {
    struct moment now;
    bool can_block;
    bool blocked;
    bool refused_to_stop;
    int close_calls;
    int programs_to_close;
    uint64_t random;
    /* Calls on a path containing this text fail. Empty means none. */
    const char *fail_read;
    const char *fail_write;
    const char *fail_append;
    bool fail_block;
    char hosts[70000];
    char blocked_hosts[70000];
};

extern struct fake_sys fake;

/* Empties every file and resets the clock to 2026-10-12 23:04. */
void fake_reset(void);

/* Creates or replaces a file. */
void fake_file_set(const char *path, const char *text);

/* The contents of a file, or NULL when it does not exist. */
const char *fake_file(const char *path);

/* Moves the clock forward. */
void fake_advance(int seconds);

#endif
