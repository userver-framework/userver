option(USERVER_DOWNLOAD_PACKAGE_CURL "Download and setup libcurl if no libcurl of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})
if (USERVER_DOWNLOAD_PACKAGE_CURL)
    find_package(CURL "7.68")
else()
    find_package_required_version(CURL "libcurl4-openssl-dev" "7.68")
endif()

if (CURL_FOUND)
    return()
endif()

include(FetchContent)
FetchContent_Declare(
  curl_external_project
  GIT_REPOSITORY https://github.com/curl/curl.git
  TIMEOUT 10
  GIT_TAG curl-7_81_0
  SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/libcurl
)

FetchContent_GetProperties(curl_external_project)
if (NOT curl_external_project_POPULATED)
  message(STATUS "Downloading libcurl from remote")
  FetchContent_Populate(curl_external_project)
endif()


set(BUILD_CURL_EXE OFF CACHE BOOL "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
if (LTO_FLAG AND LTO)
    if (NOT DEFINED LTO)
        message(FATAL_ERROR "SetupEnvironment should be included before SetupCURL")
    endif()
    set(CURL_LTO ON CACHE BOOL "")
endif()
set(CURL_DISABLE_TESTS ON)
add_subdirectory(${USERVER_ROOT_DIR}/third_party/libcurl "${CMAKE_BINARY_DIR}/third_party/libcurl")
set(CURL_VERSION "7.81" CACHE STRING "Version of the libcurl")
