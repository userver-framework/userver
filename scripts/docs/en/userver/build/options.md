# Build options


@anchor userver_libraries
## The list of userver libraries

userver is split into multiple CMake libraries.

| CMake target               | CMake option to enable building the library       | Component for install | Main documentation page                                   |
|----------------------------|---------------------------------------------------|-----------------------|-----------------------------------------------------------|
| `userver::universal`       | Always on                                         | `universal`           | @ref scripts/docs/en/index.md                             |
| `userver::universal-utest` | `USERVER_FEATURE_UTEST` (`ON` by default)         | `universal`           | @ref scripts/docs/en/userver/testing.md                   |
| `userver::core`            | `USERVER_FEATURE_CORE` (`ON` by default)          | `core`                | @ref scripts/docs/en/index.md                             |
| `userver::utest`           | `USERVER_FEATURE_CORE` + `USERVER_FEATURE_UTEST`  | `core`                | @ref scripts/docs/en/userver/testing.md                   |
| `userver::ubench`          | `USERVER_FEATURE_CORE` + `USERVER_FEATURE_UTEST`  | `core`                | @ref scripts/docs/en/userver/testing.md                   |
| `userver::chaotic`         | `USERVER_FEATURE_CHAOTIC` (`ON` by default)       | `chaotic`             | @ref scripts/docs/en/userver/chaotic.md                   |
| `userver::grpc`            | `USERVER_FEATURE_GRPC`                            | `grpc`                | @ref scripts/docs/en/userver/grpc.md                      |
| `userver::grpc-utest`      | `USERVER_FEATURE_GRPC` + `USERVER_FEATURE_UTEST`  | `grpc`                | @ref scripts/docs/en/userver/grpc.md                      |
| `userver::mongo`           | `USERVER_FEATURE_MONGODB`                         | `mongo`               | @ref scripts/docs/en/userver/mongodb.md                   |
| `userver::postgresql`      | `USERVER_FEATURE_POSTGRESQL`                      | `postgresql`          | @ref pg_driver                                            |
| `userver::redis`           | `USERVER_FEATURE_REDIS`                           | `redis`               | @ref scripts/docs/en/userver/redis.md                     |
| `userver::redis-utest`     | `USERVER_FEATURE_REDIS` + `USERVER_FEATURE_UTEST` | `redis`               | @ref scripts/docs/en/userver/redis.md                     |
| `userver::clickhouse`      | `USERVER_FEATURE_CLICKHOUSE`                      | `clickhouse`          | @ref clickhouse_driver                                    |
| `userver::kafka`           | `USERVER_FEATURE_KAFKA`                           | `kafka`               | @ref scripts/docs/en/userver/kafka.md                     |
| `userver::kafka-utest`     | `USERVER_FEATURE_KAFKA` + `USERVER_FEATURE_UTEST` | `kafka`               | @ref scripts/docs/en/userver/kafka.md                     |
| `userver::rabbitmq`        | `USERVER_FEATURE_RABBITMQ`                        | `rabbitmq`            | @ref rabbitmq_driver                                      |
| `userver::mysql`           | `USERVER_FEATURE_MYSQL`                           | `mysql`               | @ref scripts/docs/en/userver/mysql/mysql_driver.md        |
| `userver::rocks`           | `USERVER_FEATURE_ROCKS`                           | `rocks`               | TODO                                                      |
| `userver::ydb`             | `USERVER_FEATURE_YDB`                             | `ydb`                 | @ref scripts/docs/en/userver/ydb.md                       |
| `userver::otlp`            | `USERVER_FEATURE_OTLP`                            | `otlp`                | @ref opentelemetry "OpenTelemetry Protocol"               |
| `userver::s3api`           | `USERVER_FEATURE_S3API`                           | `s3api`               | @ref scripts/docs/en/userver/libraries/s3api.md           |
| `userver::easy`            | `USERVER_FEATURE_EASY`                            | `easy`                | @ref scripts/docs/en/userver/libraries/easy.md            |
| `userver::grpc-reflection` | `USERVER_FEATURE_GRPC_REFLECTION`                 | `grpc-reflection`     | @ref scripts/docs/en/userver/libraries/grpc-reflection.md |

