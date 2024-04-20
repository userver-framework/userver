FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

RUN \
  DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
    postgresql-14 \
    pycodestyle \
    && \
  apt clean all

EXPOSE 8080-8100
