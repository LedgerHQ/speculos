# This Dockerfile assembles an image with all the dependencies required to run
# speculos from the command-line (--display headless or --display console, no
# GUI).
#

# Building the Speculos environment
ARG BUILDER_IMAGE=ghcr.io/ledgerhq/speculos-builder:latest
FROM ${BUILDER_IMAGE} AS builder

ADD . /speculos
WORKDIR /speculos/

RUN cmake -Bbuild -H. -DPRECOMPILED_DEPENDENCIES_DIR=/install -DWITH_VNC=1
RUN make -C build


# Preparing final image
FROM docker.io/library/python:3.10-slim

ADD . /speculos
WORKDIR /speculos

# Copying artifacts from previous build
COPY --from=builder /speculos/speculos/resources/ /speculos/speculos/resources/

# Install only the runtime deps from pyproject.toml, without building speculos
# itself: pip install . would trigger a full cmake rebuild of OpenSSL and other
# native dependencies that belong in the builder stage, not here.
RUN pip install --upgrade pip && \
    pip install tomli && \
    python3 -c "import tomli,subprocess,sys; d=tomli.load(open('pyproject.toml','rb')); subprocess.run([sys.executable,'-m','pip','install']+d['project']['dependencies'],check=True)" && \
    pip uninstall -y tomli

RUN apt-get update && \
    apt-get install -qy \
        qemu-user-static \
        libvncserver-dev \
        gdb-multiarch \
        binutils-arm-none-eabi && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/

# default port for dev env
EXPOSE 1234
EXPOSE 1236
EXPOSE 9999
EXPOSE 40000
EXPOSE 41000
EXPOSE 42000

ENTRYPOINT [ "python", "./speculos.py" ]
