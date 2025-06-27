project(userver-chaotic-openapi CXX)

include(ChaoticGen)

userver_module(
    chaotic-openapi
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES userver-core userver-chaotic
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    UTEST_LINK_LIBRARIES userver-core userver-chaotic
)

if(USERVER_BUILD_TESTS)
    userver_venv_setup(
        NAME userver-chaotic-tests
        PYTHON_OUTPUT_VAR USERVER_CHAOTIC_OPENAPI_PYTEST_PYTHON_BINARY
        REQUIREMENTS "${USERVER_ROOT_DIR}/scripts/chaotic/requirements.txt"
                     "${USERVER_ROOT_DIR}/testsuite/requirements.txt"
        UNIQUE
    )
    add_test(
        NAME chaotic-openapi-tests-pytest
        COMMAND env "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_CURRENT_SOURCE_DIR}/../chaotic"
                "${USERVER_CHAOTIC_OPENAPI_PYTEST_PYTHON_BINARY}" -m pytest -vvs "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )

    add_subdirectory(integration_tests)
    add_subdirectory(golden_tests)
endif()

_userver_directory_install(
    COMPONENT chaotic-openapi
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/chaotic_openapi"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/userver"
)
