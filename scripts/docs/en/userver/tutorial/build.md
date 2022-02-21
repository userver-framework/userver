## Build and setup

**❗ Yandex and MLU (Taxi): refer to the [uservices tutorial for build info](https://nda.ya.ru/t/JgZmw_ck44jKhx).**

## Download

Download and extract the latest release from https://github.com/userver-framework/userver

## CMake options

The following options could be used to control `cmake`:

| Option                              | Description                                                                  | Default                                          |
|-------------------------------------|------------------------------------------------------------------------------|--------------------------------------------------|
| OPEN_SOURCE_BUILD                   | Do not use internal Yandex packages                                          | OFF                                              |
| USERVER_FEATURE_DOWNLOAD_PACKAGES   | Download missing third party packages and use the downloaded versions        | ${OPEN_SOURCE_BUILD}                             |
| USERVER_FEATURE_MONGODB             | Provide asynchronous driver for MongoDB                                      | ON                                               |
| USERVER_FEATURE_POSTGRESQL          | Provide asynchronous driver for PostgreSQL                                   | ON                                               |
| USERVER_FEATURE_REDIS               | Provide asynchronous driver for Redis                                        | ON                                               |
| USERVER_FEATURE_GRPC                | Provide asynchronous driver for gRPC                                         | ON                                               |
| USERVER_FEATURE_UNIVERSAL           | Provide a universal utilities library that does not use coroutines           | ON                                               |
| USERVER_FEATURE_CRYPTOPP_BLAKE2     | Provide wrappers for blake2 algorithms of crypto++                           | ON                                               |
| USERVER_FEATURE_CRYPTOPP_BASE64_URL | Provide wrappers for Base64 URL decoding and encoding algorithms of crypto++ | ON                                               |
| USERVER_FEATURE_SPDLOG_TCP_SINK     | Use tcp_sink.h of the spdlog library for testing logs                        | ON                                               |
| USERVER_FEATURE_REDIS_HI_MALLOC     | Provide a `hi_malloc(unsigned long)` [issue][hi_malloc] workaround           | OFF                                              |
| USERVER_FEATURE_CARES_DOWNLOAD      | Download and setup c-ares if no c-ares of matching version was found         | ${USERVER_FEATURE_DOWNLOAD_PACKAGES}             |
| USERVER_FEATURE_CCTZ_DOWNLOAD       | Download and setup cctz if no cctz of matching version was found             | ${USERVER_FEATURE_DOWNLOAD_PACKAGES}             |
| USERVER_FEATURE_CURL_DOWNLOAD       | Download and setup libcurl if no libcurl of matching version was found       | ${USERVER_FEATURE_DOWNLOAD_PACKAGES}             |
| USERVER_FEATURE_FMT_DOWNLOAD        | Download and setup Fmt if no Fmt of matching version was found               | ${USERVER_FEATURE_DOWNLOAD_PACKAGES}             |
| USERVER_FEATURE_GTEST_DOWNLOAD      | Download and setup gtest if no gtest of matching version was found           | ${USERVER_FEATURE_DOWNLOAD_PACKAGES}             |
| USERVER_FEATURE_GBENCH_DOWNLOAD     | Download and setup gbench if no gbench of matching version was found         | ${USERVER_FEATURE_DOWNLOAD_PACKAGES}             |
| USERVER_FEATURE_SPDLOG_DOWNLOAD     | Download and setup Spdlog if no Spdlog of matching version was found         | ${USERVER_FEATURE_DOWNLOAD_PACKAGES}             |
| USERVER_FEATURE_NO_WERROR           | Do not treat warnings as errors                                              | ${OPEN_SOURCE_BUILD}                             |
| USERVER_IS_THE_ROOT_PROJECT         | Build tests, samples and helper tools                                        | auto-detects if userver is the top level project |

[hi_malloc]: https://bugs.launchpad.net/ubuntu/+source/hiredis/+bug/1888025

To explicitly specialize the compiler use the cmake options `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`.

Prefer avoiding Boost versions that are affected by the bug https://github.com/boostorg/lockfree/issues/59 (1.71 and above).

### Ubuntu 20.04 (Focal Fossa)

1. Install the packages:
  ```
  bash
  sudo apt install --allow-downgrades -y cmake libboost1.67-dev libboost-program-options1.67-dev libboost-filesystem1.67-dev \
    libboost-locale1.67-dev libboost-regex1.67-dev libboost-iostreams1.67-dev libboost-thread1.67-dev \
    libev-dev zlib1g-dev \
    libcurl4-openssl-dev libcrypto++-dev libyaml-cpp-dev libssl-dev libcctz-dev \
    libhttp-parser-dev libjemalloc-dev libmongoc-dev libbson-dev libldap2-dev  libpq5=12.* libpq-dev=12.* \
    postgresql-server-dev-12 libkrb5-dev libhiredis-dev libgrpc-dev libgrpc++-dev \
    libgrpc++1 protobuf-compiler-grpc libprotoc-dev python3-protobuf python3-jinja2 \
    libbenchmark-dev libgtest-dev ccache git
  ```
2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DOPEN_SOURCE_BUILD=1 -DUSERVER_FEATURE_MONGODB=0 -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 -DUSERVER_FEATURE_REDIS_HI_MALLOC=1 -DUSE_LD=gold -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```

**Clang**. If you wish to use clang toolset then you have to install it
`sudo apt install llvm-12-tools clang-12 lld-12` and add th following options to cmake:
`-DCMAKE_CXX_COMPILER=clang++-12 -DCMAKE_C_COMPILER=clang-12`.

### Ubuntu 18.04 (Bionic Beaver)

1. Install the packages:
  ```
  bash
  sudo apt install --allow-downgrades -y cmake g++ libboost1.67-dev libboost-program-options1.67-dev libboost-filesystem1.67-dev \
    libboost-locale1.67-dev libboost-regex1.67-dev libboost-iostreams1.67-dev libboost-thread1.67-dev \
    libev-dev zlib1g-dev \
    libcurl4-openssl-dev libcrypto++-dev libyaml-cpp-dev libssl-dev libcctz-dev \
    libhttp-parser-dev libjemalloc-dev libmongoc-dev libbson-dev libldap2-dev  libpq5=12.* libpq-dev=12.* \
    postgresql-server-dev-12 libkrb5-dev libhiredis-dev libgrpc-dev libgrpc++-dev \
    libgrpc++1 protobuf-compiler-grpc libprotoc-dev python3-protobuf python3-jinja2 \
    libbenchmark-dev libgtest-dev ccache git
  ```

  2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DCMAKE_CXX_COMPILER=g++-8 -DCMAKE_C_COMPILER=gcc-8 -DOPEN_SOURCE_BUILD=1 -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 -DUSERVER_FEATURE_CRYPTOPP_BASE64_URL=0 -DUSERVER_FEATURE_GRPC=0 -DUSERVER_FEATURE_POSTGRESQL=0 -DUSERVER_FEATURE_MONGODB=0 -DUSE_LD=gold -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```

### Ubuntu 21.10 (Impish Indri)

1. Install the packages:
  ```
  bash
  sudo apt install cmake libboost1.74-dev libboost-program-options1.74-dev libboost-filesystem1.74-dev \
    libboost-locale1.74-dev libboost-regex1.74-dev libboost-iostreams1.74-dev libboost-thread1.74-dev \
    libev-dev zlib1g-dev \
    libcurl4-openssl-dev libcrypto++-dev libyaml-cpp-dev libssl-dev libfmt-dev libcctz-dev \
    libhttp-parser-dev libjemalloc-dev libmongoc-dev libbson-dev libldap2-dev libpq-dev \
    postgresql-server-dev-13 libkrb5-dev libhiredis-dev libgrpc-dev libgrpc++-dev \
    libgrpc++1 protobuf-compiler-grpc libprotoc-dev python3-protobuf python3-jinja2 \
    libc-ares-dev libspdlog-dev libbenchmark-dev libgtest-dev ccache git
  ```
2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DOPEN_SOURCE_BUILD=1 -DUSERVER_FEATURE_MONGODB=0 -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```

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