Make sure to:

1. Enable the CMake options to build the libraries you need
2. Link against the libraries

The details vary depending on the method of building userver:

* `find_package` + CPM + CMake Presets as used in @ref service_templates "service templates"
* @ref userver_install "userver install"
* @ref userver_cpm "CPM"
* @ref userver_conan "Conan"


@anchor cmake_options
## CMake options

To explicitly specialize the compiler, use the cmake options `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`.
For example, to use `clang-12` compiler, install it and add the following options to cmake Configure:

```shell
cmake ... -DCMAKE_C_COMPILER=clang-12 -DCMAKE_CXX_COMPILER=clang++-12
```

The exact format of setting cmake options varies depending on the method of building userver:

* `find_package` + CPM + CMake Presets as used in @ref service_templates "service templates"
* @ref userver_install "userver install"

### CMake options for selecting userver libraries to build

| Option                            | Description                                                                       | Default                                                     |
|-----------------------------------|-----------------------------------------------------------------------------------|-------------------------------------------------------------|
| `USERVER_FEATURE_CORE`            | Provide a core library with coroutines, otherwise build only `userver::universal` | `ON`                                                        |
| `USERVER_FEATURE_CHAOTIC`         | Provide chaotic-codegen for jsonschema                                            | `ON`                                                        |
| `USERVER_FEATURE_UTEST`           | Provide `utest` and `ubench` for unit testing and benchmarking coroutines         | `${USERVER_FEATURE_CORE}`                                   |
| `USERVER_FEATURE_TESTSUITE`       | Enable functional tests via testsuite                                             | `ON`                                                        |
| `USERVER_FEATURE_MONGODB`         | Provide asynchronous driver for MongoDB                                           | `${USERVER_BUILD_ALL_COMPONENTS}`                NOT `*BSD` |
| `USERVER_FEATURE_POSTGRESQL`      | Provide asynchronous driver for PostgreSQL                                        | `${USERVER_BUILD_ALL_COMPONENTS}`                           |
| `USERVER_FEATURE_REDIS`           | Provide asynchronous driver for Redis                                             | `${USERVER_BUILD_ALL_COMPONENTS}`                           |
| `USERVER_FEATURE_CLICKHOUSE`      | Provide asynchronous driver for ClickHouse                                        | `${USERVER_BUILD_ALL_COMPONENTS}`                           |
| `USERVER_FEATURE_GRPC`            | Provide asynchronous driver for gRPC                                              | `${USERVER_BUILD_ALL_COMPONENTS}`                           |
| `USERVER_FEATURE_KAFKA`           | Provide asynchronous driver for Apache Kafka                                      | `${USERVER_BUILD_ALL_COMPONENTS}`                           |
| `USERVER_FEATURE_RABBITMQ`        | Provide asynchronous driver for RabbitMQ (AMQP 0-9-1)                             | `${USERVER_BUILD_ALL_COMPONENTS}`                           |
| `USERVER_FEATURE_MYSQL`           | Provide asynchronous driver for MySQL/MariaDB                                     | `${USERVER_BUILD_ALL_COMPONENTS}`                           |
| `USERVER_FEATURE_ROCKS`           | Provide asynchronous driver for RocksDB                                           | `${USERVER_BUILD_ALL_COMPONENTS}`                           |
| `USERVER_FEATURE_YDB`             | Provide asynchronous driver for YDB                                               | `${USERVER_BUILD_ALL_COMPONENTS}` AND C++ standard >= 20    |
| `USERVER_FEATURE_OTLP`            | Provide Logger for OpenTelemetry protocol                                         | `${USERVER_BUILD_ALL_COMPONENTS}`                           |
| `USERVER_FEATURE_GRPC_REFLECTION` | Provide reflection service for gRPC                                               | `${USERVER_BUILD_ALL_COMPONENTS}`                           |
| `USERVER_FEATURE_S3API`           | Provide S3 client for gRPC                                                        | `${USERVER_BUILD_ALL_COMPONENTS}`                           |

