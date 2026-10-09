#include "core/kv.h"

#include "core/text.h"

#include <string.h>

char *kv_line(char **cursor) {
    char *line = *cursor;
    if (line == NULL || *line == '\0') {
        return NULL;
    }
    char *end = strchr(line, '\n');
    if (end == NULL) {
        *cursor = NULL;
    } else {
        *end = '\0';
        *cursor = end + 1;
    }
    return line;
}

bool kv_split(char *line, char **key, char **value) {
    char *start = text_trim(line);
    char *equals = strchr(start, '=');
    if (start[0] == '#' || equals == NULL) {
        return false;
    }
    *equals = '\0';
    *key = text_trim(start);
    *value = text_trim(equals + 1);
    return true;
}
