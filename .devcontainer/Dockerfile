FROM ghcr.io/userver-framework/ubuntu-24.04-userver-base:latest

# Setup clang toolchain, add "user" user
COPY scripts/docker/setup-dev.sh /userver_tmp/
RUN /userver_tmp/setup-dev.sh && rm -rf /userver_tmp

RUN \
  apt update && DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
    pycodestyle \
    graphviz \
    wget \
    && \
  apt clean all

# Build and install userver
COPY ./ /app/userver

EXPOSE 8080-8100
