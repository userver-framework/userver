project(userver-chaotic CXX)

include(ChaoticGen)

userver_module(
    chaotic
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    LINK_LIBRARIES userver::universal
    NO_CORE_LINK
)

if(USERVER_BUILD_TESTS)
    userver_venv_setup(
        NAME userver-chaotic-tests
        PYTHON_OUTPUT_VAR USERVER_CHAOTIC_PYTEST_PYTHON_BINARY
        REQUIREMENTS "${USERVER_ROOT_DIR}/scripts/chaotic/requirements.txt"
                     "${USERVER_ROOT_DIR}/testsuite/requirements.txt"
        UNIQUE
    )
    add_test(NAME chaotic-tests-pytest
             COMMAND env "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}" "${USERVER_CHAOTIC_PYTEST_PYTHON_BINARY}" -m pytest
                     -vvs "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )

    add_subdirectory(integration_tests)
    add_subdirectory(golden_tests)
endif()

_userver_directory_install(
    COMPONENT chaotic
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/chaotic"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/userver"
)
_userver_directory_install(
    COMPONENT chaotic
    PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/bin/chaotic-gen"
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
_userver_directory_install(
    COMPONENT chaotic
    FILES "${USERVER_ROOT_DIR}/cmake/ChaoticGen.cmake" "${USERVER_ROOT_DIR}/cmake/install/userver-chaotic-config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)
_userver_directory_install(
    COMPONENT chaotic
    DIRECTORY "${USERVER_ROOT_DIR}/scripts/chaotic"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)
