#include "suites.h"
#include "test.h"
#include "ui/canvas.h"

#include <stddef.h>
#include <string.h>

enum { W = 120, H = 60, RED = 0xFF0000, BLUE = 0x0000FF };

static uint32_t pixels[W * H];
static struct canvas canvas = {pixels, W, H};
static const struct text_style plain = {&font_regular, RED, 1};
static const struct text_style triple = {&font_bold, RED, 3};

static void clear(void) {
    memset(pixels, 0, sizeof pixels);
}

static uint32_t at(int x, int y) {
    return pixels[y * W + x];
}

static int painted(void) {
    int count = 0;
    for (int i = 0; i < W * H; i++) {
        count += pixels[i] != 0;
    }
    return count;
}

static void rectangles_know_what_they_hold(void) {
    struct rect r = {10, 20, 30, 40};
    struct rect inner = rect_inset(r, 2);
    CHECK(rect_holds(r, 10, 20));
    CHECK(rect_holds(r, 39, 59));
    CHECK(!rect_holds(r, 40, 59));
    CHECK(!rect_holds(r, 39, 60));
    CHECK(!rect_holds(r, 9, 20));
    CHECK(!rect_holds(r, 10, 19));
    CHECK_INT(inner.x, 12);
    CHECK_INT(inner.w, 26);
    CHECK_INT(inner.h, 36);
}

static void fill_is_clipped_to_the_canvas(void) {
    clear();
    canvas_fill(&canvas, (struct rect){-5, -5, 10, 10}, RED);
    CHECK_INT(painted(), 25);
    CHECK_INT(at(4, 4), RED);
    CHECK_INT(at(5, 5), 0);
    canvas_fill(&canvas, (struct rect){W - 2, H - 2, 50, 50}, BLUE);
    CHECK_INT(painted(), 29);
    canvas_fill(&canvas, (struct rect){W + 5, 0, 5, 5}, BLUE);
    canvas_fill(&canvas, (struct rect){0, H + 5, 5, 5}, BLUE);
    CHECK_INT(painted(), 29);
}

static void edge_lights_top_left_and_shades_bottom_right(void) {
    clear();
    canvas_edge(&canvas, (struct rect){10, 10, 20, 10}, RED, BLUE);
    CHECK_INT(at(10, 10), RED);
    CHECK_INT(at(20, 10), RED);
    CHECK_INT(at(10, 15), RED);
    CHECK_INT(at(29, 15), BLUE);
    CHECK_INT(at(20, 19), BLUE);
    CHECK_INT(at(15, 15), 0);
}

static void dotted_border_skips_every_other_pixel(void) {
    clear();
    canvas_dotted(&canvas, (struct rect){10, 10, 21, 11}, RED);
    CHECK_INT(at(10, 10), RED);
    CHECK_INT(at(11, 10), 0);
    CHECK_INT(at(12, 10), RED);
    CHECK_INT(at(10, 12), RED);
    CHECK_INT(at(10, 11), 0);
    CHECK_INT(at(30, 20), RED);
}

static void text_reports_its_width_and_draws_inside_it(void) {
    clear();
    CHECK_INT(canvas_text(&canvas, (struct point){5, 5}, &plain, "A"), 9);
    CHECK(painted() > 10);
    CHECK_INT(canvas_text_width(&plain, "A"), 9);
    CHECK_INT(canvas_text_width(&plain, ""), 0);
    CHECK_INT(canvas_text_width(&triple, "A"),
              3 * canvas_text_width(&(struct text_style){&font_bold, 0, 1}, "A"));
    CHECK_INT(canvas_line_height(&plain), 15);
    CHECK_INT(canvas_line_height(&triple), 45);
    for (int y = 0; y < H; y++) {
        for (int x = 14; x < W; x++) {
            CHECK_INT(at(x, y), 0);
        }
    }
}

static void bytes_outside_ascii_draw_as_a_question_mark(void) {
    CHECK_INT(canvas_text_width(&plain, "\xE2\x80\x94"), canvas_text_width(&plain, "?"));
    CHECK_INT(canvas_text_width(&plain, "\x01"), canvas_text_width(&plain, "?"));
    CHECK_INT(canvas_text_width(&plain, "\x7F"), canvas_text_width(&plain, "?"));
}

static void zoomed_text_paints_blocks(void) {
    clear();
    canvas_text(&canvas, (struct point){0, 0}, &triple, "l");
    int big = painted();
    clear();
    canvas_text(&canvas, (struct point){0, 0}, &(struct text_style){&font_bold, RED, 1}, "l");
    CHECK_INT(big, 9 * painted());
}

static void wrapped_text_breaks_at_spaces_and_stops_at_the_box(void) {
    int line = canvas_line_height(&plain);
    clear();
    CHECK_INT(canvas_text_wrapped(&canvas, (struct rect){0, 0, 100, 60}, &plain, "one"), line);
    CHECK_INT(
        canvas_text_wrapped(&canvas, (struct rect){0, 0, 60, 60}, &plain, "aaaa bbbb cccc dddd"),
        2 * line);
    CHECK_INT(canvas_text_wrapped(&canvas, (struct rect){0, 0, 60, line + 2}, &plain,
                                  "aaaa bbbb cccc dddd eeee ffff"),
              line);
    CHECK_INT(canvas_text_wrapped(&canvas, (struct rect){0, 0, 100, 60}, &plain, ""), 0);
}

static void a_word_wider_than_the_box_is_broken_mid_word(void) {
    char long_word[400];
    int line = canvas_line_height(&plain);
    memset(long_word, 'i', sizeof long_word - 1);
    long_word[sizeof long_word - 1] = '\0';
    clear();
    CHECK_INT(canvas_text_wrapped(&canvas, (struct rect){0, 0, 30, 60}, &plain, "mmmmmmmmmmmm"),
              4 * line);
    CHECK_INT(canvas_text_wrapped(&canvas, (struct rect){0, 0, 2, 60}, &plain, "mm"), 2 * line);
    /* 399 narrow letters fit a very wide box in pieces of at most 255. */
    CHECK_INT(canvas_text_wrapped(&canvas, (struct rect){0, 0, 100000, 60}, &plain, long_word),
              2 * line);
}

static void pictures_map_characters_to_colours(void) {
    static const char *const rows[] = {"r.b", ".r.", NULL};
    static const uint32_t palette[] = {'r', RED, 'b', BLUE, 0};
    clear();
    canvas_picture(&canvas, (struct point){10, 10}, rows, palette);
    CHECK_INT(at(10, 10), RED);
    CHECK_INT(at(11, 10), 0);
    CHECK_INT(at(12, 10), BLUE);
    CHECK_INT(at(11, 11), RED);
    CHECK_INT(painted(), 3);
}

void suite_canvas(void) {
    RUN(rectangles_know_what_they_hold);
    RUN(fill_is_clipped_to_the_canvas);
    RUN(edge_lights_top_left_and_shades_bottom_right);
    RUN(dotted_border_skips_every_other_pixel);
    RUN(text_reports_its_width_and_draws_inside_it);
    RUN(bytes_outside_ascii_draw_as_a_question_mark);
    RUN(zoomed_text_paints_blocks);
    RUN(wrapped_text_breaks_at_spaces_and_stops_at_the_box);
    RUN(a_word_wider_than_the_box_is_broken_mid_word);
    RUN(pictures_map_characters_to_colours);
}
