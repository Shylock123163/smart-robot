#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

python3 lubancat_web_detect.py --weights models/best.pt --camera 0 --port 5005 --device cpu
