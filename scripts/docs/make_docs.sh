#!/bin/sh

# Exit on any error
set -e

cd "$(dirname "$0")/../.."

if [ -z "$BUILD_DIR" ]; then
    echo "!!! Set BUILD_DIR environment variable to cmake build directory."
    echo "!!! See userver/scripts/docs/README.md"
    exit 2
fi

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "!!! Fully configure userver before running this script."
    echo "!!! See userver/scripts/docs/README.md"
    exit 2
fi

download_and_extract_doxygen() {
    if [ ! -d "$BUILD_DIR/doxygen-1.17.0" ]; then
        echo "Doxygen not found in $BUILD_DIR or version is too low. Downloading..."
        wget --no-verbose https://github.com/doxygen/doxygen/releases/download/Release_1_17_0/doxygen-1.17.0.linux.bin.tar.gz -P "$BUILD_DIR"
        tar -xzf "$BUILD_DIR/doxygen-1.17.0.linux.bin.tar.gz" -C "$BUILD_DIR"
        export DOXYGEN="$BUILD_DIR/doxygen-1.17.0/bin/doxygen"
        echo "Doxygen has been successfully downloaded."
    else
        echo "Using already downloaded doxygen in $BUILD_DIR."
        export DOXYGEN="$BUILD_DIR/doxygen-1.17.0/bin/doxygen"
    fi
}

# Download doxygen if ${DOXYGEN} not set explicitly
if [ -z "$DOXYGEN" ]; then
    download_and_extract_doxygen
fi
echo "Doxygen: $DOXYGEN"


# Run userver codegen to avoid doxygen errors with invalid includes.
CMAKE_COMMAND=$(grep -oP 'CMAKE_COMMAND:INTERNAL=\K.*' "$BUILD_DIR/CMakeCache.txt")
CMAKE_VERSION=$("$CMAKE_COMMAND" --version | grep -oP '\d+\.\d+')

echo "Building target userver-codegen."
"$CMAKE_COMMAND" --build "$BUILD_DIR" --target userver-codegen

echo "Building target userver-gen-components-schema-docs."
"$CMAKE_COMMAND" --build "$BUILD_DIR" --target userver-gen-components-schema-docs
rm -rf scripts/docs/en/components_schema
mkdir scripts/docs/en/components_schema
cp -rf $BUILD_DIR/components-schema/* scripts/docs/en/components_schema/

echo "Building target userver-gen-dynamic-configs-docs."
"$CMAKE_COMMAND" --build "$BUILD_DIR" --target userver-gen-dynamic-configs-docs
rm -rf scripts/docs/en/dynamic_configs
mkdir scripts/docs/en/dynamic_configs
cp $BUILD_DIR/docs-dynamic-configs/* scripts/docs/en/dynamic_configs/

# Fetch yandex-taxi-testsuite sources so Doxygen can resolve @ref to its fixtures.
download_yandex_taxi_testsuite() {
    TESTSUITE_CLONE="$BUILD_DIR/yandex-taxi-testsuite"
    TESTSUITE_ARCHIVE="$BUILD_DIR/yandex-taxi-testsuite.tar.gz"

    echo "Downloading yandex-taxi-testsuite (develop) for documentation..."
    wget --no-verbose -O "$TESTSUITE_ARCHIVE" \
        "https://github.com/yandex/yandex-taxi-testsuite/archive/refs/heads/develop.tar.gz"

    rm -rf "$TESTSUITE_CLONE"
    # Remove any previous partial extract dirs, keep the archive.
    find "$BUILD_DIR" -maxdepth 1 -type d -name 'yandex-taxi-testsuite-*' -exec rm -rf {} +
    tar -xzf "$TESTSUITE_ARCHIVE" -C "$BUILD_DIR"
    EXTRACTED="$(find "$BUILD_DIR" -maxdepth 1 -type d -name 'yandex-taxi-testsuite-*' | head -n 1)"
    if [ -z "$EXTRACTED" ]; then
        echo "!!! Failed to extract yandex-taxi-testsuite archive."
        exit 1
    fi
    mv "$EXTRACTED" "$TESTSUITE_CLONE"
    echo "yandex-taxi-testsuite (develop) has been downloaded to $TESTSUITE_CLONE."
}

download_yandex_taxi_testsuite

# Place the package at scripts/docs/en/testsuite so it is picked up via INPUT
# `scripts/docs/en/` (Python modules keep the `testsuite.*` prefix).
TESTSUITE_DOCS_ROOT="scripts/docs/en/testsuite"
rm -rf "$TESTSUITE_DOCS_ROOT"
cp -a "$BUILD_DIR/yandex-taxi-testsuite/testsuite" "$TESTSUITE_DOCS_ROOT"
find "$TESTSUITE_DOCS_ROOT" -type d -name '__pycache__' -prune -exec rm -rf {} +
rm -rf "$TESTSUITE_DOCS_ROOT/tests"
python3 scripts/docs/prepare_testsuite_for_doxygen.py "$TESTSUITE_DOCS_ROOT"

# Run doxygen.
rm -rf "$BUILD_DIR/docs" || :

if [ -z "$NO_DEFAULT_HTML_CLEANUP" ]; then
    # Generate versions for docs
    python3 scripts/docs/generate_versions.py
fi

echo "Running doxygen..."
(
    cat scripts/docs/doxygen.conf;
    echo "${DOXYFILE_OVERRIDES:-}";
) | $DOXYGEN - 2>&1 | python3 scripts/docs/clean_doxygen_logs.py | tee "$BUILD_DIR/doxygen.err.log"
echo "A copy of doxygen logs is in: $BUILD_DIR/doxygen.err.log"

# Apply minor clean-ups to doxygen output.
if [ -z "$NO_DEFAULT_HTML_CLEANUP" ]; then
    echo "userver.tech" > "$BUILD_DIR/docs/html/CNAME"
    cp "$BUILD_DIR/docs/html/d8/dee/md_en_2userver_2404.html" "$BUILD_DIR/docs/html/404.html" || :
    sed -i 's|\.\./\.\./|/|g' "$BUILD_DIR/docs/html/404.html"
fi

# Cleanup
rm -rf scripts/docs/en/dynamic_configs || :
rm -rf "$TESTSUITE_DOCS_ROOT" || :
