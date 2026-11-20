#!/usr/bin/env bash
# EduChain - verification/gui.sh
# Abre la GUI de Frama-C (frama-c-gui) con WP + Eva sobre un modulo, para
# explorar visualmente precondiciones, postcondiciones y el estado (verde/rojo)
# de cada obligacion de prueba. Pensado para la sustentacion.
#
# Uso:
#   bash verification/gui.sh                # abre todos los modulos (solo WP)
#   bash verification/gui.sh credentials    # un modulo con WP + RTE
#   bash verification/gui.sh requests        # idem
#   bash verification/gui.sh --eva           # abre con Eva (analisis de valores) + harness
#
# Requiere: WSLg (DISPLAY configurado) y frama-c-gui instalado via opam.
set -u

# Cargar node via nvm (Ivette lo requiere) si esta disponible.
if ! command -v node >/dev/null 2>&1; then
  export NVM_DIR="$HOME/.nvm"
  [ -s "$NVM_DIR/nvm.sh" ] && . "$NVM_DIR/nvm.sh" && nvm use 24 >/dev/null 2>&1 || true
fi

# Cargar entorno opam si frama-c-gui no esta en PATH.
if ! command -v frama-c-gui >/dev/null 2>&1; then
  eval "$(opam env 2>/dev/null)" || true
fi
if ! command -v frama-c-gui >/dev/null 2>&1; then
  echo "ERROR: frama-c-gui no esta en PATH. Instala Frama-C via opam."
  exit 2
fi

cd "$(dirname "$0")/.." || exit 1
CPP='-cpp-extra-args=-Iinclude -Iverification'
ALL="src/security.c src/audit.c src/professionals.c src/experience.c src/credentials.c src/requests.c"

# Aviso si no hay servidor grafico.
if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
  echo "AVISO: no hay DISPLAY/WAYLAND_DISPLAY; la ventana podria no abrir."
  echo "En WSL2 moderno (WSLg) deberia estar configurado automaticamente."
fi

case "${1:-}" in
  --eva)
    echo "Abriendo Frama-C GUI con Eva (harness eva_main)..."
    exec frama-c-gui -eva -eva-domains cvalue "$CPP" $ALL \
         verification/eva_harness.c -main eva_main
    ;;
  "")
    echo "Abriendo Frama-C GUI con WP sobre todos los modulos..."
    exec frama-c-gui -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 "$CPP" $ALL
    ;;
  *)
    MOD="src/$1.c"
    if [ ! -f "$MOD" ]; then
      echo "ERROR: no existe $MOD. Modulos: security audit professionals experience credentials requests"
      exit 1
    fi
    echo "Abriendo Frama-C GUI con WP + RTE sobre $MOD..."
    exec frama-c-gui -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 "$CPP" "$MOD"
    ;;
esac
