FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest

# Setup clang toolchain, add "user" user
COPY scripts/docker/setup-dev.sh /userver_tmp/
RUN /userver_tmp/setup-dev.sh && rm -rf /userver_tmp

# Install postgresql server
RUN \
  apt update && DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
    postgresql-14 \
    pycodestyle \
    && \
  apt clean all

# Build and install userver
COPY ./ /app/userver

RUN ( \
  cd /app/userver \
  && PACKAGE_OPTIONS='-D CPACK_DEBIAN_PACKAGE_DEPENDS=libc6' ./scripts/build_and_install_all.sh \
  && rm -rf /app/userver \
)

EXPOSE 8080-8100
