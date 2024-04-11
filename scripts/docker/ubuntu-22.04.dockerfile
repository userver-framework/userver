FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest

COPY ./ /app/userver

RUN ( \
  cd /app/userver \
  && ./scripts/build_and_install_all.sh \
  && rm -rf /app/userver \
)

EXPOSE 8080-8100
