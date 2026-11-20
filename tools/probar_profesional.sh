#!/usr/bin/env bash
# Compila y ejecuta la CLI interactiva de registro de profesionales.
set -e
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect
gcc -std=c99 -Wall -Wextra -Iinclude \
    src/security.c src/audit.c src/professionals.c tools/probar_profesional.c \
    -o /tmp/probar_pro
/tmp/probar_pro
