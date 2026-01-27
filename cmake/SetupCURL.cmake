option(USERVER_DOWNLOAD_PACKAGE_CURL "Download and setup libcurl if no libcurl of matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)
option(USERVER_FORCE_DOWNLOAD_CURL "Download Curl even if there is an installed system package"
       ${USERVER_FORCE_DOWNLOAD_PACKAGES}
)

if(NOT USERVER_FORCE_DOWNLOAD_CURL)
    # Curl has too many dependencies to reliably link with all of them statically without CMake Config
    if(USERVER_USE_STATIC_LIBS)
        list(REVERSE CMAKE_FIND_LIBRARY_SUFFIXES)
    endif()

    if(USERVER_DOWNLOAD_PACKAGE_CURL)
        find_package(CURL "7.68")
    else()
        find_package(CURL "7.68" REQUIRED)
    endif()

    if(USERVER_USE_STATIC_LIBS)
        list(REVERSE CMAKE_FIND_LIBRARY_SUFFIXES)
    endif()

    if(CURL_FOUND)
        if("${CURL_VERSION_STRING}" VERSION_GREATER_EQUAL "7.88.0" AND "${CURL_VERSION_STRING}" VERSION_LESS_EQUAL
                                                                       "8.1.2"
        )
            if(USERVER_DOWNLOAD_PACKAGE_CURL)
                message(STATUS "libcurl versions from 7.88.0 to 8.1.2 may crash on HTTP/2 "
                               "requests and thus are unsupported, downloading a supported version"
                )
            else()
                message(
                    FATAL_ERROR
                        "libcurl versions from 7.88.0 to 8.1.2 may crash on HTTP/2 "
                        "requests and thus are unsupported, upgrade or turn on USERVER_DOWNLOAD_PACKAGE_CURL"
                )
            endif()
        else()
            return()
        endif()
    endif()
endif()

set(CURL_EXTRA_OPTIONS)
if(USERVER_LTO)
    list(APPEND CURL_EXTRA_OPTIONS "CURL_LTO ON")
endif()

find_package(libnghttp2 REQUIRED)

if(CPM_PACKAGE_libnghttp2_SOURCE_DIR)
    list(APPEND CURL_EXTRA_OPTIONS "NGHTTP2_INCLUDE_DIR ${NGHTTP2_INCLUDE_DIR}" "NGHTTP2_LIBRARY ${NGHTTP2_LIBRARY}")
endif()

include(DownloadUsingCPM)
cpmaddpackage(
    NAME CURL
    VERSION 7.81
    GITHUB_REPOSITORY curl/curl
    GIT_TAG curl-7_81_0
    GIT_SHALLOW TRUE
    OPTIONS "BUILD_CURL_EXE OFF" "BUILD_SHARED_LIBS OFF" "CURL_DISABLE_TESTS ON" "CURL_DISABLE_LDAP ON"
            "HAVE_DLOPEN TRUE" "USE_NGHTTP2 TRUE" ${CURL_EXTRA_OPTIONS}
)

mark_targets_as_system("${CURL_SOURCE_DIR}")
write_package_stub(CURL)
