include Makefile.common

.PHONY: clean
clean: check-yandex-env
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BUILD_CHECK_DIR)
	@rm -rf $(DOCS_DIR)

.PHONY: gen
gen:
	$(PYTHON3_BIN_DIR)python3 plugins/external_deps/impl/cmake_generator.py --repo-dir=. --build-dir=cmake

bionic-%: check-yandex-env
	docker-compose run --rm taxi-userver-bionic make $*

docker-bionic-pull: check-yandex-env
	docker pull registry.yandex.net/taxi/taxi-integration-bionic-base

build-release: check-yandex-env
	$(MAKE) build BUILD_TYPE=Release

.PHONY: internal-docs
internal-docs: check-yandex-env
	@if ! git diff --quiet scripts/docs/doxygen.conf; then \
	    echo "!!! This command should run on an unchanged or commited scripts/docs/doxygen.conf"; \
	    exit 1; \
	fi
	@if [ -z "${OAUTH_TOKEN}" ]; then \
	    echo "!!! No OAUTH_TOKEN environment variable provided"; \
	    exit 2; \
	fi
	rm -rf docs/html || :
	@echo "Checking aws tool configuration"
	@aws --endpoint-url=https://s3.mds.yandex.net s3 ls > /dev/null
	@(cd scripts/docs/ && taxi-python3 ./download.py --token ${OAUTH_TOKEN})
	doxygen scripts/docs/doxygen.conf
	aws --endpoint-url=https://s3.mds.yandex.net s3 rm --recursive s3://userver/
	aws --endpoint-url=https://s3.mds.yandex.net s3 cp --recursive docs/html/ s3://userver/


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

