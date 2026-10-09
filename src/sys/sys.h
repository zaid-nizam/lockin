#ifndef LOCKIN_SYS_SYS_H
#define LOCKIN_SYS_SYS_H

/* Everything lockin needs from the operating system that is not a window.
 * One implementation per OS: sys_linux*.c today. Porting to another OS means
 * writing these functions for it; nothing above this header changes.
 *
 * The functions keep process-wide facts (who ran the program, where its
 * folders are), so sys_start must be called once before any other. */

#include "core/config.h"
#include "core/daytime.h"
#include "core/err.h"
#include "core/text.h"

#include <stdint.h>

enum sys_dir { SYS_DIR_CONFIG, SYS_DIR_STATE, SYS_DIR_DATA, SYS_DIR_COUNT };

/* Works out which user ran the program (the one behind sudo, if any), stops
 * acting as root for ordinary file work, and creates the three folders. */
bool sys_start(struct error *err);

/* True when the program was started with the rights sys_block needs. */
bool sys_can_block(void);

/* The folder for settings, state or data, without a trailing slash. */
const char *sys_dir(enum sys_dir which);

/* The current local time. */
void sys_now(struct moment *out);

/* 64 unpredictable bits. */
uint64_t sys_random(void);

/* Reads a text file into `out`. A missing file reads as empty. When the file
 * is larger than the buffer only its tail is kept, starting at a line break,
 * and `out->truncated` is set. */
bool sys_read(const char *path, struct sbuf *out, struct error *err);

/* Replaces a file with `text` in one step: a reader sees the old file or the
 * new one, never half of each. */
bool sys_write(const char *path, const char *text, struct error *err);

/* Adds `text` to the end of a file, creating it if needed. */
bool sys_append(const char *path, const char *text, struct error *err);

/* Reads the hosts file as the system currently sees it. */
bool sys_hosts(struct sbuf *out, struct error *err);

/* Makes `hosts` the system's hosts file until the next reboot. The file on
 * disk is not touched, so a crash or power cut cannot leave the block stuck.
 * Calling it again replaces the text. */
bool sys_block(const char *hosts, struct error *err);

/* True when sys_block has been applied since the last reboot. */
bool sys_blocked(void);

/* Force-closes the invoking user's programs whose process name is in the
 * settings' kill list, ignoring case. Returns how many were closed. */
int sys_close_programs(const struct config *config);

/* From now on a fatal signal (a crash) appends the signal number and a stack
 * trace to `path` and to stderr before the process dies. */
void sys_trace_crashes(const char *path);

/* Makes the process ignore the polite ways of stopping it: Ctrl-C, closing
 * the terminal it was started from, and plain `kill`. */
void sys_refuse_to_stop(void);

#endif
