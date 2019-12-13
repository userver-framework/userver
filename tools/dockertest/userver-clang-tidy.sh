#!/usr/bin/env bash

/taxi/tools/run_as_user.sh make clean cmake
/taxi/tools/run_as_user.sh ln -s "$BUILD_DIR/compile_commands.json" compile_commands.json
/taxi/tools/run_as_user.sh make teamcity-clang-tidy
