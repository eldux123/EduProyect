#!/usr/bin/env bash
set -e
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect
SRC="src/security.c src/audit.c src/professionals.c src/experience.c src/credentials.c"
TST="tests/test_professionals.c tests/test_experience.c tests/test_credentials.c tests/test_attacks.c tests/test_main.c"

echo "### Build normal (-Wall -Wextra) ###"
gcc -std=c99 -Wall -Wextra -Iinclude -Itests $SRC $TST -o /tmp/educhain_tests
/tmp/educhain_tests

echo
echo "### Build con ASan + UBSan ###"
gcc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Iinclude -Itests $SRC $TST -o /tmp/educhain_tests_san
/tmp/educhain_tests_san
