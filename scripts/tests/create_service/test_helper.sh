#!/bin/sh

# Create a service from the userver service template using an *installed*
# userver, then build and run its tests.
#
# Usage:
#   test_helper.sh <userver-install-dir> [userver-create-service-args...]
#
# <userver-install-dir> is the prefix userver was installed into (the one that
# contains bin/userver-create-service and lib*/cmake/userver/). Any extra
# CMake configuration flags (compilers, C++ standard, ...) may be passed via
# the CMAKE_OPTS environment variable.

set -e

INSTALL_DIR=$1
shift

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SERVICE_DIR="${SCRIPT_DIR}/new-service"

rm -rf "${SERVICE_DIR}"
"${INSTALL_DIR}/bin/userver-create-service" "$@" "${SERVICE_DIR}"

cd "${SERVICE_DIR}"
# Word splitting of CMAKE_OPTS is intentional here.
# shellcheck disable=SC2086
cmake -B build -S . -DCMAKE_PREFIX_PATH="${INSTALL_DIR}" ${CMAKE_OPTS}
cmake --build build -j "$(nproc)"
(cd build && ctest -V)

cd "${SCRIPT_DIR}"
rm -rf "${SERVICE_DIR}"
