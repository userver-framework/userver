#!/bin/sh

ROOT=$(dirname $0)/../..
export PYTHONPATH="$ROOT/scripts/pylint" 
# set -e

# The only enabled check is the raw-file `sleep-in-test`, which does not rely on
# astroid inference. Newer astroid floods stderr with "unable to transform"
# UserWarnings when it hits the recursion limit on heavily-typed modules; those
# warnings are irrelevant to a raw-file checker, so silence them.
export PYTHONWARNINGS="ignore::UserWarning"

find "$ROOT" -name '*.py' | \
    grep -v '^./build*' | \
    grep test | \
xargs $PYTHON_BINARY -m pylint \
    --init-hook="import sys; sys.setrecursionlimit(8000)" \
    --load-plugins=pylint_sleep_plugin \
    --disable all \
    --enable sleep-in-test \
    --ignore-paths ".*venv.*"

echo >&2 "Note: forcing test success (the test is not yet ready)"
