#ifndef LOCKIN_CORE_CONFIG_H
#define LOCKIN_CORE_CONFIG_H

#include "core/text.h"

enum {
    CONFIG_PLANS = 3,
    CONFIG_PLAN_MAX = 200,
    CONFIG_HOSTS = 64,
    CONFIG_HOST_MAX = 64,
    CONFIG_NAMES = 16,
    CONFIG_NAME_MAX = 16,
    CONFIG_PATH_MAX = 256,
};

/* Everything the user can set in lockin.conf. Clock values are minutes after
 * midnight. */
struct config {
    int slot_am;
    int slot_pm;
    int hard_stop;
    int commit_minutes;
    int scale;
    char project[CONFIG_PATH_MAX];
    char plans[CONFIG_PLANS][CONFIG_PLAN_MAX];
    int plan_count;
    char block[CONFIG_HOSTS][CONFIG_HOST_MAX];
    int block_count;
    char allow[CONFIG_HOSTS][CONFIG_HOST_MAX];
    int allow_count;
    char kill[CONFIG_NAMES][CONFIG_NAME_MAX];
    int kill_count;
};

/* The commented file written on first run. */
extern const char config_template[];

/* Fills in the built-in defaults. */
void config_defaults(struct config *c);

/* Applies `text` (modified in place) on top of what `c` already holds. Every
 * problem is appended to `problems`, one per line with its line number, so
 * the user can fix them all in one pass. Returns true when there were none. */
bool config_parse(struct config *c, char *text, struct sbuf *problems);

#endif
