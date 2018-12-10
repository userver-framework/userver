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

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MACOS found)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos)
endif()

option(LTO "Use -flto=thin for link time optimizations" ON)
message(STATUS "LTO: ${LTO}")
if(LTO)
  # ar from binutils fails to link -flto=thin with the following message:
  # ../../src/libyandex-taxi-userver.a: error adding symbols: Archive has no index; run ranlib to add one
  # In Debian/Ubuntu llvm-ar is installed with version suffix.
  # In Mac OS with brew llvm-ar is installed without version suffix
  # into /usr/local/opt/llvm/ with brew
  if (MACOS)
    set(LLVM_PATH_HINTS /usr/local/opt/llvm/bin)
  endif()
  find_program(LLVM_AR NAMES llvm-ar-6.0 llvm-ar HINTS ${LLVM_PATH_HINTS})
  find_program(LLVM_RANLIB NAMES llvm-ranlib-6.0 llvm-ranlib HINTS ${LLVM_PATH_HINTS})
  if (NOT LLVM_AR)
    message(FATAL_ERROR "LLVM archiver (llvm-ar) not found, you can disable it by specifying -DLTO=OFF")
  endif()
  if (NOT LLVM_RANLIB)
    message(FATAL_ERROR "LLVM archiver (llvm-ranlib) not found, you can disable it by specifying -DLTO=OFF")
  endif()
  set(CMAKE_AR ${LLVM_AR})
  set(CMAKE_RANLIB ${LLVM_RANLIB})

  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS} -flto=thin")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} -flto=thin")
  add_compile_options ("$<$<CONFIG:RELEASE>:-flto=thin>")
endif(LTO)



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
  if (${Boost_VERSION} STRGREATER "105900")
    message(STATUS "Boost Coroutines are deprecated after 1.59, adding deprecation suppression flag")
    add_definitions("-DBOOST_COROUTINES_NO_DEPRECATION_WARNING")
  endif()
  if (MACOS AND ${Boost_VERSION} STRLESS "106800")
    message(FATAL_ERROR "Boost Locale version less that 1.68 uses features deleted from standard. Please update your boost distribution.")
  endif()

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

# build type specific
if (CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "Test")
  add_compile_options ("-O0")
else ()
  add_compile_options ("-O3")
  add_definitions ("-DNDEBUG")
endif ()

# pretty file name for logging
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFILENAME='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

include(SetupLinker)

enable_testing ()

set (CMAKE_INSTALL_DO_STRIP "NO")
set (CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)
