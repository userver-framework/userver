project(userver-rabbitmq CXX)

if (USERVER_CONAN)
  find_package(amqpcpp REQUIRED CONFIG)
else()
  include(SetupAmqpCPP)
endif()

userver_module(rabbitmq
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE amqpcpp

    DBTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_rmqtest.cpp"
    DBTEST_DATABASES rabbitmq
)

_userver_directory_install(
    COMPONENT rabbitmq
    FILES "${USERVER_ROOT_DIR}/cmake/SetupAmqpCPP.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)

if (USERVER_BUILD_TESTS)
  set_tests_properties(${PROJECT_NAME}-dbtest PROPERTIES ENVIRONMENT
          "TESTSUITE_RABBITMQ_SERVER_START_TIMEOUT=120.0")

  add_subdirectory(functional_tests)
endif()
