#ifndef LOCKIN_CORE_KV_H
#define LOCKIN_CORE_KV_H

#include <stdbool.h>

/* Returns the next line of a mutable text buffer and advances `*cursor`, or
 * NULL when the text is used up. The newline is replaced by a terminator. */
char *kv_line(char **cursor);

/* Splits "key = value" in place and trims both sides. Returns false for
 * blank lines, # comments and lines with no '='. */
bool kv_split(char *line, char **key, char **value);

#endif
