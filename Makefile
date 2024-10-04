CMAKE_COMMON_FLAGS ?= -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
CMAKE_DEBUG_FLAGS ?= -DUSERVER_SANITIZE=addr;ub
CMAKE_RELEASE_FLAGS ?=
KERNEL := $(shell uname -s)
NPROCS ?= $(shell nproc)
DOXYGEN ?= doxygen
DOCKER_COMPOSE ?= docker-compose

# NOTE: use Makefile.local for customization
-include Makefile.local

CMAKE_DEBUG_FLAGS += -DCMAKE_BUILD_TYPE=Debug $(CMAKE_COMMON_FLAGS)
CMAKE_RELEASE_FLAGS += -DCMAKE_BUILD_TYPE=Release $(CMAKE_COMMON_FLAGS)

.DEFAULT_GOAL := all
.PHONY: all
all: test-debug test-release

# Requires doxygen 1.10.0+
.PHONY: docs
docs:
	@rm -rf docs/*
	@$(DOXYGEN) --version >/dev/null 2>&1 || { \
		echo "!!! No Doxygen found."; \
		exit 2; \
	}
	@{ \
		DOXYGEN_VERSION_MIN="1.10.0" && \
		DOXYGEN_VERSION_CUR=$$($(DOXYGEN) --version | awk -F " " '{print $$1}') && \
		DOXYGEN_VERSION_VALID=$$(printf "%s\n%s\n" "$$DOXYGEN_VERSION_MIN" "$$DOXYGEN_VERSION_CUR" | sort -C && echo 0 || echo 1) && \
		if [ "$$DOXYGEN_VERSION_VALID" != "0" ]; then \
			echo "!!! Doxygen expected version is $$DOXYGEN_VERSION_MIN, but $$DOXYGEN_VERSION_CUR found."; \
			echo "!!! See userver/scripts/docs/README.md"; \
			exit 2; \
		fi \
	}
	@( \
	    cat scripts/docs/doxygen.conf; \
	    echo OUTPUT_DIRECTORY=docs \
	  ) | $(DOXYGEN) -
	@echo 'userver.tech' > docs/html/CNAME
	@cp docs/html/d8/dee/md_en_2userver_2404.html docs/html/404.html || :
	@sed -i 's|\.\./\.\./|/|g' docs/html/404.html

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
	rm -rf docs/
