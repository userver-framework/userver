include_guard(GLOBAL)

option(USERVER_DOWNLOAD_PACKAGE_KAFKA "Download and setup librdkafka if no librdkafka matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

set(USERVER_KAFKA_VERSION "2.3.0")

include("${CMAKE_CURRENT_LIST_DIR}/SetupCURL.cmake")

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(libzstd REQUIRED)
find_package(lz4 REQUIRED)
find_package(SASL2 REQUIRED)

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if(USERVER_DOWNLOAD_PACKAGE_KAFKA)
    find_package(RdKafka QUIET)
  else()
    find_package(RdKafka REQUIRED)
  endif()

  if(RdKafka_FOUND)
    target_link_libraries(RdKafka
      INTERFACE
        ZLIB::ZLIB
        lz4::lz4 CURL::libcurl
        OpenSSL::SSL OpenSSL::Crypto
        SASL2::SASL2 zstd::zstd
    )
    return()
  endif()
endif()

include(DownloadUsingCPM)

message(STATUS "Downloading librdkafka v${USERVER_KAFKA_VERSION}")
CPMAddPackage(
  NAME RdKafka
  GITHUB_REPOSITORY confluentinc/librdkafka
  VERSION ${USERVER_KAFKA_VERSION}
  OPTIONS
  "RDKAFKA_BUILD_STATIC ON"
  "RDKAFKA_BUILD_EXAMPLES OFF"
  "RDKAFKA_BUILD_TESTS OFF"
  "WITH_SSL ON"
  "WITH_SASL ON"
  "WITH CURL ON"
  "WITH_ZLIB ON"
  "WITH_ZSTD ON"
  "WITH_LIBDL OFF"
  "ENABLE_LZ4_EXT ON"
)

set(KAFKA_CPM TRUE)

target_compile_options(rdkafka PRIVATE "-Wno-ignored-qualifiers")
mark_targets_as_system("${RdKafka_SOURCE_DIR}")
add_library(RdKafka::rdkafka ALIAS rdkafka)
