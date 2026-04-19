#!/bin/bash
set -euo pipefail

retry() {
    local -r -i max_attempts="$1"; shift
    local -i attempt_num=1
    until "$@"
    do
        if ((attempt_num==max_attempts))
        then
            echo "Attempt $attempt_num failed and there are no more attempts left!"
            return 1
        else
            echo "Attempt $attempt_num failed! Trying again in $attempt_num seconds..."
            sleep $((attempt_num++))
        fi
    done
}

BUILD_DIR=build_debug

if [[ "${1:-}" == "--clean" ]]; then
    rm -rf ${BUILD_DIR}
fi

if [ ! -f ${BUILD_DIR}/CMakeCache.txt ]; then
    retry ${CONFIGURE_RETRIES:-3} cmake -S./ -B ${BUILD_DIR} \
        -DCMAKE_BUILD_TYPE=Debug \
        -DUSERVER_INSTALL=ON \
        -DUSERVER_SANITIZE="ub addr" \
        ${BUILD_OPTIONS:-""} \
        -GNinja
fi

cmake --build ${BUILD_DIR} -- -j$(nproc)

cpack -G DEB --config ${BUILD_DIR}/CPackConfig.cmake \
    -D CPACK_INSTALL_CMAKE_PROJECTS="${BUILD_DIR};userver;ALL;/" \
    ${PACKAGE_OPTIONS:-""}

sudo apt install -y --no-install-recommends ./libuserver-all-dev*.deb