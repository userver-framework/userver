#!/usr/bin/env bash

/taxi/tools/run_as_user.sh make clean cmake
/taxi/tools/run_as_user.sh ln -s "$BUILD_DIR/compile_commands.json" compile_commands.json
/taxi/tools/run_as_user.sh flatc --cpp --gen-object-api --filename-suffix '.fbs' -o samples/flatbuf_service/ samples/flatbuf_service/flatbuffer_schema.fbs
/taxi/tools/run_as_user.sh make teamcity-clang-tidy
