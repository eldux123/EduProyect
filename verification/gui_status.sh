#!/usr/bin/env bash
echo "=== procesos de la GUI vivos ==="
pgrep -af 'frama-c-gui|ivette|electron' 2>/dev/null | grep -v pgrep | head -20
echo "=== fin ==="
