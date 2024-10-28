# Build options


@anchor userver_libraries
## The list of userver libraries

userver is split into multiple CMake libraries.

| CMake target               | CMake option to enable building the library       | Component for install | Main documentation page                            |
|----------------------------|---------------------------------------------------|-----------------------|----------------------------------------------------|
| `userver::universal`       | Always on                                         | `universal`           | @ref scripts/docs/en/index.md                      |
| `userver::universal-utest` | `USERVER_FEATURE_UTEST` (`ON` by default)         | `universal`           | @ref scripts/docs/en/userver/testing.md            |
| `userver::core`            | `USERVER_FEATURE_CORE` (`ON` by default)          | `core`                | @ref scripts/docs/en/index.md                      |
| `userver::utest`           | `USERVER_FEATURE_CORE` + `USERVER_FEATURE_UTEST`  | `core`                | @ref scripts/docs/en/userver/testing.md            |
| `userver::ubench`          | `USERVER_FEATURE_CORE` + `USERVER_FEATURE_UTEST`  | `core`                | @ref scripts/docs/en/userver/testing.md            |
| `userver::grpc`            | `USERVER_FEATURE_GRPC`                            | `grpc`                | @ref scripts/docs/en/userver/grpc.md               |
| `userver::grpc-utest`      | `USERVER_FEATURE_GRPC` + `USERVER_FEATURE_UTEST`  | `grpc`                | @ref scripts/docs/en/userver/grpc.md               |
| `userver::mongo`           | `USERVER_FEATURE_MONGODB`                         | `mongo`               | @ref scripts/docs/en/userver/mongodb.md            |
| `userver::postgresql`      | `USERVER_FEATURE_POSTGRESQL`                      | `postgresql`          | @ref pg_driver                                     |
| `userver::redis`           | `USERVER_FEATURE_REDIS`                           | `redis`               | @ref scripts/docs/en/userver/redis.md              |
| `userver::clickhouse`      | `USERVER_FEATURE_CLICKHOUSE`                      | `clickhouse`          | @ref clickhouse_driver                             |
| `userver::kafka`           | `USERVER_FEATURE_KAFKA`                           | `kafka`               | @ref scripts/docs/en/userver/kafka.md              |
| `userver::kafka-utest`     | `USERVER_FEATURE_KAFKA` + `USERVER_FEATURE_UTEST` | `kafka`               | @ref scripts/docs/en/userver/kafka.md              |
| `userver::rabbitmq`        | `USERVER_FEATURE_RABBITMQ`                        | `rabbitmq`            | @ref rabbitmq_driver                               |
| `userver::mysql`           | `USERVER_FEATURE_MYSQL`                           | `mysql`               | @ref scripts/docs/en/userver/mysql/mysql_driver.md |
| `userver::rocks`           | `USERVER_FEATURE_ROCKS`                           | `rocks`               | TODO                                               |
| `userver::ydb`             | `USERVER_FEATURE_YDB`                             | `ydb`                 | @ref scripts/docs/en/userver/ydb.md                |
| `userver::otlp`            | `USERVER_FEATURE_OTLP`                            | `otlp`                | @ref opentelemetry "OpenTelemetry Protocol"        |

Make sure to:

1. Enable the CMake options to build the libraries you need
2. Link against the libraries

The details vary depending on the method of building userver:

* `add_subdirectory(userver)` as used in @ref service_templates "service templates"
* @ref userver_install "userver install"
* @ref userver_cpm "CPM"
* @ref userver_conan "Conan"


@anchor cmake_options
## CMake options

The following CMake options are used by userver:

