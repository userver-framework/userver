option(USERVER_DOWNLOAD_PACKAGE_CURL "Download and setup libcurl if no libcurl of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if(USERVER_DOWNLOAD_PACKAGE_CURL)
    find_package(CURL "7.68" QUIET)
  else()
    find_package_required_version(CURL "libcurl4-openssl-dev" "7.68")
  endif()

  if(CURL_FOUND)
    if("${CURL_VERSION_STRING}" VERSION_GREATER_EQUAL "7.88.0" AND
        "${CURL_VERSION_STRING}" VERSION_LESS_EQUAL "8.1.2")
      if(USERVER_DOWNLOAD_PACKAGE_CURL)
        message(STATUS
            "libcurl versions from 7.88.0 to 8.1.2 may crash on HTTP/2 "
            "requests and thus are unsupported, downloading a supported version"
        )
      else()
        message(FATAL_ERROR
            "libcurl versions from 7.88.0 to 8.1.2 may crash on HTTP/2 "
            "requests and thus are unsupported, upgrade or turn on "
            "USERVER_DOWNLOAD_PACKAGE_CURL"
        )
      endif()
    else()
      return()
    endif()
  endif()
endif()

set(CURL_LTO_OPTION)
if(USERVER_LTO)
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
