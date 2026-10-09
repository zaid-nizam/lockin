#include "app/app.h"

#include "core/blocklist.h"
#include "sys/sys.h"

#include <stdio.h>
#include <string.h>

enum { HOSTS_BYTES = 64 * 1024 };

void app_fail(struct app *app, const struct error *err) {
    text_copy(app->notice, sizeof app->notice, err->text);
    store_log_error(err);
}

static void remember(struct app *app, const struct record *row) {
    if (app->row_count == APP_ROWS) {
        memmove(app->rows, app->rows + 1, (APP_ROWS - 1) * sizeof app->rows[0]);
        app->row_count--;
    }
    app->rows[app->row_count++] = *row;
    app->last = *row;
}

/* Puts the block list over the hosts file. On a second lock in one boot the
 * file read here already holds a block, which the renderer replaces. */
static bool block_sites(struct app *app) {
    char original[HOSTS_BYTES], blocked[HOSTS_BYTES * 2], line[80];
    struct sbuf before = sbuf_on(original, sizeof original);
    struct sbuf after = sbuf_on(blocked, sizeof blocked);
    struct error err;
    if (!sys_hosts(&before, &err)) {
        app_fail(app, &err);
        return false;
    }
    int hosts = blocklist_render(&app->config, original, &after);
    if (before.truncated || after.truncated) {
        err_set(&err, ERR_BLOCK,
                "The hosts file is larger than %d KB, more than lockin "
                "handles. Trim /etc/hosts and try again.",
                HOSTS_BYTES / 1024);
        app_fail(app, &err);
        return false;
    }
    if (!sys_block(blocked, &err)) {
        app_fail(app, &err);
        return false;
    }
    snprintf(line, sizeof line, "sites blocked until reboot: %d hosts", hosts);
    store_log("INFO", line);
    return true;
}

void action_lock(struct app *app) {
    struct error err;
    if (app->screen != SCREEN_START || !block_sites(app)) {
        return;
    }
    if (!store_state_save(&app->state, &err)) {
        app_fail(app, &err);
    }
    app->blocked = true;
    sys_refuse_to_stop();
    app_tick(app, false);
    session_begin(&app->session, app->slot, &app->now);
    app->session.week = app->week;
    app->session.coin = app->coin;
    app->session.next_present = app->next_step[0] != '\0';
    form_reset(&app->form, app);
    app->notice[0] = '\0';
    app_show(app, SCREEN_SESSION);
}

void action_missed(struct app *app) {
    struct record row;
    struct error err;
    if (app->screen != SCREEN_START) {
        return;
    }
    session_missed(app->slot, app->week, &row);
    if (!store_row_append(&row, &err)) {
        app_fail(app, &err);
        return;
    }
    remember(app, &row);
    store_log("INFO", "slot logged as missed");
    app_show(app, SCREEN_MISSED);
}

void action_save(struct app *app) {
    struct record row;
    struct error err;
    if (app->screen != SCREEN_END) {
        return;
    }
    session_record(&app->session, &app->now, &row);
    if (!form_check(&app->form, &row)) {
        return;
    }
    /* NEXT.md first: if the log write then fails the user can press Save
     * again, and writing the same next step twice does no harm. */
    if (!store_next_save(&app->config, app->form.next, &err) || !store_row_append(&row, &err)) {
        app_fail(app, &err);
        return;
    }
    remember(app, &row);
    text_copy(app->next_step, sizeof app->next_step, app->form.next);
    text_copy(app->state.bedtime, sizeof app->state.bedtime, app->form.bedtime);
    text_copy(app->state.waketime, sizeof app->state.waketime, app->form.waketime);
    if (!store_state_save(&app->state, &err)) {
        app_fail(app, &err);
    }
    store_log("INFO", "session logged");
    app_show(app, SCREEN_DONE);
}

void action_close(struct app *app) {
    if (!app->blocked) {
        app->quit = true;
        return;
    }
    text_copy(app->notice, sizeof app->notice,
              "Locked. This window stays open until you shut down.");
}
