# NOTE: use Makefile.local for customization
-include Makefile.local

include Makefile.common

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BUILD_CHECK_DIR)
	@rm -rf $(DOCS_DIR)

.PHONY: gen
gen:
	$(PYTHON3_BIN_DIR)python3 generator.py --log-level=INFO --build-dir=$(BUILD_DIR)
