#!/usr/bin/env bash
# Cierra TODAS las instancias de la GUI de Frama-C / Ivette.
pkill -f 'ivette' 2>/dev/null
pkill -f 'frama-c-gui' 2>/dev/null
pkill -f 'server-socket /tmp/ivette' 2>/dev/null
sleep 2
if pgrep -f 'frama-c-gui|ivette' >/dev/null 2>&1; then
  echo "quedan procesos, forzando..."
  pkill -9 -f 'ivette' 2>/dev/null
  pkill -9 -f 'frama-c-gui' 2>/dev/null
  sleep 1
fi
pgrep -af 'frama-c-gui|ivette' | grep -v pgrep | head && echo "AUN VIVOS" || echo "todo cerrado"
