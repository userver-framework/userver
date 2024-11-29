include_guard(GLOBAL)

option(USERVER_DOWNLOAD_PACKAGE_ROCKS "Download and setup RocksDB if no RocksDB matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if(USERVER_DOWNLOAD_PACKAGE_ROCKS)
    find_package(RocksDB QUIET)
  else()
    find_package(RocksDB REQUIRED)
  endif()

  if(RocksDB_FOUND)
    return()
  endif()
endif()

find_package(libgflags REQUIRED)
find_package(libsnappy REQUIRED)
find_package(libz REQUIRED)
find_package(libzip REQUIRED)
find_package(libzstd REQUIRED)

include(DownloadUsingCPM)

CPMAddPackage(
  NAME rocksdb
  GITHUB_REPOSITORY facebook/rocksdb
  GIT_TAG v9.7.4
  OPTIONS
  "ROCKSDB_BUILD_SHARED OFF"
  "WITH_TESTS OFF"
  "WITH_BENCHMARK_TOOLS OFF"
  "WITH_TOOLS OFF"
  "USE_RTTI ON"
)

mark_targets_as_system("${rocksdb_SOURCE_DIR}")
write_package_stub(rocksdb)
