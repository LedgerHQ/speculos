---
sort: 3
---

# Continuous Integration

## Speculos builder

The Dockerfile `build.Dockerfile` builds a container with all required
dependencies to build speculos:

```shell
docker build -f build.Dockerfile -t ghcr.io/ledgerhq/speculos-builder .
```

The resulting container is pushed on
[GitHub Packages](https://ghcr.io/ledgerhq/speculos-builder) by the CI and
can eventually be used by the CI itself.

The image can also be pushed manually with appropriate credentials:

```shell
docker push ghcr.io/ledgerhq/speculos-builder:latest
docker image tag ghcr.io/ledgerhq/speculos-builder:latest ghcr.io/ledgerhq/speculos-builder:$(git rev-parse --short HEAD)
docker push ghcr.io/ledgerhq/speculos-builder:$(git rev-parse --short HEAD)
```


## Speculos

The Dockerfile `Dockerfile` builds a container with all required dependencies to
run speculos from the command-line:

```shell
docker build -f Dockerfile -t ghcr.io/ledgerhq/speculos .
```

This should be done by the CI, which publish the resulting image on
[GitHub Packages](https://ghcr.io/ledgerhq/speculos).
