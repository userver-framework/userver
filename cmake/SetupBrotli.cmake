option(USERVER_DOWNLOAD_PACKAGE_BROTLI "Download and setup Brotli if no Brotli of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

set(USERVER_BROTLI_VERSION 1.1.0)

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_BROTLI)
    find_package(Brotli ${USERVER_BROTLI_VERSION} QUIET)
  else()
    find_package(Brotli ${USERVER_BROTLI_VERSION} REQUIRED)
  endif()

  if (Brotli_FOUND)
    if(NOT TARGET Brotli::dec)
      add_library(Brotli::dec ALIAS brotlidec)
    endif()
    if(NOT TARGET Brotli::enc)
      add_library(Brotli::enc ALIAS brotlienc)
    endif()
    return()
  endif()
endif()

include(DownloadUsingCPM)
CPMAddPackage(
  NAME Brotli
  VERSION ${USERVER_BROTLI_VERSION}
  GITHUB_REPOSITORY google/brotli
  OPTIONS
  "BROTLI_DISABLE_TESTS TRUE"
  "BUILD_SHARED_LIBS OFF"
)

set(Brotli_FOUND TRUE)
set(Brotli_VERSION ${USERVER_BROTLI_VERSION})
add_custom_target(Brotli)
write_package_stub(Brotli)

if(NOT TARGET Brotli::dec)
  add_library(Brotli::dec ALIAS brotlidec)
endif()
if(NOT TARGET Brotli::enc)
  add_library(Brotli::enc ALIAS brotlienc)
endif()
