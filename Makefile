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

.PHONY: docker-cmake docker-make docker-tests

docker-cmake:
	docker-compose run --rm userver-ubuntu bash -c 'cmake -DUSERVER_GOOGLE_COMMON_PROTOS=/app/api-common-protos -B./build -S./'

docker-make: docker-cmake
	docker-compose run --rm userver-ubuntu bash -c 'cd build && make -j $(nproc)'

docker-tests: docker-make
	docker-compose run --rm userver-ubuntu bash -c 'cd build && ulimit -n 4096 && ctest -V'
