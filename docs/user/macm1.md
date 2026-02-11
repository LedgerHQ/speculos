# Docker - for Mac M1

## Option A: Use the install script (recommended)

Create a `scripts` folder at the project root.  
In it create `install-speculos-macm1.sh`, from the contents below.

```shell
./scripts/install-speculos-macm1.sh
```

The script **first tries to pull the official arm64 image** (`ghcr.io/ledgerhq/speculos`). That avoids the common `make -C build` failure when building from source on M1. If the pull succeeds, you get a working `speculos` image without building.

If you want to force building from source instead (e.g. to get a specific commit), run:

```shell
FORCE_BUILD=1 ./scripts/install-speculos-macm1.sh
```

### install-speculos-macm1.sh

```shell
#!/bin/bash
# Install Speculos Docker image for Mac M1 (aarch64)

set -e

# Set to 1 to skip pull and force building from source (e.g. after a failed build)
FORCE_BUILD="${FORCE_BUILD:-0}"

echo "==> Pulling official Speculos image for arm64 (avoids build failures on M1)..."
if [ "$FORCE_BUILD" = "1" ]; then
  echo "    FORCE_BUILD=1: skipping pull, will build from source."
else
  if docker pull --platform linux/arm64 ghcr.io/ledgerhq/speculos:latest 2>/dev/null; then
    docker tag ghcr.io/ledgerhq/speculos:latest speculos
    echo ""
    echo "Done. Using official image. Run Speculos (e.g. from a speculos clone with apps/):"
    echo "  docker run --rm -it -v \$(pwd)/apps:/speculos/apps -p 41000:41000 -p 5001:5001 speculos --display headless --vnc-port 41000 --api-port 5001 apps/btc.elf"
    echo ""
    exit 0
  fi
  echo "    Pull failed or image not found; will build from source."
fi

SPECULOS_DIR="${SPECULOS_DIR:-$HOME/speculos}"
# Resolve to absolute path so Docker always gets a valid context
SPECULOS_DIR="$(cd "$(dirname "$SPECULOS_DIR")" && pwd)/$(basename "$SPECULOS_DIR")"

echo "==> Cloning Speculos into $SPECULOS_DIR"
if [ ! -d "$SPECULOS_DIR" ]; then
  git clone https://github.com/LedgerHQ/speculos.git "$SPECULOS_DIR"
else
  echo "    Directory exists, pulling latest..."
  (cd "$SPECULOS_DIR" && git pull)
fi

# Dockerfile lives in Dockerfiles/ subdirectory (not repo root)
DOCKERFILE="$SPECULOS_DIR/Dockerfiles/Dockerfile"
if [ ! -f "$DOCKERFILE" ]; then
  echo "Error: Dockerfile not found at $DOCKERFILE"
  exit 1
fi

echo "==> Patching Dockerfile for Mac M1 (aarch64)"
# Replace default builder with aarch64 builder for M1
sed -i.bak 's|FROM ghcr.io/ledgerhq/speculos-builder:latest AS builder|FROM ghcr.io/ledgerhq/speculos-builder-aarch64:latest AS builder|' "$DOCKERFILE"
# Ensure final stage uses arm64 (avoids wrong arch when building with -f)
sed -i.bak 's|^FROM docker.io/library/python:3.10-slim|FROM --platform=linux/arm64 docker.io/library/python:3.10-slim|' "$DOCKERFILE"
rm -f "${DOCKERFILE}.bak"

echo "==> Building Docker image (--platform linux/arm64; use FORCE_BUILD=1 to skip pull next time)"
# Explicit platform and plain progress so make errors are visible if build fails
docker build --platform linux/arm64 --progress=plain -f "$DOCKERFILE" "$SPECULOS_DIR" -t speculos

echo ""
echo "Done. Run Speculos from the speculos repo root:"
echo "  cd $SPECULOS_DIR"
echo "  docker run --rm -it -v \$(pwd)/apps:/speculos/apps -p 41000:41000 -p 5001:5001 speculos --display headless --vnc-port 41000 --api-port 5001 apps/btc.elf"
echo ""
echo "See docs/SPECULOS_MAC_M1_SETUP.md for more options."
```
---

## Option B: Manual setup

### 1. Pull the official image (no build)

On M1, the official image is multi-arch; Docker will pull the arm64 variant:

```shell
docker pull --platform linux/arm64 ghcr.io/ledgerhq/speculos:latest
docker tag ghcr.io/ledgerhq/speculos:latest speculos
```

Then run Speculos from a directory that has an `apps/` folder (e.g. a speculos clone):

```shell
docker run --rm -it -v $(pwd)/apps:/speculos/apps -p 41000:41000 -p 5001:5001 speculos --display headless --vnc-port 41000 --api-port 5001 apps/btc.elf
```

### 2. Or clone and build from source

**Clone Speculos** from a directory of your choice (e.g. your home or projects folder):

```shell
git clone https://github.com/LedgerHQ/speculos.git
cd speculos
```

**Use the M1/AArch64 builder image:** edit the **Dockerfile** (in `Dockerfiles/Dockerfile` if present, otherwise repo root) and change **line 1** from:

```dockerfile
FROM ghcr.io/ledgerhq/speculos-builder:latest AS builder
```

to:

```dockerfile
FROM ghcr.io/ledgerhq/speculos-builder-aarch64:latest AS builder
```

So the image matches your Mac M1 (ARM64) architecture.

**Build the Docker image** from the **root of the speculos repo**:

```shell
docker build ./ -t speculos
```

When it finishes, you should see the image:

```shell
docker image ls
# REPOSITORY   TAG     IMAGE ID       CREATED        SIZE
# speculos     latest  ...             ...            593MB
```

**Run Speculos** from the **root of the speculos project**:

```shell
docker run --rm -it -v $(pwd)/apps:/speculos/apps --publish 41000:41000 --publish 5001:5001 speculos --display headless --vnc-port 41000 --api-port 5001 apps/btc.elf
```

- **VNC:** `localhost:41000`
- **API:** `localhost:5001`
- The `apps/` folder is mounted via `-v` so you can use your own `.elf` apps.

### Arguments and advanced usage

All arguments supported by `speculos.py` can be passed on the Docker command line. Publish container ports with `-p` when required.

- **Interactive shell (debug):**  
  `docker run --rm -it -v $(pwd)/apps:/speculos/apps -p 1234:1234 -p 5000:5000 -p 40000:40000 -p 41000:41000 --entrypoint /bin/bash speculos`

- **Custom model, APDU, VNC, SDK, seed:**  
  `docker run --rm -it -v "$(pwd)"/apps:/speculos/apps -p 1234:1234 -p 5000:5000 -p 40000:40000 -p 41000:41000 speculos --model nanos ./apps/btc.elf --sdk 2.0 --seed "secret" --display headless --apdu-port 40000 --vnc-port 41000`

- **docker-compose:**  
  `docker-compose up [-d]` — edit `docker-compose.yml` for port forwarding and environment variables. Default configuration is nanos / 2.0 / btc.elf / seed "secret".

## Requirements

- **Docker** installed and running (Docker Desktop for Mac with Apple Silicon).
- **Git** to clone the repo.

## Troubleshooting

- **`make -C build` did not complete successfully (exit code 2)**  
  Building from source on M1 often hits this. Use **Option A** or **Option B (pull)** so you use the official arm64 image instead of building. If you must build, run the script with `FORCE_BUILD=1`; the build uses `--platform linux/arm64` and `--progress=plain` so you can see the real compile error.

