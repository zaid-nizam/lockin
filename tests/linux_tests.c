/* Tests of the real Linux system layer. Each scenario needs its own process
 * (privileges are dropped for good, mounts are changed), so the scenario is
 * picked by name and tests/system.sh sets the stage for each. */

#include "core/blocklist.h"
#include "sys/sys.h"
#include "test.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static const char *argument = "";

static void path_in_data(const char *name, char *out, size_t cap) {
    struct sbuf sb = sbuf_on(out, cap);
    sbuf_putf(&sb, "%s/%s", sys_dir(SYS_DIR_DATA), name);
}

static void clock_and_random_work(void) {
    struct moment now;
    struct error err;
    CHECK(sys_start(&err));
    sys_now(&now);
    CHECK(now.year >= 2026 && now.month >= 1 && now.month <= 12);
    CHECK(now.hour >= 0 && now.hour <= 23 && now.second <= 60);
    CHECK(now.epoch > 1700000000);
    CHECK(sys_random() != sys_random());
}

static void files_are_written_whole_and_read_back(void) {
    char path[400], text[64];
    struct sbuf out = sbuf_on(text, sizeof text);
    struct error err;
    struct stat info;
    CHECK(sys_start(&err));
    CHECK(stat(sys_dir(SYS_DIR_CONFIG), &info) == 0 && S_ISDIR(info.st_mode));
    CHECK(stat(sys_dir(SYS_DIR_STATE), &info) == 0);
    path_in_data("note", path, sizeof path);
    CHECK(sys_read(path, &out, &err));
    CHECK_INT(out.len, 0);
    CHECK(sys_write(path, "first\n", &err));
    CHECK(sys_write(path, "second\n", &err));
    CHECK(sys_append(path, "third\n", &err));
    CHECK(sys_read(path, &out, &err));
    CHECK_STR(text, "second\nthird\n");
    CHECK(stat(path, &info) == 0 && info.st_uid == geteuid());
}

static void a_big_file_reads_as_its_last_whole_lines(void) {
    char path[400], text[32];
    struct sbuf out = sbuf_on(text, sizeof text);
    struct error err;
    CHECK(sys_start(&err));
    path_in_data("big", path, sizeof path);
    CHECK(sys_write(path, "line one is long\nline two\nline three\nfour\n", &err));
    CHECK(sys_read(path, &out, &err));
    CHECK(out.truncated);
    CHECK_STR(text, "line two\nline three\nfour\n");
    CHECK(sys_write(path, "0123456789012345678901234567890123456789", &err));
    out = sbuf_on(text, sizeof text);
    CHECK(sys_read(path, &out, &err));
    CHECK_STR(text, "");
}

