/* EduChain - test_main.c  (runner) */
#include <stdio.h>
#include "test_framework.h"

int g_tests_run = 0;
int g_tests_failed = 0;

void test_professionals(void);
void test_experience(void);
void test_credentials(void);
void test_attacks(void);

int main(void)
{
    printf("=== EduChain test suite ===\n");
    test_professionals();
    test_experience();
    test_credentials();
    test_attacks();
    printf("\n=== Resultado: %d pruebas, %d fallos ===\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