### CMake options for building everything

@see @ref scripts/docs/en/userver/build/userver.md

| Option                         | Description                                                                                          | Default |
|--------------------------------|------------------------------------------------------------------------------------------------------|---------|
| `USERVER_BUILD_ALL_COMPONENTS` | Build all userver libraries compatible with the host except for ones disabled by `USERVER_FEATURE_*` | `OFF`   |
| `USERVER_BUILD_TESTS`          | Build unit tests, functional tests and benchmarks for userver itself                                 | `OFF`   |
| `USERVER_BUILD_SAMPLES`        | Build userver samples                                                                                | `OFF`   |

### CMake options for features that are unavailable for some platforms

@see @ref scripts/docs/en/userver/build/dependencies.md

| Option                                 | Description                                                                                                       | Default                                     |
|----------------------------------------|-------------------------------------------------------------------------------------------------------------------|---------------------------------------------|
| `USERVER_FEATURE_CRYPTOPP_BLAKE2`      | Provide wrappers for blake2 algorithms of crypto++                                                                | `ON`                                        |
| `USERVER_FEATURE_PATCH_LIBPQ`          | Apply patches to the libpq (add portals support), requires `libpq.a`                                              | `ON`                                        |
| `USERVER_FEATURE_CRYPTOPP_BASE64_URL`  | Provide wrappers for Base64 URL decoding and encoding algorithms of crypto++                                      | `ON`                                        |
| `USERVER_FEATURE_REDIS_HI_MALLOC`      | Provide a `hi_malloc(unsigned long)` [issue][hi_malloc] workaround                                                | `OFF`                                       |
| `USERVER_FEATURE_REDIS_TLS`            | SSL/TLS support for Redis driver                                                                                  | `OFF`                                       |
| `USERVER_FEATURE_STACKTRACE`           | Allow capturing stacktraces using `boost::stacktrace`                                                             | `ON` except for macOS, `*BSD` and old Boost |
| `USERVER_FEATURE_JEMALLOC`             | Use jemalloc memory allocator                                                                                     | `ON`                                        |
| `USERVER_FEATURE_DWCAS`                | Require double-width compare-and-swap                                                                             | `ON`                                        |
| `USERVER_FEATURE_GRPC_CHANNELZ`        | Enable Channelz for gRPC                                                                                          | `ON` for "sufficiently new" gRPC versions   |
| `USERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB` | Allows mysql driver to leak memory instead of aborting in some rare cases when linked against `libmariadb3<3.3.4` | `OFF`                                       |
| `USERVER_DISABLE_PHDR_CACHE`           | Disable caching of `dl_phdr_info` items, which interferes with `dlopen`                                           | `OFF`                                       |
| `USERVER_DISABLE_RSEQ_ACCELERATION`    | Disable rseq-based optimizations, which may not work depending on kernel/glibc/distro/etc version                 | `OFF` for x86 Linux, `ON` otherwise         |
| `USERVER_FEATURE_UBOOST_CORO`          | Build with vendored version of Boost.context and Boost.coroutine2, is needed for sanitizers builds                | `OFF` for arm64 macOS, `ON` otherwise       |

[hi_malloc]: https://bugs.launchpad.net/ubuntu/+source/hiredis/+bug/1888025

### CMake options for downloading userver dependencies

