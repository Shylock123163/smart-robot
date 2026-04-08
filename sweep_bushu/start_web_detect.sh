#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

mkdir -p "$SCRIPT_DIR/logs"

exec python3 "$SCRIPT_DIR/deploy_web_detect.py" --config "$SCRIPT_DIR/config.yaml"
