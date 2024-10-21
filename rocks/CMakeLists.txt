project(userver-rocks CXX)

include(SetupRocksDeps)

userver_module(rocks
  SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBRARIES
    RocksDB::rocksdb
)
