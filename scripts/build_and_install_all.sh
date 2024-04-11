#!/bin/bash

###
### Provide additional CMake configuration options or override the
### existing ones via BUILD_OPTIONS variable.
###

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

ALL_FEATURES="
    -DUSERVER_FEATURE_CLICKHOUSE=1 \
    -DUSERVER_FEATURE_GRPC=1 \
    -DUSERVER_FEATURE_MYSQL=1 \
    -DUSERVER_FEATURE_MONGODB=1 \
    -DUSERVER_FEATURE_POSTGRESQL=1 \
    -DUSERVER_FEATURE_REDIS=1 \
    -DUSERVER_FEATURE_RABBITMQ=1 \
    -DUSERVER_FEATURE_YDB=0 \
"

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
  # Retry at most 5 times in case of network errors and problems with CPM
  retry 5 cmake -S./ -B ${BUILD_DIR} \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DUSERVER_INSTALL=ON \
      -DUSERVER_SANITIZE="ub addr" \
      ${ALL_FEATURES} \
      ${BUILD_OPTIONS:-""} \
      -GNinja
  cmake --build ${BUILD_DIR}
  cmake --install ${BUILD_DIR}
  cmake --build ${BUILD_DIR} --target clean
done

ccache --clear
