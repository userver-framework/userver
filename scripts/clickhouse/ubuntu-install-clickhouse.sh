#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

install_clickhouse_client() {
  CLICKHOUSE_VERSION=${CLICKHOUSE_VERSION:=v2.5.1}

  # Installing Clickhouse C++ client libraries from sources
  git clone --depth 1 -b ${CLICKHOUSE_VERSION} https://github.com/ClickHouse/clickhouse-cpp.git
  (cd clickhouse-cpp && mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBUILD_SHARED_LIBS=ON .. && make -j $(nproc) && make install)
  rm -rf clickhouse-cpp/
}

install_clickhouse_server() {
  CLICKHOUSE_BINARY_URL=${CLICKHOUSE_BINARY_URL:=https://builds.clickhouse.com/master/amd64compat/clickhouse}

  getent group clickhouse >/dev/null || groupadd --system clickhouse
  id clickhouse >/dev/null 2>&1 || useradd --system --gid clickhouse \
    --home-dir /var/lib/clickhouse --shell /bin/bash clickhouse

  curl --fail --location --retry 5 --show-error --silent \
    "${CLICKHOUSE_BINARY_URL}" --output /tmp/clickhouse
  chmod 0755 /tmp/clickhouse
  /tmp/clickhouse install --user clickhouse --group clickhouse
  rm /tmp/clickhouse

  test -r /etc/clickhouse-server/config.xml
  test -r /etc/clickhouse-server/users.xml
  clickhouse local --query "SELECT 1"
}

case "${1:-client}" in
  client)
    install_clickhouse_client
    ;;
  server)
    install_clickhouse_server
    ;;
  *)
    echo "Usage: $0 [client|server]" >&2
    exit 2
    ;;
esac
