#!/usr/bin/env bash

if [ "$CONTAINER_NAME" != "userver-tests" ]; then
    echo "userver-tests.sh should be run in \"userver-tests\" service!"
    exit 1
fi

mkdir -p $CCACHE_DIR
mkdir -p $CORES_DIR
chmod 777 "$CCACHE_DIR"
chmod 777 "$CORES_DIR"
/tools/run_as_user.sh ccache -M ${CCACHE_SIZE:-40G}
echo "$CORES_DIR/core-%e-%s-%u-%g-%p-%t" > /proc/sys/kernel/core_pattern
/tools/run_as_user.sh mkdir -p /userver/build
/tools/run_as_user.sh bash -c 'cd /userver/build && cmake $CMAKE_OPTS -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ..'
/tools/run_as_user.sh bash -c 'cd /userver/build && make -j$(NPROCS)'

/tools/run_as_user.sh bash -c 'cd /userver/build && ulimit -c unlimited; ctest -V'
