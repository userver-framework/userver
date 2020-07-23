#!/usr/bin/env bash

if [ "$CONTAINER_NAME" != "taxi-userver-tests" ]; then
    echo "userver-tests.sh should be run in \"taxi-userver-tests\" service!"
    exit 1
fi

if [ "$IS_TEAMCITY" != "1" ]; then
    echo "userver-tests.sh should be run in Teamcity!"
    exit 1
fi

chmod 777 "$CCACHE_DIR"
chmod 777 "$CORES_DIR"
/taxi/tools/run_as_user.sh ccache -M ${CCACHE_SIZE:-40G}
echo "$CORES_DIR/core-%e-%s-%u-%g-%p-%t" > /proc/sys/kernel/core_pattern
if [ "$PULL_REQUEST_TYPE" = "ASAN" ]; then
    echo "Build userver with asan"
    /taxi/tools/run_as_user.sh make $MAKE_OPTS build-with-asan
elif [ "$PULL_REQUEST_TYPE" = "UBSAN" ]; then
    echo "Build userver with ubsan"
    /taxi/tools/run_as_user.sh make $MAKE_OPTS build-with-ubsan
elif [ "$PULL_REQUEST_TYPE" = "SANITIZE" ]; then
    echo "Build userver with default set of sanitizers"
    /taxi/tools/run_as_user.sh make $MAKE_OPTS build-with-sanitizers
fi
/taxi/tools/run_as_user.sh bash -c 'ulimit -c unlimited; make test'
