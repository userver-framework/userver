#!/bin/sh

# Run the create-service tests against an *installed* userver.
#
# Usage:
#   INSTALL_ROOT=<userver-install-dir> [CMAKE_OPTS=<cmake-flags>] test.sh
#
# INSTALL_ROOT is the prefix userver was installed into (the one that
# contains bin/userver-create-service and lib*/cmake/userver/). Extra CMake
# configuration flags (compilers, C++ standard, sanitizers, ...) may be passed
# via the CMAKE_OPTS environment variable; they are forwarded to every test.
#
# The script tests the base (no database) service template plus every optional
# feature (mongo, postgresql, grpc) that is actually present in the install.

set -e

INSTALL_DIR=${INSTALL_ROOT}
if [ -z "${INSTALL_DIR}" ]; then
    echo "Usage: INSTALL_ROOT=<userver-install-dir> $0" >&2
    exit 1
fi

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

# A feature is considered installed if its CMake config file is present, i.e.
# `userver-<feature>-config.cmake` lives somewhere under the install prefix's
# `cmake/userver` directory (the location is multiarch-dependent, so we search).
feature_installed() {
    feature=$1
    find "${INSTALL_DIR}" -type f -name "userver-${feature}-config.cmake" 2>/dev/null | grep -q .
}

# The base test (no optional features) is always run.
echo "=== create-service test: base ==="
"${SCRIPT_DIR}/test_helper.sh" "${INSTALL_DIR}"

for FEATURE in mongo postgresql grpc; do
    if feature_installed "${FEATURE}"; then
        echo "=== create-service test: --${FEATURE} ==="
        "${SCRIPT_DIR}/test_helper.sh" "${INSTALL_DIR}" "--${FEATURE}"
    else
        echo "=== create-service test: --${FEATURE} skipped (not installed) ==="
    fi
done
