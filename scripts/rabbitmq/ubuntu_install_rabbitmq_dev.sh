#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

AMQP_VERSION=${AMQP_VERSION:=v4.3.18}

# Installing amqp/rabbitmq client libraries from sources
git clone --depth 1 -b ${AMQP_VERSION} https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git amqp-cpp
(cd amqp-cpp && mkdir build && cd build && \
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 .. && make -j $(nproc) && make install)
rm -rf amqp-cpp/
