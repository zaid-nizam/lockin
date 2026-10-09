#include "core/config.h"

#include "core/blocklist.h"
#include "core/daytime.h"
#include "core/kv.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

const char config_template[] =
    "# lockin settings. Lines starting with # are ignored.\n"
    "# Check this file with: lockin --check\n"
    "\n"
    "# Your two daily start times, 24 hour clock.\n"
    "slot_am = 11:00\n"
    "slot_pm = 23:00\n"
    "\n"
    "# When the late session must end so you still sleep 7 to 9 hours.\n"
    "hard_stop = 02:00\n"
    "\n"
    "# The only promise you make when you start: this many minutes.\n"
    "commit_minutes = 10\n"
    "\n"
    "# Folder of the project you are working on. NEXT.md is kept here.\n"
    "# Leave empty to keep NEXT.md with the session log.\n"
    "project =\n"
    "\n"
    "# Window size: 0 picks for you, 1 to 3 force a zoom level.\n"
    "scale = 0\n"
    "\n"
    "# Your if-then card. Up to three plan lines replace the built-in ones.\n"
    "# plan = If I feel the urge to open Discord, then I press Urge and do one more step.\n"
    "\n"
    "# Extra hosts to block, and built-in hosts to leave alone.\n"
    "# block = example.com\n"
    "# allow = soundcloud.com\n"
    "\n"
    "# Extra programs to close while locked, by process name.\n"
    "# kill = telegram-desktop\n";

static const char *const default_plans[CONFIG_PLANS] = {
    "If it is slot time and I am at the desk, then I press Lock in before touching anything else.",
    "If I feel the urge to open Discord or play a song, then I press Urge and do one more step.",
    "If I am stuck for 10 minutes, then I write the smallest failing test or a printf for the bug.",
};

static const char *const default_kill[] = {"Discord", "DiscordCanary", "DiscordPTB",
                                           "vesktop", "legcord",       "spotify"};

void config_defaults(struct config *c) {
    memset(c, 0, sizeof *c);
    c->slot_am = 11 * 60;
    c->slot_pm = 23 * 60;
    c->hard_stop = 2 * 60;
    c->commit_minutes = 10;
    c->plan_count = CONFIG_PLANS;
    for (int i = 0; i < CONFIG_PLANS; i++) {
        text_copy(c->plans[i], CONFIG_PLAN_MAX, default_plans[i]);
    }
    c->kill_count = (int)(sizeof default_kill / sizeof default_kill[0]);
    for (int i = 0; i < c->kill_count; i++) {
        text_copy(c->kill[i], CONFIG_NAME_MAX, default_kill[i]);
    }
}

static bool set_number(int *field, const char *value, int low, int high) {
    char *end;
    long parsed = strtol(value, &end, 10);
    *field = (int)parsed;
    return value[0] != '\0' && *end == '\0' && parsed >= low && parsed <= high;
}

static bool set_project(struct config *c, const char *value) {
    text_copy(c->project, CONFIG_PATH_MAX, value);
    return (value[0] == '\0' || value[0] == '/') && strlen(value) < CONFIG_PATH_MAX;
}

/* Longer names are accepted and cut to fit: the kernel keeps only the first
 * 15 bytes of a process name, so that is all there is to match against. */
static bool process_name_valid(const char *name) {
    size_t len = strlen(name);
    for (const char *c = name; *c != '\0'; c++) {
        if (!isalnum((unsigned char)*c) && strchr("._-", *c) == NULL) {
            return false;
        }
    }
    return len > 0 && len < CONFIG_HOST_MAX;
}

/* Appends `value` to a fixed list of fixed-size strings. */
static bool add_item(char *list, int *count, size_t width, const char *value) {
    int limit = width == CONFIG_NAME_MAX ? CONFIG_NAMES : CONFIG_HOSTS;
    if (*count >= limit) {
        return false;
    }
    text_copy(list + (size_t)*count * width, width, value);
    (*count)++;
    return true;
}

struct parse {
    struct config *config;
    struct sbuf *problems;
    int line;
    bool plans_replaced;
};

