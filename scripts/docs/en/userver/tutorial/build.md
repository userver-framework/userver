## Configure and Build

## CMake options

The following options could be used to control `cmake`:

| Option                                 | Description                                                                  | Default                                          |
|----------------------------------------|------------------------------------------------------------------------------|--------------------------------------------------|
| USERVER_FEATURE_MONGODB                | Provide asynchronous driver for MongoDB                                      | ON if platform is x86\*; OFF otherwise           |
| USERVER_FEATURE_POSTGRESQL             | Provide asynchronous driver for PostgreSQL                                   | ON                                               |
| USERVER_FEATURE_REDIS                  | Provide asynchronous driver for Redis                                        | ON                                               |
| USERVER_FEATURE_CLICKHOUSE             | Provide asynchronous driver for ClickHouse                                   | ON if platform is x86\*; OFF otherwise           |
| USERVER_FEATURE_GRPC                   | Provide asynchronous driver for gRPC                                         | ON                                               |
| USERVER_FEATURE_RABBITMQ               | Provide asynchronous driver for RabbitMQ (AMQP 0-9-1)                        | ${USERVER_OPEN_SOURCE_BUILD}                     |
| USERVER_FEATURE_UNIVERSAL              | Provide a universal utilities library that does not use coroutines           | ON                                               |
| USERVER_FEATURE_UTEST                  | Provide 'utest' and 'ubench' for unit testing and benchmarking coroutines    | ON                                               |
| USERVER_FEATURE_CRYPTOPP_BLAKE2        | Provide wrappers for blake2 algorithms of crypto++                           | ON                                               |
| USERVER_FEATURE_PATCH_LIBPQ            | Apply patches to the libpq (add portals support), requires libpq.a           | ON                                               |
| USERVER_FEATURE_CRYPTOPP_BASE64_URL    | Provide wrappers for Base64 URL decoding and encoding algorithms of crypto++ | ON                                               |
| USERVER_FEATURE_SPDLOG_TCP_SINK        | Use tcp_sink.h of the spdlog library for testing logs                        | ON                                               |
| USERVER_FEATURE_REDIS_HI_MALLOC        | Provide a `hi_malloc(unsigned long)` [issue][hi_malloc] workaround           | OFF                                              |
| USERVER_FEATURE_REDIS_TLS              | SSL/TLS support for Redis driver                                             | OFF                                              |
| USERVER_FEATURE_STACKTRACE             | Allow capturing stacktraces using boost::stacktrace                          | ON                                               |
| USERVER_FEATURE_JEMALLOC               | Use jemalloc memory allocator                                                | ON                                               |
| USERVER_FEATURE_DWCAS                  | Require double-width compare-and-swap                                        | ON                                               |
| USERVER_FEATURE_TESTSUITE              | Enable functional tests via testsuite                                        | ON                                               |
| USERVER_FEATURE_GRPC_CHANNELZ          | Enable Channelz for gRPC                                                     | ${USERVER_OPEN_SOURCE_BUILD}                     |
| USERVER_CHECK_PACKAGE_VERSIONS         | Check package versions                                                       | ON                                               |
| USERVER_SANITIZE                       | Build with sanitizers support, allows combination of values via 'val1 val2'  | ''                                               |
| USERVER_SANITIZE_BLACKLIST             | Path to file that is passed to the -fsanitize-blacklist option               | ''                                               |
| USERVER_USE_LD                         | Linker to use, e.g. 'gold' or 'lld'                                          | ''                                               |
| USERVER_LTO                            | Use link time optimizations                                                  | OFF for Debug build, ON for all the other builds |
| USERVER_OPEN_SOURCE_BUILD              | Do not use internal Yandex packages                                          | auto-detects                                     |
| USERVER_NO_WERROR                      | Do not treat warnings as errors                                              | ${USERVER_OPEN_SOURCE_BUILD}                     |
| USERVER_DOWNLOAD_PACKAGES              | Download missing third party packages and use the downloaded versions        | ${USERVER_OPEN_SOURCE_BUILD}                     |
| USERVER_DOWNLOAD_PACKAGE_CARES         | Download and setup c-ares if no c-ares of matching version was found         | ${USERVER_DOWNLOAD_PACKAGES}                     |
| USERVER_DOWNLOAD_PACKAGE_CCTZ          | Download and setup cctz if no cctz of matching version was found             | ${USERVER_DOWNLOAD_PACKAGES}                     |
| USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP | Download and setup clickhouse-cpp                                            | ${USERVER_DOWNLOAD_PACKAGES}                     |
| USERVER_DOWNLOAD_PACKAGE_CURL          | Download and setup libcurl if no libcurl of matching version was found       | ${USERVER_DOWNLOAD_PACKAGES}                     |
| USERVER_DOWNLOAD_PACKAGE_FMT           | Download and setup Fmt if no Fmt of matching version was found               | ${USERVER_DOWNLOAD_PACKAGES}                     |
| USERVER_DOWNLOAD_PACKAGE_GTEST         | Download and setup gtest if no gtest of matching version was found           | ${USERVER_DOWNLOAD_PACKAGES}                     |
| USERVER_DOWNLOAD_PACKAGE_GBENCH        | Download and setup gbench if no gbench of matching version was found         | ${USERVER_DOWNLOAD_PACKAGES}                     |
| USERVER_DOWNLOAD_PACKAGE_SPDLOG        | Download and setup Spdlog if no Spdlog of matching version was found         | ${USERVER_DOWNLOAD_PACKAGES}                     |
| USERVER_DOWNLOAD_PACKAGE_CRYPTOPP      | Download and setup CryptoPP if no CryptoPP of matching version was found     | ${USERVER_DOWNLOAD_PACKAGES}                     |
| USERVER_IS_THE_ROOT_PROJECT            | Build tests, samples and helper tools                                        | auto-detects if userver is the top level project |

