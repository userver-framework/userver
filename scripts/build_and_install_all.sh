#!/bin/bash

###
### Provide additional CMake configuration options or override the
### existing ones via BUILD_OPTIONS and PACKAGE_OPTIONS variables.
###

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

BUILD_OPTIONS="
    -DUSERVER_BUILD_ALL_COMPONENTS=1 \
    -DUSERVER_FEATURE_YDB=0 \
    -DUSERVER_FEATURE_GRPC_CHANNELZ=0 \
    ${BUILD_OPTIONS:-""} \
"
CONFIGURE_RETRIES="${CONFIGURE_RETRIES:-5}"

"$(dirname "$(realpath "$0")")/build_and_install.sh"
