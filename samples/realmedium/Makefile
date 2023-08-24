CMAKE_COMMON_FLAGS ?= -DUSERVER_OPEN_SOURCE_BUILD=1 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

CMAKE_DEBUG_FLAGS ?= -DUSERVER_SANITIZE='addr ub'
CMAKE_RELEASE_FLAGS ?=
CMAKE_OS_FLAGS ?= -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 -DUSERVER_FEATURE_REDIS_HI_MALLOC=1
NPROCS ?= $(shell nproc)
CLANG_FORMAT ?= clang-format

# NOTE: use Makefile.local for customization
-include Makefile.local

all: test-debug test-release

# Debug cmake configuration
build_debug/Makefile:
	@git submodule update --init
	@mkdir -p build_debug
	@cd build_debug && \
      cmake -DCMAKE_BUILD_TYPE=Debug $(CMAKE_COMMON_FLAGS) $(CMAKE_DEBUG_FLAGS) $(CMAKE_OS_FLAGS) $(CMAKE_OPTIONS) ..

# Release cmake configuration
build_release/Makefile:
	@git submodule update --init
	@mkdir -p build_release
	@cd build_release && \
      cmake -DCMAKE_BUILD_TYPE=Release $(CMAKE_COMMON_FLAGS) $(CMAKE_RELEASE_FLAGS) $(CMAKE_OS_FLAGS) $(CMAKE_OPTIONS) ..

# build using cmake
build-impl-%: build_%/Makefile
	@cmake --build build_$* -j $(NPROCS) --target realmedium

# test
test-impl-%: build-impl-%
	@cmake --build build_$* -j $(NPROCS) --target realmedium_unittest
	@cd build_$* && ((test -t 1 && GTEST_COLOR=1 PYTEST_ADDOPTS="--color=yes" ctest -V) || ctest -V)
	@pep8 tests

# testsuite service runner
service-impl-start-%: build-impl-%
	@cd ./build_$* && $(MAKE) start-realmedium

# clean
clean-impl-%:
	cd build_$* && $(MAKE) clean

# dist-clean
.PHONY: dist-clean
dist-clean:
	@rm -rf build_*
	@rm -f ./configs/static_config.yaml

# format
.PHONY: format
format:
	@find src -name '*pp' -type f | xargs $(CLANG_FORMAT) -i
	@find tests -name '*.py' -type f | xargs autopep8 -i

.PHONY: cmake-debug build-debug test-debug clean-debug cmake-release build-release test-release clean-release install install-debug docker-cmake-debug docker-build-debug docker-test-debug docker-clean-debug docker-cmake-release docker-build-release docker-test-release docker-clean-release docker-install docker-install-debug docker-start-service-debug docker-start-service docker-clean-data

install-debug: build-debug
	@cd build_debug && \
		cmake --install . -v --component realmedium

install: build-release
	@cd build_release && \
		cmake --install . -v --component realmedium

# Hide target, use only in docker environment
--debug-start-in-docker: install
	@ulimit -n 100000
	@sed -i 's/config_vars.yaml/config_vars.docker.yaml/g' /home/user/.local/etc/realmedium/static_config.yaml
	@psql 'postgresql://user:password@service-postgres:5432/realmedium_db-1' -f ./postgresql/schemas/db-1.sql
	@/home/user/.local/bin/realmedium \
		--config /home/user/.local/etc/realmedium/static_config.yaml

# Hide target, use only in docker environment
--debug-start-in-docker-debug: install-debug
	@sed -i 's/config_vars.yaml/config_vars.docker.yaml/g' /home/user/.local/etc/realmedium/static_config.yaml
	@psql 'postgresql://user:password@service-postgres:5432/realmedium_db-1' -f ./postgresql/schemas/db-1.sql
	@/home/user/.local/bin/realmedium \
		--config /home/user/.local/etc/realmedium/static_config.yaml

# Start targets makefile in docker enviroment
docker-impl-%:
	docker-compose run --rm realmedium-service make $*

# Build and runs service in docker environment
docker-start-service-debug:
	@docker-compose run -p 8080:8080 --rm realmedium-service make -- --debug-start-in-docker-debug

# Build and runs service in docker environment
docker-start-service:
	@docker-compose run -p 8080:8080 --rm realmedium-service make -- --debug-start-in-docker

# Stop docker container and remove PG data
docker-clean-data:
	@docker-compose down -v
	@rm -rf ./.pgdata

# Explicitly specifying the targets to help shell with completions
cmake-debug: build_debug/Makefile
cmake-release: build_release/Makefile

build-debug: build-impl-debug
build-release: build-impl-release

test-debug: test-impl-debug
test-release: test-impl-release

service-start-debug: service-impl-start-debug
service-start-release: service-impl-start-release

clean-debug: clean-impl-debug
clean-release: clean-impl-release

docker-cmake-debug: docker-impl-cmake-debug
docker-cmake-release: docker-impl-cmake-release

docker-build-debug: docker-impl-build-debug
docker-build-release: docker-impl-build-release

docker-test-debug: docker-impl-test-debug
docker-test-release: docker-impl-test-release

docker-clean-debug: docker-impl-clean-debug
docker-clean-release: docker-impl-clean-release

docker-install: docker-impl-install
docker-install-debug: docker-impl-install-debug
