#include "platform/platform.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <poll.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum { WIDTH_MAX = 640, HEIGHT_MAX = 520 };

/* The one window. Its scaled copy of the canvas lives in static storage so
 * opening a window cannot fail for lack of memory; untouched pages of it
 * cost nothing at scale 1. */
static struct {
    Display *display;
    Window window;
    GC gc;
    XImage *image;
    Atom close_request;
    int width;
    int height;
    int scale;
} x11;

static uint32_t scaled[WIDTH_MAX * HEIGHT_MAX * PF_SCALE_MAX * PF_SCALE_MAX];

static int pick_scale(int wanted) {
    if (wanted >= 1 && wanted <= PF_SCALE_MAX) {
        return wanted;
    }
    int screen_height = DisplayHeight(x11.display, DefaultScreen(x11.display));
    if (screen_height >= 2800) {
        return 3;
    }
    return screen_height >= 1800 ? 2 : 1;
}

/* The canvas is 0xRRGGBB; refuse displays that would show it in wrong colours. */
static bool true_color(void) {
    const Visual *visual = DefaultVisual(x11.display, DefaultScreen(x11.display));
    int depth = DefaultDepth(x11.display, DefaultScreen(x11.display));
    return depth >= 24 && visual->red_mask == 0xFF0000 && visual->green_mask == 0x00FF00 &&
           visual->blue_mask == 0x0000FF;
}

static void create_window(const struct pf_window *window) {
    Display *d = x11.display;
    int screen = DefaultScreen(d);
    unsigned width = (unsigned)(x11.width * x11.scale);
    unsigned height = (unsigned)(x11.height * x11.scale);
    XSizeHints size = {.flags = PMinSize | PMaxSize,
                       .min_width = (int)width,
                       .max_width = (int)width,
                       .min_height = (int)height,
                       .max_height = (int)height};
    XClassHint class_hint = {.res_name = (char[]){"lockin"}, .res_class = (char[]){"Lockin"}};
    x11.window = XCreateSimpleWindow(d, RootWindow(d, screen), 0, 0, width, height, 0, 0, 0xD4D0C8);
    XStoreName(d, x11.window, window->title);
    XSetWMNormalHints(d, x11.window, &size);
    XSetClassHint(d, x11.window, &class_hint);
    XSelectInput(d, x11.window,
                 ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask |
                     PointerMotionMask);
    x11.close_request = XInternAtom(d, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(d, x11.window, &x11.close_request, 1);
    x11.gc = XCreateGC(d, x11.window, 0, NULL);
    x11.image = XCreateImage(d, DefaultVisual(d, screen), (unsigned)DefaultDepth(d, screen),
                             ZPixmap, 0, (char *)scaled, width, height, 32, 0);
    XMapWindow(d, x11.window);
    XFlush(d);
}

bool pf_open(const struct pf_window *window, struct error *err) {
    const char *name = getenv("DISPLAY");
    if (window->width > WIDTH_MAX || window->height > HEIGHT_MAX) {
        return err_set(err, ERR_DISPLAY,
                       "A %dx%d window is larger than the %dx%d this build "
                       "supports.",
                       window->width, window->height, WIDTH_MAX, HEIGHT_MAX);
    }
    x11.display = XOpenDisplay(NULL);
    if (x11.display == NULL) {
        return err_set(err, ERR_DISPLAY,
                       "Cannot open the X display \"%s\". Run lockin from a "
                       "terminal inside your desktop session; on Wayland, "
                       "XWayland must be running.",
                       name != NULL ? name : "(DISPLAY is not set)");
    }
    if (!true_color()) {
        pf_close();
        return err_set(err, ERR_DISPLAY,
                       "This display is not 24-bit true colour, which lockin "
                       "needs. Set the X server to depth 24.");
    }
    x11.width = window->width;
    x11.height = window->height;
    x11.scale = pick_scale(window->scale);
    create_window(window);
    return true;
}

void pf_close(void) {
    if (x11.image != NULL) {
        /* The pixels are static storage; stop Xlib from freeing them. */
        x11.image->data = NULL;
        XDestroyImage(x11.image);
    }
    if (x11.display != NULL) {
        XCloseDisplay(x11.display);
    }
    memset(&x11, 0, sizeof x11);
}

static int key_code(XKeyEvent *press) {
    static const struct {
        KeySym sym;
        int code;
    } named[] = {{XK_BackSpace, KEY_BACKSPACE},
                 {XK_ISO_Left_Tab, KEY_BACKTAB},
                 {XK_Return, KEY_ENTER},
                 {XK_KP_Enter, KEY_ENTER},
                 {XK_Escape, KEY_ESCAPE}};
    char typed[8];
    KeySym sym = NoSymbol;
    int count = XLookupString(press, typed, sizeof typed, &sym, NULL);
    if (sym == XK_Tab) {
        return (press->state & ShiftMask) != 0 ? KEY_BACKTAB : KEY_TAB;
    }
    for (size_t i = 0; i < sizeof named / sizeof named[0]; i++) {
        if (named[i].sym == sym) {
            return named[i].code;
        }
    }
    return count == 1 && typed[0] >= ' ' && typed[0] <= '~' ? typed[0] : 0;
}

static bool mouse_event(const XEvent *in, struct pf_event *out) {
    static const enum pf_kind kinds[] = {
        [ButtonPress] = PF_PRESS, [ButtonRelease] = PF_RELEASE, [MotionNotify] = PF_MOVE};
    out->kind = kinds[in->type];
    /* Press, release and motion events share the position fields. */
    out->x = in->xbutton.x / x11.scale;
    out->y = in->xbutton.y / x11.scale;
    return in->type == MotionNotify || in->xbutton.button == Button1;
}

/* Turns an X event into ours. Returns false for events lockin ignores. */
static bool translate(XEvent *in, struct pf_event *out) {
    memset(out, 0, sizeof *out);
    switch (in->type) {
    case Expose:
        out->kind = PF_REDRAW;
        return in->xexpose.count == 0;
    case ClientMessage:
        out->kind = PF_CLOSE;
        return (Atom)in->xclient.data.l[0] == x11.close_request;
    case KeyPress:
        out->kind = PF_KEY;
        out->key = key_code(&in->xkey);
        return out->key != 0;
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
        return mouse_event(in, out);
    default:
        return false;
    }
}

bool pf_wait(int timeout_ms, struct pf_event *event) {
    for (;;) {
        while (XPending(x11.display) > 0) {
            XEvent incoming;
            XNextEvent(x11.display, &incoming);
            if (translate(&incoming, event)) {
                return true;
            }
        }
        struct pollfd connection = {ConnectionNumber(x11.display), POLLIN, 0};
        if (poll(&connection, 1, timeout_ms) <= 0) {
            return false;
        }
    }
}

void pf_show(const uint32_t *pixels) {
    int scale = x11.scale;
    int out_width = x11.width * scale;
    for (int y = 0; y < x11.height * scale; y++) {
        const uint32_t *row = pixels + (ptrdiff_t)(y / scale) * x11.width;
        for (int x = 0; x < out_width; x++) {
            scaled[y * out_width + x] = row[x / scale];
        }
    }
    XPutImage(x11.display, x11.window, x11.gc, x11.image, 0, 0, 0, 0, (unsigned)out_width,
              (unsigned)(x11.height * scale));
    XFlush(x11.display);
}
