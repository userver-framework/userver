FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest

COPY scripts/docker/install-additional-compilers-ubuntu-22.04.sh /userver_tmp/
RUN (cd /userver_tmp && ./install-additional-compilers-ubuntu-22.04.sh)
RUN rm -rf /userver_tmp

RUN DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends redis-server postgresql-14

# fix for porto layers
RUN mkdir -p /place/berkanavt/ && DEBIAN_FRONTEND=noninteractive apt install -y fuse dupload libuv1 libuv1-dev
