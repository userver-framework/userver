include Makefile.common

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BUILD_CHECK_DIR)
	@rm -rf $(DOCS_DIR)

.PHONY: gen
gen:
	$(PYTHON3_BIN_DIR)python3 generator.py --log-level=INFO --build-dir=$(BUILD_DIR)

bionic-%:
	docker-compose run --rm taxi-userver-bionic make $*

docker-bionic-pull:
	docker pull registry.yandex.net/taxi/taxi-bionic-base

build-release:
	$(MAKE) build BUILD_TYPE=Release
