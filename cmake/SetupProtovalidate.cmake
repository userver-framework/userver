# @ingroup download
option(USERVER_DOWNLOAD_PACKAGE_PROTOVALIDATE_CC "Download and setup protovalidate-cc if no matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)
# @ingroup dependencies
set(USERVER_PROTOVALIDATE_CC_PATH
    ""
    CACHE PATH "Path to the folder with google common proto files"
)

if(NOT USERVER_PROTOVALIDATE_CC_PATH AND USERVER_DOWNLOAD_PACKAGE_PROTOVALIDATE_CC)
    include(DownloadUsingCPM)
    cpmaddpackage(
        NAME protovalidate_cc
        VERSION 0.6.0
        URL https://github.com/bufbuild/protovalidate-cc/archive/v0.6.0.tar.gz
        URL_HASH SHA256=f93183d8b6a93ab489f59ce9edd43a21ac194f7ac8f004b008139ed8ca0abb9a
    )
else()
    find_package(protovalidate_cc CONFIG REQUIRED)
endif()
