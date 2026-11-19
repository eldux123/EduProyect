#!/usr/bin/env bash
# EduChain - ui/run_ui.sh
# Compila y ejecuta la CLI interactiva (ui/educhain_cli.c) enlazando los
# modulos de src/. Misma convencion que run_tests.sh / run_demo.sh:
# gcc, -std=c99 -Wall -Wextra, -Iinclude. Script independiente; NO toca
# run_tests.sh ni la logica verificada.
#
# Uso:
#   bash ui/run_ui.sh          # build normal (-Wall -Wextra) y ejecuta la CLI
#   bash ui/run_ui.sh --asan   # build con ASan/UBSan y ejecuta la CLI
#   bash ui/run_ui.sh --build  # solo compila (ambas builds), no ejecuta
set -e
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect

SRC="src/security.c src/audit.c src/professionals.c src/experience.c src/credentials.c"
UI="ui/educhain_cli.c"

echo "### Build CLI normal (-Wall -Wextra) ###"
gcc -std=c99 -Wall -Wextra -Iinclude $SRC $UI -o /tmp/educhain_cli

case "${1:-}" in
  --asan)
    echo "### Build CLI con ASan + UBSan ###"
    gcc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -fno-omit-frame-pointer \
        -Iinclude $SRC $UI -o /tmp/educhain_cli_san
    echo "--- build OK, ejecutando (ASan/UBSan) ---"
    exec /tmp/educhain_cli_san
    ;;
  --build)
    echo "### Build CLI con ASan + UBSan ###"
    gcc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -fno-omit-frame-pointer \
        -Iinclude $SRC $UI -o /tmp/educhain_cli_san
    echo "--- ambas builds OK (no se ejecuta) ---"
    ;;
  *)
    echo "--- build OK, ejecutando ---"
    exec /tmp/educhain_cli
    ;;
esac
