project(userver-mysql CXX)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE MYSQL_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp")
file(GLOB_RECURSE MYSQL_BENCH_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES})

if(USERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB)
  target_compile_definitions(${PROJECT_NAME}
                             PRIVATE -DUSERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB)
endif()

find_package(libmariadb REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC userver-core)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

_userver_directory_install(
  COMPONENT mysql DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/..)
_userver_install_targets(COMPONENT mysql TARGETS ${PROJECT_NAME})

_userver_directory_install(
  COMPONENT
  mysql
  FILES
  "${USERVER_ROOT_DIR}/cmake/install/userver-mysql-config.cmake"
  "${USERVER_ROOT_DIR}/cmake/modules/Findlibmariadb.cmake"
  DESTINATION
  "${CMAKE_INSTALL_LIBDIR}/cmake/userver")

if(USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}-mysqltest ${MYSQL_TEST_SOURCES})
  target_include_directories(
    ${PROJECT_NAME}-mysqltest
    PRIVATE $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>)
  target_link_libraries(${PROJECT_NAME}-mysqltest
                        PRIVATE userver-utest ${PROJECT_NAME} mariadb)
  userver_add_utest(NAME ${PROJECT_NAME}-mysqltest DATABASES mysql)

  add_executable(${PROJECT_NAME}-benchmark ${MYSQL_BENCH_SOURCES})
  target_include_directories(
    ${PROJECT_NAME}-benchmark
    PRIVATE $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>)
  target_link_libraries(${PROJECT_NAME}-benchmark
                        PRIVATE userver-ubench ${PROJECT_NAME} mariadb)

  add_subdirectory(functional_tests)
endif()
