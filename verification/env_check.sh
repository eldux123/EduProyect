#!/usr/bin/env bash
eval "$(opam env 2>/dev/null)"
echo "=== frama-c ==="
frama-c -version
echo "=== provers (why3) ==="
why3 config list-provers 2>/dev/null || why3 prove --list-provers 2>/dev/null
echo "=== which ==="
which frama-c alt-ergo z3 2>/dev/null
