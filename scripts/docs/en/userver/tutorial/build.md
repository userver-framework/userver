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
For example to use clang-12 compiler install it and add the following options to cmake:
`-DCMAKE_CXX_COMPILER=clang++-12 -DCMAKE_C_COMPILER=clang-12`

Prefer avoiding Boost versions that are affected by the bug https://github.com/boostorg/lockfree/issues/59 (1.71 and above).


## Installation instructions 


### Ubuntu 20.04 (Focal Fossa)

1. Install the build and tests dependencies from ubuntu-20.04.md file:
  ```
  bash
  sudo apt install --allow-downgrades -y $(cat scripts/docs/en/deps/ubuntu-20.04.md | tr '\n' ' ')
  ```
2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DOPEN_SOURCE_BUILD=1 -DUSERVER_FEATURE_MONGODB=0 -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 -DUSERVER_FEATURE_REDIS_HI_MALLOC=1 -DUSE_LD=gold -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```


### Ubuntu 18.04 (Bionic Beaver)

1. Install the build and tests dependencies from ubuntu-18.04.md file:
  ```
  bash
  sudo apt install --allow-downgrades -y $(cat scripts/docs/en/deps/ubuntu-20.04.md | tr '\n' ' ')
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

1. Install the build and tests dependencies from ubuntu-21.10.md file:
  ```
  bash
  sudo apt install $(cat scripts/docs/en/deps/ubuntu-21.10.md | tr '\n' ' ')
  ```
2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DOPEN_SOURCE_BUILD=1 -DUSERVER_FEATURE_MONGODB=0 -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```


### MacOS

MacOS recommended only for development as it may have performance issues in some cases. 
At least MacOS 10.15 required with [Xcode](https://apps.apple.com/us/app/xcode/id497799835) and [Homebrew](https://brew.sh/).

Follow the cmake hints for installation of required packets and experiment with the cmake options.


### Other POSIX based platforms

userver works well on modern POSIX platforms. Follow the cmake hints for installation of required packets and experiment with the cmake options.

Feel free to provide a PR with instructions for your favorite platform at https://github.com/userver-framework/userver.

## Run tests
```
bash
cd build_release
ctest -V
```
