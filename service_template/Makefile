PROJECT_NAME = service_template
NPROCS ?= $(shell nproc)
CLANG_FORMAT ?= clang-format
DOCKER_IMAGE ?= ghcr.io/userver-framework/ubuntu-24.04-userver:latest
# If we're under TTY, pass "-it" to "docker run"
DOCKER_ARGS = $(shell /bin/test -t 0 && /bin/echo -it || echo)
PRESETS ?= debug release debug-custom release-custom

.PHONY: all
all: test-debug test-release

# Run cmake
.PHONY: $(addprefix cmake-, $(PRESETS))
$(addprefix cmake-, $(PRESETS)): cmake-%:
	cmake --preset $*

$(addsuffix /CMakeCache.txt, $(addprefix build-, $(PRESETS))): build-%/CMakeCache.txt:
	$(MAKE) cmake-$*

# Build using cmake
.PHONY: $(addprefix build-, $(PRESETS))
$(addprefix build-, $(PRESETS)): build-%: build-%/CMakeCache.txt
	cmake --build build-$* -j $(NPROCS) --target $(PROJECT_NAME)

# Test
.PHONY: $(addprefix test-, $(PRESETS))
$(addprefix test-, $(PRESETS)): test-%: build-%/CMakeCache.txt
	cmake --build build-$* -j $(NPROCS)
	cd build-$* && ((test -t 1 && GTEST_COLOR=1 PYTEST_ADDOPTS="--color=yes" ctest -V) || ctest -V)
	pycodestyle tests

# Start the service (via testsuite service runner)
.PHONY: $(addprefix start-, $(PRESETS))
$(addprefix start-, $(PRESETS)): start-%:
	cmake --build build-$* -v --target start-$(PROJECT_NAME)

# Cleanup data
.PHONY: $(addprefix clean-, $(PRESETS))
$(addprefix clean-, $(PRESETS)): clean-%:
	cmake --build build-$* --target clean

.PHONY: dist-clean
dist-clean:
	rm -rf build*
	rm -rf tests/__pycache__/
	rm -rf tests/.pytest_cache/
	rm -rf .ccache
	rm -rf .vscode/.cache
	rm -rf .vscode/compile_commands.json

# Install
.PHONY: $(addprefix install-, $(PRESETS))
$(addprefix install-, $(PRESETS)): install-%: build-%
	cmake --install build-$* -v --component $(PROJECT_NAME)

.PHONY: install
install: install-release

# Format the sources
.PHONY: format
format:
	find src -name '*pp' -type f | xargs $(CLANG_FORMAT) -i
	find tests -name '*.py' -type f | xargs autopep8 -i

# Start targets makefile in docker wrapper.
# The docker mounts the whole service's source directory,
# so you can do some stuff as you wish, switch back to host (non-docker) system
# and still able to access the results.
.PHONY: $(addprefix docker-cmake-, $(PRESETS)) $(addprefix docker-build-, $(PRESETS)) $(addprefix docker-test-, $(PRESETS)) $(addprefix docker-clean-, $(PRESETS))
$(addprefix docker-cmake-, $(PRESETS)) $(addprefix docker-build-, $(PRESETS)) $(addprefix docker-test-, $(PRESETS)) $(addprefix docker-clean-, $(PRESETS)): docker-%:
	docker run $(DOCKER_ARGS) \
		--network=host \
		-v $$PWD:$$PWD \
		-w $$PWD \
		$(DOCKER_IMAGE) \
		env CCACHE_DIR=$$PWD/.ccache \
		    HOME=$$HOME \
		    $$PWD/run_as_user.sh $(shell /bin/id -u) $(shell /bin/id -g) make $*
