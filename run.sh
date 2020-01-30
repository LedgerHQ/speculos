#!/usr/bin/env bash
set -euxo pipefail

source $(pipenv --venv)/bin/activate
websockify 50000 localhost:40000 --cert ./certificate/ws.pem &  # for apdu
websockify 60000 localhost:41000 --cert ./certificate/ws.pem & # for vnc

exec ./speculos.py ${DEBUG_MODE:-} \
    -m ${DEVICE_MODEL} \
    ./apps/${APP_FILE} \
    --sdk ${SDK_VERSION} \
    --seed "${DEVICE_SEED}" \
    ${EXTRA_OPTIONS:-} \
    --display headless \
    --vnc-port 41000 \
    --apdu-port 40000
