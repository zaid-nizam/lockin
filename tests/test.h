#ifndef LOCKIN_TESTS_TEST_H
#define LOCKIN_TESTS_TEST_H

/* A test harness small enough to read in a minute. A test is a function; a
 * failed CHECK prints where and what, and the run exits non-zero. */

#include <stdbool.h>

void test_run(const char *name, void (*test)(void));
void test_fail(const char *file, int line, const char *what);
void test_fail_int(const char *file, int line, long long actual, long long expected);
void test_fail_str(const char *file, int line, const char *actual, const char *expected);
bool test_same(const char *a, const char *b);
int test_summary(void);

#define RUN(test) test_run(#test, test)

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            test_fail(__FILE__, __LINE__, #condition);                                             \
        }                                                                                          \
    } while (0)

#define CHECK_INT(actual, expected)                                                                \
    do {                                                                                           \
        long long actual_value = (long long)(actual);                                              \
        long long expected_value = (long long)(expected);                                          \
        if (actual_value != expected_value) {                                                      \
            test_fail_int(__FILE__, __LINE__, actual_value, expected_value);                       \
        }                                                                                          \
    } while (0)

#define CHECK_STR(actual, expected)                                                                \
    do {                                                                                           \
        const char *actual_text = (actual);                                                        \
        if (!test_same(actual_text, (expected))) {                                                 \
            test_fail_str(__FILE__, __LINE__, actual_text, (expected));                            \
        }                                                                                          \
    } while (0)

#define CHECK_HAS(text, part) CHECK(strstr((text), (part)) != NULL)

#endif
