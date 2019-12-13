#!/usr/bin/env bash

chmod 777 "$CCACHE_DIR"
chmod 777 "$CORES_DIR"
/taxi/tools/run_as_user.sh ccache -M ${CCACHE_SIZE:-40G}
echo "$CORES_DIR/core-%e-%s-%u-%g-%p-%t" > /proc/sys/kernel/core_pattern
if [ "$PULL_REQUEST_TYPE" = "ASAN" ]; then
    echo "Build userver with asan"
    /taxi/tools/run_as_user.sh make build-with-asan
elif [ "$PULL_REQUEST_TYPE" = "UBSAN" ]; then
    echo "Build userver with ubsan"
    /taxi/tools/run_as_user.sh make build-with-ubsan
elif [ "$PULL_REQUEST_TYPE" = "SANITIZE" ]; then
    echo "Build userver with default set of sanitizers"
    /taxi/tools/run_as_user.sh make build-with-sanitizers
fi
/taxi/tools/run_as_user.sh bash -c 'ulimit -c unlimited; make test'
