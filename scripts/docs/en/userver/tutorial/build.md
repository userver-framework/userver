## Build and setup


## Step by step guide

@todo Fill me

### Download


### Makefile targets:
* `all`/`build` -- builds all the targets in debug mode
* `build-release` -- builds all the targets in release mode (useful for benchmarks)
* `clean` -- removes the build files
* `check-pep8` -- checks all the python files with linters
* `smart-check-pep8` -- checks all the python files differing from HEAD with linters
* `format` -- formats all the code with all the formatters
* `smart-format` -- formats all the files differing from HEAD with all the formatters
* `test` -- runs the tests
* `docs` -- generates the HTML documentation

### Build

Recommended platforms:
* For development and production:
  * Ubuntu 16.04 Xenial, or
  * Ubuntu 18.04 Bionic, or
* For development only (may have performance issues in some cases):
  * MacOS 10.15 with [Xcode](https://apps.apple.com/us/app/xcode/id497799835) and [Homebrew](https://brew.sh/)
* clang-9


@todo fill the build instructions

```
bash
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-core_unittest
```

### Run tests
```
bash
./userver/core/userver-core_unittest
```
