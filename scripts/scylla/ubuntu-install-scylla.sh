#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

SCYLLA_CPP_DRIVER_VERSION=${SCYLLA_CPP_DRIVER_VERSION:=1.1.0}
RELEASE_URL="https://github.com/scylladb/cpp-rs-driver/releases/download/v${SCYLLA_CPP_DRIVER_VERSION}"

apt update
apt install -y --no-install-recommends ca-certificates curl

# Prebuilt packages of the ScyllaDB cpp-rs-driver, which provides the
# Cassandra compatible C++ API
curl -fsSLO "${RELEASE_URL}/scylla_cpp_driver_${SCYLLA_CPP_DRIVER_VERSION}_amd64.deb"
curl -fsSLO "${RELEASE_URL}/scylla_cpp_driver-dev_${SCYLLA_CPP_DRIVER_VERSION}_amd64.deb"
apt install -y --no-install-recommends ./scylla_cpp_driver*_amd64.deb
rm -f ./scylla_cpp_driver*.deb

echo /usr/lib64 > /etc/ld.so.conf.d/scylla-cpp-driver.conf
ln -sf /usr/lib64/libscylladb.so /usr/lib/x86_64-linux-gnu/libscylla-cpp-driver.so
ldconfig
