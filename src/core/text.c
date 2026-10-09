#include "core/text.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

struct sbuf sbuf_on(char *storage, size_t cap) {
    struct sbuf sb = {storage, cap, 0, false};
    storage[0] = '\0';
    return sb;
}

void sbuf_put(struct sbuf *sb, const char *s) {
    sbuf_putn(sb, s, strlen(s));
}

void sbuf_putn(struct sbuf *sb, const char *s, size_t count) {
    size_t want = count;
    size_t room = sb->cap - sb->len - 1;
    if (want > room) {
        want = room;
        sb->truncated = true;
    }
    memcpy(sb->data + sb->len, s, want);
    sb->len += want;
    sb->data[sb->len] = '\0';
}

void sbuf_putf(struct sbuf *sb, const char *fmt, ...) {
    char piece[512];
    va_list args;
    va_start(args, fmt);
    /* NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized): va_start is the line above. */
    int wrote = vsnprintf(piece, sizeof piece, fmt, args);
    va_end(args);
    if (wrote >= (int)sizeof piece) {
        sb->truncated = true;
    }
    sbuf_put(sb, piece);
}

void text_copy(char *dst, size_t cap, const char *src) {
    struct sbuf sb = sbuf_on(dst, cap);
    sbuf_put(&sb, src);
}

char *text_trim(char *s) {
    while (isspace((unsigned char)*s)) {
        s++;
    }
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        len--;
    }
    s[len] = '\0';
    return s;
}

bool text_same_nocase(const char *a, const char *b) {
    while (*a != '\0' && tolower((unsigned char)*a) == tolower((unsigned char)*b)) {
        a++;
        b++;
    }
    return *a == *b;
}

char text_next_ascii(const char **s) {
    unsigned char first = (unsigned char)**s;
    (*s)++;
    if (first < 0x80) {
        return (char)first;
    }
    /* Skip the continuation bytes so one code point draws as one '?'. */
    while (((unsigned char)**s & 0xC0) == 0x80) {
        (*s)++;
    }
    return '?';
}
