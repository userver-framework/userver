include_guard(GLOBAL)

option(USERVER_DOWNLOAD_PACKAGE_KAFKA "Download and setup librdkafka if no librdkafka matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

set(USERVER_KAFKA_VERSION "2.4.0")

find_package(OpenSSL COMPONENTS SSL Crypto REQUIRED)
find_package(CURL REQUIRED)
find_package(libz REQUIRED)

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if(USERVER_DOWNLOAD_PACKAGE_KAFKA)
    find_package(RdKafka QUIET)
  else()
    find_package(RdKafka REQUIRED)
  endif()

  if(RdKafka_FOUND)
    message(STATUS "RdKafka libraries: ${RdKafka_LIBRARIES}")
    message(STATUS "RdKafka include dirs: ${RdKafka_INCLUDE_DIRS}")
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
  "WITH_ZSTD OFF"
  "WITH_LIBDL OFF"
  "ENABLE_LZ4_EXT OFF"
)

set(KAFKA_CPM TRUE)

set(RdKafka_LIBRARIES "${RdKafka_BINARY_DIR}/src/librdkafka.a")
set(RdKafka_INCLUDE_DIRS "${RdKafka_SOURCE_DIR}/src")

message(STATUS "RdKafka libraries: ${RdKafka_LIBRARIES}")
message(STATUS "RdKafka include dirs: ${RdKafka_INCLUDE_DIRS}")

target_compile_options(rdkafka PRIVATE "-Wno-ignored-qualifiers")
