#!/usr/bin/env bash
# Prueba un modelo de memoria de WP distinto. Uso: _run_model.sh <modelo> <archivo.c>
eval "$(opam env)"
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect
frama-c -wp -wp-rte -wp-model "$1" -wp-prover alt-ergo,z3 -wp-timeout 20 \
  -cpp-extra-args="-Iinclude -Iverification" "$2" 2>&1 \
  | grep -E "Proved goals|Qed:|Alt-Ergo|Z3|Timeout:|Failed:|\[Failure\]|\[Timeout\]|User Error|invalid"
