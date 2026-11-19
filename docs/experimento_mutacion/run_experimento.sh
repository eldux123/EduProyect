#!/usr/bin/env bash
# =====================================================================
# EduChain - experimento de mutacion (reproducible). Entregable 1.
#
# Demuestra que Frama-C WP DETECTA una violacion deliberada de una propiedad
# de autorizacion en security_can, con el patron antes/despues A -> B -> C -> D.
#
# GARANTIAS DE AISLAMIENTO:
#   - Trabaja SOLO en una copia temporal en /tmp/educhain_mutacion.
#   - NO escribe nada en el repositorio original.
#   - Usa -wp-cache none para no tocar la cache WP versionada (.frama-c/wp/cache).
#
# USO (desde la raiz del repo, en WSL Ubuntu):
#   wsl -d Ubuntu bash docs/experimento_mutacion/run_experimento.sh
# =====================================================================
set -u

# --- localizar la raiz del repo ---
# 1) a partir de la ubicacion del script (modo normal: se ejecuta en su sitio);
# 2) si eso no da una raiz valida (p. ej. el script se copio a /tmp), se usa git;
# 3) como ultimo recurso, el directorio de trabajo actual.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" 2>/dev/null && pwd)"
REPO="$(cd "$SCRIPT_DIR/../.." 2>/dev/null && pwd)"
if [ -z "${REPO:-}" ] || [ ! -f "$REPO/src/security.c" ]; then
  REPO="$(git rev-parse --show-toplevel 2>/dev/null)"
fi
if [ -z "${REPO:-}" ] || [ ! -f "$REPO/src/security.c" ]; then
  REPO="$(pwd)"
fi
if [ ! -f "$REPO/src/security.c" ]; then
  echo "ERROR: no encuentro la raiz del repo (falta src/security.c en '$REPO')." >&2
  echo "       Ejecuta el script desde la raiz del repo o desde su ubicacion original." >&2
  exit 2
fi

# --- cargar entorno opam si frama-c no esta en PATH ---
if ! command -v frama-c >/dev/null 2>&1; then
  eval "$(opam env 2>/dev/null)" || true
fi
if ! command -v frama-c >/dev/null 2>&1; then
  echo "ERROR: frama-c no esta en PATH. Instala Frama-C via opam y reintenta." >&2
  exit 2
fi

WORK=/tmp/educhain_mutacion
ORIG="$WORK/original"
MUT="$WORK/mutado"
OUT="$WORK/salidas"

rm -rf "$WORK"
mkdir -p "$ORIG/src" "$ORIG/include" "$ORIG/verification" "$OUT"

# --- copia aislada: solo lo necesario para verificar security.c ---
cp "$REPO/src/security.c"                 "$ORIG/src/security.c"
cp "$REPO/include/security.h"             "$ORIG/include/security.h"
cp "$REPO/include/common.h"               "$ORIG/include/common.h"
cp "$REPO/verification/specifications.h"  "$ORIG/verification/specifications.h"
cp -r "$ORIG" "$MUT"

# --- flags WP ---
WPFLAGS_Z3="-wp -wp-rte -wp-prover z3 -wp-cache none -wp-timeout 20 -wp-fct security_can"
WPFLAGS_AE="-wp -wp-rte -wp-prover alt-ergo,z3 -wp-cache none -wp-timeout 20 -wp-fct security_can"
CPP() { echo "-cpp-extra-args=-I$1/include -I$1/verification"; }

{
  echo "################################################################"
  echo "# EduChain - experimento de mutacion - $(date -u '+%Y-%m-%d %H:%M UTC')"
  echo "# frama-c: $(frama-c -version 2>&1 | head -1)"
  echo "# alt-ergo: $(alt-ergo --version 2>&1 | head -1)"
  echo "################################################################"
} > "$OUT/resumen.txt"

# --- captura de versiones de herramientas (entregable 6: registrar versiones) ---
{
  echo "# EduChain - experimento de mutacion - entorno capturado (regenerado en la corrida)"
  echo "fecha_utc: $(date -u '+%Y-%m-%d %H:%M UTC')"
  echo "frama-c: $(frama-c -version 2>&1 | head -1)"
  echo "alt-ergo: $(alt-ergo --version 2>&1 | head -1)"
  echo "--- why3 provers detectados (incluye Z3 usado en FASE C.2) ---"
  why3 config list-provers 2>&1 || true
  echo "--- binarios ---"
  command -v frama-c
  command -v alt-ergo
  command -v why3 2>/dev/null || true
} > "$OUT/env_capturado.txt"

# ============================ FASE A: estado ORIGINAL ============================
echo ">>> FASE A: verificando security.c ORIGINAL (se espera 114/114)"
{
  echo "### FASE A - ORIGINAL"
  echo "### frama-c $WPFLAGS_AE $(CPP "$ORIG") src/security.c"
} > "$OUT/faseA_wp.txt"
( cd "$ORIG" && frama-c $WPFLAGS_AE "$(CPP "$ORIG")" src/security.c ) >> "$OUT/faseA_wp.txt" 2>&1
grep -E "Proved goals|Qed:|Alt-Ergo|Timeout|Failed|Failure" "$OUT/faseA_wp.txt" | sed 's/^/[A] /' | tee -a "$OUT/resumen.txt"

