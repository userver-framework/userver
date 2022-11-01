option(USERVER_DOWNLOAD_PACKAGE_LLHTTP "Download and setup llhttp" ${USERVER_DOWNLOAD_PACKAGES})
if (NOT USERVER_DOWNLOAD_PACKAGE_LLHTTP)
    MESSAGE(FATAL_ERROR "Please enable USERVER_DOWNLOAD_PACKAGE_LLHTTP")
endif()

include(FetchContent)
FetchContent_Declare(
  llhttp_external_project
  URL "https://github.com/nodejs/llhttp/archive/refs/tags/release/v8.1.0.tar.gz"
  SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/llhttp
)
FetchContent_GetProperties(llhttp_external_project)
if(NOT llhttp_external_project_POPULATED)
    message(STATUS "Downloading llhttp from remote")
    FetchContent_Populate(llhttp_external_project)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "we")
set(BUILD_STATIC_LIBS ON CACHE BOOL "idk")
add_subdirectory(${USERVER_ROOT_DIR}/third_party/llhttp "${CMAKE_BINARY_DIR}/third_party/llhttp")
add_library(llhttp ALIAS llhttp_static)
unset(BUILD_SHARED_LIBS)
unset(BUILD_STATIC_LIBS)
set(llhttp_VERSION "8.1.0" CACHE STRING "Version of the llhttp")
