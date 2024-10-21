project(userver-clickhouse CXX)

userver_module(clickhouse
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}

  DBTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*_chtest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/tests/utils_test.cpp
  DBTEST_DATABASES
    clickhouse
)

if (USERVER_CONAN)
  find_package(clickhouse-cpp REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE clickhouse-cpp-lib::clickhouse-cpp-lib)
else()
  include(SetupClickhouseCPP)
  target_link_libraries(${PROJECT_NAME} PRIVATE clickhouse-cpp)
endif()

_userver_directory_install(COMPONENT clickhouse FILES
	  "${USERVER_ROOT_DIR}/cmake/modules/Findclickhouse-cpp.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

if (USERVER_IS_THE_ROOT_PROJECT)
  set_tests_properties(${PROJECT_NAME}-dbtest PROPERTIES ENVIRONMENT
          "TESTSUITE_CLICKHOUSE_SERVER_START_TIMEOUT=120.0")

  add_subdirectory(functional_tests)
endif()
