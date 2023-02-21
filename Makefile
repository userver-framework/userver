include Makefile.common

.PHONY: clean
clean: check-yandex-env
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BUILD_CHECK_DIR)
	@rm -rf $(DOCS_DIR)

.PHONY: gen
gen:
	$(PYTHON3_BIN_DIR)python3 scripts/external_deps/cmake_generator.py --repo-dir=. --build-dir=cmake

bionic-%: check-yandex-env
	docker-compose run --rm taxi-userver-bionic make $*

docker-bionic-pull: check-yandex-env
	docker pull registry.yandex.net/taxi/taxi-integration-bionic-base

build-release: check-yandex-env
	$(MAKE) build BUILD_TYPE=Release

.PHONY: opensource-docs
opensource-docs:
	@( \
	    cat scripts/docs/doxygen.conf; \
	    echo "LAYOUT_FILE = scripts/docs/layout_opensource.xml"; \
	    echo "USE_MDFILE_AS_MAINPAGE = scripts/docs/en/landing.md"; \
	    echo "HTML_HEADER = scripts/docs/header_opensource.html"; \
	    echo 'PROJECT_BRIEF = "C++ Async Framework (beta)"'; \
	    echo OUTPUT_DIRECTORY=$(DOCS_DIR) \
	  ) | doxygen -
	@echo 'userver.tech' > $(DOCS_DIR)/html/CNAME

# Debug cmake configuration
build_debug/Makefile:
	@mkdir -p build_debug
	@cd build_debug && \
      cmake -DCMAKE_BUILD_TYPE=Debug $(CMAKE_OPTS) ..

# Release cmake configuration
build_release/Makefile:
	@mkdir -p build_release
	@cd build_release && \
      cmake -DCMAKE_BUILD_TYPE=Release $(CMAKE_OPTS) ..

# Run cmake
.PHONY: cmake-debug cmake-release
cmake-debug cmake-release: cmake-%: build_%/Makefile

# Build using cmake
.PHONY: build-debug build-release
build-debug build-release: build-%: cmake-%
	@cd build_$* && $(MAKE) -j $(NPROCS)

# Start targets makefile in docker environment
.PHONY: docker-cmake-debug docker-build-debug docker-test-debug docker-clean-debug docker-cmake-release docker-build-release docker-test-release docker-clean-release

docker-cmake-debug docker-build-debug docker-test-debug docker-clean-debug docker-cmake-release docker-build-release docker-test-release docker-clean-release: docker-%:
	docker-compose run --rm userver-ubuntu $(MAKE) $*

# Stop docker container and remove PG data
.PHONY: docker-clean-data
docker-clean-data:
	@docker-compose down -v

# Test
.PHONY: test-debug test-release
test-debug test-release: test-%: build-%
	@cd build_$* && ulimit -n 4096 && ((test -t 1 && GTEST_COLOR=1 PYTEST_ADDOPTS="--color=yes" ctest -V) || ctest -V)

.PHONY: dist-clean
dist-clean:
	@rm -rf build_*
