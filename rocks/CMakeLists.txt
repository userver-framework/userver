project(userver-rocks CXX)

include(SetupRocksDB)

userver_module(
    rocks
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES RocksDB::rocksdb
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
)
