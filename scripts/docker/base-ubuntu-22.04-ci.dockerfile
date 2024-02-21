FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest

# Fix for porto layers and install a bunch of compilers and databases
RUN mkdir -p /place/berkanavt/ && \
  DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
    fuse dupload libuv1 libuv1-dev \
    redis-server \
    postgresql-14 \
    clang-16 \
    lld-16 \
    clang-14 \
    lld-14 \
    g++-11 \
    gcc-11 \
    g++-13 \
    gcc-13
