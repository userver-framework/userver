FROM ghcr.io/userver-framework/ubuntu-24.04-userver-base:latest

# The Dockerfile contains:
#  * all userver build dependencies
#  * all userver test dependencies (e.g. for testsuite)
#  * built userver itself

RUN apt install -y clang-18
RUN git clone https://github.com/userver-framework/userver \
  && cd userver \
  && BUILD_OPTIONS='-DUSERVER_FEATURE_CLICKHOUSE=OFF -DCMAKE_CXX_COMPILER=clang++-18 -DCMAKE_C_COMPILER=clang-18' \
     ./scripts/build_and_install_all.sh \
  && cd .. \
  && rm -rf userver/
