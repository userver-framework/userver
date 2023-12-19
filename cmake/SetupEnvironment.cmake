include_guard(GLOBAL)

message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_LIST_DIR}"
  "${CMAKE_BINARY_DIR}"
  "${CMAKE_BINARY_DIR}/cmake_generated"
)
set(CMAKE_PREFIX_PATH
  "${CMAKE_BINARY_DIR}/package_stubs"
  ${CMAKE_PREFIX_PATH}
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
message(STATUS "C++ standard ${CMAKE_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_compile_options("-pipe" "-g" "-gz" "-fPIC")
add_definitions("-DPIC=1")

option(COMPILATION_TIME_TRACE "Generate clang compilation trace" OFF)
if(COMPILATION_TIME_TRACE)
  add_compile_options("-ftime-trace")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(MACOS found)
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
  else()
    message(STATUS "ccache: enabled, but not found")
  endif()
else()
  message(STATUS "ccache: disabled")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(CLANG found)
endif()

if(MACOS AND NOT USERVER_CONAN)
  set(Boost_NO_BOOST_CMAKE ON)
endif()

# Build type specific
if (CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "Test")
  add_definitions("-D_GLIBCXX_ASSERTIONS")
  add_definitions(-DBOOST_ENABLE_ASSERT_HANDLER)
else()
  add_definitions("-DNDEBUG")

  # enable additional glibc checks (used in debian packaging, requires -O)
  add_definitions(-D_FORTIFY_SOURCE=2)
endif()

enable_testing()
