#include "test.h"

#include <stdio.h>
#include <string.h>

static int failures;
static int tests_run;
static const char *current;

void test_run(const char *name, void (*test)(void)) {
    current = name;
    tests_run++;
    test();
}

static void where(const char *file, int line) {
    failures++;
    printf("FAIL %s (%s:%d)\n", current, file, line);
}

void test_fail(const char *file, int line, const char *what) {
    where(file, line);
    printf("  not true: %s\n", what);
}

void test_fail_int(const char *file, int line, long long actual, long long expected) {
    where(file, line);
    printf("  got %lld, expected %lld\n", actual, expected);
}

void test_fail_str(const char *file, int line, const char *actual, const char *expected) {
    where(file, line);
    printf("  got      \"%s\"\n  expected \"%s\"\n", actual, expected);
}

bool test_same(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

int test_summary(void) {
    printf("%d tests, %d failed\n", tests_run, failures);
    return failures == 0 ? 0 : 1;
}
