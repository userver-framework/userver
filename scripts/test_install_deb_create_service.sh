#!/usr/bin/bash

set -euo pipefail

DISTRO=${DISTRO:-ubuntu-22.04}
case "$DISTRO" in
    ubuntu-22.04 | ubuntu-24.04 | ubuntu-26.04) ;;
    *)
        echo "ERROR: unsupported DISTRO '$DISTRO'." >&2
        echo "Supported: ubuntu-22.04, ubuntu-24.04, and ubuntu-26.04." >&2
        exit 1
        ;;
esac

apt update

apt install -y \
    dpkg-dev \
    debhelper-compat \
    dh-cmake \
    dh-cmake-compat \
    dh-sequence-ctest \
    dh-sequence-cpack \
    postgresql \
    $(cat "scripts/docs/en/deps/$DISTRO.md")

PACKAGE_VERSION="$(cat version.txt)"
PACKAGE_VERSION="${PACKAGE_VERSION//-/\~}~$(date +%Y%m%d%H%M)"
PACKAGE_ARCH="$(dpkg-architecture -qDEB_HOST_ARCH)"
PACKAGE_MULTIARCH="$(dpkg-architecture -qDEB_HOST_MULTIARCH)"

# Step 1: generate debian/ directory
make gen-debian-directory \
    DISTRO="$DISTRO" \
    VERSION="$PACKAGE_VERSION"

# Step 2: build binary Debian packages
# dpkg-buildpackage writes .deb files to the parent directory
dpkg-buildpackage -us -uc -b

# Step 3: install the four component packages from parent directory
# $VAR (not ${VAR}) is required here: a.yaml treats ${...} as a
# CI/JMESPath expression, so a plain ${PACKAGE_VERSION} fails with
# "No value resolved for expression". $ escapes to a literal $ for bash.
CHAOTIC_PACKAGE="../libuserver-chaotic-dev_${PACKAGE_VERSION}_${PACKAGE_ARCH}.deb"
case "$DISTRO" in
    ubuntu-22.04 | ubuntu-24.04) REQUIRED_DEPENDENCIES="libgcc-s1 libc6" ;;
    ubuntu-26.04) REQUIRED_DEPENDENCIES="python3-pydantic python3-yaml" ;;
esac
for REQUIRED_DEPENDENCY in $REQUIRED_DEPENDENCIES; do
    if ! dpkg-deb --field "$CHAOTIC_PACKAGE" Depends |
        grep -Eq "(^|, )${REQUIRED_DEPENDENCY}([[:space:](,]|$)"; then
        echo "ERROR: libuserver-chaotic-dev does not depend on ${REQUIRED_DEPENDENCY}." >&2
        exit 1
    fi
done

if [ "$DISTRO" = ubuntu-26.04 ] &&
    dpkg-deb --contents "$CHAOTIC_PACKAGE" |
        grep -E '/userver/(pydantic|pydantic_core|yaml)/' >/dev/null; then
    echo "ERROR: libuserver-chaotic-dev contains vendored Python modules on ubuntu-26.04." >&2
    exit 1
fi

dpkg -i \
    "../libuserver-universal-dev_${PACKAGE_VERSION}_${PACKAGE_ARCH}.deb" \
    "../libuserver-core-dev_${PACKAGE_VERSION}_${PACKAGE_ARCH}.deb" \
    "../libuserver-postgresql-dev_${PACKAGE_VERSION}_${PACKAGE_ARCH}.deb" \
    "$CHAOTIC_PACKAGE"

PYTHONPATH="/usr/lib/${PACKAGE_MULTIARCH}/userver" python3 -c \
    'import pydantic; import pydantic_core; import yaml'

# Step 4: create a temporary service with userver-create-service
TEST_USER="userver-test"
TEST_HOME="/home/$TEST_USER"
useradd --system --user-group --create-home --home-dir "$TEST_HOME" \
    --shell /usr/sbin/nologin "$TEST_USER"

run_as_test_user() {
    runuser --user "$TEST_USER" -- env HOME="$TEST_HOME" "$@"
}

TEMP_DIR="$(run_as_test_user mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT
SERVICE_DIR="$TEMP_DIR/service"

run_as_test_user userver-create-service --postgresql "$SERVICE_DIR"

# Step 5: build and test the generated service
run_as_test_user cmake \
    -B "$SERVICE_DIR/build" \
    -S "$SERVICE_DIR" \
    -DCMAKE_BUILD_TYPE=Release
run_as_test_user cmake --build "$SERVICE_DIR/build" -j "$(nproc)"
run_as_test_user ctest --test-dir "$SERVICE_DIR/build" -V
