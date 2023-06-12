set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

option(USERVER_DOWNLOAD_PACKAGE_CARES "Download and setup c-ares if no c-ares of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

if (USERVER_DOWNLOAD_PACKAGE_CARES)
  find_package(c-ares 1.16)
else()
  find_package(c-ares 1.16 REQUIRED)
endif()

if (c-ares_FOUND)
  if(NOT TARGET c-ares::cares)
    add_library(c-ares::cares ALIAS c-ares)
  endif()
  return()
endif()


include(FetchContent)
FetchContent_Declare(
    c-ares_external_project
    GIT_REPOSITORY https://github.com/c-ares/c-ares
    TIMEOUT 10
    GIT_TAG cares-1_19_0
    SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/c-ares
)
FetchContent_GetProperties(c-ares_external_project)
if(NOT c-ares_external_project_POPULATED)
  message(STATUS "Downloading c-ares from remote")
  FetchContent_Populate(c-ares_external_project)
endif()

set(CARES_INSTALL OFF CACHE BOOL "")
set(CARES_BUILD_TOOLS OFF CACHE BOOL "")
set(CARES_SHARED OFF CACHE BOOL "")
set(CARES_STATIC ON CACHE BOOL "")
add_subdirectory(${USERVER_ROOT_DIR}/third_party/c-ares "${CMAKE_BINARY_DIR}/third_party/c-ares")
set(c-ares_VERSION "1.19.0" CACHE STRING "Version of the c-ares")
