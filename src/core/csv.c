#include "core/csv.h"

#include <string.h>

void csv_put(struct sbuf *out, const char *field, bool last) {
    bool quote = strpbrk(field, ",\"") != NULL;
    if (quote) {
        sbuf_put(out, "\"");
    }
    if (field[0] != '\0' && strchr("=+-@", field[0]) != NULL) {
        sbuf_put(out, "'");
    }
    for (const char *c = field; *c != '\0'; c++) {
        char piece[3] = {*c, '\0', '\0'};
        if (*c == '"') {
            piece[1] = '"';
        }
        if (*c == '\n' || *c == '\r') {
            piece[0] = ' ';
        }
        sbuf_put(out, piece);
    }
    if (quote) {
        sbuf_put(out, "\"");
    }
    sbuf_put(out, last ? "\n" : ",");
}

/* Unquotes the field at `read` in place and returns where the next one
 * starts, or NULL when this was the last field on the line. */
static char *take_field(char *read) {
    char *write = read;
    bool quoted = *read == '"';
    if (quoted) {
        read++;
    }
    while (*read != '\0') {
        if (quoted && read[0] == '"' && read[1] == '"') {
            read++;
        } else if (quoted && read[0] == '"') {
            quoted = false;
            read++;
            continue;
        } else if (!quoted && read[0] == ',') {
            *write = '\0';
            return read + 1;
        }
        *write++ = *read++;
    }
    *write = '\0';
    return NULL;
}

int csv_split(char *line, char **fields, int max) {
    int count = 0;
    line[strcspn(line, "\r\n")] = '\0';
    while (line != NULL && count < max) {
        fields[count++] = line;
        line = take_field(line);
    }
    return count;
}
