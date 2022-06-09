#!/bin/sh

GRPC_SCRIPTS_PATH="$(dirname $(realpath $0))"
GEN_USERVER_WRAPPERS="${GRPC_SCRIPTS_PATH}/generator.py"

if taxi-python3 --version > /dev/null 2>&1; then
  PYTHON=taxi-python3
else
  PYTHON=python3
fi

${PYTHON} "${GEN_USERVER_WRAPPERS}" "$@"
