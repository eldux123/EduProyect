#!/usr/bin/env bash
# EduChain - verify.sh
# Ejecuta Frama-C WP (verificacion deductiva) y Eva (analisis de valores).
# En WSL2 Debian/Ubuntu, con Frama-C instalado via opam:
#   opam install -y frama-c alt-ergo && why3 config detect
# Uso:  bash verification/verify.sh
set -u
cd "$(dirname "$0")/.." || exit 1

# Cargar entorno opam si frama-c no esta en PATH.
if ! command -v frama-c >/dev/null 2>&1; then
  eval "$(opam env 2>/dev/null)" || true
fi
if ! command -v frama-c >/dev/null 2>&1; then
  echo "ERROR: frama-c no esta en PATH. Sin frama-c NO hay resultados de verificacion."
  exit 2
fi

SRC="src/security.c src/audit.c src/professionals.c src/experience.c src/credentials.c"
CPP='-cpp-extra-args=-Iinclude -Iverification'
PROVERS="alt-ergo,z3"

echo "===================================================================="
frama-c -version
echo " Provers WP: $PROVERS"
echo "===================================================================="
echo " [1/2] WP + RTE  (verificacion deductiva por funcion)"
echo "--------------------------------------------------------------------"
: > verification/wp_results.txt
for f in $SRC; do
  echo "#### $f ####" | tee -a verification/wp_results.txt
  frama-c -wp -wp-rte -wp-prover "$PROVERS" -wp-timeout 20 \
    "$CPP" "$f" 2>&1 | grep -E "Proved goals|Qed|Alt-Ergo|Z3|Timeout|Unreachable|Terminating|Failed|Unknown|Parsing|Warning" \
    | tee -a verification/wp_results.txt
  echo "" | tee -a verification/wp_results.txt
done

echo "===================================================================="
echo " [2/2] Eva (analisis de valores: OOB, overflow, no-init, div0)"
echo "--------------------------------------------------------------------"
frama-c -eva -eva-domains cvalue \
  "$CPP" $SRC verification/eva_harness.c -main eva_main 2>&1 \
  | tee verification/eva_results.txt | grep -E "Warning|alarm|assert|out of bound|overflow|division|initiali|Analyzed|EVA|\[eva\]|experiment"

echo "===================================================================="
echo " Hecho. Revisa verification/wp_results.txt y verification/eva_results.txt"
echo " WP demuestra obligaciones; Eva analiza valores; las pruebas unitarias"
echo " solo observan. No afirmar 'verificado' sin la salida de WP."
