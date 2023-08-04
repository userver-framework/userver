option(USERVER_DOWNLOAD_PACKAGE_CURL "Download and setup libcurl if no libcurl of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_CURL)
      find_package(CURL "7.68" QUIET)
  else()
      find_package_required_version(CURL "libcurl4-openssl-dev" "7.68")
  endif()

  if (CURL_FOUND)
      return()
  endif()
endif()

set(CURL_LTO_OPTION)
if (LTO_FLAG AND LTO)
  if (NOT DEFINED LTO)
    message(FATAL_ERROR "SetupEnvironment should be included before SetupCURL")
  endif()
  set(CURL_LTO_OPTION "CURL_LTO ON")
endif()

include(DownloadUsingCPM)
CPMAddPackage(
    NAME curl
    VERSION 7.81
    GITHUB_REPOSITORY curl/curl
    GIT_TAG curl-7_81_0
    OPTIONS
    "BUILD_CURL_EXE OFF"
    "BUILD_SHARED_LIBS OFF"
    "CURL_DISABLE_TESTS ON"
    ${CURL_LTO_OPTION}
)
