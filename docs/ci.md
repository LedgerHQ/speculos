## Speculos builder

The Dockerfile `build.Dockerfile` builds a container with all required
dependencies to build speculos:

```shell
docker build -f build.Dockerfile -t ledgerhq/speculos-builder .
```

The resulting container is pushed on
[Docker Hub](https://hub.docker.com/r/ledgerhq/speculos-builder) (by Ledger
employees, obviously):

```shell
docker push ledgerhq/speculos-builder:latest
docker image tag ledgerhq/speculos-builder:latest ledgerhq/speculos-builder:$(git rev-parse --short HEAD)
docker push ledgerhq/speculos-builder:$(git rev-parse --short HEAD)
```

This container can eventually be used by the CI.


## Speculos

The Dockerfile `Dockerfile` builds a container with all required dependencies to
run speculos from the command-line:

```shell
docker build -f Dockerfile -t ledgerhq/speculos .
```

This should be done by the CI, which publish the resulting image on
[Docker Hub](https://hub.docker.com/r/ledgerhq/speculos).
