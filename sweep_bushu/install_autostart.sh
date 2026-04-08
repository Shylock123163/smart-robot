#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SERVICE_NAME="sweep-bushu.service"
SERVICE_PATH="/etc/systemd/system/${SERVICE_NAME}"
PYTHON_BIN="$(command -v python3)"

if [ -z "$PYTHON_BIN" ]; then
  echo "python3 not found"
  exit 1
fi

sudo tee "$SERVICE_PATH" >/dev/null <<EOF
[Unit]
Description=Sweep Bushu Clutter Detection Web Service
After=network.target

[Service]
Type=simple
User=$(whoami)
WorkingDirectory=${SCRIPT_DIR}
ExecStart=${PYTHON_BIN} ${SCRIPT_DIR}/deploy_web_detect.py --config ${SCRIPT_DIR}/config.yaml
Restart=always
RestartSec=2
StandardOutput=append:${SCRIPT_DIR}/logs/service.log
StandardError=append:${SCRIPT_DIR}/logs/service.log

[Install]
WantedBy=multi-user.target
EOF

mkdir -p "${SCRIPT_DIR}/logs"
sudo systemctl daemon-reload
sudo systemctl enable "${SERVICE_NAME}"
sudo systemctl restart "${SERVICE_NAME}"
sudo systemctl status "${SERVICE_NAME}" --no-pager
