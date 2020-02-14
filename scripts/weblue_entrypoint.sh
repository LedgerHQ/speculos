#!/usr/bin/env bash
set -euxo pipefail

APDU_PORT=40000
VNC_PORT=41000

websockify 50000 localhost:${APDU_PORT} --cert ./certificate/ws.pem &  # for apdu
websockify 60000 localhost:${VNC_PORT} --cert ./certificate/ws.pem &  # for vnc
exec ./speculos.py ${DEBUG_MODE:-} -m ${DEVICE_MODEL} --sdk ${SDK_VERSION} --seed "${DEVICE_SEED}" --rampage ${RAMPAGE} --display headless --vnc-port ${VNC_PORT} --apdu-port ${APDU_PORT} ./apps/${APP_FILE}
