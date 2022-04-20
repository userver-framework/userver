option(USERVER_DOWNLOAD_PACKAGE_CCTZ "Download and setup cctz if no cctz of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})
if (USERVER_DOWNLOAD_PACKAGE_CCTZ)
    find_package(cctz)
else()
    find_package(cctz REQUIRED)
endif()

if (cctz_FOUND)
    return()
endif()

include(FetchContent)
FetchContent_Declare(
  cctz_external_project
  GIT_REPOSITORY https://github.com/google/cctz.git
  TIMEOUT 10
  GIT_TAG v2.3
  SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/cctz
)

FetchContent_GetProperties(cctz_external_project)
if (NOT cctz_external_project)
  message(STATUS "Downloading cctz from remote")
  FetchContent_Populate(cctz_external_project)
endif()

set(BUILD_TOOLS OFF CACHE BOOL "")
set(BUILD_EXAMPLES OFF CACHE BOOL "")
set(BUILD_TESTING OFF CACHE BOOL "")
add_subdirectory(${USERVER_ROOT_DIR}/third_party/cctz "${CMAKE_BINARY_DIR}/third_party/cctz")
set(cctz_VERSION "2.3" CACHE STRING "Version of the cctz")
