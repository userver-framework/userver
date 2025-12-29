#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

GRPC_VERSION=${GRPC_VERSION:=v1.54.3}

sudo apt purge -y libabsl-dev libprotobuf-dev libprotoc-dev protobuf-compiler libgrpc++-dev libgrpc++1 libgrpc-dev protobuf-compiler-grpc

mkdir -p /tmp/grpc
git clone --recurse-submodules -b ${GRPC_VERSION} --depth 1 --shallow-submodules https://github.com/grpc/grpc /tmp/grpc
(cd /tmp/grpc && mkdir build && cd build && \
  cmake \
    -DgRPC_INSTALL=ON \
    -DgRPC_BUILD_TESTS=OFF \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DgRPC_CARES_PROVIDER=package \
    -DgRPC_RE2_PROVIDER=package \
    -DgRPC_SSL_PROVIDER=package \
    -DgRPC_ZLIB_PROVIDER=package \
    .. && \
  make -j $(nproc) && make install)
rm -rf /tmp/grpc
