#ifndef LOCKIN_SYS_SYS_LINUX_H
#define LOCKIN_SYS_SYS_LINUX_H

/* Shared by the Linux implementation files only. */

#include <stdbool.h>
#include <sys/types.h>

/* The user lockin works for: the one behind sudo, or whoever ran it. */
uid_t linux_user(void);

/* Switches the effective user to root (true) or back to linux_user (false).
 * Returns false when the process has no root to switch to. */
bool linux_act_as_root(bool on);

/* Writes all of `text` to `fd`, retrying short writes. */
bool linux_write_all(int fd, const char *text);

#endif
