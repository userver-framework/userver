#!/bin/bash

###
### Provide additional CMake configuration options or override the
### existing ones via BUILD_OPTIONS and PACKAGE_OPTIONS variables.
###

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

# Helper from https://stackoverflow.com/questions/7449772/how-to-retry-a-command-in-bash
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

for BUILD_TYPE in Debug Release; do
  BUILD_DIR=build_${BUILD_TYPE,,}  # ',,' to lowercase the value

  BUILD_TYPE_SANITIZERS=''
  if [ "$BUILD_TYPE" == "Debug" ]; then
    BUILD_TYPE_SANITIZERS='ub addr'
  fi

  # Retry in case of network errors and problems with CPM
  retry ${CONFIGURE_RETRIES:-1} cmake -S./ -B ${BUILD_DIR} \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DUSERVER_INSTALL=ON \
      -DUSERVER_SANITIZE="${BUILD_TYPE_SANITIZERS}" \
      ${BUILD_OPTIONS:-""} \
      -GNinja
  cmake --build ${BUILD_DIR} -- -j$(nproc)
done

cpack -G DEB --config build_release/CPackConfig.cmake -D CPACK_INSTALL_CMAKE_PROJECTS="build_debug;userver;ALL;/;build_release;userver;ALL;/" ${PACKAGE_OPTIONS:-""}
DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends ./libuserver-all-dev*.deb

rm -rf ./build_debug/ ./build_release/

ccache --clear
