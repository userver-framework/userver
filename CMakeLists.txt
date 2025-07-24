cmake_minimum_required(VERSION 3.14...4.0)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    if(NOT DEFINED $CACHE{CMAKE_POLICY_VERSION_MINIMUM})
        set(CMAKE_POLICY_VERSION_MINIMUM
            3.14
            CACHE INTERNAL ""
        )
    endif()
endif()
cmake_policy(SET CMP0025 NEW)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.15")
    cmake_policy(SET CMP0093 NEW)
endif()
set(CMAKE_POLICY_DEFAULT_CMP0028 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0074 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(userver CXX)

set(USERVER_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

option(USERVER_INSTALL "Prepare build of userver to install in system" OFF)
if(CMAKE_VERSION GREATER_EQUAL 3.31)
    cmake_policy(SET CMP0177 NEW)
endif()

set(USERVER_AVAILABLE_COMPONENTS universal)

option(USERVER_FEATURE_CORE "Provide a core library with coroutines, otherwise build only userver-universal" ON)
option(USERVER_FEATURE_CHAOTIC "Provide chaotic-codegen for jsonschema" ON)
option(USERVER_FEATURE_CHAOTIC_EXPERIMENTAL "Provide chaotic-codegen for openapi" OFF)

set(USERVER_BUILD_PLATFORM_X86 OFF)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^x86")
    set(USERVER_BUILD_PLATFORM_X86 ${USERVER_FEATURE_CORE})
endif()

function(_require_userver_core FEATURE)
    if(NOT USERVER_FEATURE_CORE)
        message(FATAL_ERROR "'${FEATURE}' requires 'USERVER_FEATURE_CORE=ON'")
    endif()
endfunction()

option(USERVER_FEATURE_UTEST "Provide 'utest' and 'ubench' for gtest and gbenchmark integration" ON)
if(USERVER_FEATURE_UTEST)
    message(STATUS "Building utest with gtest and ubench with gbench")
endif()

option(USERVER_BUILD_TESTS "Build unit tests, functional tests and benchmarks for userver itself" OFF)
option(USERVER_BUILD_SAMPLES "Build userver samples" OFF)

if(USERVER_BUILD_TESTS AND NOT USERVER_FEATURE_UTEST)
    message(FATAL_ERROR "Running unit tests for userver requires utest, disabling it is meaningless")
endif()

if(USERVER_INSTALL AND (USERVER_BUILD_TESTS OR USERVER_BUILD_SAMPLES))
    message(FATAL_ERROR "For USERVER_INSTALL, please turn off USERVER_BUILD_TESTS and USERVER_BUILD_SAMPLES "
                        "to avoid accidentally installing them"
    )
endif()

option(USERVER_BUILD_ALL_COMPONENTS "\
Build all libraries except for those explicitly turned off by 'USERVER_FEATURE_*'\
and except for libraries disabled for the current platform" OFF
)

set(USERVER_LIB_ENABLED_DEFAULT OFF)
if(USERVER_FEATURE_CORE AND USERVER_BUILD_ALL_COMPONENTS)
    set(USERVER_LIB_ENABLED_DEFAULT ON)
endif()

set(USERVER_MONGODB_DEFAULT OFF)
if(USERVER_FEATURE_CORE
   AND USERVER_BUILD_ALL_COMPONENTS
   AND NOT CMAKE_SYSTEM_NAME MATCHES "BSD"
)
    set(USERVER_MONGODB_DEFAULT ON)
endif()

set(USERVER_YDB_DEFAULT OFF)
if(USERVER_FEATURE_CORE
   AND USERVER_BUILD_ALL_COMPONENTS
   AND DEFINED CMAKE_CXX_STANDARD
   AND CMAKE_CXX_STANDARD GREATER_EQUAL 20
)
    set(USERVER_YDB_DEFAULT ON)
endif()

option(USERVER_CONAN "Build with Conan packages" "${CONAN_EXPORTED}")

option(USERVER_DOWNLOAD_PACKAGES "Download missing third party packages and use the downloaded versions" ON)
option(USERVER_FORCE_DOWNLOAD_PACKAGES
       "Download all possible third party packages even if a system package is available" OFF
)

option(USERVER_FEATURE_CRYPTOPP_BLAKE2 "Provide wrappers for blake2 algorithms of crypto++" ON)
if(NOT USERVER_FEATURE_CRYPTOPP_BLAKE2)
    add_compile_definitions("USERVER_NO_CRYPTOPP_BLAKE2=1")
endif()

option(USERVER_FEATURE_CRYPTOPP_BASE64_URL
       "Provide wrappers for Base64 URL decoding and encoding algorithms of crypto++" ON
)
if(NOT USERVER_FEATURE_CRYPTOPP_BASE64_URL)
    add_compile_definitions("USERVER_NO_CRYPTOPP_BASE64_URL=1")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "BSD")
    set(JEMALLOC_DEFAULT OFF)
else()
    set(JEMALLOC_DEFAULT ON)
endif()
option(USERVER_FEATURE_JEMALLOC "Enable linkage with jemalloc memory allocator" ${JEMALLOC_DEFAULT})

option(USERVER_DISABLE_PHDR_CACHE "Disable caching of dl_phdr_info items, which interferes with dlopen" OFF)
option(USERVER_FEATURE_STACK_USAGE_MONITOR "Enable coroutine stack usage monitor if available" ON)

set(USERVER_DISABLE_RSEQ_DEFAULT ON)
if(USERVER_BUILD_PLATFORM_X86 AND CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(USERVER_DISABLE_RSEQ_DEFAULT OFF)
    message(STATUS "rseq-based acceleration is enabled by default")
endif()
option(USERVER_DISABLE_RSEQ_ACCELERATION "Disable rseq-based optimizations" ${USERVER_DISABLE_RSEQ_DEFAULT})

option(USERVER_CHECK_PACKAGE_VERSIONS "Check package versions" ON)

option(USERVER_FEATURE_MONGODB "Provide asynchronous driver for MongoDB" "${USERVER_MONGODB_DEFAULT}")
option(USERVER_FEATURE_POSTGRESQL "Provide asynchronous driver for PostgreSQL" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_REDIS "Provide asynchronous driver for Redis" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_GRPC "Provide asynchronous driver for gRPC" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_CLICKHOUSE "Provide asynchronous driver for ClickHouse" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_KAFKA "Provide asynchronous driver for Apache Kafka" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_RABBITMQ "Provide asynchronous driver for RabbitMQ" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_MYSQL "Provide asynchronous driver for MariaDB/MySQL" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_ROCKS "Provide asynchronous driver for Rocks" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_YDB "Provide asynchronous driver for YDB" "${USERVER_YDB_DEFAULT}")
option(USERVER_FEATURE_OTLP "Provide asynchronous OTLP exporters" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_SQLITE "Provide asynchronous driver for SQLite" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_ODBC "Provide asynchronous wrapper around ODBC" "${USERVER_LIB_ENABLED_DEFAULT}")

set(CMAKE_DEBUG_POSTFIX d)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(CheckSupportedPlatform)

if(USERVER_CONAN AND NOT DEFINED CMAKE_FIND_PACKAGE_PREFER_CONFIG)
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
endif()

set(USERVER_USE_STATIC_LIBS_DEFAULT OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14.0)
    set(USERVER_USE_STATIC_LIBS_DEFAULT ON)
endif()
option(USERVER_USE_STATIC_LIBS "Tries to find all dependencies as static libraries"
       "${USERVER_USE_STATIC_LIBS_DEFAULT}"
)

if(USERVER_USE_STATIC_LIBS)
    if(USERVER_USE_STATIC_LIBS_DEFAULT)
        set(_use_static_log_level STATUS)
    else()
        set(_use_static_log_level WARNING)
    endif()
    message(
        ${_use_static_log_level}
        "USERVER_USE_STATIC_LIBS turned ON, prioritized to find static libraries. Not all dependencies may be taken into account and linkage may be broken"
    )
endif()
include(UserverPreferStaticLibs)

include(UserverSetupEnvironment)
userver_setup_environment()

include(PrepareInstall)
include(UserverModule)

if(USERVER_INSTALL)
    include(GNUInstallDirs)
endif()

include(ModuleHelpers)
include(GetUserverVersion)
include(AddGoogleTests)
include(FindPackageRequired)
include(IncludeWhatYouUse)
include(UserverTestsuite)
include(UserverCodegenTarget)
include(CheckCompileFlags)
include(CMakePackageConfigHelpers)

set(USERVER_THIRD_PARTY_DIRS
    ${USERVER_ROOT_DIR}/third_party
    CACHE INTERNAL ""
)

init_debian_depends()

include(SetupGTest)

if(USERVER_FEATURE_GRPC)
    include(SetupProtobuf)
endif()

if(USERVER_BUILD_TESTS)
    include(testsuite/SetupUserverTestsuiteEnv.cmake)
    add_subdirectory(testsuite)
endif()

add_subdirectory(universal)

if(USERVER_FEATURE_CHAOTIC)
    add_subdirectory(chaotic)
    list(APPEND USERVER_AVAILABLE_COMPONENTS chaotic)

    if(USERVER_FEATURE_CHAOTIC_EXPERIMENTAL)
        _require_userver_core(USERVER_FEATURE_CHAOTIC_EXPERIMENTAL)
        add_subdirectory(chaotic-openapi)
        list(APPEND USERVER_AVAILABLE_COMPONENTS chaotic-openapi)
    endif()
endif()

if(USERVER_FEATURE_CORE)
    if(NOT USERVER_FEATURE_CHAOTIC)
        message(FATAL_ERROR "'USERVER_FEATURE_CORE' requires 'USERVER_FEATURE_CHAOTIC=ON'")
    endif()
    add_subdirectory(core)
    list(APPEND USERVER_AVAILABLE_COMPONENTS core)
endif()

if(USERVER_FEATURE_MONGODB)
    _require_userver_core("USERVER_FEATURE_MONGODB")
    add_subdirectory(mongo)
    list(APPEND USERVER_AVAILABLE_COMPONENTS mongo)
endif()

if(USERVER_FEATURE_POSTGRESQL)
    _require_userver_core("USERVER_FEATURE_POSTGRESQL")
    add_subdirectory(postgresql)
    list(APPEND USERVER_AVAILABLE_COMPONENTS postgresql)
endif()

if(USERVER_FEATURE_REDIS)
    _require_userver_core("USERVER_FEATURE_REDIS")
    add_subdirectory(redis)
    list(APPEND USERVER_AVAILABLE_COMPONENTS redis)
endif()

if(USERVER_FEATURE_GRPC)
    _require_userver_core("USERVER_FEATURE_GRPC")
    add_subdirectory(grpc)
    list(APPEND USERVER_AVAILABLE_COMPONENTS grpc)
endif()

if(USERVER_FEATURE_OTLP)
    if(NOT USERVER_FEATURE_GRPC)
        message(FATAL_ERROR "'USERVER_FEATURE_OTLP' requires 'USERVER_FEATURE_GRPC=ON'")
    endif()
    _require_userver_core("USERVER_FEATURE_OTLP")
    add_subdirectory(otlp)
    list(APPEND USERVER_AVAILABLE_COMPONENTS otlp)
endif()

if(USERVER_FEATURE_CLICKHOUSE)
    _require_userver_core("USERVER_FEATURE_CLICKHOUSE")
    add_subdirectory(clickhouse)
    list(APPEND USERVER_AVAILABLE_COMPONENTS clickhouse)
endif()

if(USERVER_FEATURE_KAFKA)
    _require_userver_core("USERVER_FEATURE_KAFKA")
    add_subdirectory(kafka)
    list(APPEND USERVER_AVAILABLE_COMPONENTS kafka)
endif()

if(USERVER_FEATURE_RABBITMQ)
    _require_userver_core("USERVER_FEATURE_RABBITMQ")
    add_subdirectory(rabbitmq)
    list(APPEND USERVER_AVAILABLE_COMPONENTS rabbitmq)
endif()

if(USERVER_FEATURE_MYSQL)
    _require_userver_core("USERVER_FEATURE_MYSQL")
    add_subdirectory(mysql)
    list(APPEND USERVER_AVAILABLE_COMPONENTS mysql)
endif()

if(USERVER_FEATURE_ROCKS)
    _require_userver_core("USERVER_FEATURE_ROCKS")
    add_subdirectory(rocks)
endif()

if(USERVER_FEATURE_YDB)
    _require_userver_core("USERVER_FEATURE_YDB")
    add_subdirectory(ydb)
    list(APPEND USERVER_AVAILABLE_COMPONENTS ydb)
endif()

if(USERVER_FEATURE_SQLITE)
    _require_userver_core("USERVER_FEATURE_SQLITE")
    add_subdirectory(sqlite)
    list(APPEND USERVER_AVAILABLE_COMPONENTS sqlite)
endif()

if(USERVER_FEATURE_ODBC)
    _require_userver_core("USERVER_FEATURE_ODBC")
    add_subdirectory(odbc)
    list(APPEND USERVER_AVAILABLE_COMPONENTS odbc)
endif()

add_subdirectory(libraries)

if(USERVER_BUILD_TESTS)
    add_subdirectory(scripts/gdb/tests)
endif()

_userver_export_targets()
_userver_make_install_config()

if(USERVER_BUILD_SAMPLES AND USERVER_FEATURE_CORE)
    add_subdirectory(samples)
endif()

if(USERVER_INSTALL)
    include(cmake/UserverPack.cmake)
endif()

if(USERVER_FEATURE_CORE)
    _userver_directory_install(
        COMPONENT core
        PROGRAMS "${USERVER_ROOT_DIR}/scripts/userver-create-service"
        DESTINATION ${CMAKE_INSTALL_BINDIR}/
    )
    _userver_directory_install(
        COMPONENT core
        DIRECTORY "${USERVER_ROOT_DIR}/service_template"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/userver/
    )
endif()

if(USERVER_BUILD_TESTS)
    function(_add_create_service_test FEATURE CMAKE_OPTS)
        add_test(NAME create-service${FEATURE}
                 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/create-service-test-helper" "${CMAKE_CURRENT_SOURCE_DIR}"
                         "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_OPTS}" ${FEATURE}
        )
    endfunction()

    _add_create_service_test("" "")
    if(USERVER_FEATURE_MONGODB)
        _add_create_service_test("--mongo" "")
    endif()
    if(USERVER_FEATURE_POSTGRESQL)
        _add_create_service_test("--postgresql" "")
    endif()
    if(USERVER_FEATURE_GRPC)
        _add_create_service_test(
            "--grpc"
            "-DUSERVER_GOOGLE_COMMON_PROTOS=${USERVER_GOOGLE_COMMON_PROTOS} -DUSERVER_DOWNLOAD_PACKAGES=${USERVER_DOWNLOAD_PACKAGES}"
        )
    endif()
endif()

_userver_print_features_list()
