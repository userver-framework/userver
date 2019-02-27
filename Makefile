# NOTE: use Makefile.local for customization
-include Makefile.local

include Makefile.common

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BUILD_CHECK_DIR)
	@rm -rf $(DOCS_DIR)
