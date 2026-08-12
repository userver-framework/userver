project(userver-sqldto)

if(USERVER_BUILD_TESTS)
    userver_venv_setup(
        NAME userver-sqldto-tests
        PYTHON_OUTPUT_VAR USERVER_SQLDTO_TESTS_PYTHON_BINARY
        REQUIREMENTS "${USERVER_ROOT_DIR}/scripts/sqldto/sqldto/generator/requirements.txt"
                     "${USERVER_ROOT_DIR}/testsuite/requirements.txt"
        UNIQUE
    )

    userver_venv_setup(
        NAME userver-sqldto-canonize-tests
        PYTHON_OUTPUT_VAR USERVER_SQLDTO_CANONIZE_TESTS_PYTHON_BINARY
        REQUIREMENTS "${USERVER_ROOT_DIR}/scripts/sqldto/sqldto/dumper/requirements.txt"
                     "${USERVER_ROOT_DIR}/testsuite/requirements.txt"
        UNIQUE
    )

    add_subdirectory(tests/unit_tests)
    add_subdirectory(tests/golden_tests)

    option(USERVER_SQLDTO_INTEGRATION_TESTS "Whether to run integration tests of sqldto" ON)
    if(USERVER_SQLDTO_INTEGRATION_TESTS)
        add_subdirectory(tests/integration_tests)
    endif()
endif()

_userver_directory_install(
    COMPONENT postgresql
    DIRECTORY "${USERVER_ROOT_DIR}/scripts/sqldto"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/scripts"
)
