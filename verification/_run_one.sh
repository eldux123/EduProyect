#!/usr/bin/env bash
# Uso: _run_one.sh <archivo.c>
eval "$(opam env)"
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect
frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 \
  -cpp-extra-args="-Iinclude -Iverification" "$1" 2>&1 \
  | grep -E "Proved goals|Qed:|Alt-Ergo|Z3|Timeout:|Failed:|Unreachable|Terminating|\[Failure\]|\[Timeout\]"
