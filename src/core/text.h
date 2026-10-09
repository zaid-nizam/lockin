#ifndef LOCKIN_CORE_TEXT_H
#define LOCKIN_CORE_TEXT_H

#include <stdbool.h>
#include <stddef.h>

/* A string builder over caller-owned storage. It never allocates and never
 * overruns: output that does not fit is dropped and `truncated` is set. */
struct sbuf {
    char *data;
    size_t cap;
    size_t len;
    bool truncated;
};

/* Starts an empty builder on `storage`, which must hold at least one byte. */
struct sbuf sbuf_on(char *storage, size_t cap);

/* Appends a string. */
void sbuf_put(struct sbuf *sb, const char *s);

/* Appends the first `count` bytes of a string. */
void sbuf_putn(struct sbuf *sb, const char *s, size_t count);

/* Appends formatted text of up to 511 bytes per call. */
void sbuf_putf(struct sbuf *sb, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/* Copies `src` into `dst`, truncating to fit, and always terminates it. */
void text_copy(char *dst, size_t cap, const char *src);

/* Removes leading and trailing whitespace in place and returns the new start. */
char *text_trim(char *s);

/* Compares two ASCII strings ignoring case. */
bool text_same_nocase(const char *a, const char *b);

/* Reads one UTF-8 sequence at `*s` and advances past it. Returns the byte for
 * ASCII and '?' for everything else, because the bitmap font is ASCII only. */
char text_next_ascii(const char **s);

#endif
