#include "sys/sys_linux.h"
#include "sys/sys.h"

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

enum { DIR_MAX = 256 };

/* Facts about this process, set once by sys_start. */
static struct {
    uid_t user;
    bool root;
    char dirs[SYS_DIR_COUNT][DIR_MAX];
} self;

uid_t linux_user(void) {
    return self.user;
}

bool linux_act_as_root(bool on) {
    return self.root && seteuid(on ? 0 : self.user) == 0;
}

bool linux_write_all(int fd, const char *text) {
    size_t left = strlen(text);
    while (left > 0) {
        ssize_t wrote = write(fd, text, left);
        if (wrote < 0 && errno == EINTR) {
            continue;
        }
        if (wrote <= 0) {
            return false;
        }
        text += wrote;
        left -= (size_t)wrote;
    }
    return true;
}

/* The uid sudo was called from, or our own when sudo was not involved. */
static uid_t invoking_user(void) {
    const char *sudo_uid = getenv("SUDO_UID");
    char *end;
    if (geteuid() != 0 || sudo_uid == NULL) {
        return getuid();
    }
    unsigned long parsed = strtoul(sudo_uid, &end, 10);
    return *end == '\0' && sudo_uid[0] != '\0' ? (uid_t)parsed : getuid();
}

static bool make_dirs(char *path) {
    for (char *slash = strchr(path + 1, '/'); slash != NULL; slash = strchr(slash + 1, '/')) {
        *slash = '\0';
        mkdir(path, 0755);
        *slash = '/';
    }
    return mkdir(path, 0700) == 0 || errno == EEXIST;
}

static bool prepare_dir(enum sys_dir which, const char *home, struct error *err) {
    static const char *const xdg[] = {"XDG_CONFIG_HOME", "XDG_STATE_HOME", "XDG_DATA_HOME"};
    static const char *const fallback[] = {".config", ".local/state", ".local/share"};
    const char *base = getenv(xdg[which]);
    struct sbuf path = sbuf_on(self.dirs[which], DIR_MAX);
    if (base != NULL && base[0] == '/') {
        sbuf_putf(&path, "%s/lockin", base);
    } else {
        sbuf_putf(&path, "%s/%s/lockin", home, fallback[which]);
    }
    if (path.truncated || !make_dirs(self.dirs[which])) {
        return err_set(err, ERR_IO,
                       "Cannot create the folder %s: %s. Create it yourself and "
                       "make sure you own it.",
                       self.dirs[which], strerror(errno));
    }
    return true;
}

bool sys_start(struct error *err) {
    self.root = geteuid() == 0;
    self.user = invoking_user();
    struct passwd *who = getpwuid(self.user);
    if (who == NULL) {
        return err_set(err, ERR_PRIVILEGE,
                       "No account found for user id %u. Run lockin from "
                       "your own login with: sudo lockin",
                       (unsigned)self.user);
    }
    if (self.root && self.user != 0 &&
        (initgroups(who->pw_name, who->pw_gid) != 0 || setegid(who->pw_gid) != 0 ||
         seteuid(self.user) != 0)) {
        return err_set(err, ERR_PRIVILEGE, "Cannot switch to user %s for file work: %s.",
                       who->pw_name, strerror(errno));
    }
    for (int which = 0; which < SYS_DIR_COUNT; which++) {
        if (!prepare_dir((enum sys_dir)which, who->pw_dir, err)) {
            return false;
        }
    }
    return true;
}

bool sys_can_block(void) {
    return self.root;
}

const char *sys_dir(enum sys_dir which) {
    return self.dirs[which];
}

void sys_now(struct moment *out) {
    struct timespec now;
    struct tm local;
    clock_gettime(CLOCK_REALTIME, &now);
    localtime_r(&now.tv_sec, &local);
    out->year = local.tm_year + 1900;
    out->month = local.tm_mon + 1;
    out->day = local.tm_mday;
    out->hour = local.tm_hour;
    out->minute = local.tm_min;
    out->second = local.tm_sec;
    out->epoch = now.tv_sec;
}

uint64_t sys_random(void) {
    uint64_t value = 0;
    if (getrandom(&value, sizeof value, 0) != (ssize_t)sizeof value) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        value = (uint64_t)now.tv_nsec ^ ((uint64_t)getpid() << 32);
    }
    return value;
}

/* Reads up to cap-1 bytes from the end of `fd`. */
static bool read_tail(int fd, struct sbuf *out) {
    struct stat info;
    if (fstat(fd, &info) != 0) {
        return false;
    }
    off_t room = (off_t)out->cap - 1;
    if (S_ISREG(info.st_mode) && info.st_size > room) {
        out->truncated = true;
        lseek(fd, info.st_size - room, SEEK_SET);
    }
    while (out->len < out->cap - 1) {
        ssize_t got = read(fd, out->data + out->len, out->cap - 1 - out->len);
        if (got < 0 && errno == EINTR) {
            continue;
        }
        if (got <= 0) {
            out->data[out->len] = '\0';
            return got == 0;
        }
        out->len += (size_t)got;
    }
    out->data[out->len] = '\0';
    return true;
}

bool sys_read(const char *path, struct sbuf *out, struct error *err) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0 && errno == ENOENT) {
        return true;
    }
    bool ok = fd >= 0 && read_tail(fd, out);
    if (!ok) {
        err_set(err, ERR_IO, "Cannot read %s: %s. Check that you own the file.", path,
                strerror(errno));
    }
    if (fd >= 0) {
        close(fd);
    }
    if (ok && out->truncated) {
        /* The cut landed mid-line; start at the next whole line. */
        char *line = strchr(out->data, '\n');
        size_t skip = line != NULL ? (size_t)(line + 1 - out->data) : out->len;
        memmove(out->data, out->data + skip, out->len - skip + 1);
        out->len -= skip;
    }
    return ok;
}

static bool write_file(const char *path, int flags, const char *text) {
    int fd = open(path, flags | O_WRONLY | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (fd < 0) {
        return false;
    }
    bool ok = linux_write_all(fd, text) && fsync(fd) == 0;
    return close(fd) == 0 && ok;
}

bool sys_write(const char *path, const char *text, struct error *err) {
    char staging[DIR_MAX + 64];
    struct sbuf name = sbuf_on(staging, sizeof staging);
    sbuf_putf(&name, "%s.new", path);
    if (name.truncated || !write_file(staging, O_TRUNC, text) || rename(staging, path) != 0) {
        return err_set(err, ERR_IO,
                       "Cannot save %s: %s. Check the folder exists and that you "
                       "own it.",
                       path, strerror(errno));
    }
    return true;
}

bool sys_append(const char *path, const char *text, struct error *err) {
    if (!write_file(path, O_APPEND, text)) {
        return err_set(err, ERR_IO,
                       "Cannot add to %s: %s. Check that you own the file and the "
                       "disk is not full.",
                       path, strerror(errno));
    }
    return true;
}
