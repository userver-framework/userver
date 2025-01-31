FROM ghcr.io/userver-framework/ubuntu-24.04-userver-base:latest

# The Dockerfile contains:
#  * all userver build dependencies
#  * all userver test dependencies (e.g. for testsuite)
#  * built userver itself

ARG COMPILER=gcc
ENV COMPILER=$COMPILER
COPY scripts/select-compiler.sh /userver_tmp/

RUN if [ "$COMPILER" = clang ]; then \
      apt install -y clang-18 lld-18; \
    fi

RUN git clone https://github.com/userver-framework/userver \
  && cd userver \
  && env $(/userver_tmp/select-compiler.sh) ./scripts/build_and_install_all.sh \
  && cd .. \
  && rm -rf userver/