| Option                                   | Description                                                                             | Default                              |
|------------------------------------------|-----------------------------------------------------------------------------------------|--------------------------------------|
| `USERVER_DOWNLOAD_PACKAGES`              | Download missing third party packages and use the downloaded versions                   | `ON`                                 |
| `USERVER_DOWNLOAD_PACKAGE_BROTLI`        | Download and setup Brotli if no Brotli of matching version was found                    | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_DOWNLOAD_PACKAGE_CARES`         | Download and setup c-ares if no c-ares of matching version was found                    | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_DOWNLOAD_PACKAGE_CCTZ`          | Download and setup cctz if no cctz of matching version was found                        | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP` | Download and setup clickhouse-cpp                                                       | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_DOWNLOAD_PACKAGE_CRYPTOPP`      | Download and setup CryptoPP if no CryptoPP of matching version was found                | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_DOWNLOAD_PACKAGE_CURL`          | Download and setup libcurl if no libcurl of matching version was found                  | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_DOWNLOAD_PACKAGE_FMT`           | Download and setup Fmt if no Fmt of matching version was found                          | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_DOWNLOAD_PACKAGE_GBENCH`        | Download and setup gbench if no gbench of matching version was found                    | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_DOWNLOAD_PACKAGE_GRPC`          | Download and setup gRPC if no gRPC of matching version was found                        | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_DOWNLOAD_PACKAGE_GTEST`         | Download and setup gtest if no gtest of matching version was found                      | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_DOWNLOAD_PACKAGE_PROTOBUF`      | Download and setup Protobuf if no Protobuf of matching version was found                | `${USERVER_DOWNLOAD_PACKAGE_GRPC}`   |
| `USERVER_DOWNLOAD_PACKAGE_KAFKA`         | Download and setup librdkafka if no librdkafka matching version was found               | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_DOWNLOAD_PACKAGE_YDBCPPSDK`     | Download and setup ydb-cpp-sdk if no ydb-cpp-sdk of matching version was found          | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_FORCE_DOWNLOAD_PACKAGES`        | Download all possible third-party packages even if there is an installed system package | `OFF`                                |
| `USERVER_FORCE_DOWNLOAD_ABSEIL`          | Download Abseil even if it exists in a system                                           | `${USERVER_DOWNLOAD_PACKAGES}`       |
| `USERVER_FORCE_DOWNLOAD_CURL`            | Download Curl even if it exists in a system                                             | `${USERVER_FORCE_DOWNLOAD_PACKAGES}` |
| `USERVER_FORCE_DOWNLOAD_PROTOBUF`        | Download Protobuf even if there is an installed system package                          | `${USERVER_FORCE_DOWNLOAD_PACKAGES}` |
| `USERVER_FORCE_DOWNLOAD_GRPC`            | Download gRPC even if there is an installed system package                              | `${USERVER_FORCE_DOWNLOAD_PACKAGES}` |

### CMake options for paths to dependencies

| Option                                | Description                                                                                                    | Default                             |
|---------------------------------------|----------------------------------------------------------------------------------------------------------------|-------------------------------------|
| `USERVER_PYTHON_PATH`                 | Path to the python3 binary for use in testsuite tests                                                          | `python3`                           |
| `USERVER_PG_SERVER_INCLUDE_DIR`       | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL server headers", e.g. /usr/include/postgresql/15/server | autodetected                        |
| `USERVER_PG_SERVER_LIBRARY_DIR`       | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL server libraries", e.g. /usr/lib/postgresql/15/lib      | autodetected                        |
| `USERVER_PG_INCLUDE_DIR`              | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL libpq headers", e.g. /usr/local/include                 | autodetected                        |
| `USERVER_PG_LIBRARY_DIR`              | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL libpq libraries", e.g. /usr/local/lib                   | autodetected                        |
| `USERVER_GOOGLE_COMMON_PROTOS_TARGET` | Name of cmake target preparing google common proto library                                                     | Builds `userver-api-common-protos`  |
| `USERVER_GOOGLE_COMMON_PROTOS`        | Path to the folder with google common proto files                                                              | Downloads automatically             |
| `USERVER_GRPC_PROTO_TARGET`           | Name of cmake target preparing grpc proto library                                                              | Builds `userver-grpc-proto-contrib` |
| `USERVER_GRPC_PROTO`                  | Path to the folder with grpc proto files                                                                       | Downloads automatically             |

### CMake options for various compilation modes

