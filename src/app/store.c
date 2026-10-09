#include "app/store.h"

#include "core/kv.h"
#include "sys/sys.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PATH_MAX_LEN = 400, LOG_BYTES = 256 * 1024 };

static void path_in(enum sys_dir dir, const char *name, struct sbuf *out) {
    sbuf_putf(out, "%s/%s", sys_dir(dir), name);
}

/* Reads a file whose absence is normal. A file that exists but cannot be
 * read is logged and treated as empty, so the program still starts. */
static void read_or_log(const char *path, struct sbuf *out) {
    struct error err;
    if (!sys_read(path, out, &err)) {
        store_log_error(&err);
    }
}

void store_log_path(struct sbuf *out) {
    path_in(SYS_DIR_STATE, "lockin.log", out);
}

void store_log(const char *level, const char *message) {
    char path[PATH_MAX_LEN], line[512];
    struct sbuf file = sbuf_on(path, sizeof path);
    struct sbuf text = sbuf_on(line, sizeof line);
    struct moment now;
    struct error ignored;
    sys_now(&now);
    sbuf_putf(&text, "%04d-%02d-%02d %02d:%02d:%02d %s ", now.year, now.month, now.day, now.hour,
              now.minute, now.second, level);
    sbuf_put(&text, message);
    sbuf_put(&text, "\n");
    fputs(line, stderr);
    store_log_path(&file);
    /* The log is the last resort for reporting; if it cannot be written,
     * stderr above is all there is. */
    sys_append(path, line, &ignored);
}

void store_log_error(const struct error *err) {
    char line[400];
    struct sbuf text = sbuf_on(line, sizeof line);
    sbuf_putf(&text, "%s ", err_name(err->code));
    sbuf_put(&text, err->text);
    store_log("ERROR", line);
}

bool store_config(struct config *config, struct sbuf *problems, struct error *err) {
    char path[PATH_MAX_LEN], text[16 * 1024];
    struct sbuf file = sbuf_on(path, sizeof path);
    struct sbuf content = sbuf_on(text, sizeof text);
    path_in(SYS_DIR_CONFIG, "lockin.conf", &file);
    config_defaults(config);
    if (!sys_read(path, &content, err)) {
        return false;
    }
    if (content.len == 0) {
        return sys_write(path, config_template, err);
    }
    if (!config_parse(config, text, problems)) {
        return err_set(err, ERR_CONFIG,
                       "%s has settings that are not valid. Fix them and start "
                       "again.",
                       path);
    }
    return true;
}

static void state_set(struct state *state, const char *key, const char *value) {
    struct {
        const char *key;
        char *field;
        size_t cap;
    } texts[] = {
        {"wish", state->woop.wish, sizeof state->woop.wish},
        {"outcome", state->woop.outcome, sizeof state->woop.outcome},
        {"obstacle", state->woop.obstacle, sizeof state->woop.obstacle},
        {"plan", state->woop.plan, sizeof state->woop.plan},
        {"bedtime", state->bedtime, sizeof state->bedtime},
        {"waketime", state->waketime, sizeof state->waketime},
    };
    if (strcmp(key, "seed") == 0) {
        state->seed = strtoull(value, NULL, 10);
    }
    for (size_t i = 0; i < sizeof texts / sizeof texts[0]; i++) {
        if (strcmp(key, texts[i].key) == 0) {
            text_copy(texts[i].field, texts[i].cap, value);
        }
    }
}

void store_state_load(struct state *state) {
    char path[PATH_MAX_LEN], text[4096];
    struct sbuf file = sbuf_on(path, sizeof path);
    struct sbuf content = sbuf_on(text, sizeof text);
    char *cursor = text;
    memset(state, 0, sizeof *state);
    path_in(SYS_DIR_STATE, "state", &file);
    read_or_log(path, &content);
    for (char *line = kv_line(&cursor); line != NULL; line = kv_line(&cursor)) {
        char *key, *value;
        if (kv_split(line, &key, &value)) {
            state_set(state, key, value);
        }
    }
    if (state->seed == 0) {
        state->seed = sys_random() | 1;
    }
}

bool store_state_save(const struct state *state, struct error *err) {
    char path[PATH_MAX_LEN], text[1024];
    struct sbuf file = sbuf_on(path, sizeof path);
    struct sbuf content = sbuf_on(text, sizeof text);
    path_in(SYS_DIR_STATE, "state", &file);
    sbuf_putf(&content, "seed = %" PRIu64 "\n", state->seed);
    sbuf_putf(&content, "wish = %s\noutcome = %s\n", state->woop.wish, state->woop.outcome);
    sbuf_putf(&content, "obstacle = %s\nplan = %s\n", state->woop.obstacle, state->woop.plan);
    sbuf_putf(&content, "bedtime = %s\nwaketime = %s\n", state->bedtime, state->waketime);
    return sys_write(path, text, err);
}

int store_rows_load(struct record *rows, int max) {
    char path[PATH_MAX_LEN], text[LOG_BYTES];
    struct sbuf file = sbuf_on(path, sizeof path);
    struct sbuf content = sbuf_on(text, sizeof text);
    char *cursor = text;
    int count = 0;
    path_in(SYS_DIR_DATA, "sessions.csv", &file);
    read_or_log(path, &content);
    for (char *line = kv_line(&cursor); line != NULL; line = kv_line(&cursor)) {
        if (count == max) {
            /* Full: drop the oldest row to keep the newest. */
            memmove(rows, rows + 1, (size_t)(max - 1) * sizeof rows[0]);
            count--;
        }
        count += record_read(line, &rows[count]);
    }
    return count;
}

bool store_row_append(const struct record *row, struct error *err) {
    char path[PATH_MAX_LEN], probe[8], text[512];
    struct sbuf file = sbuf_on(path, sizeof path);
    struct sbuf existing = sbuf_on(probe, sizeof probe);
    struct sbuf line = sbuf_on(text, sizeof text);
    path_in(SYS_DIR_DATA, "sessions.csv", &file);
    if (!sys_read(path, &existing, err)) {
        return false;
    }
    bool empty = existing.len == 0 && !existing.truncated;
    if (empty && !sys_append(path, record_header, err)) {
        return false;
    }
    record_write(row, &line);
    return sys_append(path, text, err);
}

static void next_path(const struct config *config, struct sbuf *out) {
    if (config->project[0] != '\0') {
        sbuf_putf(out, "%s/NEXT.md", config->project);
    } else {
        path_in(SYS_DIR_DATA, "NEXT.md", out);
    }
}

void store_next_load(const struct config *config, char out[STORE_NEXT_MAX]) {
    char path[PATH_MAX_LEN], text[2048];
    struct sbuf file = sbuf_on(path, sizeof path);
    struct sbuf content = sbuf_on(text, sizeof text);
    char *cursor = text;
    out[0] = '\0';
    next_path(config, &file);
    read_or_log(path, &content);
    for (char *line = kv_line(&cursor); line != NULL && out[0] == '\0'; line = kv_line(&cursor)) {
        text_copy(out, STORE_NEXT_MAX, text_trim(line));
    }
}

bool store_next_save(const struct config *config, const char *step, struct error *err) {
    char path[PATH_MAX_LEN], text[STORE_NEXT_MAX + 2];
    struct sbuf file = sbuf_on(path, sizeof path);
    struct sbuf content = sbuf_on(text, sizeof text);
    next_path(config, &file);
    sbuf_putf(&content, "%s\n", step);
    return sys_write(path, text, err);
}
