#!/usr/bin/env bash

if [ "$CONTAINER_NAME" != "userver-service-sample" ]; then
    echo "start-simple.sh should be run in \"userver-service-sample\" container!"
    exit 1
fi

if [[ -z "${SERVICE_NAME}" ]]; then
	echo "env SERVICE_NAME is required"
	exit 1
fi

if [ ! -f ${BUILD_DIR}/samples/${SERVICE_NAME}/userver-samples-${SERVICE_NAME} ]; then
	mkdir -p $CCACHE_DIR
	mkdir -p $CORES_DIR
	chmod 777 "$CCACHE_DIR"
	chmod 777 "$CORES_DIR"
	/tools/run_as_user.sh ccache -M ${CCACHE_SIZE:-40G}
	echo "$CORES_DIR/core-%e-%s-%u-%g-%p-%t" > /proc/sys/kernel/core_pattern
	/tools/run_as_user.sh mkdir -p /userver/build
	/tools/run_as_user.sh bash -c 'cd /userver/build && cmake $CMAKE_OPTS -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ..'
	/tools/run_as_user.sh bash -c 'cd /userver/build && make -j $(NPROCS)'
fi

mkdir -p /etc/$SERVICE_NAME
cp /userver/samples/$SERVICE_NAME/dynamic_config_fallback.json /etc/$SERVICE_NAME/
/tools/run_as_user.sh /userver/build/samples/$SERVICE_NAME/userver-samples-${SERVICE_NAME} --config /userver/samples/$SERVICE_NAME/static_config.yaml