| Option                                 | Description                                                                                                 | Default                                                     |
|----------------------------------------|-------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------|
| `USERVER_CHECK_PACKAGE_VERSIONS`       | Check package versions                                                                                      | `ON`                                                        |
| `USERVER_SANITIZE`                     | Build with sanitizers support, allows combination of values via 'val1 val2'. Available: `addr`, `mem`, `ub` | (no sanitizers)                                             |
| `USERVER_SANITIZE_BLACKLIST`           | Path to file that is passed to the -fsanitize-blacklist option                                              | (no blacklist)                                              |
| `USERVER_USE_LD`                       | Linker to use, e.g. `gold` or `lld`                                                                         | `lld` for Clang, system linker otherwise (typically GNU ld) |
| `USERVER_USE_STATIC_LIBS`              | Tries to find all dependencies as static libraries                                                          | `ON` for `Clang` not older than `14` version                |
| `USERVER_USE_CCACHE`                   | Use ccache if present, disable for benchmarking build times                                                 | `ON`                                                        |
| `USERVER_LTO`                          | Use link time optimizations (SEE NOTE BELOW)                                                                | `OFF`                                                       |
| `USERVER_LTO_CACHE`                    | Use LTO cache if present, disable for benchmarking build times                                              | `ON`                                                        |
| `USERVER_LTO_CACHE_DIR`                | LTO cache directory                                                                                         | `${CMAKE_CURRENT_BINARY_DIR}/.ltocache`                     |
| `USERVER_LTO_CACHE_SIZE_MB`            | LTO cache size limit in MB                                                                                  | `6000`                                                      |
| `USERVER_PGO_GENERATE`                 | Generate PGO profile                                                                                        | `OFF`                                                       |
| `USERVER_PGO_USE`                      | Path to PGO profile file                                                                                    | (no path)                                                   |
| `USERVER_COMPILATION_TIME_TRACE`       | Generate Clang compilation time trace                                                                       | `OFF`                                                       |
| `USERVER_NO_WERROR`                    | Do not treat warnings as errors                                                                             | `ON`                                                        |
| `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` | Logs of this and below levels are removed from binary. Possible values: trace, info, debug, warning, error  | `OFF`                                                       |
| `USERVER_PIP_USE_SYSTEM_PACKAGES`      | Use system python packages inside venv. Useful for Docker, CI and other controlled environments             | `OFF`                                                       |
| `USERVER_PIP_OPTIONS`                  | Options for all pip calls. Useful for passing `--no-index` option to prevent network usage                  | (no options)                                                |
| `USERVER_INSTALL`                      | Build userver for further installation                                                                      | `OFF`                                                       |
| `USERVER_CONAN`                        | Build userver using Conan packages                                                                          | `ON` if build is launched from Conan, `OFF` otherwise       |
| `USERVER_CHAOTIC_FORMAT`               | Whether to format generated code if FORMAT option is missing                                                | `ON`                                                        |

@warning Using LTO can lead to [some problems](https://github.com/userver-framework/userver/issues/242). We don't recommend using `USERVER_LTO`.

### CMake options for static linking

It is possible to build userver based services with libraries statically linked in.

@warning The support is platform dependant, as a result some libraries on some platforms may linked dynamically. Feel free to provide a PR to support your favourite platform.

Userver does not build dynamic libraries itself, but most of its dependencies do. CMake (by default) prefers dynamic libraries on Unix-like operating systems.

- Use `USERVER_USE_STATIC_LIBS=ON` CMake option to prefer static libraries for all userver dependencies (ON by default for "sufficiently new" `Clang` versions).
With the option, CMake tries to find all dependencies as static libraries (and dependencies of dependencies), fallbacks to dynamic libraries when no static found.
- To link statically with `libstdc++` or `libc++`, use `CMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc"`.
- To force fully static binary (with statically linked `libc`), use `CMAKE_EXE_LINKER_FLAGS="-static"`. In such case, all dependencies must be provided as static libraries. Also `userver` must be build with `USERVER_DISABLE_PHDR_CACHE=ON` (without this flag, it can lead to endless memory allocation).

Some dependencies usually should be build from source for statically linked executable:
1. `Curl`. Use `USERVER_FORCE_DOWNLOAD_CURL=ON` to download and build Curl from source.
2. `cctz`, `yaml-cpp`, `fmt` often have no static libraries in their packages, so they should be build from source and installed in your host system (for instance, in `/usr/local`).

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/build/dependencies.md | @ref scripts/docs/en/userver/build/userver.md ⇨
@htmlonly </div> @endhtmlonly
