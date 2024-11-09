include_guard(GLOBAL)

if(userver_universal_FOUND)
  return()
endif()

include("${USERVER_CMAKE_DIR}/ModuleHelpers.cmake")

find_package(Threads)
find_package(Boost REQUIRED COMPONENTS
    program_options
    filesystem
    regex
    stacktrace_basic
    OPTIONAL_COMPONENTS
    stacktrace_backtrace
)
find_package(Iconv REQUIRED)

_userver_macos_set_default_dir(OPENSSL_ROOT_DIR "brew;--prefix;openssl")
find_package(OpenSSL REQUIRED)

find_package(fmt "8.1.1" REQUIRED)
if(NOT TARGET fmt)
  add_library(fmt ALIAS fmt::fmt)
endif()

find_package(cctz REQUIRED)
find_package(CryptoPP REQUIRED)
find_package(libyamlcpp REQUIRED)
find_package(libzstd REQUIRED)
if (USERVER_IMPL_FEATURE_JEMALLOC AND
    NOT USERVER_SANITIZE AND
    NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
  find_package(Jemalloc REQUIRED)
endif()

include("${USERVER_CMAKE_DIR}/AddGoogleTests.cmake")
include("${USERVER_CMAKE_DIR}/Sanitizers.cmake")
include("${USERVER_CMAKE_DIR}/UserverSetupEnvironment.cmake")
include("${USERVER_CMAKE_DIR}/UserverVenv.cmake")

userver_setup_environment()
_userver_make_sanitize_blacklist()

include("${USERVER_CMAKE_DIR}/SetupGTest.cmake")
include("${USERVER_CMAKE_DIR}/SetupGBench.cmake")

if(NOT USERVER_IMPL_ORIGINAL_CXX_STANDARD STREQUAL CMAKE_CXX_STANDARD)
  target_compile_definitions(userver::universal INTERFACE
      "USERVER_IMPL_ORIGINAL_CXX_STANDARD=${USERVER_IMPL_ORIGINAL_CXX_STANDARD}"
  )
endif()

set(userver_universal_FOUND TRUE)
