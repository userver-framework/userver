_userver_module_begin(
    NAME RocksDB
    VERSION 8.9.1
    DEBIAN_NAMES librocksdb-dev
    FORMULA_NAMES rocksdb
    PACMAN_NAMES rocksdb
    PKG_CONFIG_NAMES rocksdb
)

_userver_module_find_include(
    NAMES rocksdb/db.h
)

_userver_module_find_library(
    NAMES rocksdb
)

_userver_module_end()
