#!/usr/bin/env bash
set -e
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect
gcc -std=c99 -Wall -Wextra -Iinclude src/security.c src/audit.c src/professionals.c src/experience.c src/credentials.c main.c -o /tmp/demo
echo "--- build OK ---"
/tmp/demo
