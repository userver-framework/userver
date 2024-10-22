project(userver-mysql CXX)

find_package(libmariadb REQUIRED)

userver_module(mysql
  SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}

  LINK_LIBRARIES_PRIVATE
    mariadb

  UBENCH_LINK_LIBRARIES
    mariadb
  UTEST_LINK_LIBRARIES
    mariadb
  DBTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mysqltests/*.cpp
  DBTEST_DATABASES
    mysql
)

if (USERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB)
  target_compile_definitions(${PROJECT_NAME} PRIVATE -DUSERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB)
endif()

_userver_directory_install(COMPONENT mysql FILES
    "${USERVER_ROOT_DIR}/cmake/modules/Findlibmariadb.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_subdirectory(functional_tests)
endif()
