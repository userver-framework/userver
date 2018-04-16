cmake_minimum_required (VERSION 3.5)

message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

add_compile_options ("-pipe" "-fno-omit-frame-pointer")
add_compile_options ("-fexceptions" "-g")
add_compile_options ("-frtti" "-ftemplate-depth-128")
add_compile_options ("-fPIC")
add_definitions ("-DPIC")

# warnings
add_compile_options ("-Wall" "-Wextra" "-Wpedantic" "-Werror")

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

# all and extra do not enable theirs
add_compile_options_if_supported ("-Wdisabled-optimization" "-Winvalid-pch")
add_compile_options_if_supported ("-Wlogical-op" "-Wuseless-cast" "-Wold-style-cast" "-Wformat=2")
add_compile_options_if_supported ("-Wno-error=deprecated-declarations")
add_compile_options_if_supported ("-ftemplate-depth=200")

if (NOT CLANG) # bug in clang https://llvm.org/bugs/show_bug.cgi?id=24979
  add_compile_options (
    "-Wno-error=maybe-uninitialized" # false positive for boost::options on Release
    "-Wlogical-op"
    "-Wuseless-cast")
endif()
if (CLANG)
  add_compile_options ("-Wno-undefined-var-template")
  add_compile_options ("-Wno-unused-private-field")
  add_compile_options ("-Wno-unused-const-variable")
  add_compile_options ("-Wno-pessimizing-move")
  add_compile_options ("-Wno-absolute-value")
  add_compile_options ("-Wno-mismatched-tags")
  add_compile_options ("-Wno-missing-braces") #-Wmissing-braces is buggy in some versions on clang
  add_compile_options ("-Wno-braced-scalar-init")
endif ()

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

# get current version from changelog
execute_process(COMMAND dpkg-parsechangelog
    -l ${CMAKE_SOURCE_DIR}/debian/changelog
    --show-field Version
  OUTPUT_VARIABLE DEBVERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if (NOT DEBVERSION)
  set(DEBVERSION 0.0.0)
endif()
