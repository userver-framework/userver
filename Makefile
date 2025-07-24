CMAKE_COMMON_FLAGS ?= -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
CMAKE_DEBUG_FLAGS ?= '-DUSERVER_SANITIZE=addr;ub'
CMAKE_RELEASE_FLAGS ?=
KERNEL := $(shell uname -s)
NPROCS ?= $(shell nproc)
DOCKER_COMPOSE ?= docker-compose

# NOTE: use Makefile.local for customization
-include Makefile.local

CMAKE_DEBUG_FLAGS += -DCMAKE_BUILD_TYPE=Debug $(CMAKE_COMMON_FLAGS)
CMAKE_RELEASE_FLAGS += -DCMAKE_BUILD_TYPE=Release $(CMAKE_COMMON_FLAGS)

.DEFAULT_GOAL := all
.PHONY: all
all: test-debug test-release

.PHONY: docs
docs:
	BUILD_DIR=$(BUILD_DIR) ./scripts/docs/make_docs.sh

.PHONY: docs-upload
docs-upload:
	BUILD_DIR=$(BUILD_DIR) ./scripts/docs/upload_docs.sh

.PHONY: docs-internal
docs-internal:
	BUILD_DIR=$(BUILD_DIR) ../scripts/userver/docs/make_docs.sh

.PHONY: docs-internal-upload
docs-internal-upload:
	BUILD_DIR=$(BUILD_DIR) OAUTH_TOKEN=$(OAUTH_TOKEN) ../scripts/userver/docs/upload_docs.sh

# Run cmake
.PHONY: cmake-debug
cmake-debug:
	cmake -B build_debug $(CMAKE_DEBUG_FLAGS)

.PHONY: cmake-release
cmake-release:
	cmake -B build_release $(CMAKE_RELEASE_FLAGS)

# Build using cmake
.PHONY: build-debug build-release
build-debug build-release: build-%: cmake-%
	cmake --build build_$* -j $(NPROCS)

# Test
.PHONY: test-debug test-release
test-debug test-release: test-%: build-%
	ulimit -n 4096
	cd build_$* && ((test -t 1 && GTEST_COLOR=1 PYTEST_ADDOPTS="--color=yes" ctest -V) || ctest -V)

# Start targets makefile in docker environment
.PHONY: docker-cmake-debug docker-build-debug docker-test-debug docker-cmake-release docker-build-release docker-test-release
docker-cmake-debug docker-build-debug docker-test-debug docker-cmake-release docker-build-release docker-test-release: docker-%:
	$(DOCKER_COMPOSE) run --rm userver-ubuntu make $*

# Stop docker container and remove PG data
.PHONY: docker-clean-data
docker-clean-data:
	$(DOCKER_COMPOSE) down -v

.PHONY: docker-enter
docker-enter:
	$(DOCKER_COMPOSE) run --rm userver-ubuntu bash

.PHONY: docker-run-ydb docker-kill
docker-run-ydb:
	$(DOCKER_COMPOSE) run -d --rm --service-ports run-ydb

docker-kill:
	$(DOCKER_COMPOSE) kill

# clean build folders
.PHONY: dist-clean
dist-clean:
	rm -rf build_*
