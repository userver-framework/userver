project(userver-core CXX)

include(GetUserverVersion)
message(STATUS "Userver version ${USERVER_VERSION}")

include(CheckFunctionExists)
check_function_exists("accept4" HAVE_ACCEPT4)
check_function_exists("pipe2" HAVE_PIPE2)

set(BUILD_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/build_config.hpp)
if(${CMAKE_SOURCE_DIR}/.git/HEAD IS_NEWER_THAN ${BUILD_CONFIG})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/build_config.hpp.in
    ${BUILD_CONFIG}
    ESCAPE_QUOTES @ONLY)
endif()

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/internal/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/internal/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp
)

list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

file(GLOB_RECURSE BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*_benchmark.cpp
)
file(GLOB_RECURSE LIBUBENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/main.cpp
)
list (REMOVE_ITEM SOURCES ${BENCH_SOURCES} ${LIBUBENCH_SOURCES})

file(GLOB_RECURSE INTERNAL_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/internal/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/internal/*.hpp
)

list (REMOVE_ITEM SOURCES ${INTERNAL_SOURCES})

find_package(Boost REQUIRED CONFIG COMPONENTS
    program_options
    filesystem
    locale
    regex
    iostreams
)
find_package_required(ZLIB "zlib1g-dev")

find_package(Iconv REQUIRED)
find_package_required(OpenSSL "libssl-dev")

if (USERVER_CONAN)
    find_package(c-ares REQUIRED CONFIG)
    find_package(CURL REQUIRED CONFIG)
    find_package(cryptopp REQUIRED CONFIG)
    find_package(libnghttp2 REQUIRED CONFIG)
    find_package(libev REQUIRED CONFIG)

    find_package(concurrentqueue REQUIRED CONFIG)
else()
    include(SetupCAres)
    include(SetupCURL)
    include(SetupCryptoPP)
    find_package(Nghttp2 REQUIRED)
    find_package(LibEv REQUIRED)
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES})

# thread_local usage in Moodycamel is incompatible with userver scheduler
# that migrates coroutines between threads.
target_compile_definitions(${PROJECT_NAME} PUBLIC MOODYCAMEL_NO_THREAD_LOCAL)

if (USERVER_DISABLE_PHDR_CACHE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE USERVER_DISABLE_PHDR_CACHE)
endif()

if (USERVER_DISABLE_RSEQ_ACCELERATION)
  target_compile_definitions(${PROJECT_NAME} PRIVATE USERVER_DISABLE_RSEQ_ACCELERATION)
else()
  add_subdirectory(${USERVER_THIRD_PARTY_DIRS}/librseq librseq)
  target_link_libraries(${PROJECT_NAME} PRIVATE userver-librseq)
endif()

# https://github.com/jemalloc/jemalloc/issues/820
if (USERVER_FEATURE_JEMALLOC AND NOT USERVER_SANITIZE AND NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set_property(
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/jemalloc.cpp
    APPEND PROPERTY COMPILE_FLAGS -DUSERVER_FEATURE_JEMALLOC_ENABLED=1
  )
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-universal
    Boost::locale
    CURL::libcurl
  PRIVATE
    Boost::filesystem
    Boost::program_options
    Boost::iostreams
    Boost::regex
    Iconv::Iconv
    OpenSSL::Crypto
    OpenSSL::SSL
    ZLIB::ZLIB
)

add_subdirectory(${USERVER_THIRD_PARTY_DIRS}/llhttp llhttp)

add_subdirectory(${USERVER_THIRD_PARTY_DIRS}/http-parser http-parser)

target_link_libraries(${PROJECT_NAME} PRIVATE userver-http-parser userver-llhttp)

set(USERVER_UBOOST_CORO_DEFAULT ON)
if(CMAKE_SYSTEM_NAME MATCHES "Darwin" AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
  # Use system Boost.Context and Boost.Coroutine2 with latest patches
  # for arm64, otherwise userver will crash at startup.
  #
  # NOTE: this makes it practically impossible to build userver with sanitizers
  # on macOS, because Boost.Context and Boost.Coroutine2 will cause
  # an odr violation when linking sanitized usages in userver with stock Debug
  # static library.
  set(USERVER_UBOOST_CORO_DEFAULT OFF)
  if("${Boost_VERSION_STRING}" VERSION_LESS "1.83.0")
    message(FATAL_ERROR
        "Please update Boost to at least 1.83.0, it contains important fixes "
        "for Boost.Context and Boost.Coroutine2")
  endif()
endif()

option(USERVER_FEATURE_UBOOST_CORO "Use vendored boost context instead of a system one" "${USERVER_UBOOST_CORO_DEFAULT}")

if (USERVER_FEATURE_UBOOST_CORO)
    add_subdirectory(${USERVER_THIRD_PARTY_DIRS}/uboost_coro uboost_coro_build)
    target_link_libraries(${PROJECT_NAME}
      PRIVATE
        userver-uboost-coro
    )
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/uboost_coro/include
    )
else()
    find_package(Boost REQUIRED CONFIG COMPONENTS
        context
        coroutine
    )
    target_link_libraries(${PROJECT_NAME}
      PRIVATE
        Boost::context
    )
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/sys_coro/include
    )
endif()

if (USERVER_CONAN)
    target_link_libraries(${PROJECT_NAME}
      PUBLIC
        c-ares::cares
        concurrentqueue::concurrentqueue
      PRIVATE
        cryptopp::cryptopp
        libev::libev
        libnghttp2::nghttp2
    )
else()
    target_link_libraries(${PROJECT_NAME}
      PUBLIC
        c-ares::cares
      PRIVATE
        CryptoPP
        Nghttp2
        LibEv
    )

    target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
         $<BUILD_INTERFACE:${USERVER_THIRD_PARTY_DIRS}/moodycamel/include>
    )
    _userver_directory_install(COMPONENT core
        DIRECTORY ${USERVER_THIRD_PARTY_DIRS}/moodycamel/include/moodycamel
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/userver/third_party
    )
endif()

if (NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" AND NOT CMAKE_SYSTEM_NAME MATCHES "BSD")
  target_link_libraries(${PROJECT_NAME} PUBLIC atomic)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${USERVER_THIRD_PARTY_DIRS}/pfr/include>
)
_userver_directory_install(COMPONENT core
  DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_directory_install(COMPONENT core
  DIRECTORY
    ${USERVER_THIRD_PARTY_DIRS}/pfr/include/boost
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/userver/third_party
)

target_include_directories(${PROJECT_NAME} SYSTEM BEFORE PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libc_include_fixes>
)

# The bug is only triggered with optimizations enabled -- TAXICOMMON-1729
set_property(
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/errno_test.cpp
  APPEND PROPERTY COMPILE_FLAGS -O2
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../universal/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Must go below all the target options and definitions.
_check_compile_flags(${PROJECT_NAME})

if (USERVER_FEATURE_UTEST)
  add_library(userver-core-internal OBJECT ${INTERNAL_SOURCES})
  target_compile_definitions(userver-core-internal PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)
  target_include_directories(userver-core-internal PUBLIC
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/internal/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../universal/internal/include>
  )
  target_link_libraries(userver-core-internal
    PUBLIC
      ${PROJECT_NAME}
  )
  _userver_directory_install(COMPONENT core
    DIRECTORY
      ${CMAKE_CURRENT_SOURCE_DIR}/internal/include
      ${CMAKE_CURRENT_SOURCE_DIR}/../universal/internal/include
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
  )
  _userver_install_targets(COMPONENT core TARGETS userver-core-internal)
endif()

if (USERVER_FEATURE_UTEST)
  add_subdirectory(utest)
endif()

if (USERVER_BUILD_TESTS)
    add_executable(${PROJECT_NAME}-unittest ${UNIT_TEST_SOURCES})
    target_include_directories (${PROJECT_NAME}-unittest PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    )
    target_link_libraries(${PROJECT_NAME}-unittest PRIVATE
      userver-utest
      userver-core-internal
    )

    add_google_tests(${PROJECT_NAME}-unittest)
    add_subdirectory(functional_tests)
endif()

if (USERVER_FEATURE_UTEST)
    add_library(userver-ubench ${LIBUBENCH_SOURCES})
    target_include_directories(userver-ubench PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>)
    target_compile_definitions(userver-ubench PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)
    target_link_libraries(userver-ubench
      PUBLIC
        ${PROJECT_NAME}
        userver-universal-internal-ubench
      PRIVATE
        userver-core-internal
    )
    _userver_install_targets(COMPONENT core TARGETS userver-ubench userver-universal-internal-ubench)
endif()

if (USERVER_BUILD_TESTS)
    add_executable(${PROJECT_NAME}-benchmark ${BENCH_SOURCES})

    target_link_libraries(${PROJECT_NAME}-benchmark
      PUBLIC
        userver-ubench
      PRIVATE
        userver-core-internal
    )
    add_google_benchmark_tests(${PROJECT_NAME}-benchmark)
endif()

_userver_install_targets(COMPONENT core TARGETS ${PROJECT_NAME})
_userver_directory_install(COMPONENT core FILES
    "${USERVER_ROOT_DIR}/cmake/UserverTestsuite.cmake"
    "${USERVER_ROOT_DIR}/cmake/install/userver-core-config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)
_userver_directory_install(COMPONENT core FILES
    "${USERVER_ROOT_DIR}/cmake/modules/Findc-ares.cmake"
    "${USERVER_ROOT_DIR}/cmake/modules/FindNghttp2.cmake"
    "${USERVER_ROOT_DIR}/cmake/modules/FindLibEv.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules
)

file(GLOB_RECURSE TESTSUITE_INSTALL_FILES
    "${USERVER_ROOT_DIR}/testsuite/*.txt"
    "${USERVER_ROOT_DIR}/testsuite/*.py"
)

_userver_directory_install(COMPONENT core
    FILES ${TESTSUITE_INSTALL_FILES}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/testsuite
)

_userver_directory_install(COMPONENT core
    PROGRAMS "${USERVER_ROOT_DIR}/testsuite/env.in"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/testsuite
)

_userver_directory_install(COMPONENT core DIRECTORY
    "${USERVER_ROOT_DIR}/testsuite/pytest_plugins/"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/testsuite/pytest_plugins/
)
