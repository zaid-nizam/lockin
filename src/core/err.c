#include "core/err.h"

#include <stdarg.h>
#include <stdio.h>

bool err_set(struct error *err, enum err_code code, const char *fmt, ...) {
    va_list args;
    err->code = code;
    va_start(args, fmt);
    /* NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized): va_start is two lines up. */
    vsnprintf(err->text, sizeof err->text, fmt, args);
    va_end(args);
    return false;
}

const char *err_name(enum err_code code) {
    static const char *const names[] = {"OK",          "E_IO",    "E_CONFIG", "E_INPUT",
                                        "E_PRIVILEGE", "E_BLOCK", "E_DISPLAY"};
    return names[code];
}
