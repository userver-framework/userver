include Makefile.common

.PHONY: clean
clean: check-yandex-env
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BUILD_CHECK_DIR)
	@rm -rf $(DOCS_DIR)

.PHONY: gen
gen: check-yandex-env
	$(PYTHON3_BIN_DIR)python3 generator.py --log-level=INFO --build-dir=$(BUILD_DIR)

bionic-%: check-yandex-env
	docker-compose run --rm taxi-userver-bionic make $*

docker-bionic-pull: check-yandex-env
	docker pull registry.yandex.net/taxi/taxi-integration-bionic-base

build-release: check-yandex-env
	$(MAKE) build BUILD_TYPE=Release
