project(userver-odbc CXX)

userver_module(
    odbc
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE odbc
    DBTEST_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    UTEST_LINK_LIBRARIES odbc
    DBTEST_DATABASES postgresql
    DBTEST_ENV POSTGRES_TEST_DSN=postgresql://testsuite@localhost:15433/postgres
    COMPONENT_DEPENDS core
    EMBED_FILES src/storages/odbc/component.yaml
)

userver_target_generate_chaotic_dynamic_configs(${PROJECT_NAME}-dynamic-configs dynamic_configs/*.yaml)
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}-dynamic-configs)

_userver_install_targets(
    COMPONENT odbc TARGETS ${PROJECT_NAME}-dynamic-configs
)

_userver_directory_install(
    COMPONENT odbc
    FILES "${USERVER_ROOT_DIR}/cmake/modules/Findlibodbc.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules"
)

if(USERVER_BUILD_TESTS)
    add_subdirectory(functional_tests)
endif()
