#!/bin/sh

GRPC_SCRIPTS_PATH="$(dirname "$(realpath "$0")")"
GEN_USERVER_WRAPPERS="${GRPC_SCRIPTS_PATH}/generator.py"

"${USERVER_GRPC_PYTHON_BINARY}" "${GEN_USERVER_WRAPPERS}" "$@"
