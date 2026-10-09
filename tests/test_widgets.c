#include "suites.h"
#include "test.h"
#include "ui/ui.h"

#include <string.h>

enum { W = 300, H = 160 };

static uint32_t pixels[W * H];
static struct ui ui;

static const struct rect first = {10, 60, 80, 24};
static const struct rect second = {100, 60, 80, 24};
static const struct rect box = {10, 100, 120, 21};

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

/* One frame with two buttons and a text box; reports what fired. */
static char text[8];
static bool fired_first, fired_second;

static void frame(enum button_flags first_flags, enum button_flags second_flags) {
    ui_begin(&ui);
    fired_first = ui_button(&ui, first, "First", first_flags);
    fired_second = ui_button(&ui, second, "Second", second_flags);
    ui_field(&ui, box, text, sizeof text);
    ui_end(&ui);
}

static void a_button_fires_when_the_click_ends_inside_it(void) {
    fresh();
    frame(0, 0);
    mouse(PF_PRESS, 20, 70);
    frame(0, 0);
    CHECK(!fired_first);
    mouse(PF_RELEASE, 20, 70);
    frame(0, 0);
    CHECK(fired_first && !fired_second);
}

static void dragging_off_a_button_cancels_the_click(void) {
    fresh();
    frame(0, 0);
    mouse(PF_PRESS, 20, 70);
    frame(0, 0);
    mouse(PF_MOVE, 200, 150);
    frame(0, 0);
    mouse(PF_RELEASE, 200, 150);
    frame(0, 0);
    CHECK(!fired_first && !fired_second);
    mouse(PF_PRESS, 250, 150);
    frame(0, 0);
    mouse(PF_RELEASE, 120, 70);
    frame(0, 0);
    CHECK(!fired_second);
}

static void a_pressed_button_looks_pressed(void) {
    fresh();
    frame(0, 0);
    CHECK_INT(at(second.x, second.y), COLOR_HIGHLIGHT);
    CHECK_INT(at(first.x, first.y), COLOR_TEXT);
    mouse(PF_PRESS, 120, 70);
    frame(0, 0);
    CHECK_INT(at(second.x, second.y), COLOR_TEXT);
    CHECK_INT(at(second.x + 1, second.y + 1), COLOR_SHADOW);
}

static void a_disabled_button_never_fires_and_is_greyed(void) {
    fresh();
    frame(0, BUTTON_DISABLED);
    mouse(PF_PRESS, 120, 70);
    frame(0, BUTTON_DISABLED);
    mouse(PF_RELEASE, 120, 70);
    frame(0, BUTTON_DISABLED);
    CHECK(!fired_second);
    key(' ');
    frame(0, BUTTON_DISABLED);
    CHECK(!fired_second);
    CHECK(has_color(second, COLOR_SHADOW));
}

static void tab_moves_focus_forward_and_back_and_wraps(void) {
    fresh();
    frame(0, 0);
    CHECK_INT(ui.focus, 0);
    key(KEY_TAB);
    frame(0, 0);
    CHECK_INT(ui.focus, 1);
    key(KEY_TAB);
    frame(0, 0);
    key(KEY_TAB);
    frame(0, 0);
    CHECK_INT(ui.focus, 0);
    key(KEY_BACKTAB);
    frame(0, 0);
    CHECK_INT(ui.focus, 2);
}

static void space_and_enter_press_the_focused_button(void) {
    fresh();
    frame(0, 0);
    key(' ');
    frame(0, 0);
    CHECK(fired_first && !fired_second);
    key(KEY_TAB);
    frame(0, 0);
    key(KEY_ENTER);
    frame(0, 0);
    CHECK(!fired_first && fired_second);
}

static void enter_in_a_text_box_presses_the_default_button_only(void) {
    fresh();
    frame(0, BUTTON_DEFAULT);
    mouse(PF_PRESS, 20, 110);
    frame(0, BUTTON_DEFAULT);
    mouse(PF_RELEASE, 20, 110);
    frame(0, BUTTON_DEFAULT);
    CHECK_INT(ui.focus, 2);
    key(KEY_ENTER);
    frame(0, BUTTON_DEFAULT);
    CHECK(!fired_first && fired_second);
    /* With focus on another button, Enter belongs to that button. */
    fresh();
    frame(0, BUTTON_DEFAULT);
    key(KEY_ENTER);
    frame(0, BUTTON_DEFAULT);
    CHECK(fired_first && !fired_second);
}

static void typing_edits_the_focused_box_within_its_capacity(void) {
    fresh();
    text[0] = '\0';
    frame(0, 0);
    key('a');
    frame(0, 0);
    CHECK_STR(text, "");
    key(KEY_TAB);
    key(KEY_TAB);
    frame(0, 0);
    CHECK_INT(ui.focus, 1);
    key(KEY_TAB);
    frame(0, 0);
    for (const char *c = "hello world"; *c != '\0'; c++) {
        key(*c);
        frame(0, 0);
    }
    CHECK_STR(text, "hello w");
    key(KEY_BACKSPACE);
    key(KEY_BACKSPACE);
    frame(0, 0);
    CHECK_STR(text, "hello");
    key(KEY_ESCAPE);
    key(200);
    frame(0, 0);
    CHECK_STR(text, "hello");
    for (int i = 0; i < 9; i++) {
        key(KEY_BACKSPACE);
        frame(0, 0);
    }
    CHECK_STR(text, "");
}

static void the_caret_shows_only_in_the_focused_box(void) {
    struct rect inside = rect_inset(box, 3);
    fresh();
    text[0] = '\0';
    ui.caret = true;
    frame(0, 0);
    CHECK(!has_color(inside, COLOR_TEXT));
    mouse(PF_PRESS, 20, 110);
    frame(0, 0);
    CHECK(has_color(inside, COLOR_TEXT));
    ui.caret = false;
    frame(0, 0);
    CHECK(!has_color(inside, COLOR_TEXT));
}

static void a_long_value_shows_its_end(void) {
    char wide[64] = "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm";
    fresh();
    ui_begin(&ui);
    ui_field(&ui, box, wide, sizeof wide);
    ui_end(&ui);
    CHECK(!has_color((struct rect){box.x + box.w, box.y, 40, box.h}, COLOR_TEXT));
    CHECK(has_color(rect_inset(box, 3), COLOR_TEXT));
}

static void more_keys_than_fit_in_a_frame_are_dropped(void) {
    fresh();
    for (int i = 0; i < UI_KEYS + 5; i++) {
        key('a');
    }
    CHECK_INT(ui.key_count, UI_KEYS);
    CHECK(ui_key(&ui, 'a'));
    CHECK(!ui_key(&ui, 'b'));
    ui_input(&ui, &(struct pf_event){PF_REDRAW, 0, 0, 0});
    ui_input(&ui, &(struct pf_event){PF_CLOSE, 0, 0, 0});
    CHECK_INT(ui.key_count, UI_KEYS);
}

void suite_widgets(void) {
    RUN(a_button_fires_when_the_click_ends_inside_it);
    RUN(dragging_off_a_button_cancels_the_click);
    RUN(a_pressed_button_looks_pressed);
    RUN(a_disabled_button_never_fires_and_is_greyed);
    RUN(tab_moves_focus_forward_and_back_and_wraps);
    RUN(space_and_enter_press_the_focused_button);
    RUN(enter_in_a_text_box_presses_the_default_button_only);
    RUN(typing_edits_the_focused_box_within_its_capacity);
    RUN(the_caret_shows_only_in_the_focused_box);
    RUN(a_long_value_shows_its_end);
    RUN(more_keys_than_fit_in_a_frame_are_dropped);
}
