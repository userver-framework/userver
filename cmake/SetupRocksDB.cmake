include_guard(GLOBAL)

option(USERVER_DOWNLOAD_PACKAGE_ROCKS "Download and setup RocksDB if no RocksDB matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if(USERVER_DOWNLOAD_PACKAGE_ROCKS)
    find_package(RocksDB QUIET CONFIG)
  else()
    find_package(RocksDB REQUIRED CONFIG)
  endif()

  if(RocksDB_FOUND)
    return()
  endif()
endif()

find_package(libgflags REQUIRED)
find_package(libsnappy REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(libzstd REQUIRED)

include(DownloadUsingCPM)

CPMAddPackage(
  NAME rocksdb
  GITHUB_REPOSITORY facebook/rocksdb
  GIT_TAG v9.7.4
  OPTIONS
  "ROCKSDB_BUILD_SHARED OFF"
  "WITH_SNAPPY ON"
  "WITH_BZ2 ON"
  "WITH_ZSTD ON"
  "WITH_TESTS OFF"
  "WITH_BENCHMARK_TOOLS OFF"
  "WITH_TOOLS OFF"
  "WITH_CORE_TOOLS OFF"
  "WITH_TRACE_TOOLS OFF"
  "USE_RTTI ON"
  "GFLAGS_SHARED FALSE"
)

mark_targets_as_system("${rocksdb_SOURCE_DIR}")
add_library(RocksDB::rocksdb ALIAS rocksdb)
