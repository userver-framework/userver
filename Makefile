-include Makefile.local

# NOTE: use Makefile.local for customization
BUILD_TYPE ?= Release
BUILD_DIR ?= build
CMAKE_DIR = $(CURDIR)

KERNEL := $(shell uname -s)
ifeq ($(KERNEL),Linux)
  NPROCS := $(shell nproc --all)
else ifeq ($(KERNEL),Darwin)
  NPROCS := $(shell sysctl -n hw.ncpu)
else
  $(warning Cannot determine CPU count for $(KERNEL), falling back to 1)
  NPROCS := 1
endif
export NPROCS

.PHONY: all
all: build

.PHONY: cmake
cmake: $(BUILD_DIR)/Makefile

$(BUILD_DIR)/Makefile:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && \
      cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_OPTS) $(CMAKE_DIR)

.PHONY: init
init: cmake

.PHONY: build
build: build-all

build-%: init
	$(MAKE) -j$(NPROCS) -C $(BUILD_DIR) $(patsubst build-%,%,$@)

.PHONY: test
test: build
	@cd $(BUILD_DIR) && ctest -V

.PHONY: clang-format
clang-format:
	@tools/smart-clang-format.sh --all

.PHONY: smart-clang-format
smart-clang-format:
	@tools/smart-lang-format.sh

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
