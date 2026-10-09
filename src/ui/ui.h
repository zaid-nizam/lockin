#ifndef LOCKIN_UI_UI_H
#define LOCKIN_UI_UI_H

/* An immediate-mode toolkit in the Windows 2000 style. There is no widget
 * tree: each frame the caller draws every control again, and each control
 * call both paints itself and reports what the user did to it.
 *
 *     ui_begin(&ui);
 *     if (ui_button(&ui, where, "Save", BUTTON_DEFAULT)) { save(); }
 *     ui_end(&ui);
 *
 * Controls take keyboard focus in the order they are drawn. */

#include "platform/platform.h"
#include "ui/canvas.h"

#include <stddef.h>

enum { UI_KEYS = 16, UI_HEADER_HEIGHT = 52, UI_STATUS_HEIGHT = 22 };

struct ui {
    struct canvas canvas;
    int mouse_x;
    int mouse_y;
    bool held;
    bool pressed;
    bool released;
    int keys[UI_KEYS];
    int key_count;
    int focus;
    int active;
    int drawn;
    int drawn_last_frame;
    bool button_focused;
    bool button_was_focused;
    bool caret;
};

enum button_flags { BUTTON_PLAIN = 0, BUTTON_DEFAULT = 1, BUTTON_DISABLED = 2 };

/* One option of a radio group: `*value` is the group's selection and `mine`
 * is the value this option stands for. */
struct choice {
    int *value;
    int mine;
};

/* Feeds one platform event into the toolkit. */
void ui_input(struct ui *ui, const struct pf_event *event);

/* Starts a frame: paints the window background and moves focus on Tab. */
void ui_begin(struct ui *ui);

/* Ends a frame: forgets this frame's clicks and key presses. */
void ui_end(struct ui *ui);

/* Puts focus on the first control. Call when the screen changes. */
void ui_reset_focus(struct ui *ui);

/* True once per press of `key` this frame. */
bool ui_key(const struct ui *ui, int key);

/* A push button. Returns true when clicked, or activated with Space or
 * Enter while focused. A BUTTON_DEFAULT button also fires on Enter when
 * focus is on something that is not a button. */
bool ui_button(struct ui *ui, struct rect r, const char *label, enum button_flags flags);

/* A one-line text box editing `text` in place (capacity `cap` bytes). Typing
 * adds to the end and Backspace removes from it. */
void ui_field(struct ui *ui, struct rect r, char *text, size_t cap);

/* A check box with its label. */
void ui_check(struct ui *ui, struct point at, const char *label, bool *on);

/* One radio button of a group. */
void ui_radio(struct ui *ui, struct point at, const char *label, struct choice choice);

/* Plain, bold and triple-size text. */
void ui_label(struct ui *ui, struct point at, const char *text);
void ui_label_bold(struct ui *ui, struct point at, const char *text);
void ui_label_big(struct ui *ui, struct point at, const char *text);

/* Text wrapped inside a box. Returns the height used. */
int ui_text(struct ui *ui, struct rect box, const char *text);

/* A framed group with a title set into its top edge. */
void ui_group(struct ui *ui, struct rect r, const char *title);

/* The white strip across the top of the window: a bold title and one line
 * saying what this screen is for. */
void ui_header(struct ui *ui, const char *title, const char *subtitle);

/* The status bar along the bottom, with a wide left pane and a narrow right. */
void ui_status(struct ui *ui, const char *left, const char *right);

/* A progress bar filled to `permille` thousandths. */
void ui_progress(struct ui *ui, struct rect r, int permille);

/* A sunken, read-only line of text. */
void ui_readout(struct ui *ui, struct rect r, const char *text);

/* A raised column heading for a table. */
void ui_column(struct ui *ui, struct rect r, const char *title);

#endif
