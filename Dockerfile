# This Dockerfile assembles an image with all the dependencies required to run
# speculos from the command-line (--display headless or --display console, no
# GUI).
#

# Building the Speculos environment
FROM ghcr.io/ledgerhq/speculos-builder:latest AS builder-app

ADD . /speculos
WORKDIR /speculos/

RUN cmake -Bbuild -H. -DPRECOMPILED_DEPENDENCIES_DIR=/install -DWITH_VNC=1
RUN make -C build

FROM docker.io/library/python:3.10-slim AS builder-pyqt5

RUN apt-get update && apt-get install -qy --no-install-recommends \
    qt5-qmake \
    build-essential \
    qtbase5-dev

RUN python -m pip install \
    --no-cache-dir \
    --config-settings \
    --confirm-license= \
    --verbose \
    pyqt5

# Preparing final image
FROM docker.io/library/python:3.10-slim

ADD . /speculos
WORKDIR /speculos

# Copying artifacts from previous build
COPY --from=builder-app /speculos/speculos/resources/ /speculos/speculos/resources/
COPY --from=builder-pyqt5 /usr/local/lib/python3.10/site-packages/ /usr/local/lib/python3.10/site-packages/

RUN pip install --upgrade pip pipenv
RUN pipenv install --deploy --system

RUN apt-get update && apt-get install -qy \
    qemu-user-static \
    libvncserver-dev \
    gdb-multiarch \
    binutils-arm-none-eabi \
    libqt5widgets5 \
    libqt5gui5 \
    libqt5core5a

RUN apt-get clean && rm -rf /var/lib/apt/lists/

# default port for dev env
EXPOSE 1234
EXPOSE 1236
EXPOSE 9999
EXPOSE 40000
EXPOSE 41000
EXPOSE 42000

ENTRYPOINT [ "python", "./speculos.py" ]
