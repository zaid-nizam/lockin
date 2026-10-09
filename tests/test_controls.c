#include "suites.h"
#include "test.h"
#include "ui/ui.h"

#include <string.h>

enum { W = 300, H = 160 };

static uint32_t pixels[W * H];
static struct ui ui;

static void fresh(void) {
    memset(&ui, 0, sizeof ui);
    memset(pixels, 0, sizeof pixels);
    ui.canvas = (struct canvas){pixels, W, H};
    ui_reset_focus(&ui);
}

static uint32_t at(int x, int y) {
    return pixels[y * W + x];
}

static bool has_color(struct rect r, uint32_t color) {
    for (int y = r.y; y < r.y + r.h; y++) {
        for (int x = r.x; x < r.x + r.w; x++) {
            if (at(x, y) == color) {
                return true;
            }
        }
    }
    return false;
}

static void mouse(enum pf_kind kind, int x, int y) {
    ui_input(&ui, &(struct pf_event){kind, x, y, 0});
}

static void key(int code) {
    ui_input(&ui, &(struct pf_event){PF_KEY, 0, 0, code});
}

static void check_box_toggles_by_click_and_by_space(void) {
    bool on = false;
    fresh();
    for (int step = 0; step < 3; step++) {
        static const enum pf_kind kinds[] = {PF_MOVE, PF_PRESS, PF_RELEASE};
        mouse(kinds[step], 60, 26);
        ui_begin(&ui);
        ui_check(&ui, (struct point){10, 20}, "Co-work room", &on);
        ui_end(&ui);
    }
    CHECK(on);
    CHECK(has_color((struct rect){10, 21, 13, 13}, COLOR_TEXT));
    key(' ');
    ui_begin(&ui);
    ui_check(&ui, (struct point){10, 20}, "Co-work room", &on);
    ui_end(&ui);
    CHECK(!on);
}

static void radio_buttons_select_one_value(void) {
    int value = 0;
    fresh();
    for (int step = 0; step < 3; step++) {
        static const enum pf_kind kinds[] = {PF_PRESS, PF_RELEASE, PF_MOVE};
        mouse(kinds[step], 105, 26);
        ui_begin(&ui);
        ui_radio(&ui, (struct point){10, 20}, "One", (struct choice){&value, 1});
        ui_radio(&ui, (struct point){100, 20}, "Two", (struct choice){&value, 2});
        ui_end(&ui);
    }
    CHECK_INT(value, 2);
    key(KEY_BACKTAB);
    ui_begin(&ui);
    ui_radio(&ui, (struct point){10, 20}, "One", (struct choice){&value, 1});
    ui_radio(&ui, (struct point){100, 20}, "Two", (struct choice){&value, 2});
    ui_end(&ui);
    key(' ');
    ui_begin(&ui);
    ui_radio(&ui, (struct point){10, 20}, "One", (struct choice){&value, 1});
    ui_radio(&ui, (struct point){100, 20}, "Two", (struct choice){&value, 2});
    ui_end(&ui);
    CHECK_INT(value, 1);
}

static void progress_bar_fills_in_proportion(void) {
    struct rect bar = {10, 10, 204, 16};
    struct rect left = {12, 12, 20, 12}, middle = {120, 12, 20, 12}, right = {192, 12, 20, 12};
    fresh();
    ui_begin(&ui);
    ui_progress(&ui, bar, 0);
    CHECK(!has_color(left, COLOR_NAVY));
    ui_progress(&ui, bar, 500);
    CHECK(has_color(left, COLOR_NAVY) && !has_color(middle, COLOR_NAVY));
    ui_progress(&ui, bar, 1000);
    CHECK(has_color(middle, COLOR_NAVY) && has_color(right, COLOR_NAVY));
    ui_end(&ui);
}

static void static_parts_draw_in_their_places(void) {
    fresh();
    ui_begin(&ui);
    CHECK_INT(at(5, 5), COLOR_FACE);
    ui_header(&ui, "Title", "What this screen is for");
    CHECK_INT(at(2, 2), COLOR_WINDOW);
    CHECK_INT(at(2, UI_HEADER_HEIGHT - 2), COLOR_SHADOW);
    CHECK(has_color((struct rect){W - 40, 10, 40, 30}, COLOR_GOLD));
    ui_status(&ui, "Left", "Right");
    CHECK(has_color((struct rect){0, H - UI_STATUS_HEIGHT, W, UI_STATUS_HEIGHT}, COLOR_TEXT));
    ui_group(&ui, (struct rect){10, 70, 100, 40}, "Group");
    CHECK_INT(at(10, 90), COLOR_SHADOW);
    CHECK_INT(at(11, 90), COLOR_HIGHLIGHT);
    ui_readout(&ui, (struct rect){120, 70, 100, 21}, "Read only");
    CHECK_INT(at(121, 71), COLOR_DARK);
    ui_column(&ui, (struct rect){120, 100, 80, 20}, "Column");
    CHECK_INT(at(120, 100), COLOR_HIGHLIGHT);
    ui_label(&ui, (struct point){230, 70}, "a");
    ui_label_bold(&ui, (struct point){230, 90}, "a");
    ui_label_big(&ui, (struct point){230, 110}, "1");
    CHECK(has_color((struct rect){230, 110, 30, 45}, COLOR_TEXT));
    CHECK_INT(ui_text(&ui, (struct rect){10, 120, 50, 40}, "two words"), 30);
    ui_end(&ui);
}

void suite_controls(void) {
    RUN(check_box_toggles_by_click_and_by_space);
    RUN(radio_buttons_select_one_value);
    RUN(progress_bar_fills_in_proportion);
    RUN(static_parts_draw_in_their_places);
}
