#include "fake_sys.h"

#include <string.h>

enum { FILES = 12, FILE_BYTES = 300 * 1024, PATH_BYTES = 400 };

struct fake_sys fake;

static struct {
    char path[PATH_BYTES];
    char text[FILE_BYTES];
} files[FILES];

void fake_reset(void) {
    memset(&fake, 0, sizeof fake);
    memset(files, 0, sizeof files);
    fake.now = (struct moment){2026, 10, 12, 23, 4, 0, 1791846240};
    fake.can_block = true;
    fake.random = 0x1234abcdULL;
    fake.fail_read = fake.fail_write = fake.fail_append = "";
    text_copy(fake.hosts, sizeof fake.hosts, "127.0.0.1 localhost\n");
}

static int find(const char *path, bool create) {
    for (int i = 0; i < FILES; i++) {
        if (strcmp(files[i].path, path) == 0) {
            return i;
        }
    }
    for (int i = 0; create && i < FILES; i++) {
        if (files[i].path[0] == '\0') {
            text_copy(files[i].path, PATH_BYTES, path);
            return i;
        }
    }
    return -1;
}

void fake_file_set(const char *path, const char *text) {
    int i = find(path, true);
    text_copy(files[i].text, FILE_BYTES, text);
}

const char *fake_file(const char *path) {
    int i = find(path, false);
    return i < 0 ? NULL : files[i].text;
}

void fake_advance(int seconds) {
    int64_t stamp = moment_stamp(&fake.now) + seconds / 60;
    fake.now.epoch += seconds;
    civil_date(stamp / MINUTES_PER_DAY, &fake.now.year, &fake.now.month, &fake.now.day);
    fake.now.hour = (int)(stamp % MINUTES_PER_DAY) / 60;
    fake.now.minute = (int)(stamp % MINUTES_PER_DAY) % 60;
}

static bool failing(const char *pattern, const char *path) {
    return pattern[0] != '\0' && strstr(path, pattern) != NULL;
}

bool sys_start(struct error *err) {
    (void)err;
    return true;
}

bool sys_can_block(void) {
    return fake.can_block;
}

const char *sys_dir(enum sys_dir which) {
    static const char *const dirs[] = {"/cfg", "/state", "/data"};
    return dirs[which];
}

void sys_now(struct moment *out) {
    *out = fake.now;
}

uint64_t sys_random(void) {
    return fake.random;
}

bool sys_read(const char *path, struct sbuf *out, struct error *err) {
    const char *text = fake_file(path);
    if (failing(fake.fail_read, path)) {
        return err_set(err, ERR_IO, "Cannot read %s: fake failure.", path);
    }
    if (text == NULL) {
        return true;
    }
    size_t length = strlen(text);
    size_t room = out->cap - 1;
    if (length > room) {
        /* Same contract as the real one: keep the tail from a line start. */
        const char *tail = strchr(text + (length - room), '\n');
        out->truncated = true;
        text = tail != NULL ? tail + 1 : text + length;
    }
    sbuf_put(out, text);
    return true;
}

bool sys_write(const char *path, const char *text, struct error *err) {
    if (failing(fake.fail_write, path)) {
        return err_set(err, ERR_IO, "Cannot save %s: fake failure.", path);
    }
    fake_file_set(path, text);
    return true;
}

bool sys_append(const char *path, const char *text, struct error *err) {
    if (failing(fake.fail_append, path)) {
        return err_set(err, ERR_IO, "Cannot add to %s: fake failure.", path);
    }
    int i = find(path, true);
    struct sbuf file = {files[i].text, FILE_BYTES, strlen(files[i].text), false};
    sbuf_put(&file, text);
    return true;
}

bool sys_hosts(struct sbuf *out, struct error *err) {
    if (failing(fake.fail_read, "/etc/hosts")) {
        return err_set(err, ERR_IO, "Cannot read /etc/hosts: fake failure.");
    }
    sbuf_put(out, fake.blocked ? fake.blocked_hosts : fake.hosts);
    return true;
}

bool sys_block(const char *hosts, struct error *err) {
    if (fake.fail_block) {
        return err_set(err, ERR_BLOCK, "Cannot put the block list over /etc/hosts: fake failure.");
    }
    text_copy(fake.blocked_hosts, sizeof fake.blocked_hosts, hosts);
    fake.blocked = true;
    return true;
}

bool sys_blocked(void) {
    return fake.blocked;
}

int sys_close_programs(const struct config *config) {
    (void)config;
    fake.close_calls++;
    return fake.programs_to_close;
}

void sys_trace_crashes(const char *path) {
    (void)path;
}

void sys_refuse_to_stop(void) {
    fake.refused_to_stop = true;
}
