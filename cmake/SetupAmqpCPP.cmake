# @ingroup download
option(USERVER_DOWNLOAD_PACKAGE_AMQPCPP "Download and setup amqp-cpp" ${USERVER_DOWNLOAD_PACKAGES})

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
    if(USERVER_DOWNLOAD_PACKAGE_AMQPCPP)
        find_package(amqpcpp QUIET)
    else()
        find_package(amqpcpp REQUIRED)
    endif()

    if(amqpcpp_FOUND)
        return()
    endif()
endif()

include(DownloadUsingCPM)
cpmaddpackage(
    NAME amqp-cpp
    VERSION 4.3.18
    URL https://github.com/CopernicaMarketingSoftware/AMQP-CPP/archive/v4.3.18.tar.gz
    URL_HASH SHA256=cc2c1fc5da00a1778c2804306e06bdedc782a5f74762b9d9b442d3a498dd0c4f
)

target_compile_options(amqpcpp PRIVATE "-Wno-unused-parameter")
