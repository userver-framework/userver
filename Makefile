KERNEL := $(shell uname -s)

CMAKE_COMMON_FLAGS ?= -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
CMAKE_DEBUG_FLAGS ?= -DUSERVER_SANITIZE='addr ub'
CMAKE_RELEASE_FLAGS ?=
CMAKE_OS_FLAGS ?=

NPROCS ?= $(shell nproc)

ifeq ($(KERNEL),Darwin)
CMAKE_COMMON_FLAGS += -DUSERVER_NO_WERROR=1 -DUSERVER_CHECK_PACKAGE_VERSIONS=0 \
  -DUSERVER_DOWNLOAD_PACKAGE_CRYPTOPP=1 \
  -DOPENSSL_ROOT_DIR=$(shell brew --prefix openssl@1.1) \
  -DUSERVER_PG_INCLUDE_DIR=$(shell pg_config --includedir) \
  -DUSERVER_PG_LIBRARY_DIR=$(shell pg_config --libdir) \
  -DUSERVER_PG_SERVER_LIBRARY_DIR=$(shell pg_config --pkglibdir) \
  -DUSERVER_PG_SERVER_INCLUDE_DIR=$(shell pg_config --includedir-server)
endif

# NOTE: use Makefile.local for customization
-include Makefile.local

.PHONY: gen
gen:
	python3 scripts/external_deps/cmake_generator.py --repo-dir=. --build-dir=cmake

.PHONY: docs
docs:
	@rm -rf docs/*
	@( \
	    cat scripts/docs/doxygen.conf; \
	    echo "LAYOUT_FILE = scripts/docs/layout_opensource.xml"; \
	    echo "USE_MDFILE_AS_MAINPAGE = scripts/docs/en/landing.md"; \
	    echo "HTML_HEADER = scripts/docs/header_opensource.html"; \
	    echo 'PROJECT_BRIEF = "C++ Async Framework (beta)"'; \
	    echo OUTPUT_DIRECTORY=docs \
	  ) | doxygen -
	@echo 'userver.tech' > docs/html/CNAME
	@cp docs/html/df/d86/md_en_userver_404.html docs/html/404.html
	@sed -i 's|\.\./\.\./|/|g' docs/html/404.html

# Debug cmake configuration
build_debug/Makefile:
	@mkdir -p build_debug
	@cd build_debug && \
      cmake -DCMAKE_BUILD_TYPE=Debug $(CMAKE_COMMON_FLAGS) $(CMAKE_DEBUG_FLAGS) $(CMAKE_OS_FLAGS) ..

# Release cmake configuration
build_release/Makefile:
	@mkdir -p build_release
	@cd build_release && \
	  cmake -DCMAKE_BUILD_TYPE=Release $(CMAKE_COMMON_FLAGS) $(CMAKE_RELEASE_FLAGS) $(CMAKE_OS_FLAGS) ..

# Run cmake
.PHONY: cmake-debug cmake-release
cmake-debug cmake-release: cmake-%: build_%/Makefile

# Build using cmake
.PHONY: build-debug build-release
build-debug build-release: build-%: cmake-%
	@cmake --build build_$* -j $(NPROCS)

# Test
.PHONY: test-debug test-release
test-debug test-release: test-%: build-%
	@cd build_$* && ulimit -n 4096 && ((test -t 1 && GTEST_COLOR=1 PYTEST_ADDOPTS="--color=yes" ctest -V) || ctest -V)

# Start targets makefile in docker environment
.PHONY: docker-cmake-debug docker-build-debug docker-test-debug docker-cmake-release docker-build-release docker-test-release
docker-cmake-debug docker-build-debug docker-test-debug docker-cmake-release docker-build-release docker-test-release: docker-%:
	@docker-compose run --rm userver-ubuntu $(MAKE) $*

# Stop docker container and remove PG data
.PHONY: docker-clean-data
docker-clean-data:
	@docker-compose down -v

# clean build folders
.PHONY: dist-clean
dist-clean:
	@rm -rf build_*
	@rm -rf docs/
