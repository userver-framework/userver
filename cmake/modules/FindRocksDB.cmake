_userver_module_begin(
    NAME RocksDB
    DEBIAN_NAMES librocksdb-dev
    FORMULA_NAMES rocksdb
    RPM_NAMES rocksdb-devel
    PACMAN_NAMES rocksdb
    PKG_CONFIG_NAMES rocksdb
)

_userver_module_find_include(NAMES rocksdb/db.h)

_userver_module_find_library(NAMES rocksdb)

_userver_module_end()

if(RocksDB_FOUND AND NOT TARGET RocksDB::rocksdb)
    add_library(RocksDB::rocksdb ALIAS RocksDB)
endif()
