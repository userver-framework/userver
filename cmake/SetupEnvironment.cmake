include_guard(GLOBAL)

message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_LIST_DIR}"
  "${CMAKE_BINARY_DIR}"
  "${CMAKE_BINARY_DIR}/cmake_generated"
)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (NOT DEFINED CMAKE_CXX_STANDARD)
set (CMAKE_CXX_STANDARD 17)
endif()
message(STATUS "C++ standard ${CMAKE_CXX_STANDARD}")
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_compile_options ("-pipe" "-g" "-gz" "-fPIC")
add_definitions ("-DPIC")
add_definitions(-DUSERVER)

option(USERVER_NO_WERROR "Do not treat warnings as errors" ON)
if (NOT USERVER_NO_WERROR)
  message(STATUS "Forcing warnings as errors!")
  add_compile_options ("-Werror")
endif()

option(COMPILATION_TIME_TRACE "Generate clang compilation trace" OFF)
if(COMPILATION_TIME_TRACE)
  add_compile_options("-ftime-trace")
endif()

# warnings
add_compile_options ("-Wall" "-Wextra" "-Wpedantic")

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(MACOS found)
  # disable pkg-config as it's broken by homebrew -- TAXICOMMON-2264
  set(PKG_CONFIG_EXECUTABLE "")
endif()

include(SetupLinker)
include(SetupLTO)
include(RequireDWCAS)

option(USE_CCACHE "Use ccache for build" ON)
if (USE_CCACHE)
  find_program(CCACHE_EXECUTABLE ccache)
  if (CCACHE_EXECUTABLE)
    message(STATUS "ccache: enabled")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
  else ()
    message(STATUS "ccache: enabled, but not found")
  endif()
else ()
  message (STATUS "ccache: disabled")
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(CLANG found)
endif()

include(CheckCXXCompilerFlag)
macro(add_cxx_compile_options_if_supported)
  foreach(OPTION ${ARGN})
    message(STATUS "Checking " ${OPTION})
    STRING(REPLACE "-" "_" FLAG ${OPTION})
    STRING(REPLACE "=" "_" FLAG ${FLAG})
    set(FLAG "HAS${FLAG}")
    if (CLANG)
      CHECK_CXX_COMPILER_FLAG("-Werror -Wunknown-warning-option ${OPTION}" "${FLAG}")
    else()
      CHECK_CXX_COMPILER_FLAG("-Werror ${OPTION}" "${FLAG}")
    endif()
    if (${${FLAG}})
      message(STATUS "Checking " ${OPTION} " - found")
      add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${OPTION}>")
    else()
      message(STATUS "Checking " ${OPTION} " - not found")
    endif()
  endforeach()
endmacro()

# check stdlib is recent enough
include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(variant HAS_CXX17_VARIANT)
message(STATUS "variant: ${HAS_CXX17_VARIANT}")
if(NOT HAS_CXX17_VARIANT)
  message(FATAL_ERROR "You have an outdated standard C++ library")
endif(NOT HAS_CXX17_VARIANT)

if(MACOS AND NOT USERVER_CONAN)
    set(Boost_NO_BOOST_CMAKE ON)
endif()
find_package(Boost REQUIRED)

add_cxx_compile_options_if_supported ("-ftemplate-backtrace-limit=0")

# all and extra do not enable theirs
add_cxx_compile_options_if_supported ("-Wdisabled-optimization" "-Winvalid-pch")
add_cxx_compile_options_if_supported ("-Wlogical-op" "-Wformat=2")
add_cxx_compile_options_if_supported ("-Wno-error=deprecated-declarations")
add_cxx_compile_options_if_supported ("-Wimplicit-fallthrough")

# This warning is unavoidable in generic code with templates
add_cxx_compile_options_if_supported ("-Wno-useless-cast")

# gives false positives
if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
  add_compile_options("-Wno-range-loop-analysis")
endif()

message (STATUS "boost: ${Boost_VERSION}")
if (CLANG)
  if (MACOS AND Boost_FOUND)
    # requires Boost_FOUND to make a valid expression
    if (${Boost_VERSION} VERSION_LESS "1.68")
      message(FATAL_ERROR "Boost Locale version less that 1.68 uses features deleted from standard. Please update your boost distribution.")
    endif()
  endif()

  #  bug on xenial https://bugs.launchpad.net/ubuntu/+source/llvm-toolchain-3.8/+bug/1664321
  add_definitions (-DBOOST_REGEX_NO_EXTERNAL_TEMPLATES=1)
endif()

# build type specific
if (CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "Test")
  add_definitions ("-D_GLIBCXX_ASSERTIONS")
  add_definitions(-DBOOST_ENABLE_ASSERT_HANDLER)
else ()
  add_definitions ("-DNDEBUG")

  # enable additional glibc checks (used in debian packaging, requires -O)
  add_definitions(-D_FORTIFY_SOURCE=2)
endif ()

enable_testing ()
