## Building Docker container for userver

The docker files for building images on top of different operating systems
are available at scripts/docker. To build the image run the `docker build`
command from the root of the project. For example:

* to build a image with all the build dependencies installed:
```
docker build -t ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest -f scripts/docker/base-ubuntu-22.04.dockerfile .
```

* to build a image with all the build dependencies installed; with current userver built and installed:
```
docker build -t ghcr.io/userver-framework/ubuntu-22.04-userver:latest -f scripts/docker/ubuntu-22.04.dockerfile .
```

* to build a image with all the build dependencies installed; with PostgreSQL database; with current userver built and installed:
```
docker build -t ghcr.io/userver-framework/ubuntu-22.04-userver-pg:latest -f scripts/docker/ubuntu-22.04-pg.dockerfile .
```

* to build image with all build dependencies and additional set of compilers, database servers and workarounds for CI:
```
docker build -t ghcr.io/userver-framework/ubuntu-22.04-userver-base-ci:latest -f scripts/docker/base-ubuntu-22.04-ci.dockerfile .
```

Tests in the above images require IPv6 support. Follow the
https://docs.docker.com/config/daemon/ipv6/ instructions to enable IPv6 in
docker container, for example:
```
docker run --rm -it --network ip6net --entrypoint bash ghcr.io/userver-framework/ubuntu-22.04-userver-base-ci:latest
```
