if (TARGET CryptoPP)
    return()
endif()

option(
    USERVER_DOWNLOAD_PACKAGE_CRYPTOPP
    "Download and setup CryptoPP if no CryptoPP of matching version was found"
    ${USERVER_DOWNLOAD_PACKAGES}
)

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_CRYPTOPP)
      find_package(CryptoPP QUIET)
  else()
      find_package(CryptoPP REQUIRED)
  endif()

  if (CryptoPP_FOUND)
      return()
  endif()
endif()

include(DownloadUsingCPM)

CPMAddPackage(
    NAME cryptopp-cmake
    VERSION 8.9.0
    GITHUB_REPOSITORY abdes/cryptopp-cmake
    GIT_TAG CRYPTOPP_8_9_0
    OPTIONS
    "CRYPTOPP_BUILD_SHARED OFF"
    "CRYPTOPP_BUILD_TESTING OFF"
    "CRYPTOPP_USE_INTERMEDIATE_OBJECTS_TARGET OFF"
    "CRYPTOPP_USE_OPENMP OFF"
    "CRYPTOPP_DATA_DIR"
)

add_library(CryptoPP INTERFACE)
target_link_libraries(CryptoPP INTERFACE cryptopp)
get_filename_component(cryptopp_parent_directory "${cryptopp_SOURCE_DIR}" DIRECTORY)
target_include_directories(CryptoPP INTERFACE "${cryptopp_parent_directory}")
target_compile_options(cryptopp PRIVATE "-Wno-implicit-fallthrough")

_userver_install_targets(COMPONENT universal TARGETS CryptoPP)
