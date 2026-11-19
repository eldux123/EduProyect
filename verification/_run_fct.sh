#!/usr/bin/env bash
# Uso: _run_fct.sh <funcion> <archivo.c>
eval "$(opam env)"
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect
frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 -wp-fct "$1" \
  -cpp-extra-args="-Iinclude -Iverification" "$2" 2>&1 \
  | grep -E "Proved goals|Qed:|Alt-Ergo|Timeout:|Failed:|\[Failure\]|\[Timeout\]"