[hi_malloc]: https://bugs.launchpad.net/ubuntu/+source/hiredis/+bug/1888025

To explicitly specialize the compiler use the cmake options `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`.
For example to use clang-12 compiler install it and add the following options to cmake:
`-DCMAKE_CXX_COMPILER=clang++-12 -DCMAKE_C_COMPILER=clang-12`

Prefer avoiding Boost versions that are affected by the bug https://github.com/boostorg/lockfree/issues/59 (1.71 and above).


## Installation instructions

Download and extract the latest release from https://github.com/userver-framework/userver

Follow the platforms specific instructions:

### Ubuntu 22.04 (Jammy Jellyfish)

1. Install the build and test dependencies from ubuntu-22.04.md file, configure git:
  ```
  bash
  sudo apt install $(cat scripts/docs/en/deps/ubuntu-22.04.md | tr '\n' ' ')
  git config --global --add safe.directory $(pwd)/third_party/clickhouse-cpp
  ```
2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```


### Ubuntu 21.10 (Impish Indri)

1. Install the build and test dependencies from ubuntu-21.10.md file:
  ```
  bash
  sudo apt install $(cat scripts/docs/en/deps/ubuntu-21.10.md | tr '\n' ' ')
  ```
2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```

### Ubuntu 20.04 (Focal Fossa)

1. Install the build and test dependencies from ubuntu-20.04.md file:
  ```
  bash
  sudo apt install --allow-downgrades -y $(cat scripts/docs/en/deps/ubuntu-20.04.md | tr '\n' ' ')
  ```
2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 -DUSERVER_FEATURE_REDIS_HI_MALLOC=1 -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```


### Ubuntu 18.04 (Bionic Beaver)

1. Install the build and test dependencies from ubuntu-18.04.md file:
  ```
  bash
  sudo apt install --allow-downgrades -y $(cat scripts/docs/en/deps/ubuntu-18.04.md | tr '\n' ' ')
  ```

2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DCMAKE_CXX_COMPILER=g++-8 -DCMAKE_C_COMPILER=gcc-8 -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 \
        -DUSERVER_FEATURE_CRYPTOPP_BASE64_URL=0 -DUSERVER_FEATURE_GRPC=0 -DUSERVER_FEATURE_POSTGRESQL=0 \
        -DUSERVER_FEATURE_MONGODB=0 -DUSERVER_USE_LD=gold -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```

### Fedora 35

1. Install the build and test dependencies from fedora-35.md file:
  ```
  bash
  sudo dnf install -y $(cat scripts/docs/en/deps/fedora-35.md | tr '\n' ' ')
  ```

2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DUSERVER_FEATURE_STACKTRACE=0 -DUSERVER_FEATURE_PATCH_LIBPQ=0 -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```

### Fedora 36

1. Install the build and test dependencies from fedora-36.md file:
  ```
  bash
  sudo dnf install -y $(cat scripts/docs/en/deps/fedora-36.md | tr '\n' ' ')
  ```

2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DUSERVER_FEATURE_STACKTRACE=0 -DUSERVER_FEATURE_PATCH_LIBPQ=0 -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```

### Debian 11 32-bit

1. Install the build and test dependencies from debian-11x32.md file:
  ```
  bash
  sudo apt install -y $(cat scripts/docs/en/deps/debian-11x32.md | tr '\n' ' ')
  ```

