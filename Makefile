# NOTE: use Makefile.local for customization
-include Makefile.local

KERNEL := $(shell uname -s)
ifeq ($(KERNEL),Darwin)
  -include Makefile.macos
endif

BUILD_TYPE ?= Release
BUILD_DIR ?= build
DOCS_DIR ?= docs
CMAKE_DIR = $(CURDIR)
ifeq ($(origin CC),default)
  CC := clang-6.0
endif
ifeq ($(origin CXX),default)
  CXX := clang++-6.0
endif
SCAN_BUILD = scan-build-5.0
SCAN_BUILD_OPTS = -o $(PWD)/static-analyzer-report/
BUILD_CHECK_DIR ?= build-check

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
	@echo "Makefile: CC = ${CC} CXX = ${CXX}"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && \
      cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_OPTS) $(CMAKE_DIR)

.PHONY: init
init: cmake

.PHONY: build
build: build-all

build-%: init
	$(MAKE) -j$(NPROCS) -C $(BUILD_DIR) $(patsubst build-%,%,$@)

.PHONY: deb
deb:
	DEB_BUILD_OPTIONS="parallel=$(NPROCS)" debuild -b

.PHONY: clang-static-analyzer
clang-static-analyzer:
	mkdir -p $(BUILD_CHECK_DIR)
	cd $(BUILD_CHECK_DIR) && $(SCAN_BUILD) $(SCAN_BUILD_OPTS) cmake .. && $(SCAN_BUILD) $(SCAN_BUILD_OPTS) $(MAKE) -j$(NPROCS)

.PHONY: cppcheck
cppcheck: $(BUILD_DIR)/Makefile
	cd $(BUILD_DIR) && $(MAKE) -j$(NPROCS) cppcheck

.PHONY: test
test: build
	@cd $(BUILD_DIR) && ctest -V

.PHONY: clang-format
clang-format:
	@tools/smart-clang-format.sh --all

.PHONY: smart-clang-format
smart-clang-format:
	@tools/smart-clang-format.sh

.PHONY: docs
docs:
	@(cat doxygen.conf; echo OUTPUT_DIRECTORY=$(DOCS_DIR)) | doxygen -

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BUILD_CHECK_DIR)
	@rm -rf $(DOCS_DIR)