# ============================ FASE B: mutacion M1 ============================
echo ">>> FASE B: aplicando mutacion M1 (VALIDATE_EXP autoriza ademas a ROLE_VERIFIER)"
python3 - "$MUT/src/security.c" << 'PY'
import sys, io
p = sys.argv[1]; s = io.open(p, encoding='utf-8').read()
old = "    case OP_VALIDATE_EXP:\n        return (caller_role == ROLE_HR || caller_role == ROLE_COORD) ? 1 : 0;"
new = "    case OP_VALIDATE_EXP:\n        return (caller_role == ROLE_HR || caller_role == ROLE_COORD || caller_role == ROLE_VERIFIER) ? 1 : 0;"
assert old in s, "ERROR: ancla de mutacion M1 no encontrada en security.c"
io.open(p, 'w', encoding='utf-8').write(s.replace(old, new))
print("mutacion M1 aplicada")
PY
diff -u "$ORIG/src/security.c" "$MUT/src/security.c" > "$OUT/mutacion_M1.diff" 2>&1 || true

# ============================ FASE C: deteccion ============================
echo ">>> FASE C: verificando security.c MUTADO"
echo ">>>   (C.1) con alt-ergo,z3  -> se espera Timeout (NO concluyente)"
{
  echo "### FASE C.1 - MUTADO - prover alt-ergo,z3 (contraste: Timeout, NO prueba deteccion)"
  echo "### frama-c $WPFLAGS_AE $(CPP "$MUT") src/security.c"
} > "$OUT/faseC_wp_alt-ergo.txt"
( cd "$MUT" && frama-c $WPFLAGS_AE "$(CPP "$MUT")" src/security.c ) >> "$OUT/faseC_wp_alt-ergo.txt" 2>&1
grep -E "Proved goals|Qed:|Alt-Ergo|Timeout|Failed|Failure" "$OUT/faseC_wp_alt-ergo.txt" | sed 's/^/[C.1 AE] /' | tee -a "$OUT/resumen.txt"

echo ">>>   (C.2) con z3          -> se espera Failure (DETECCION concluyente)"
{
  echo "### FASE C.2 - MUTADO - prover z3 (resultado concluyente: Failure/refutacion)"
  echo "### frama-c $WPFLAGS_Z3 $(CPP "$MUT") src/security.c"
} > "$OUT/faseC_wp_z3.txt"
( cd "$MUT" && frama-c $WPFLAGS_Z3 "$(CPP "$MUT")" src/security.c ) >> "$OUT/faseC_wp_z3.txt" 2>&1
grep -E "Proved goals|Qed:|Z3|Timeout|Failed|Failure" "$OUT/faseC_wp_z3.txt" | sed 's/^/[C.2 Z3] /' | tee -a "$OUT/resumen.txt"

# diagnostico de rigor: FASE C.2 debe producir al menos un [Failure]
if grep -q "\[Failure\]" "$OUT/faseC_wp_z3.txt"; then
  echo "DIAGNOSTICO: M1 reprodujo el Failure esperado (deteccion confirmada por Z3)." | tee -a "$OUT/resumen.txt"
else
  echo "DIAGNOSTICO: M1 NO produjo Failure con Z3. Revisar entorno o usar la mutacion de respaldo M2" | tee -a "$OUT/resumen.txt"
  echo "            (ISSUE/ACTIVATE_CRED -> 'return 1;'). Ver comparacion.md." | tee -a "$OUT/resumen.txt"
fi

# ============================ FASE D: restauracion ============================
echo ">>> FASE D: re-verificando security.c ORIGINAL (se espera de nuevo 114/114)"
{
  echo "### FASE D - ORIGINAL restaurado"
  echo "### frama-c $WPFLAGS_AE $(CPP "$ORIG") src/security.c"
} > "$OUT/faseD_wp.txt"
( cd "$ORIG" && frama-c $WPFLAGS_AE "$(CPP "$ORIG")" src/security.c ) >> "$OUT/faseD_wp.txt" 2>&1
grep -E "Proved goals|Qed:|Alt-Ergo|Timeout|Failed|Failure" "$OUT/faseD_wp.txt" | sed 's/^/[D] /' | tee -a "$OUT/resumen.txt"

# descartar la copia mutada (FASE D: descartar la copia modificada)
rm -rf "$MUT"

# ============================ evidencia de no-modificacion ============================
echo ">>> Evidencia: git status del repo original (debe estar limpio en preexistentes)"
{
  echo "### git status --porcelain de archivos preexistentes (debe salir vacio)"
  ( cd "$REPO" && git status --porcelain src/ include/ verification/ .frama-c/ tests/ main.c )
  echo "### (fin; vacio = sin cambios)"
} | tee -a "$OUT/resumen.txt"

echo
echo ">>> LISTO. Salidas completas en: $OUT"
echo ">>> Resumen:"
cat "$OUT/resumen.txt"
