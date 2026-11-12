#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

BASE64_VERSION=${BASE64_VERSION:=0.5.2}
BASE64_SHA256=${BASE64_SHA256:=723a0f9f4cf44cf79e97bcc315ec8f85e52eb104c8882942c3f2fba95acc080d}

# Install aklomp/base64
mkdir -p /tmp/base64
curl -fsSL -o /tmp/base64/base64.tar.gz https://github.com/aklomp/base64/archive/v${BASE64_VERSION}.tar.gz
echo "${BASE64_SHA256}  /tmp/base64/base64.tar.gz" | sha256sum -c -
tar -xzf /tmp/base64/base64.tar.gz -C /tmp/base64 --strip-components=1
(cd /tmp/base64 && mkdir build && cd build && \
  cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBASE64_BUILD_CLI=OFF \
    -DBASE64_BUILD_TESTS=OFF \
    -DBASE64_WITH_OpenMP=OFF \
    .. && \
  make -j $(nproc) && make install)
rm -rf /tmp/base64
