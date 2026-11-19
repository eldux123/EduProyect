#!/usr/bin/env bash
eval "$(opam env)"
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect
frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 15 -wp-print \
  -cpp-extra-args="-Iinclude -Iverification" \
  -wp-fct professional_register src/professionals.c > /tmp/wp_detail.txt 2>&1
echo "=== goal headers (Post-condition / Assigns) ==="
grep -nE "^Goal|Post-condition|Assigns|Prove:" /tmp/wp_detail.txt | grep -iE "Post-condition|Assigns|Prove:" | head -40
