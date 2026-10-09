/* Test helper: asks a window to close the way a window manager does, by
 * sending it WM_DELETE_WINDOW. The end-to-end tests run without a window
 * manager, so nothing else would.
 *
 *   x11_send WINDOW_ID
 */

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    Display *display = XOpenDisplay(NULL);
    if (argc != 2 || display == NULL) {
        fprintf(stderr, "usage: x11_send WINDOW_ID (with DISPLAY set)\n");
        return 2;
    }
    Window window = (Window)strtoul(argv[1], NULL, 0);
    XEvent event = {0};
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = (long)XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSendEvent(display, window, False, NoEventMask, &event);
    XCloseDisplay(display);
    return 0;
}
