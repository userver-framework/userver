## Configure and Build

## CMake options

The following CMake options are used by userver:

| Option                                 | Description                                                                                                           | Default                                                |
|----------------------------------------|-----------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------|
| USERVER_FEATURE_CORE                   | Provide a core library with coroutines, otherwise build only `userver-universal`                                      | ON                                                     |
| USERVER_FEATURE_MONGODB                | Provide asynchronous driver for MongoDB                                                                               | ${USERVER_IS_THE_ROOT_PROJECT} AND x86\* AND NOT \*BSD |
| USERVER_FEATURE_POSTGRESQL             | Provide asynchronous driver for PostgreSQL                                                                            | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_REDIS                  | Provide asynchronous driver for Redis                                                                                 | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_CLICKHOUSE             | Provide asynchronous driver for ClickHouse                                                                            | ${USERVER_IS_THE_ROOT_PROJECT} AND x86\*               |
| USERVER_FEATURE_GRPC                   | Provide asynchronous driver for gRPC                                                                                  | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_RABBITMQ               | Provide asynchronous driver for RabbitMQ (AMQP 0-9-1)                                                                 | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_MYSQL                  | Provide asynchronous driver for MySQL/MariaDB                                                                         | ${USERVER_IS_THE_ROOT_PROJECT}                         |
| USERVER_FEATURE_UTEST                  | Provide 'utest' and 'ubench' for unit testing and benchmarking coroutines                                             | ${USERVER_FEATURE_CORE}                                |
| USERVER_FEATURE_CRYPTOPP_BLAKE2        | Provide wrappers for blake2 algorithms of crypto++                                                                    | ON                                                     |
| USERVER_FEATURE_PATCH_LIBPQ            | Apply patches to the libpq (add portals support), requires libpq.a                                                    | ON                                                     |
| USERVER_FEATURE_CRYPTOPP_BASE64_URL    | Provide wrappers for Base64 URL decoding and encoding algorithms of crypto++                                          | ON                                                     |
| USERVER_FEATURE_REDIS_HI_MALLOC        | Provide a `hi_malloc(unsigned long)` [issue][hi_malloc] workaround                                                    | OFF                                                    |
| USERVER_FEATURE_REDIS_TLS              | SSL/TLS support for Redis driver                                                                                      | OFF                                                    |
| USERVER_FEATURE_STACKTRACE             | Allow capturing stacktraces using boost::stacktrace                                                                   | OFF if platform is not \*BSD; ON otherwise             |
| USERVER_FEATURE_JEMALLOC               | Use jemalloc memory allocator                                                                                         | ON                                                     |
| USERVER_FEATURE_DWCAS                  | Require double-width compare-and-swap                                                                                 | ON                                                     |
| USERVER_FEATURE_TESTSUITE              | Enable functional tests via testsuite                                                                                 | ON                                                     |
| USERVER_FEATURE_GRPC_CHANNELZ          | Enable Channelz for gRPC                                                                                              | ON for "sufficiently new" gRPC versions                |
| USERVER_CHECK_PACKAGE_VERSIONS         | Check package versions                                                                                                | ON                                                     |
| USERVER_SANITIZE                       | Build with sanitizers support, allows combination of values via 'val1 val2'                                           | ''                                                     |
| USERVER_SANITIZE_BLACKLIST             | Path to file that is passed to the -fsanitize-blacklist option                                                        | ''                                                     |
| USERVER_USE_LD                         | Linker to use, e.g. 'gold' or 'lld'                                                                                   | ''                                                     |
| USERVER_LTO                            | Use link time optimizations                                                                                           | OFF for Debug build, ON for all the other builds       |
| USERVER_LTO_CACHE                      | Use LTO cache if present, disable for benchmarking build times                                                        | ON                                                                |
| USERVER_LTO_CACHE_DIR                  | LTO cache directory                                                                                                   | `${CMAKE_CURRENT_BINARY_DIR}/.ltocache`                           |
| USERVER_LTO_CACHE_SIZE_MB              | LTO cache size limit in MB                                                                                            | 6000                                                              |
| USERVER_USE_CCACHE                     | Use ccache if present, disable for benchmarking build times                                                           | ON                                                                |
| USERVER_COMPILATION_TIME_TRACE         | Generate Clang compilation time trace                                                                                 | OFF                                                               |
| USERVER_NO_WERROR                      | Do not treat warnings as errors                                                                                       | ON                                                     |
| USERVER_PYTHON_PATH                    | Path to the python3 binary for use in testsuite tests                                                                 | python3                                                |
| USERVER_FEATURE_ERASE_LOG_WITH_LEVEL   | Logs of this and below levels are removed from binary. Possible values: trace, info, debug, warning, error            | OFF                                                    |
| USERVER_DOWNLOAD_PACKAGES              | Download missing third party packages and use the downloaded versions                                                 | ON                                                     |
| USERVER_PIP_USE_SYSTEM_PACKAGES        | Use system python packages inside virtualenv                                                                          | OFF                                                    |
| USERVER_PIP_OPTIONS                    | Options for all pip calls                                                                                             | ''                                                     |
| USERVER_DOWNLOAD_PACKAGE_CARES         | Download and setup c-ares if no c-ares of matching version was found                                                  | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_CCTZ          | Download and setup cctz if no cctz of matching version was found                                                      | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP | Download and setup clickhouse-cpp                                                                                     | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_CRYPTOPP      | Download and setup CryptoPP if no CryptoPP of matching version was found                                              | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_CURL          | Download and setup libcurl if no libcurl of matching version was found                                                | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_FMT           | Download and setup Fmt if no Fmt of matching version was found                                                        | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_GBENCH        | Download and setup gbench if no gbench of matching version was found                                                  | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_GRPC          | Download and setup gRPC if no gRPC of matching version was found                                                      | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_GTEST         | Download and setup gtest if no gtest of matching version was found                                                    | ${USERVER_DOWNLOAD_PACKAGES}                           |
| USERVER_DOWNLOAD_PACKAGE_PROTOBUF      | Download and setup Protobuf if no Protobuf of matching version was found                                              | ${USERVER_DOWNLOAD_PACKAGE_GRPC}                       |
| USERVER_FORCE_DOWNLOAD_PACKAGES        | Download all possible third-party packages even if there is an installed system package                               | OFF                                                    |
| USERVER_IS_THE_ROOT_PROJECT            | Contributor mode: build userver tests, samples and helper tools                                                       | auto-detects if userver is the top level project       |
| USERVER_GOOGLE_COMMON_PROTOS_TARGET    | Name of cmake target preparing google common proto library                                                            | Builds userver-api-common-protos                       |
| USERVER_GOOGLE_COMMON_PROTOS           | Path to the folder with google common proto files                                                                     | Downloads automatically                                |
| USERVER_PG_SERVER_INCLUDE_DIR          | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL server headers", for example /usr/include/postgresql/15/server | autodetected                                           |
| USERVER_PG_SERVER_LIBRARY_DIR          | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL server libraries", for example /usr/lib/postgresql/15/lib      | autodetected                                           |
| USERVER_PG_INCLUDE_DIR                 | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL libpq headers", for example /usr/local/include                 | autodetected                                           |
| USERVER_PG_LIBRARY_DIR                 | Path to the folder with @ref POSTGRES_LIBS "PostgreSQL libpq libraries", for example /usr/local/lib                   | autodetected                                           |
| USERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB   | Allows mysql driver to leak memory instead of aborting in some rare cases when linked against libmariadb3<3.3.4       | OFF                                                    |
| USERVER_DISABLE_PHDR_CACHE             | Disable caching of dl_phdr_info items, which interferes with dlopen                                                   | OFF                                                    |
| USERVER_FEATURE_UBOOST_CORO            | Build with vendored version of Boost.context and Boost.coroutine2, is needed for sanitizers builds                    | ON                                                     |

