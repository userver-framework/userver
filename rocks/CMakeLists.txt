project(userver-rocks CXX)

include(SetupRocksDB)

userver_module(rocks
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES rocksdb
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
)

_userver_directory_install(COMPONENT rocks
  FILES
  "${USERVER_ROOT_DIR}/cmake/modules/FindRocksDB.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)