2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DCMAKE_C_FLAGS='-D_FILE_OFFSET_BITS=64' -DCMAKE_CXX_FLAGS='-D_FILE_OFFSET_BITS=64' \
        -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```

### Gentoo

1. Install the build and test dependencies from gentoo.md file:
  ```
  bash
  sudo emerge --ask --update --oneshot $(cat scripts/docs/en/deps/gentoo.md | tr '\n' ' ')
  ```

2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DUSERVER_CHECK_PACKAGE_VERSIONS=0 -DUSERVER_FEATURE_GRPC=0 \
        -DUSERVER_FEATURE_CLICKHOUSE=0 -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```
  If you have multiple python version installed and get ModuleNotFoundError
  use -DPython3_EXECUTABLE="/path/to/python" (e.g. /usr/bin/python3.10)
  to choose working python version.

### Arch

1. Install the build and test dependencies from arch.md file:

    * Using an AUR helper (pikaur in this example)
```
bash
pikaur -S $(cat scripts/docs/en/deps/arch.md | tr '\n' ' ')
```
    * No AUR helper
```
bash
sudo pacman -S $(cat scripts/docs/en/deps/arch.md | grep -v -- '-git' | tr '\n' ' ')
cat scripts/docs/en/deps/arch.md | grep -- '-git' | while read ;\
  do \
    DIR=$(mktemp -d) ;\
    git clone https://aur.archlinux.org/$REPLY.git $DIR ;\
    pushd $DIR ;\
    yes|makepkg -si ;\
    popd ;\
    rm -rf $DIR ;\
  done
```

2. Build the userver:
  ```
  bash
  mkdir build_release
  cd build_release
  cmake -DUSERVER_FEATURE_PATCH_LIBPQ=0 -DCMAKE_BUILD_TYPE=Release ..
  make -j$(nproc)
  ```

### MacOS

MacOS is recommended only for development as it may have performance issues in some cases.
At least MacOS 10.15 required with [Xcode](https://apps.apple.com/us/app/xcode/id497799835) and [Homebrew](https://brew.sh/).

Start with the following command:
```
bash
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DUSERVER_NO_WERROR=1 -DUSERVER_CHECK_PACKAGE_VERSIONS=0 \
      -DUSERVER_FEATURE_REDIS_HI_MALLOC=1 \
      -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 -DUSERVER_DOWNLOAD_PACKAGE_CRYPTOPP=1 \
      -DUSERVER_FEATURE_CLICKHOUSE=0 \
      -DUSERVER_FEATURE_RABBITMQ=0 \
      -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@1.1) \
      -DUSERVER_PG_INCLUDE_DIR=$(pg_config --includedir) -DUSERVER_PG_LIBRARY_DIR=$(pg_config --libdir) \
      -DUSERVER_PG_PKGLIB_DIR=$(pg_config --pkglibdir) -DUSERVER_PG_SERVER_INCLUDE_DIR=$(pg_config --includedir-server) \
      ..
```

Follow the cmake hints for the installation of required packets and keep calling cmake with the options.

To run the tests, increase the limits of open files count via:
```
ulimit -n 4096
```

### Other POSIX based platforms

🐙 **userver** works well on modern POSIX platforms. Follow the cmake hints for the installation of required packets and experiment with the CMake options.

Feel free to provide a PR with instructions for your favorite platform at https://github.com/userver-framework/userver.


### Docker

@note Currently, only x86_64 and x86 architectures support ClickHouse and MongoDB drivers
as the native libraries for those databases do not support other architectures.
Those drivers are disabled on other architectures via CMake options.

Docker images in userver provide the following functionality:
- build and start all userver tests:
```
bash
docker-compose run --rm userver-tests
```
- build `hello_service` sample:
```
bash
docker-compose run --rm userver-service-sample
```
or
```
bash
SERVICE_NAME=hello_service docker-compose run --rm userver-service-sample
```
- execute commands in userver dev environment:
```
bash
docker-compose run --rm userver-debian bash
```


Each step of the `userver-tests` could be executed separately:

Start CMake:
```
docker-compose run --rm userver-debian bash -c 'cmake $CMAKE_OPTS -B./build -S./'
```
Build userver:
```
docker-compose run --rm userver-debian bash -c 'cd /userver/build && make -j $(nproc)'
```
Run all test:
```
docker-compose run --rm userver-debian bash -c 'cd /userver/build && ctest -V'
```


## Run framework tests
To run tests and make sure that the framework works fine use the following command:
```
bash
cd build_release
ctest -V
```


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_supported_platforms | @ref md_en_userver_beta_state ⇨
@htmlonly </div> @endhtmlonly
