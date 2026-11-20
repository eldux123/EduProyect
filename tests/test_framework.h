/* EduChain - test_framework.h  (mini framework de asserts para pruebas) */
#ifndef EDUCHAIN_TEST_FRAMEWORK_H
#define EDUCHAIN_TEST_FRAMEWORK_H

#include <stdio.h>

extern int g_tests_run;
extern int g_tests_failed;

#define CHECK(cond, msg) do {                                   \
    g_tests_run++;                                              \
    if (!(cond)) {                                             \
        g_tests_failed++;                                      \
        printf("  [FAIL] %s (%s:%d)\n", (msg), __FILE__, __LINE__); \
    } else {                                                   \
        printf("  [ ok ] %s\n", (msg));                        \
    }                                                          \
} while (0)

#define CHECK_EQ(got, want, msg) do {                           \
    g_tests_run++;                                              \
    if ((got) != (want)) {                                     \
        g_tests_failed++;                                      \
        printf("  [FAIL] %s: got=%d want=%d (%s:%d)\n",        \
               (msg), (int)(got), (int)(want), __FILE__, __LINE__); \
    } else {                                                   \
        printf("  [ ok ] %s\n", (msg));                        \
    }                                                          \
} while (0)

#endif /* EDUCHAIN_TEST_FRAMEWORK_H */