| Option                                 | Description                                                                                                     | Default                                                |
|----------------------------------------|-----------------------------------------------------------------------------------------------------------------|--------------------------------------------------------|
| USERVER_FEATURE_CORE                   | Provide a core library with coroutines, otherwise build only `userver::universal`                               | ON                                                     |
| USERVER_FEATURE_MONGODB                | Provide asynchronous driver for MongoDB                                                                         | ${USERVER_IS_THE_ROOT_PROJECT} AND x86\* AND NOT \*BSD |
| USERVER_FEATURE_POSTGRESQL             | Provide asynchronous driver for PostgreSQL                                                                      | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_REDIS                  | Provide asynchronous driver for Redis                                                                           | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_CLICKHOUSE             | Provide asynchronous driver for ClickHouse                                                                      | ${USERVER_IS_THE_ROOT_PROJECT} AND x86\*               |
| USERVER_FEATURE_GRPC                   | Provide asynchronous driver for gRPC                                                                            | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_KAFKA                  | Provide asynchronous driver for Apache Kafka                                                                    | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_RABBITMQ               | Provide asynchronous driver for RabbitMQ (AMQP 0-9-1)                                                           | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_MYSQL                  | Provide asynchronous driver for MySQL/MariaDB                                                                   | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_ROCKS                  | Provide asynchronous driver for RocksDB                                                                         | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_YDB                    | Provide asynchronous driver for YDB                                                                             | ${USERVER_IS_THE_ROOT_PROJECT} AND C++ standard >= 20  |
| USERVER_FEATURE_OTLP                   | Provide Logger for OpenTelemetry protocol                                                                       | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_UTEST                  | Provide 'utest' and 'ubench' for unit testing and benchmarking coroutines                                       | ${USERVER_FEATURE_CORE}                                |
| USERVER_FEATURE_CRYPTOPP_BLAKE2        | Provide wrappers for blake2 algorithms of crypto++                                                              | ON                                                     |
| USERVER_FEATURE_PATCH_LIBPQ            | Apply patches to the libpq (add portals support), requires libpq.a                                              | ON                                                     |
| USERVER_FEATURE_CRYPTOPP_BASE64_URL    | Provide wrappers for Base64 URL decoding and encoding algorithms of crypto++                                    | ON                                                     |
| USERVER_FEATURE_REDIS_HI_MALLOC        | Provide a `hi_malloc(unsigned long)` [issue][hi_malloc] workaround                                              | OFF                                                    |
| USERVER_FEATURE_REDIS_TLS              | SSL/TLS support for Redis driver                                                                                | OFF                                                    |
| USERVER_FEATURE_STACKTRACE             | Allow capturing stacktraces using boost::stacktrace                                                             | OFF if platform is not \*BSD; ON otherwise             |
| USERVER_FEATURE_JEMALLOC               | Use jemalloc memory allocator                                                                                   | ON                                                     |
| USERVER_FEATURE_DWCAS                  | Require double-width compare-and-swap                                                                           | ON                                                     |
| USERVER_FEATURE_TESTSUITE              | Enable functional tests via testsuite                                                                           | ON                                                     |
| USERVER_FEATURE_GRPC_CHANNELZ          | Enable Channelz for gRPC                                                                                        | ON for "sufficiently new" gRPC versions                |
| USERVER_CHECK_PACKAGE_VERSIONS         | Check package versions                                                                                          | ON                                                     |
| USERVER_SANITIZE                       | Build with sanitizers support, allows combination of values via 'val1 val2'                                     | ''                                                     |
| USERVER_SANITIZE_BLACKLIST             | Path to file that is passed to the -fsanitize-blacklist option                                                  | ''                                                     |
| USERVER_USE_LD                         | Linker to use, e.g. 'gold' or 'lld'                                                                             | ''                                                     |
| USERVER_LTO                            | Use link time optimizations                                                                                     | OFF                                                    |
| USERVER_LTO_CACHE                      | Use LTO cache if present, disable for benchmarking build times                                                  | ON                                                     |
| USERVER_LTO_CACHE_DIR                  | LTO cache directory                                                                                             | `${CMAKE_CURRENT_BINARY_DIR}/.ltocache`                |
| USERVER_LTO_CACHE_SIZE_MB              | LTO cache size limit in MB                                                                                      | 6000                                                   |
| USERVER_USE_CCACHE                     | Use ccache if present, disable for benchmarking build times                                                     | ON                                                     |
| USERVER_COMPILATION_TIME_TRACE         | Generate Clang compilation time trace                                                                           | OFF                                                    |
| USERVER_NO_WERROR                      | Do not treat warnings as errors                                                                                 | ON                                                     |
| USERVER_PYTHON_PATH                    | Path to the python3 binary for use in testsuite tests                                                           | python3                                                |
| USERVER_FEATURE_ERASE_LOG_WITH_LEVEL   | Logs of this and below levels are removed from binary. Possible values: trace, info, debug, warning, error      | OFF                                                    |
| USERVER_PIP_USE_SYSTEM_PACKAGES        | Use system python packages inside venv. Useful for Docker, CI and other controlled environments                 | OFF                                                    |
| USERVER_PIP_OPTIONS                    | Options for all pip calls. Useful for `--no                                                                     | ''                                                     |
| USERVER_DOWNLOAD_PACKAGES              | Download missing third party packages and use the downloaded versions                                           | ON                                                     |
| USERVER_DOWNLOAD_PACKAGE_BROTLI        | Download and setup Brotli if no Brotli of matching version was found                                            | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_CARES         | Download and setup c-ares if no c-ares of matching version was found                                            | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_CCTZ          | Download and setup cctz if no cctz of matching version was found                                                | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP | Download and setup clickhouse-cpp                                                                               | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_CRYPTOPP      | Download and setup CryptoPP if no CryptoPP of matching version was found                                        | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_CURL          | Download and setup libcurl if no libcurl of matching version was found                                          | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_FMT           | Download and setup Fmt if no Fmt of matching version was found                                                  | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_GBENCH        | Download and setup gbench if no gbench of matching version was found                                            | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_GRPC          | Download and setup gRPC if no gRPC of matching version was found                                                | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_GTEST         | Download and setup gtest if no gtest of matching version was found                                              | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_PROTOBUF      | Download and setup Protobuf if no Protobuf of matching version was found                                        | ${USERVER_DOWNLOAD_PACKAGE_GRPC}                       |
| USERVER_DOWNLOAD_PACKAGE_KAFKA         | Download and setup librdkafka if no librdkafka matching version was found                                       | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_YDBCPPSDK     | Download and setup ydb-cpp-sdk if no ydb-cpp-sdk of matching version was found                                  | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_FORCE_DOWNLOAD_PACKAGES        | Download all possible third-party packages even if there is an installed system package                         | OFF                                                    |
| USERVER_FORCE_DOWNLOAD_PROTOBUF        | Download Protobuf even if there is an installed system package                                                  | ${USERVER_FORCE_DOWNLOAD_PACKAGES}                     |
| USERVER_FORCE_DOWNLOAD_GRPC            | Download gRPC even if there is an installed system package                                                      | ${USERVER_FORCE_DOWNLOAD_PACKAGES}                     |
| USERVER_INSTALL                        | Build userver for further installation                                                                          | OFF                                                    |
| USERVER_IS_THE_ROOT_PROJECT            | Contributor mode: build userver tests, samples and helper tools                                                 | auto-detects if userver is the top level project       |
| USERVER_GOOGLE_COMMON_PROTOS_TARGET    | Name of cmake target preparing google common proto library                                                      | Builds userver-api-common-protos                       |
| USERVER_GOOGLE_COMMON_PROTOS           | Path to the folder with google common proto files                                                               | Downloads automatically                                |
| USERVER_PG_SERVER_INCLUDE_DIR          | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL server headers", e.g. /usr/include/postgresql/15/server  | autodetected                                           |
| USERVER_PG_SERVER_LIBRARY_DIR          | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL server libraries", e.g. /usr/lib/postgresql/15/lib       | autodetected                                           |
| USERVER_PG_INCLUDE_DIR                 | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL libpq headers", e.g. /usr/local/include                  | autodetected                                           |
| USERVER_PG_LIBRARY_DIR                 | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL libpq libraries", e.g. /usr/local/lib                    | autodetected                                           |
| USERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB   | Allows mysql driver to leak memory instead of aborting in some rare cases when linked against libmariadb3<3.3.4 | OFF                                                    |
| USERVER_DISABLE_PHDR_CACHE             | Disable caching of dl_phdr_info items, which interferes with dlopen                                             | OFF                                                    |
| USERVER_DISABLE_RSEQ_ACCELERATION      | Disable rseq-based optimizations, which may not work depending on kernel/glibc/distro/etc version               | OFF for x86 Linux, ON otherwise                        |
| USERVER_FEATURE_UBOOST_CORO            | Build with vendored version of Boost.context and Boost.coroutine2, is needed for sanitizers builds              | OFF for arm64 macOS, ON otherwise                      |
| USERVER_GENERATE_PROTOS_AT_CONFIGURE   | Run protoc at CMake Configure time for better IDE integration                                                   | OFF for downloaded Protobuf, ON otherwise              |

[hi_malloc]: https://bugs.launchpad.net/ubuntu/+source/hiredis/+bug/1888025

To explicitly specialize the compiler use the cmake options `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`.
For example to use clang-12 compiler install it and add the following options to cmake:
`-DCMAKE_CXX_COMPILER=clang++-12 -DCMAKE_C_COMPILER=clang-12`

@warning Using LTO can lead to [some problems](https://github.com/userver-framework/userver/issues/242). We don't recommend using `USERVER_LTO`.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/build/dependencies.md | @ref scripts/docs/en/userver/build/userver.md ⇨
@htmlonly </div> @endhtmlonly
