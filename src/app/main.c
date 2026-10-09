#include "app/app.h"

#include "core/blocklist.h"
#include "sys/sys.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>

enum { EXIT_OK = 0, EXIT_FAILED = 1, EXIT_USAGE = 2, TICK_MS = 500 };

static const char usage[] =
    "lockin " LOCKIN_VERSION ": block distractions until shutdown and run a focus session.\n"
    "\n"
    "  sudo lockin      open the window (root is needed to block sites)\n"
    "  lockin --check   check lockin.conf and say what would be blocked\n"
    "  lockin --version print the version\n"
    "  lockin --help    print this text\n";

#ifdef LOCKIN_COVERAGE
/* Coverage builds only: a locked window cannot be closed, and gcov writes
 * its counts at exit, so the end-to-end tests need a way out. */
static volatile sig_atomic_t stop_requested;

static void request_stop(int signal_number) {
    (void)signal_number;
    stop_requested = 1;
}
#endif

static bool stopping(const struct app *app) {
#ifdef LOCKIN_COVERAGE
    if (stop_requested) {
        return true;
    }
#endif
    return app->quit;
}

/* Loads the settings, printing every problem. */
static bool load_config(struct config *config) {
    char text[2048];
    struct sbuf problems = sbuf_on(text, sizeof text);
    struct error err;
    if (store_config(config, &problems, &err)) {
        return true;
    }
    store_log_error(&err);
    fputs(text, stderr);
    return false;
}

static int check(const struct config *config) {
    char hosts[64 * 1024];
    struct sbuf out = sbuf_on(hosts, sizeof hosts);
    int count = blocklist_render(config, "", &out);
    printf("Settings are valid. %d hosts would be blocked:\n%s", count, hosts);
    return EXIT_OK;
}

static int run(struct app *app) {
    struct pf_window window = {"Lock in", APP_WIDTH, APP_HEIGHT, app->config.scale};
    struct error err;
    if (!pf_open(&window, &err)) {
        store_log_error(&err);
        return EXIT_FAILED;
    }
    app_open(app);
    store_log("INFO", "started lockin " LOCKIN_VERSION);
    while (!stopping(app)) {
        struct pf_event event;
        bool got = pf_wait(TICK_MS, &event);
        if (got) {
            app_event(app, &event);
        }
        app_tick(app, !got);
        app_frame(app);
        pf_show(app->pixels);
    }
    pf_close();
    return EXIT_OK;
}

enum mode { MODE_RUN, MODE_CHECK, MODE_VERSION, MODE_HELP, MODE_USAGE };

static enum mode parse_arguments(int argc, char **argv) {
    static const char *const options[] = {
        [MODE_CHECK] = "--check", [MODE_VERSION] = "--version", [MODE_HELP] = "--help"};
    if (argc == 1) {
        return MODE_RUN;
    }
    for (int mode = MODE_CHECK; argc == 2 && mode <= MODE_HELP; mode++) {
        if (strcmp(argv[1], options[mode]) == 0) {
            return (enum mode)mode;
        }
    }
    return MODE_USAGE;
}

/* Handles the modes that need no settings. Returns false for the others. */
static bool answer_directly(enum mode mode, int *status) {
    *status = mode == MODE_USAGE ? EXIT_USAGE : EXIT_OK;
    if (mode == MODE_VERSION) {
        puts("lockin " LOCKIN_VERSION);
    } else if (mode == MODE_HELP || mode == MODE_USAGE) {
        fputs(usage, mode == MODE_HELP ? stdout : stderr);
    }
    return mode != MODE_RUN && mode != MODE_CHECK;
}

int main(int argc, char **argv) {
    static struct app app;
    char log_path[400];
    struct sbuf path = sbuf_on(log_path, sizeof log_path);
    struct error err;
    enum mode mode = parse_arguments(argc, argv);
    int status;
    if (answer_directly(mode, &status)) {
        return status;
    }
    if (!sys_start(&err)) {
        fprintf(stderr, "lockin: %s\n", err.text);
        return EXIT_FAILED;
    }
    store_log_path(&path);
    sys_trace_crashes(log_path);
#ifdef LOCKIN_COVERAGE
    signal(SIGUSR1, request_stop);
#endif
    if (!load_config(&app.config)) {
        return EXIT_USAGE;
    }
    return mode == MODE_CHECK ? check(&app.config) : run(&app);
}