[hi_malloc]: https://bugs.launchpad.net/ubuntu/+source/hiredis/+bug/1888025

To explicitly specialize the compiler use the cmake options `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`.
For example to use clang-12 compiler install it and add the following options to cmake:
`-DCMAKE_CXX_COMPILER=clang++-12 -DCMAKE_C_COMPILER=clang-12`


@anchor userver_libraries
## The list of userver libraries

userver is split into multiple CMake libraries.

| CMake target         | CMake option to enable building the library | Main documentation page                                  |
|----------------------|---------------------------------------------|----------------------------------------------------------|
| `userver-universal`  | Always on                                   | @ref scripts/docs/en/index.md                            |
| `userver-core`       | `USERVER_FEATURE_CORE` (`ON` by default)    | @ref scripts/docs/en/index.md                            |
| `userver-grpc`       | `USERVER_FEATURE_GRPC`                      | @ref scripts/docs/en/userver/grpc.md                     |
| `userver-mongo`      | `USERVER_FEATURE_MONGODB`                   | @ref scripts/docs/en/userver/mongodb.md                  |
| `userver-postgresql` | `USERVER_FEATURE_POSTGRESQL`                | @ref pg_driver                                           |
| `userver-redis`      | `USERVER_FEATURE_REDIS`                     | @ref scripts/docs/en/userver/redis.md                    |
| `userver-clickhouse` | `USERVER_FEATURE_CLICKHOUSE`                | @ref clickhouse_driver                                   |
| `userver-rabbitmq`   | `USERVER_FEATURE_RABBITMQ`                  | @ref rabbitmq_driver                                     |
| `userver-mysql`      | `USERVER_FEATURE_MYSQL`                     | @ref scripts/docs/en/userver/mysql/design_and_details.md |

