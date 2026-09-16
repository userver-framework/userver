# @ingroup download
option(USERVER_DOWNLOAD_PACKAGE_FMT "Download and setup Fmt if no Fmt of matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)
# @ingroup download
option(USERVER_FORCE_DOWNLOAD_FMT "Download fmt even if there is an installed system package"
       ${USERVER_FORCE_DOWNLOAD_PACKAGES}
)

if(NOT USERVER_FORCE_DOWNLOAD_FMT)
    if(USERVER_DOWNLOAD_PACKAGE_FMT)
        find_package(fmt "8.1.1" QUIET)
    else()
        find_package(fmt "8.1.1" REQUIRED)
    endif()

    if(fmt_FOUND)
        return()
    endif()
endif()

include(DownloadUsingCPM)
cpmaddpackage(
    NAME fmt
    VERSION 11.1.4
    URL https://github.com/fmtlib/fmt/archive/11.1.4.tar.gz
    URL_HASH SHA256=ac366b7b4c2e9f0dde63a59b3feb5ee59b67974b14ee5dc9ea8ad78aa2c1ee1e
)
