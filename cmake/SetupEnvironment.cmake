if (TAXI_SETUP_ENVIRONMENT_INCLUDED)
    return()
endif()
set(TAXI_SETUP_ENVIRONMENT_INCLUDED 1)

message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_compile_options ("-pipe" "-fno-omit-frame-pointer")
add_compile_options ("-g" "-gz")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=200")
add_compile_options ("-fPIC")
add_definitions ("-DPIC")
add_definitions(-DUSERVER)
add_definitions(-DMOCK_NOW)

option(NO_WERROR "Do not treat warnings as errors" OFF)
if (NOT NO_WERROR)
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
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/macos)
  # disable pkg-config as it's broken by homebrew -- TAXICOMMON-2264
  set(PKG_CONFIG_EXECUTABLE "")
endif()

include(SetupLinker)
include(SetupLTO)

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
macro(add_compile_options_if_supported)
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
      add_compile_options("${OPTION}")
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
  message(FATAL_ERROR "You have an outdated standard C++ library. Try installing libstdc++-7-dev or see Yandex specific instructions https://nda.ya.ru/t/hIZTqQ803VtYBV")
endif(NOT HAS_CXX17_VARIANT)

if (MACOS)
    set(Boost_NO_BOOST_CMAKE ON)
endif(MACOS)
find_package(Boost REQUIRED)

# all and extra do not enable theirs
add_compile_options_if_supported ("-Wdisabled-optimization" "-Winvalid-pch")
add_compile_options_if_supported ("-Wlogical-op" "-Wformat=2")
add_compile_options_if_supported ("-Wno-error=deprecated-declarations")
add_compile_options_if_supported ("-Wimplicit-fallthrough")

# This warning is unavoidable in generic code with templates
add_compile_options_if_supported ("-Wno-useless-cast")

# gives false positives
if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
  add_compile_options("-Wno-range-loop-analysis")
endif()

if (NOT CLANG) # bug in clang https://llvm.org/bugs/show_bug.cgi?id=24979
  add_compile_options (
    "-Wno-error=maybe-uninitialized" # false positive for boost::options on Release
  )
endif()
if (CLANG)
  message (STATUS "boost: ${Boost_VERSION}")
  if (MACOS AND Boost_FOUND)
    # requires Boost_FOUND to make a valid expression
    if (${Boost_VERSION} VERSION_LESS "106800")
      message(FATAL_ERROR "Boost Locale version less that 1.68 uses features deleted from standard. Please update your boost distribution.")
    endif()
  endif()

  #  bug on xenial https://bugs.launchpad.net/ubuntu/+source/llvm-toolchain-3.8/+bug/1664321
  add_definitions (-DBOOST_REGEX_NO_EXTERNAL_TEMPLATES=1)

  add_compile_options ("-Wno-missing-braces") # -Wmissing-braces is buggy in some versions on clang
  add_compile_options ("-Wno-braced-scalar-init")
endif()

# build type specific
if (CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "Test")
  add_compile_options ("-O0")
  add_definitions ("-D_GLIBCXX_ASSERTIONS")
  add_definitions(-DBOOST_ENABLE_ASSERT_HANDLER)
else ()
  add_compile_options ("-O3")
  add_definitions ("-DNDEBUG")

  # enable additional glibc checks (used in debian packaging, requires -O)
  add_definitions(-D_FORTIFY_SOURCE=2)
endif ()

enable_testing ()

set (CMAKE_INSTALL_DO_STRIP "NO")
set (CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND NOT MACOS)
  # The oldest CPU we support is Xeon E5-2660 0 (Sandy Bridge)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=sandybridge")
endif ()
