FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest

COPY ./ /app/userver

# Avoiding postgresql-server-dev dependency saves ~1.5GB
RUN ( \
  cd /app/userver \
  && PACKAGE_OPTIONS='-D CPACK_DEBIAN_PACKAGE_DEPENDS=libc6' ./scripts/build_and_install_all.sh \
  && rm -rf /app/userver \
)

EXPOSE 8080-8100
