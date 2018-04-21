cmake_minimum_required(VERSION 3.5)

option(USE_CCACHE "Use ccache for build" OFF)
if (USE_CCACHE)
  find_program(CCACHE_FOUND ccache)
  if (CCACHE_FOUND)
    message(STATUS "ccache found and enabled")
    set(CMAKE_C_COMPILER_LAUNCHER ccache)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
  else ()
    message(FATAL_ERROR "ccache enabled, but not found")
  endif()
else ()
  message (STATUS "ccache disabled")
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
set(USERVER_PUBLIC_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(USERVER_THIRD_PARTY_DIRS ${CMAKE_SOURCE_DIR}/third_party)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MACOS found)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CLANG found)
endif()

include(SetupEnvironment)
include(CppCheck)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)
