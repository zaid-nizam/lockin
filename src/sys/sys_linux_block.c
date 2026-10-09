#include "sys/sys.h"
#include "sys/sys_linux.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

/* /run is a tmpfs that root owns and the kernel empties at boot, which is
 * what makes the block end at the next reboot with no clean-up code. */
#define RUN_DIR      "/run/lockin"
#define BLOCKED_FILE RUN_DIR "/hosts"
#define HOSTS_FILE   "/etc/hosts"

bool sys_hosts(struct sbuf *out, struct error *err) {
    return sys_read(HOSTS_FILE, out, err);
}

bool sys_blocked(void) {
    struct stat system_hosts, ours;
    return stat(HOSTS_FILE, &system_hosts) == 0 && stat(BLOCKED_FILE, &ours) == 0 &&
           system_hosts.st_dev == ours.st_dev && system_hosts.st_ino == ours.st_ino;
}

/* Writes the file in place: the mount shows this inode, so a second lock in
 * the same boot must update it rather than replace it. */
static bool write_blocked_file(const char *hosts) {
    if (mkdir(RUN_DIR, 0755) != 0 && errno != EEXIST) {
        return false;
    }
    int fd = open(BLOCKED_FILE, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (fd < 0) {
        return false;
    }
    bool ok = linux_write_all(fd, hosts);
    return close(fd) == 0 && ok;
}

bool sys_block(const char *hosts, struct error *err) {
    if (!linux_act_as_root(true)) {
        return err_set(err, ERR_PRIVILEGE,
                       "Blocking sites needs root. Close this window and "
                       "start it with: sudo lockin");
    }
    bool ok = write_blocked_file(hosts) &&
              (sys_blocked() || mount(BLOCKED_FILE, HOSTS_FILE, NULL, MS_BIND, NULL) == 0);
    int failure = errno;
    linux_act_as_root(false);
    if (!ok) {
        return err_set(err, ERR_BLOCK,
                       "Cannot put the block list over " HOSTS_FILE ": %s. "
                       "Nothing was changed. Check that " HOSTS_FILE " exists.",
                       strerror(failure));
    }
    return true;
}

/* Reads the short process name the kernel keeps for a pid. */
static bool process_name(const char *pid, char *name, size_t cap) {
    char path[64];
    struct sbuf file = sbuf_on(path, sizeof path);
    struct sbuf text = sbuf_on(name, cap);
    struct error ignored;
    struct stat owner;
    sbuf_putf(&file, "/proc/%s", pid);
    if (stat(path, &owner) != 0 || owner.st_uid != linux_user()) {
        return false;
    }
    sbuf_put(&file, "/comm");
    if (!sys_read(path, &text, &ignored)) {
        return false;
    }
    text_trim(name);
    return true;
}

static bool listed(const char *name, const struct config *config) {
    for (int i = 0; i < config->kill_count; i++) {
        if (text_same_nocase(name, config->kill[i])) {
            return true;
        }
    }
    return false;
}

int sys_close_programs(const struct config *config) {
    DIR *proc = opendir("/proc");
    int closed = 0;
    if (proc == NULL) {
        return 0;
    }
    for (const struct dirent *entry = readdir(proc); entry != NULL; entry = readdir(proc)) {
        char name[CONFIG_NAME_MAX];
        pid_t pid = (pid_t)strtol(entry->d_name, NULL, 10);
        if (pid <= 1 || pid == getpid() || !process_name(entry->d_name, name, sizeof name)) {
            continue;
        }
        if (listed(name, config) && kill(pid, SIGKILL) == 0) {
            closed++;
        }
    }
    closedir(proc);
    return closed;
}
