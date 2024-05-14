include_guard(GLOBAL)

option(USERVER_DOWNLOAD_PACKAGE_KAFKA "Download and setup librdkafka if no librdkafka matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

set(USERVER_KAFKA_VERSION "2.4.0")

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_KAFKA)
    find_package(RdKafka QUIET)
  else()
    find_package(RdKafka REQUIRED)
  endif()

  if (RdKafka_FOUND)
    message(STATUS "RdKafka target found! No download required")
    return()
  endif()
endif()

include(DownloadUsingCPM)

message(STATUS "Downloading librdkafka v${USERVER_KAFKA_VERSION}")
CPMAddPackage(
  NAME rdkafka
  GITHUB_REPOSITORY confluentinc/librdkafka
  VERSION ${USERVER_KAFKA_VERSION}
  OPTIONS
  "RDKAFKA_BUILD_STATIC ON"
  "RDKAFKA_BUILD_EXAMPLES OFF"
  "RDKAFKA_BUILD_TESTS OFF"
  "WITH_SSL ON"
  "WITH_SASL ON"
  "WITH_ZLIB OFF"
  "WITH_ZSTD OFF"
  "ENABLE_LZ4_EXT OFF"
  "WITH_LIBDL OFF"
)

set(KAFKA_CPM TRUE)
