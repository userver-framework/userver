#!/bin/bash

set -euo pipefail

readonly YDB_CPP_SDK_VERSION="${YDB_CPP_SDK_VERSION:-3.21.0}"
readonly YDB_CPP_SDK_RELEASE_URL="https://github.com/ydb-platform/ydb-cpp-sdk/releases/download/v${YDB_CPP_SDK_VERSION}"
YDB_CPP_SDK_ARCHITECTURE="$(dpkg --print-architecture)"
readonly YDB_CPP_SDK_ARCHITECTURE
YDB_CPP_SDK_DEB_DIR="$(mktemp -d)"
readonly YDB_CPP_SDK_DEB_DIR

cleanup() {
    rm -rf "${YDB_CPP_SDK_DEB_DIR}"
}
trap cleanup EXIT

download_deb() {
    local filename="$1"
    curl \
        --fail \
        --location \
        --retry 5 \
        --retry-all-errors \
        --output "${YDB_CPP_SDK_DEB_DIR}/${filename}" \
        "${YDB_CPP_SDK_RELEASE_URL}/${filename}"
}

download_deb "yandex-googleapis-api-common-protos-1.0.0-Linux.deb"
download_deb "libydb-cpp-dev_${YDB_CPP_SDK_VERSION}_${YDB_CPP_SDK_ARCHITECTURE}.deb"
download_deb "libydb-cpp-iam-dev_${YDB_CPP_SDK_VERSION}_${YDB_CPP_SDK_ARCHITECTURE}.deb"

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends "${YDB_CPP_SDK_DEB_DIR}"/*.deb
