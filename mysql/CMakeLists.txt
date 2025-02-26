project(userver-mysql CXX)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(libmariadb REQUIRED)

userver_module(mysql
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE libmariadb ZLIB::ZLIB OpenSSL::SSL OpenSSL::Crypto

    DBTEST_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    UTEST_LINK_LIBRARIES libmariadb
    DBTEST_DATABASES mysql
    UBENCH_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks"
    UBENCH_LINK_LIBRARIES libmariadb
    UBENCH_DATABASES mysql
)

if (USERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB)
  target_compile_definitions(${PROJECT_NAME} PRIVATE -DUSERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB)
endif()
if(USERVER_MYSQL_OLD_INCLUDE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE -DUSERVER_MYSQL_OLD_INCLUDE=1)
  if(USERVER_BUILD_TESTS)
    target_compile_definitions(${PROJECT_NAME}-dbtest PRIVATE -DUSERVER_MYSQL_OLD_INCLUDE=1)
  endif()
endif()

_userver_directory_install(COMPONENT mysql FILES
    "${USERVER_ROOT_DIR}/cmake/modules/Findlibmariadb.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules"
)

if (USERVER_BUILD_TESTS)
  add_subdirectory(functional_tests)
endif()
