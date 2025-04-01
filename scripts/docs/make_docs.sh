#!/bin/sh

cd "$(dirname "$0")/../.."

if [ -z "$BUILD_DIR" ]; then
    echo "!!! Set BUILD_DIR environment variable to cmake build directory."
    echo "!!! See userver/scripts/docs/README.md"
    exit 2
fi

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "!!! Fully build userver before running this script."
    echo "!!! See userver/scripts/docs/README.md"
    exit 2
fi

download_and_extract_doxygen() {
    if [ ! -d "$BUILD_DIR/doxygen-1.13.2" ]; then
        echo "Doxygen not found in $BUILD_DIR or version is too low. Downloading..."
        wget --no-verbose https://github.com/doxygen/doxygen/releases/download/Release_1_13_2/doxygen-1.13.2.linux.bin.tar.gz -P "$BUILD_DIR"
        tar -xzf "$BUILD_DIR/doxygen-1.13.2.linux.bin.tar.gz" -C "$BUILD_DIR"
        export DOXYGEN="$BUILD_DIR/doxygen-1.13.2/bin/doxygen"
        echo "Doxygen has been successfully downloaded."
    else
        echo "Using already downloaded doxygen in $BUILD_DIR."
        export DOXYGEN="$BUILD_DIR/doxygen-1.13.2/bin/doxygen"
    fi
}

# Find doxygen, download doxygen if needed.
DOXYGEN="${DOXYGEN:-doxygen}"

if ! $DOXYGEN --version >/dev/null 2>&1; then
    download_and_extract_doxygen
fi

DOXYGEN_VERSION_MIN="1.10.0"
DOXYGEN_VERSION_CUR=$($DOXYGEN --version | awk -F " " '{print $1}')

if ! printf "%s\n%s\n" "$DOXYGEN_VERSION_MIN" "$DOXYGEN_VERSION_CUR" | sort -C; then
    download_and_extract_doxygen
fi

# Run userver codegen to avoid doxygen errors with invalid includes.
CMAKE_COMMAND=$(grep -oP 'CMAKE_COMMAND:INTERNAL=\K.*' "$BUILD_DIR/CMakeCache.txt")
CMAKE_VERSION=$("$CMAKE_COMMAND" --version | grep -oP '\d+\.\d+')

echo "Building target userver-codegen."
"$CMAKE_COMMAND" --build "$BUILD_DIR" --target userver-codegen

# Run doxygen.
rm -rf "$BUILD_DIR/docs" || :

DOXYFILE_OVERRIDES="${DOXYFILE_OVERRIDES:-}"

echo "Running doxygen..."
(
    cat scripts/docs/doxygen.conf;
    echo "$DOXYFILE_OVERRIDES";
) | $DOXYGEN - 2>&1 | python3 scripts/docs/clean_doxygen_logs.py | tee "$BUILD_DIR/doxygen.err.log"
echo "A copy of doxygen logs is in: $BUILD_DIR/doxygen.err.log"

# Apply minor clean-ups to doxygen output.
if [ -z "$NO_DEFAULT_HTML_CLEANUP" ]; then
    echo "userver.tech" > "$BUILD_DIR/docs/html/CNAME"
    cp "$BUILD_DIR/docs/html/d8/dee/md_en_2userver_2404.html" "$BUILD_DIR/docs/html/404.html" || :
    sed -i 's|\.\./\.\./|/|g' "$BUILD_DIR/docs/html/404.html"
fi
