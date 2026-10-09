#ifndef LOCKIN_CORE_CSV_H
#define LOCKIN_CORE_CSV_H

#include "core/text.h"

/* Appends one field and its separator: a comma, or a newline when `last`.
 * Fields holding commas or quotes are quoted. A field starting with = + - or
 * @ gets a leading apostrophe so a spreadsheet does not run it as a formula. */
void csv_put(struct sbuf *out, const char *field, bool last);

/* Splits one line into at most `max` fields, unquoting in place. Returns the
 * number of fields found. */
int csv_split(char *line, char **fields, int max);

#endif