Make sure to:

1. Enable the CMake options to build the libraries you need
2. Link against the libraries

The details vary depending on the method of building userver:

* `add_subsirectory(userver)` as used in @ref service_templates "service templates"
* @ref userver_cpm "CPM"
* @ref userver_conan "Conan"


## Downloading packages using CPM

userver uses [CPM](https://github.com/cpm-cmake/CPM.cmake) for downloading missing packages.

By default, we first try to find the corresponding system package.
With `USERVER_FORCE_DOWNLOAD_PACKAGES=ON`, no such attempt is made, and we skip straight to `CPMAddPackage`.
This can be useful to guarantee that the build uses the latest (known to userver) versions of third-party packages.

You can control the usage of CPM with `USERVER_DOWNLOAD_*` options.
For example, `USERVER_DOWNLOAD_PACKAGES=OFF` is useful for CI and other controlled environments
to make sure that no download attempts are made, which ensures repeatability and the best CI build speed.

For details on the download options, refer to [CPM](https://github.com/cpm-cmake/CPM.cmake) documentation.
Some advice:

- `CPM_SOURCE_CACHE` helps to avoid re-downloads with multiple userver build modes or multiple CPM-using projects;
- `CPM_USE_NAMED_CACHE_DIRECTORIES` (which userver enables by default) avoids junk library names shown in IDEs.

@anchor userver_cpm
### Downloading userver using CPM

userver itself can be downloaded and built using CPM.

```cmake
CPMAddPackage(
    NAME userver
    VERSION (userver release version or git commit hash)
    GIT_TAG (userver release version or git commit hash)
    GITHUB_REPOSITORY userver-framework/userver
    OPTIONS
    "USERVER_FEATURE_GRPC ON"
)

target_link_libraries(${PROJECT_NAME} userver-grpc)
```

Make sure to enable the CMake options to build userver libraries you need,
then link to those libraries.

@see @ref userver_libraries


@anchor service_templates
## Build dependencies and instructions for userver based services

There are prepared and ready to use service templates at the github:

* https://github.com/userver-framework/service_template
* https://github.com/userver-framework/pg_service_template
* https://github.com/userver-framework/pg_grpc_service_template

Just use the template to make your own service:

1. Press the green "Use this template" button at the top of the github template page
2. Clone the service `git clone your-service-repo && cd your-service-repo && git submodule update --init`
3. Give a proper name to your service and replace all the occurrences of "*service_template" string with that name.
4. Feel free to tweak, adjust or fully rewrite the source code of your service.

For local development of your service either

* use the docker build and tests run via `make docker-test`;
* or install the build dependencies on your local system and
  adjust the `Makefile.local` file to provide \b platform-specific \b CMake
  options to the template:

The service templates allow to kickstart the development of your production-ready service,
but there can't be a repo for each and every combination of userver libraries.
To use additional userver libraries, e.g. `userver-grpc`, add to the root `CMakeLists.txt`:

```cmake
set(USERVER_FEATURE_GRPC ON CACHE BOOL "" FORCE)
# ...
add_subdirectory(third_party/userver)
# ...
target_link_libraries(${PROJECT_NAME} userver-grpc)
```

@see @ref userver_libraries

See @ref tutorial_services for minimal usage examples of various userver libraries.


### Ubuntu 22.04 (Jammy Jellyfish)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-22.04.md "third_party/userver/scripts/docs/en/deps/ubuntu-22.04.md"

Dependencies could be installed via:
  ```
  bash
  sudo apt install --allow-downgrades -y $(cat third_party/userver/scripts/docs/en/deps/ubuntu-22.04.md | tr '\n' ' ')
  ```


### Ubuntu 21.10 (Impish Indri)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-21.10.md "third_party/userver/scripts/docs/en/deps/ubuntu-21.10.md"

Dependencies could be installed via:
  ```
  bash
  sudo apt install --allow-downgrades -y $(cat third_party/userver/scripts/docs/en/deps/ubuntu-21.10.md | tr '\n' ' ')
  ```


### Ubuntu 20.04 (Focal Fossa)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-20.04.md "third_party/userver/scripts/docs/en/deps/ubuntu-20.04.md"

Dependencies could be installed via:
  ```
  bash
  sudo apt install --allow-downgrades -y $(cat third_party/userver/scripts/docs/en/deps/ubuntu-20.04.md | tr '\n' ' ')
  ```

\b Recommended \b Makefile.local:
  ```
  CMAKE_COMMON_FLAGS += -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 -DUSERVER_FEATURE_REDIS_HI_MALLOC=1
  ```


### Ubuntu 18.04 (Bionic Beaver)

\b Dependencies: @ref scripts/docs/en/deps/ubuntu-18.04.md "third_party/userver/scripts/docs/en/deps/ubuntu-18.04.md"

Dependencies could be installed via:
  ```
  bash
  sudo apt install --allow-downgrades -y $(cat third_party/userver/scripts/docs/en/deps/ubuntu-18.04.md | tr '\n' ' ')
  ```

\b Recommended \b Makefile.local:
  ```
  CMAKE_COMMON_FLAGS += -DCMAKE_CXX_COMPILER=g++-8 -DCMAKE_C_COMPILER=gcc-8 -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 \
    -DUSERVER_FEATURE_CRYPTOPP_BASE64_URL=0 -DUSERVER_FEATURE_GRPC=0 -DUSERVER_FEATURE_POSTGRESQL=0 \
    -DUSERVER_FEATURE_MONGODB=0 -DUSERVER_USE_LD=gold
  ```


### Debian 11

\b Dependencies: @ref scripts/docs/en/deps/debian-11.md "third_party/userver/scripts/docs/en/deps/debian-11.md"


### Debian 11 32-bit

\b Dependencies: @ref scripts/docs/en/deps/debian-11.md "third_party/userver/scripts/docs/en/deps/debian-11.md" (same as above)

\b Recommended \b Makefile.local:
  ```
  CMAKE_COMMON_FLAGS += -DCMAKE_C_FLAGS='-D_FILE_OFFSET_BITS=64' -DCMAKE_CXX_FLAGS='-D_FILE_OFFSET_BITS=64'
  ```

### Fedora 35

\b Dependencies: @ref scripts/docs/en/deps/fedora-36.md "third_party/userver/scripts/docs/en/deps/fedora-35.md"

Fedora dependencies could be installed via:
  ```
  bash
  sudo dnf install -y $(cat third_party/userver/scripts/docs/en/deps/fedora-35.md | tr '\n' ' ')
  ```

\b Recommended \b Makefile.local:
  ```
  CMAKE_COMMON_FLAGS += -DUSERVER_FEATURE_STACKTRACE=0 -DUSERVER_FEATURE_PATCH_LIBPQ=0
  ```

### Fedora 36

\b Dependencies: @ref scripts/docs/en/deps/fedora-36.md "third_party/userver/scripts/docs/en/deps/fedora-36.md"

Fedora dependencies could be installed via:
  ```
  bash
  sudo dnf install -y $(cat third_party/userver/scripts/docs/en/deps/fedora-36.md | tr '\n' ' ')
  ```

\b Recommended \b Makefile.local:
  ```
  CMAKE_COMMON_FLAGS += -DUSERVER_FEATURE_STACKTRACE=0 -DUSERVER_FEATURE_PATCH_LIBPQ=0
  ```

### Gentoo

\b Dependencies: @ref scripts/docs/en/deps/gentoo.md "third_party/userver/scripts/docs/en/deps/gentoo.md"

Dependencies could be installed via:
  ```
  bash
  sudo emerge --ask --update --oneshot $(cat scripts/docs/en/deps/gentoo.md | tr '\n' ' ')
  ```

\b Recommended \b Makefile.local:
  ```
  CMAKE_COMMON_FLAGS += -DUSERVER_CHECK_PACKAGE_VERSIONS=0 -DUSERVER_FEATURE_GRPC=0 \
    -DUSERVER_FEATURE_CLICKHOUSE=0
  ```


### Arch, Monjaro

\b Dependencies: @ref scripts/docs/en/deps/arch.md "third_party/userver/scripts/docs/en/deps/arch.md"

Using an AUR helper (pikaur in this example) the dependencies could be installed as:
  ```
  bash
  pikaur -S $(cat third_party/userver/scripts/docs/en/deps/arch.md | sed 's/^makepkg|//g' | tr '\n' ' ')
  ```

Without AUR:
  ```
  bash
  sudo pacman -S $(cat third_party/userver/scripts/docs/en/deps/arch.md | grep -v -- 'makepkg|' | tr '\n' ' ')
  cat third_party/userver/scripts/docs/en/deps/arch.md | grep -oP '^makepkg\|\K.*' | while read ;\
    do \
      DIR=$(mktemp -d) ;\
      git clone https://aur.archlinux.org/$REPLY.git $DIR ;\
      pushd $DIR ;\
      yes|makepkg -si ;\
      popd ;\
      rm -rf $DIR ;\
    done
  ```

\b Recommended \b Makefile.local:
  ```
  CMAKE_COMMON_FLAGS += -DUSERVER_FEATURE_PATCH_LIBPQ=0
  ```


### MacOS

\b Dependencies: @ref scripts/docs/en/deps/macos.md "third_party/userver/scripts/docs/en/deps/macos.md".
At least MacOS 10.15 required with
[Xcode](https://apps.apple.com/us/app/xcode/id497799835) and
[Homebrew](https://brew.sh/).
  
Dependencies could be installed via:
  ```bash
  brew install $(cat third_party/userver/scripts/docs/en/deps/macos.md | tr '\n' ' ')
  ```

\b Recommended \b Makefile.local:
  ```
  CMAKE_COMMON_FLAGS += -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DUSERVER_NO_WERROR=1 -DUSERVER_CHECK_PACKAGE_VERSIONS=0 \
      -DUSERVER_FEATURE_REDIS_HI_MALLOC=1 \
      -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 -DUSERVER_DOWNLOAD_PACKAGE_CRYPTOPP=1 \
      -DUSERVER_FEATURE_CLICKHOUSE=0 \
      -DUSERVER_FEATURE_RABBITMQ=0 \
      -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) \
      -DUSERVER_PG_LIBRARY_DIR=$(pg_config --libdir) -DUSERVER_PG_INCLUDE_DIR=$(pg_config --includedir) \
      -DUSERVER_PG_SERVER_LIBRARY_DIR=$(pg_config --pkglibdir) -DUSERVER_PG_SERVER_INCLUDE_DIR=$(pg_config --includedir-server)
  ```

After that the `make test` would build and run the service tests.

@warning MacOS is recommended only for development as it may have performance issues in some cases.


### Other POSIX based platforms

🐙 **userver** works well on modern POSIX platforms. Follow the cmake hints for the installation of required packets and experiment with the CMake options.

Feel free to provide a PR with instructions for your favorite platform at https://github.com/userver-framework/userver.

If there's a strong need to build \b only the userver and run its tests, then see
@ref scripts/docs/en/userver/tutorial/build_userver.md


@anchor POSTGRES_LIBS
## PostgreSQL versions
If CMake option `USERVER_FEATURE_PATCH_LIBPQ` is on, then the same developer
version of libpq, libpgport and libpgcommon libraries should be available on
the system. If there are multiple versions of those libraries use USERVER_PG_*
CMake options to aid the build system in finding the right version.

You could also install any version of the above libraries by explicitly pinning
the version. For example in Debian/Ubuntu pinning to version 14 could be done
via the following commands:
```
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 7FCC7D46ACCC4CF8
echo "deb https://apt-archive.postgresql.org/pub/repos/apt jammy-pgdg-archive main" | sudo tee /etc/apt/sources.list.d/pgdg.list
sudo apt update

sudo mkdir -p /etc/apt/preferences.d

printf "Package: postgresql-14\nPin: version 14.5*\nPin-Priority: 1001\n" | sudo tee -a /etc/apt/preferences.d/postgresql-14
printf "Package: postgresql-client-14\nPin: version 14.5*\nPin-Priority: 1001\n" | sudo tee -a /etc/apt/preferences.d/postgresql-client-14
sudo apt install --allow-downgrades -y postgresql-14 postgresql-client-14

printf "Package: libpq5\nPin: version 14.5*\nPin-Priority: 1001\n" | sudo tee -a /etc/apt/preferences.d/libpq5
printf "Package: libpq-dev\nPin: version 14.5*\nPin-Priority: 1001\n"| sudo tee -a /etc/apt/preferences.d/libpq-dev
sudo apt install --allow-downgrades -y libpq5 libpq-dev
```

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/supported_platforms.md | @ref scripts/docs/en/userver/deploy_env.md ⇨
@htmlonly </div> @endhtmlonly
