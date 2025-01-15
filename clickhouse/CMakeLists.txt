project(userver-clickhouse CXX)

if (USERVER_CONAN)
  find_package(clickhouse-cpp REQUIRED CONFIG)
  set(CLICKHOUSE_CPP_TARGET clickhouse-cpp-lib::clickhouse-cpp-lib)
else()
  include(SetupClickhouseCPP)
  set(CLICKHOUSE_CPP_TARGET clickhouse-cpp)
endif()

userver_module(clickhouse
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE "${CLICKHOUSE_CPP_TARGET}"

    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    DBTEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*_chtest.cpp"
    # HACK: common source between unittest and dbtest targets.
    "${CMAKE_CURRENT_SOURCE_DIR}/src/storages/tests/utils_test.cpp"
    DBTEST_DATABASES clickhouse
)

target_compile_options(${PROJECT_NAME} PUBLIC "-Wno-error=pedantic")


_userver_directory_install(COMPONENT clickhouse FILES
	  "${USERVER_ROOT_DIR}/cmake/modules/Findclickhouse-cpp.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules
)

if (USERVER_BUILD_TESTS)
  set_tests_properties(${PROJECT_NAME}-dbtest PROPERTIES ENVIRONMENT
          "TESTSUITE_CLICKHOUSE_SERVER_START_TIMEOUT=120.0")

  add_subdirectory(functional_tests)
endif()
