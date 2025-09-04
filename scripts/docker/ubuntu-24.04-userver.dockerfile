FROM ghcr.io/userver-framework/ubuntu-24.04-userver-base:latest

# The Dockerfile contains:
#  * all userver build dependencies
#  * all userver test dependencies (e.g. for testsuite)
#  * built userver itself

RUN apt install -y pycodestyle sudo

RUN git clone https://github.com/userver-framework/userver \
  && cd userver \
  && BUILD_OPTIONS="-DUSERVER_FEATURE_GRPC_CHANNELZ=1" ./scripts/build_and_install_all.sh \
  && cd .. \
  && rm -rf userver/
