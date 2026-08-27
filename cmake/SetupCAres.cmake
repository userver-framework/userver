# @ingroup download
option(USERVER_DOWNLOAD_PACKAGE_CARES "Download and setup c-ares if no c-ares of matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
    if(USERVER_DOWNLOAD_PACKAGE_CARES)
        find_package(c-ares 1.16 QUIET)
    else()
        find_package(c-ares 1.16 REQUIRED)
    endif()

    if(c-ares_FOUND)
        return()
    endif()
endif()

include(DownloadUsingCPM)
cpmaddpackage(
    NAME c-ares
    VERSION 1.19.0
    URL https://github.com/c-ares/c-ares/archive/cares-1_19_0.tar.gz
    URL_HASH SHA256=948016368481b6c5063b849b6dec2a7fd659eee2174b7f3db22ff1b22055ed2a
    OPTIONS "CARES_INSTALL OFF" "CARES_BUILD_TOOLS OFF" "CARES_SHARED OFF" "CARES_STATIC ON"
)

set(c-ares_FOUND TRUE)
write_package_stub(c-ares)