static void file_failures_say_what_and_why(void) {
    char text[32], dir[400];
    struct sbuf out = sbuf_on(text, sizeof text);
    struct error err;
    CHECK(sys_start(&err));
    CHECK(!sys_write("/nonexistent-dir/file", "x", &err));
    CHECK_INT(err.code, ERR_IO);
    CHECK_HAS(err.text, "Cannot save /nonexistent-dir/file: No such file or directory");
    CHECK(!sys_append("/dev/full", "x", &err));
    CHECK_HAS(err.text, "Cannot add to /dev/full: No space left on device");
    CHECK(!sys_append("/nonexistent-dir/file", "x", &err));
    text_copy(dir, sizeof dir, sys_dir(SYS_DIR_DATA));
    CHECK(!sys_read(dir, &out, &err));
    CHECK_HAS(err.text, "Cannot read");
    path_in_data("link", dir, sizeof dir);
    CHECK(symlink("/etc/passwd", dir) == 0);
    CHECK(!sys_append(dir, "x", &err));
    CHECK(!sys_write(
        "/dev/null/x/this-path-is-far-too-long-to-stage-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "x", &err));
}

static bool resolves_to_nowhere(const char *host) {
    struct addrinfo hints = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
    struct addrinfo *found = NULL;
    if (getaddrinfo(host, NULL, &hints, &found) != 0) {
        return false;
    }
    struct sockaddr_in address;
    memcpy(&address, found->ai_addr, sizeof address);
    freeaddrinfo(found);
    return address.sin_addr.s_addr == htonl(INADDR_ANY);
}

static void blocking_swaps_the_hosts_file_and_can_be_repeated(void) {
    static char original[65536], blocked[131072], seen[131072];
    struct sbuf before = sbuf_on(original, sizeof original);
    struct sbuf after = sbuf_on(blocked, sizeof blocked);
    struct sbuf now = sbuf_on(seen, sizeof seen);
    struct config config;
    struct error err;
    config_defaults(&config);
    CHECK(sys_start(&err));
    CHECK(sys_can_block());
    CHECK(!sys_blocked());
    CHECK(sys_hosts(&before, &err));
    blocklist_render(&config, original, &after);
    CHECK(sys_block(blocked, &err));
    CHECK(sys_blocked());
    CHECK(sys_hosts(&now, &err));
    CHECK_STR(seen, blocked);
    CHECK(resolves_to_nowhere("music.youtube.com"));
    CHECK(resolves_to_nowhere("discord.com"));
    CHECK(resolves_to_nowhere("api.openai.com"));
    CHECK(sys_block("127.0.0.1 localhost\n0.0.0.0 second.example.com\n", &err));
    now = sbuf_on(seen, sizeof seen);
    CHECK(sys_hosts(&now, &err));
    CHECK_STR(seen, "127.0.0.1 localhost\n0.0.0.0 second.example.com\n");
    CHECK(resolves_to_nowhere("second.example.com"));
}

static void blocking_without_a_hosts_file_fails_cleanly(void) {
    struct error err;
    CHECK(sys_start(&err));
    CHECK(!sys_block("0.0.0.0 example.com\n", &err));
    CHECK_INT(err.code, ERR_BLOCK);
    CHECK_HAS(err.text, "Cannot put the block list over /etc/hosts");
    CHECK(!sys_blocked());
    CHECK_INT(geteuid(), 0);
}

static void an_ordinary_user_cannot_block(void) {
    struct error err;
    CHECK(sys_start(&err));
    CHECK(geteuid() != 0);
    CHECK(!sys_can_block());
    CHECK(!sys_block("0.0.0.0 example.com\n", &err));
    CHECK_INT(err.code, ERR_PRIVILEGE);
    CHECK_HAS(err.text, "sudo lockin");
}

/* Starts `argument` (a copy of sleep named lk-victim) as `uid`. */
static pid_t start_victim(uid_t uid) {
    pid_t pid = fork();
    if (pid == 0) {
        if (setgid(uid) != 0 || setuid(uid) != 0) {
            _exit(3);
        }
        execl(argument, argument, "60", (char *)NULL);
        _exit(4);
    }
    return pid;
}

static void under_sudo_it_works_as_the_user_and_blocks_as_root(void) {
    const char *sudo_uid = getenv("SUDO_UID");
    uid_t user = (uid_t)strtoul(sudo_uid != NULL ? sudo_uid : "0", NULL, 10);
    pid_t theirs = start_victim(user), roots = start_victim(0);
    char path[400];
    struct stat info;
    struct config config;
    struct error err;
    int status = 0;
    config_defaults(&config);
    text_copy(config.kill[0], CONFIG_NAME_MAX, "LK-Victim");
    usleep(300 * 1000);
    CHECK(sys_start(&err));
    CHECK_INT(geteuid(), user);
    CHECK_INT(getuid(), 0);
    path_in_data("owned", path, sizeof path);
    CHECK(sys_write(path, "x\n", &err));
    CHECK(stat(path, &info) == 0 && info.st_uid == user);
    CHECK(sys_block("127.0.0.1 localhost\n0.0.0.0 example.com\n", &err));
    CHECK(sys_blocked());
    CHECK_INT(geteuid(), user);
    CHECK_INT(sys_close_programs(&config), 1);
    CHECK(waitpid(theirs, &status, 0) == theirs && WIFSIGNALED(status));
    CHECK_INT(WTERMSIG(status), SIGKILL);
    CHECK(waitpid(roots, &status, WNOHANG) == 0);
    kill(roots, SIGKILL);
    waitpid(roots, &status, 0);
    CHECK_INT(sys_close_programs(&config), 0);
}

static void sudo_for_an_unknown_user_is_refused(void) {
    struct error err;
    CHECK(!sys_start(&err));
    CHECK_INT(err.code, ERR_PRIVILEGE);
    CHECK_HAS(err.text, "No account found for user id");
}

static void a_malformed_sudo_uid_is_ignored(void) {
    struct error err;
    CHECK(sys_start(&err));
    CHECK_INT(geteuid(), 0);
    CHECK(sys_can_block());
}

static void a_folder_that_cannot_be_made_stops_the_start(void) {
    struct error err;
    CHECK(!sys_start(&err));
    CHECK_INT(err.code, ERR_IO);
    CHECK_HAS(err.text, "Cannot create the folder");
}

static void a_crash_leaves_a_stack_trace_in_the_log(void) {
    char text[4096];
    struct sbuf out = sbuf_on(text, sizeof text);
    struct error err;
    int status = 0;
    CHECK(sys_start(&err));
    pid_t child = fork();
    if (child == 0) {
        int quiet = open("/dev/null", O_WRONLY);
        dup2(quiet, STDERR_FILENO);
        sys_trace_crashes(argument);
        raise(SIGSEGV);
    }
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFSIGNALED(status));
    CHECK_INT(WTERMSIG(status), SIGSEGV);
    CHECK(sys_read(argument, &out, &err));
    CHECK_HAS(text, "lockin crashed on signal 11");
    CHECK_HAS(text, "addr2line -f -e lockin");
    CHECK_HAS(text, "(main+");
}

static void polite_stop_signals_are_ignored_once_asked(void) {
    int status = 0;
    pid_t child = fork();
    if (child == 0) {
        sys_refuse_to_stop();
        kill(getpid(), SIGTERM);
        kill(getpid(), SIGINT);
        kill(getpid(), SIGHUP);
        kill(getpid(), SIGQUIT);
        _exit(7);
    }
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status));
    CHECK_INT(WEXITSTATUS(status), 7);
}

