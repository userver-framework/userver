FROM ghcr.io/userver-framework/ubuntu-24.04-userver:latest

RUN \
  apt update && DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
    postgresql-16 \
    pycodestyle \
    && \
  apt clean all

EXPOSE 8080-8100
