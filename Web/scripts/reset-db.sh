#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
echo "This deletes the local PostgreSQL Docker volume for SatCache."
read -rp "Type RESET to continue: " answer
if [[ "$answer" == "RESET" ]]; then
  docker compose down -v
else
  echo "Cancelled."
fi
