#include "core/experiment.h"

int experiment_week(int64_t first_day, int64_t today) {
    return today < first_day ? 1 : (int)((today - first_day) / 7) + 1;
}

enum variable experiment_variable(int week) {
    static const enum variable by_week[] = {VAR_BASELINE, VAR_SOUND, VAR_EXERCISE, VAR_COWORK};
    return week >= 1 && week <= 4 ? by_week[week - 1] : VAR_FREE;
}

/* SplitMix64 finaliser: spreads nearby inputs (consecutive days) evenly. */
static uint64_t mix(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

char experiment_coin(uint64_t seed, struct slot slot, enum variable variable) {
    bool applies = variable == VAR_SOUND || variable == VAR_COWORK ||
                   (variable == VAR_EXERCISE && slot.id == SLOT_AM);
    if (!applies) {
        return COIN_NONE;
    }
    return (mix(seed ^ mix((uint64_t)slot.stamp)) & 1) == 0 ? COIN_A : COIN_B;
}

const char *experiment_instruction(enum variable variable, char coin) {
    static const char *const text[][2] = {
        [VAR_SOUND] = {"Coin: work in silence.", "Coin: play pink noise, no music."},
        [VAR_EXERCISE] = {"Coin: start straight away, no exercise first.",
                          "Coin: walk briskly for 10 to 20 minutes, then start."},
        [VAR_COWORK] = {"Coin: work alone.", "Coin: sit in a silent co-work voice room."},
    };
    if (variable == VAR_BASELINE) {
        return "Baseline week: change nothing, just log.";
    }
    if (variable == VAR_FREE) {
        return "Experiment finished: keep what the Stats screen supports.";
    }
    if (coin == COIN_NONE) {
        return "This week tests exercise before the AM slot. Nothing to do for PM.";
    }
    return text[variable][coin == COIN_B];
}
