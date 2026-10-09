#ifndef LOCKIN_CORE_EXPERIMENT_H
#define LOCKIN_CORE_EXPERIMENT_H

#include "core/slot.h"

/* What the four-week self-experiment varies. One variable per week, so a
 * change in the log can be traced to one cause. */
enum variable { VAR_BASELINE, VAR_SOUND, VAR_EXERCISE, VAR_COWORK, VAR_FREE };

enum { COIN_A = 'A', COIN_B = 'B', COIN_NONE = 'N' };

/* Week number, starting at 1, counted from the day of the first log row. */
int experiment_week(int64_t first_day, int64_t today);

/* The variable tested in a given week. */
enum variable experiment_variable(int week);

/* The condition for one slot: COIN_A, COIN_B, or COIN_NONE when the week's
 * variable does not apply to it. The result depends only on the seed, the
 * date and the slot, so reopening the program cannot re-roll it. */
char experiment_coin(uint64_t seed, struct slot slot, enum variable variable);

/* One sentence telling the user what to do for this slot. */
const char *experiment_instruction(enum variable variable, char coin);

#endif