/* The first plan line in a file throws away the built-in card. */
static bool add_plan(struct parse *p, const char *value) {
    struct config *c = p->config;
    if (!p->plans_replaced) {
        c->plan_count = 0;
        p->plans_replaced = true;
    }
    if (c->plan_count >= CONFIG_PLANS || value[0] == '\0' || strlen(value) >= CONFIG_PLAN_MAX) {
        return false;
    }
    text_copy(c->plans[c->plan_count++], CONFIG_PLAN_MAX, value);
    return true;
}

enum setting_id {
    SET_SLOT_AM,
    SET_SLOT_PM,
    SET_HARD_STOP,
    SET_COMMIT,
    SET_SCALE,
    SET_PROJECT,
    SET_PLAN,
    SET_KILL,
    SET_BLOCK,
    SET_ALLOW,
    SET_COUNT,
};

struct setting {
    const char *key;
    const char *expects;
};

static const struct setting settings[SET_COUNT] = {
    [SET_SLOT_AM] = {"slot_am", "a time like 11:00"},
    [SET_SLOT_PM] = {"slot_pm", "a time like 23:00"},
    [SET_HARD_STOP] = {"hard_stop", "a time like 02:00"},
    [SET_COMMIT] = {"commit_minutes", "a whole number from 1 to 120"},
    [SET_SCALE] = {"scale", "0, 1, 2 or 3"},
    [SET_PROJECT] = {"project", "an absolute folder path, or nothing"},
    [SET_PLAN] = {"plan", "up to 3 lines of under 200 characters"},
    [SET_KILL] = {"kill", "up to 10 process names made of letters, digits, dots or dashes"},
    [SET_BLOCK] = {"block", "up to 64 host names like example.com"},
    [SET_ALLOW] = {"allow", "up to 64 host names like example.com"},
};

static bool apply_list(struct parse *p, enum setting_id id, const char *value) {
    struct config *c = p->config;
    switch (id) {
    case SET_PLAN:
        return add_plan(p, value);
    case SET_KILL:
        return process_name_valid(value) &&
               add_item(c->kill[0], &c->kill_count, CONFIG_NAME_MAX, value);
    case SET_BLOCK:
        return host_valid(value) && add_item(c->block[0], &c->block_count, CONFIG_HOST_MAX, value);
    default:
        return host_valid(value) && add_item(c->allow[0], &c->allow_count, CONFIG_HOST_MAX, value);
    }
}

static bool apply(struct parse *p, enum setting_id id, const char *value) {
    struct config *c = p->config;
    switch (id) {
    case SET_SLOT_AM:
        return clock_parse(value, &c->slot_am);
    case SET_SLOT_PM:
        return clock_parse(value, &c->slot_pm);
    case SET_HARD_STOP:
        return clock_parse(value, &c->hard_stop);
    case SET_COMMIT:
        return set_number(&c->commit_minutes, value, 1, 120);
    case SET_SCALE:
        return set_number(&c->scale, value, 0, 3);
    case SET_PROJECT:
        return set_project(c, value);
    default:
        return apply_list(p, id, value);
    }
}

static void apply_line(struct parse *p, char *line) {
    char *key, *value;
    if (!kv_split(line, &key, &value)) {
        return;
    }
    for (int id = 0; id < SET_COUNT; id++) {
        if (strcmp(key, settings[id].key) != 0) {
            continue;
        }
        if (!apply(p, (enum setting_id)id, value)) {
            sbuf_putf(p->problems, "line %d: %s = \"%s\" is not valid. Expected %s.\n", p->line,
                      key, value, settings[id].expects);
        }
        return;
    }
    sbuf_putf(p->problems, "line %d: unknown setting \"%s\".\n", p->line, key);
}

bool config_parse(struct config *c, char *text, struct sbuf *problems) {
    struct parse p = {c, problems, 0, false};
    for (char *line = kv_line(&text); line != NULL; line = kv_line(&text)) {
        p.line++;
        apply_line(&p, line);
    }
    if (c->slot_am == c->slot_pm) {
        sbuf_put(problems, "slot_am and slot_pm are the same time. Give two different times.\n");
    }
    return problems->len == 0;
}
