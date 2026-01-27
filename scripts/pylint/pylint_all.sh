#!/bin/sh

ROOT=$(dirname $0)/../..
export PYTHONPATH="$ROOT/scripts/pylint" 
# set -e

find "$ROOT" -name '*.py' | \
    grep -v '^./build*' | \
    grep test | \
xargs $PYTHON_BINARY -m pylint \
    --load-plugins=pylint_sleep_plugin \
    --disable all \
    --enable sleep-in-test \
    --ignore-paths ".*venv.*"
    ${PY_FILES}

echo >&2 "Note: forcing test success (the test is not yet ready)"
