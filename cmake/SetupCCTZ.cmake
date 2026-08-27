# @ingroup download
option(USERVER_DOWNLOAD_PACKAGE_CCTZ "Download and setup cctz if no cctz of matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
    if(USERVER_DOWNLOAD_PACKAGE_CCTZ)
        find_package(cctz QUIET)
    else()
        find_package(cctz REQUIRED)
    endif()

    if(cctz_FOUND)
        return()
    endif()
endif()

include(DownloadUsingCPM)
cpmaddpackage(
    NAME cctz
    VERSION 2.3
    URL https://github.com/google/cctz/archive/v2.3.tar.gz
    URL_HASH SHA256=8615b20d4e33e02a271c3b93a3b208e3d7d5d66880f5f6208b03426e448f32db
    OPTIONS "BUILD_TOOLS OFF" "BUILD_EXAMPLES OFF" "BUILD_TESTING OFF"
)
_userver_install_targets(COMPONENT universal TARGETS cctz)
