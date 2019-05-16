cmake_minimum_required (VERSION 3.8)

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
add_compile_options ("-fexceptions" "-g")
add_compile_options ("-frtti" "-ftemplate-depth-128")
add_compile_options ("-fPIC")
add_definitions ("-DPIC")
add_definitions(-DUSERVER)
add_definitions(-DMOCK_NOW)

# warnings
add_compile_options ("-Wall" "-Wextra" "-Wpedantic" "-Werror")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MACOS found)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/macos)
endif()

include(SetupLinker)
include(SetupLTO)

option(USE_CCACHE "Use ccache for build" ON)
if (USE_CCACHE)
  find_program(CCACHE_EXECUTABLE ccache)
  if (CCACHE_EXECUTABLE)
    message(STATUS "ccache found and enabled")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
  else ()
    message(WARNING "ccache enabled, but not found")
  endif()
else ()
  message (STATUS "ccache disabled")
endif ()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
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

find_package(Boost REQUIRED)

# all and extra do not enable theirs
add_compile_options_if_supported ("-Wdisabled-optimization" "-Winvalid-pch")
add_compile_options_if_supported ("-Wlogical-op" "-Wuseless-cast" "-Wformat=2")
add_compile_options_if_supported ("-Wno-error=deprecated-declarations")
add_compile_options_if_supported ("-ftemplate-depth=200")

if (NOT CLANG) # bug in clang https://llvm.org/bugs/show_bug.cgi?id=24979
  add_compile_options (
    "-Wno-error=maybe-uninitialized" # false positive for boost::options on Release
    "-Wlogical-op"
    "-Wuseless-cast")
endif()
if (CLANG)
  message (STATUS "boost: ${Boost_VERSION}")
  if (MACOS AND ${Boost_VERSION} STRLESS "106800")
    message(FATAL_ERROR "Boost Locale version less that 1.68 uses features deleted from standard. Please update your boost distribution.")
  endif()

  #  bug on xenial https://bugs.launchpad.net/ubuntu/+source/llvm-toolchain-3.8/+bug/1664321
  add_definitions (-DBOOST_REGEX_NO_EXTERNAL_TEMPLATES=1)

  add_compile_options ("-Wno-missing-braces") # -Wmissing-braces is buggy in some versions on clang
  add_compile_options ("-Wno-braced-scalar-init")
endif()

if (${Boost_VERSION} STRLESS "106300")
  add_definitions("-DBOOST_NO_CXX17_STD_APPLY" "-DBOOST_NO_CXX17_STD_INVOKE")
endif()

# build type specific
if (CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "Test")
  add_compile_options ("-O0")
else ()
  add_compile_options ("-O3")
  add_definitions ("-DNDEBUG")
endif ()

# pretty file name for logging
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFILENAME='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

enable_testing ()

set (CMAKE_INSTALL_DO_STRIP "NO")
set (CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

# The oldest CPU we support is Xeon E5-2660 0 (Sandy Bridge)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=sandybridge")