int main(int argc, char **argv) {
    static const struct {
        const char *name;
        void (*test)(void);
    } scenarios[] = {
        {"clock", clock_and_random_work},
        {"files", files_are_written_whole_and_read_back},
        {"tail", a_big_file_reads_as_its_last_whole_lines},
        {"file-errors", file_failures_say_what_and_why},
        {"block", blocking_swaps_the_hosts_file_and_can_be_repeated},
        {"block-fails", blocking_without_a_hosts_file_fails_cleanly},
        {"unprivileged", an_ordinary_user_cannot_block},
        {"sudo", under_sudo_it_works_as_the_user_and_blocks_as_root},
        {"no-user", sudo_for_an_unknown_user_is_refused},
        {"bad-uid", a_malformed_sudo_uid_is_ignored},
        {"bad-dir", a_folder_that_cannot_be_made_stops_the_start},
        {"crash", a_crash_leaves_a_stack_trace_in_the_log},
        {"refuse", polite_stop_signals_are_ignored_once_asked},
    };
    argument = argc > 2 ? argv[2] : "";
    for (size_t i = 0; argc > 1 && i < sizeof scenarios / sizeof scenarios[0]; i++) {
        if (strcmp(argv[1], scenarios[i].name) == 0) {
            test_run(scenarios[i].name, scenarios[i].test);
            return test_summary();
        }
    }
    fprintf(stderr, "usage: linux_tests SCENARIO [ARGUMENT]\n");
    return 2;
}
