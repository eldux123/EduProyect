#!/usr/bin/env bash
# EduChain - verification/setup_gui.sh
# Instala node 24 + yarn via nvm (sin sudo) para poder usar frama-c-gui (Ivette).
# Idempotente: si ya estan, no reinstala.
set -e

export NVM_DIR="$HOME/.nvm"

if [ ! -s "$NVM_DIR/nvm.sh" ]; then
  echo "=== Instalando nvm ==="
  curl -fsSL -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash
fi

# Cargar nvm en esta shell.
. "$NVM_DIR/nvm.sh"

if ! nvm ls 24 >/dev/null 2>&1; then
  echo "=== Instalando node 24 ==="
  nvm install 24
fi
nvm use 24

if ! command -v yarn >/dev/null 2>&1; then
  echo "=== Instalando yarn ==="
  npm install --global yarn
fi

echo "=== Versiones ==="
echo "node: $(node --version)"
echo "npm:  $(npm --version)"
echo "yarn: $(yarn --version 2>/dev/null || echo '?')"
echo
echo "Listo. Para lanzar la GUI:"
echo "  export NVM_DIR=\"\$HOME/.nvm\"; . \"\$NVM_DIR/nvm.sh\"; nvm use 24"
echo "  eval \"\$(opam env)\""
echo "  bash verification/gui.sh credentials"
