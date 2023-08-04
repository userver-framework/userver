option(USERVER_DOWNLOAD_PACKAGE_AMQPCPP "Download and setup amqp-cpp" ${USERVER_DOWNLOAD_PACKAGES})

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_AMQPCPP)
    find_package(amqpcpp QUIET)
  else()
    find_package(amqpcpp REQUIRED)
  endif()

  if (amqpcpp_FOUND)
    return()
  endif()
endif()

include(DownloadUsingCPM)
CPMAddPackage(
    NAME amqp-cpp
    VERSION 4.3.18
    GITHUB_REPOSITORY CopernicaMarketingSoftware/AMQP-CPP
)

target_compile_options(amqpcpp PRIVATE "-Wno-unused-parameter")
