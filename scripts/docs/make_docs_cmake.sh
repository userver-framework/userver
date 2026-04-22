# Note: This file is intentionally not used yet

if ! [ -e "$BUILD_DIR/venv-docs/.venv-settled" ]; then
    rm -rf "$BUILD_DIR/venv-docs/.venv-settled"
    python3 -m venv "$BUILD_DIR/venv-docs/"
    "$BUILD_DIR/venv-docs/bin/pip" install -r scripts/docs/cmake2md/requirements.txt
    touch "$BUILD_DIR/venv-docs/.venv-settled"
fi

mkdir -p scripts/docs/en/cmake
find cmake/ -name '*.cmake' | \
	xargs "$BUILD_DIR/venv-docs/bin/python" scripts/docs/cmake2md/cmake2md.py \
	-t layout.md.jinja:scripts/docs/en/cmake/layout.md \
	CMakeLists.txt  
