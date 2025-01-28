FROM ghcr.io/userver-framework/ubuntu-24.04-userver-base:latest

# The Dockerfile contains:
#  * all userver build dependencies
#  * all userver test dependencies (e.g. for testsuite)
#  * built userver itself

RUN git clone https://github.com/userver-framework/userver \
  && cd userver \
  && ./scripts/build_and_install_all.sh \
  && cd .. \
  && rm -rf userver/
