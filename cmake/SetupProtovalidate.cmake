option(USERVER_DOWNLOAD_PACKAGE_PROTOVALIDATE_CC "Download and setup protovalidate-cc if no matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)
set(USERVER_PROTOVALIDATE_CC_PATH
    ""
    CACHE PATH "Path to the folder with google common proto files"
)

if(NOT USERVER_PROTOVALIDATE_CC_PATH AND USERVER_DOWNLOAD_PACKAGE_PROTOVALIDATE_CC)
    include(DownloadUsingCPM)
    cpmaddpackage(NAME protovalidate_cc GIT_TAG v0.6.0 GITHUB_REPOSITORY bufbuild/protovalidate-cc)
else()
    find_package(protovalidate_cc CONFIG REQUIRED)
endif()
