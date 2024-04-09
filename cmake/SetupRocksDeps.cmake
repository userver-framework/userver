include_guard(GLOBAL)

find_library(ROCKSDB_LIBRARY rocksdb)

if(NOT ROCKSDB_LIBRARY)
  message(FATAL_ERROR "RocksDB library not found
  Linux: install lib from https://github.com/facebook/rocksdb.git
  MacOS: brew install rocksdb")
endif()
