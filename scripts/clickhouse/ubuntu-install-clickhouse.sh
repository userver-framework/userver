#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

CLICKHOUSE_VERSION=${CLICKHOUSE_VERSION:=v2.5.1}

# Installing Clickhouse C++ client libraries from sources
git clone --depth 1 -b ${CLICKHOUSE_VERSION} https://github.com/ClickHouse/clickhouse-cpp.git
(cd clickhouse-cpp && mkdir build && cd build && \
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBUILD_SHARED_LIBS=ON .. && make -j $(nproc) && make install)
rm -rf clickhouse-cpp/
