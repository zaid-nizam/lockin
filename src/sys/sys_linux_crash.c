#include "sys/sys.h"

#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

/* A signal handler gets no context argument, so the log's descriptor has to
 * live at file scope. It is written once, before the handlers are installed. */
static int crash_log = -1;

static void say(int fd, const char *text) {
    ssize_t ignored = write(fd, text, strlen(text));
    (void)ignored;
}

static void report(int fd, int signal_number) {
    void *frames[48];
    const char digits[4] = {(char)('0' + signal_number / 10 % 10), (char)('0' + signal_number % 10),
                            '\n', '\0'};
    say(fd, "\nlockin crashed on signal ");
    say(fd, digits);
    say(fd, "Stack, newest call first. Turn an address into a line with:\n"
            "  addr2line -f -e lockin <address in brackets>\n");
    backtrace_symbols_fd(frames, backtrace(frames, 48), fd);
}

/* Only async-signal-safe calls from here down. The handler was installed
 * with SA_RESETHAND, so raising the signal again ends the process the normal
 * way, with a core dump where those are enabled. */
static void on_crash(int signal_number) {
    report(STDERR_FILENO, signal_number);
    if (crash_log >= 0) {
        report(crash_log, signal_number);
    }
    raise(signal_number);
}

void sys_trace_crashes(const char *path) {
    static const int fatal[] = {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT};
    struct sigaction action = {.sa_handler = on_crash,
                               .sa_flags = (int)(SA_RESETHAND | SA_NODEFER)};
    void *warm_up[1];
    /* The first backtrace call loads libgcc, which allocates; do it now so
     * the handler never allocates. */
    backtrace(warm_up, 1);
    crash_log = open(path, O_WRONLY | O_APPEND | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0644);
    for (size_t i = 0; i < sizeof fatal / sizeof fatal[0]; i++) {
        sigaction(fatal[i], &action, NULL);
    }
}

void sys_refuse_to_stop(void) {
    static const int polite[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT};
    for (size_t i = 0; i < sizeof polite / sizeof polite[0]; i++) {
        signal(polite[i], SIG_IGN);
    }
}
