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

add_compile_options ("-pipe" "-fno-omit-frame-pointer")
add_compile_options ("-fexceptions" "-g")
add_compile_options ("-frtti" "-ftemplate-depth-128")
add_compile_options ("-fPIC")
add_definitions ("-DPIC")
add_definitions(-DUSERVER)
add_definitions(-DMOCK_NOW)

# warnings
add_compile_options ("-Wall" "-Wextra" "-Wpedantic" "-Werror")



option(USE_CCACHE "Use ccache for build" ON)
if (USE_CCACHE)
  find_program(CCACHE_FOUND ccache)
  if (CCACHE_FOUND)
    message(STATUS "ccache found and enabled")
    set(CMAKE_C_COMPILER_LAUNCHER ccache)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
  else ()
    message(WARNING "ccache enabled, but not found")
  endif()
else ()
  message (STATUS "ccache disabled")
endif ()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MACOS found)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
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

find_package(Boost REQUIRED COMPONENTS coroutine)

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
  if (${Boost_VERSION} STREQUAL "105800" AND ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message (WARNING "Boost 1.58 is known to fail to compile under clang-5.0 with debug enabled (\"ordered comparison between pointer and zero ('int' and 'void *')\"), disabling Boost assertions.")
    add_definitions("-DBOOST_DISABLE_ASSERTS")
  endif ()

  add_compile_options ("-Wno-old-style-cast")
  add_compile_options ("-Wno-undefined-var-template")
  add_compile_options ("-Wno-unused-private-field")
  add_compile_options ("-Wno-unused-const-variable")
  add_compile_options ("-Wno-pessimizing-move")
  add_compile_options ("-Wno-absolute-value")
  add_compile_options ("-Wno-mismatched-tags")
  add_compile_options ("-Wno-missing-braces") #-Wmissing-braces is buggy in some versions on clang
  add_compile_options ("-Wno-braced-scalar-init")
endif()

SET(SANITIZE_ENUM "mem, addr, thread, ub")
set(SANITIZE "" CACHE STRING "Clang sanitizer, possible values: ${SANITIZE_ENUM}")
if (NOT CLANG AND SANITIZE)
  message(FATAL_ERROR "-DSANITIZE can be set only when complied using clang.  Please set CC=clang-5.0 CXX=clang++-5.0 or smth.")
endif()
if(SANITIZE STREQUAL "")
  # no sanitizer
elseif(SANITIZE STREQUAL "ub")
  # https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
  add_compile_options("-fsanitize=undefined")
  set(CLANG_LIBRARIES "-fsanitize=undefined")
elseif(SANITIZE STREQUAL "addr")
  # http://releases.llvm.org/5.0.0/tools/clang/docs/AddressSanitizer.html
  add_compile_options("-fsanitize=address")
  add_compile_options("-fno-omit-frame-pointer")
  add_compile_options("-g")
  set(CLANG_LIBRARIES "-g -fsanitize=address")
elseif(SANITIZE STREQUAL "thread")
  # http://releases.llvm.org/5.0.0/tools/clang/docs/ThreadSanitizer.html
  add_compile_options("-fsanitize=thread")
  add_compile_options("-g")
  set(CLANG_LIBRARIES "-g -fsanitize=thread")
elseif(SANITIZE STREQUAL "mem")
  # https://clang.llvm.org/docs/MemorySanitizer.html
  add_compile_options("-fsanitize=memory")
  add_compile_options("-fno-omit-frame-pointer")
  add_compile_options("-g")
  set(CLANG_LIBRARIES "-g -fsanitize=memory")
else()
  message(FATAL_ERROR "-DSANITIZE has invalid value (${SANITIZE}), possible values: ${SANITIZE_ENUM}")
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
