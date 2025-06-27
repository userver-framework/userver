include_guard(GLOBAL)

if(userver_universal_FOUND)
    return()
endif()

include("${USERVER_CMAKE_DIR}/ModuleHelpers.cmake")

find_package(Threads REQUIRED)
find_package(
    Boost REQUIRED CONFIG
    COMPONENTS program_options filesystem stacktrace_basic
    OPTIONAL_COMPONENTS stacktrace_backtrace
)
find_package(Iconv REQUIRED)

if(Boost_USE_STATIC_LIBS AND Boost_VERSION VERSION_LESS 1.75)
    # https://github.com/boostorg/locale/issues/156
    find_package(
        ICU
        COMPONENTS uc i18n data
        REQUIRED
    )
endif()

_userver_macos_set_default_dir(OPENSSL_ROOT_DIR "brew;--prefix;openssl")
find_package(OpenSSL REQUIRED)

if(USERVER_IMPL_FEATURE_JEMALLOC
   AND NOT USERVER_SANITIZE
   AND NOT CMAKE_SYSTEM_NAME MATCHES "Darwin"
)

    find_package(jemalloc REQUIRED)
endif()

find_package(fmt REQUIRED)
find_package(cctz REQUIRED)
find_package(re2 REQUIRED)
find_package(cryptopp REQUIRED)
find_package(zstd REQUIRED)
find_package(yaml-cpp REQUIRED)

if(USERVER_CONAN)
    find_package(RapidJSON REQUIRED)
endif()

include("${USERVER_CMAKE_DIR}/AddGoogleTests.cmake")
include("${USERVER_CMAKE_DIR}/Sanitizers.cmake")
include("${USERVER_CMAKE_DIR}/UserverSetupEnvironment.cmake")
include("${USERVER_CMAKE_DIR}/UserverVenv.cmake")
include("${USERVER_CMAKE_DIR}/UserverCodegenTarget.cmake")
include("${USERVER_CMAKE_DIR}/UserverEmbedFile.cmake")

userver_setup_environment()
_userver_make_sanitize_blacklist()

if(USERVER_CONAN)
    find_package(GTest REQUIRED)
    find_package(benchmark REQUIRED)
else()
    include("${USERVER_CMAKE_DIR}/SetupGTest.cmake")
    include("${USERVER_CMAKE_DIR}/SetupGBench.cmake")
endif()

if(NOT USERVER_IMPL_ORIGINAL_CXX_STANDARD STREQUAL CMAKE_CXX_STANDARD)
    target_compile_definitions(
        userver::universal INTERFACE "USERVER_IMPL_ORIGINAL_CXX_STANDARD=${USERVER_IMPL_ORIGINAL_CXX_STANDARD}"
    )
endif()

set(userver_universal_FOUND TRUE)
