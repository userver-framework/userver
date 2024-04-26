include_guard(GLOBAL)

option(USERVER_DOWNLOAD_PACKAGE_ROCKS "Download and setup RocksDB if no RocksDB matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

set(USERVER_ROCKSDB_VERSION "8.11.3")

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_ROCKS)
    find_package(RocksDB QUIET)
  else()
    find_package(RocksDB REQUIRED)
  endif()

  if (TARGET RocksDB::rocksdb)
    return()
  endif()
endif()

find_library(GFLAGS gflags)
If(NOT GFLAGS)
  message(FATAL_ERROR "Failed to find gflags: Lunux: sudo apt-get install libgflags-dev")
endif()

find_library(SNAPPY snappy)
If(NOT SNAPPY)
  message(FATAL_ERROR "Failed to find snappy: Lunux: sudo apt-get install libsnappy-dev")
endif()

find_library(ZLIB z zlib1g)
If(NOT ZLIB)
  message(FATAL_ERROR "${ZLIB} Failed to find zlib1g: Lunux: sudo apt-get install zlib1g-dev")
endif()

find_library(BZIP2 bz2 libbz2)
If(NOT BZIP2)
  message(FATAL_ERROR "Failed to find bzip2: Lunux: sudo apt-get install libbz2-dev")
endif()

find_library(ZSTD zstd)
If(NOT ZSTD)
  message(FATAL_ERROR "Failed to find zstd: Lunux: sudo apt-get install libzstd-dev")
endif()

include(DownloadUsingCPM)

CPMAddPackage(
  NAME rocksdb
  GITHUB_REPOSITORY facebook/rocksdb
  GIT_TAG v${USERVER_ROCKSDB_VERSION}
)
add_library(RocksDB::rocksdb ALIAS rocksdb)
