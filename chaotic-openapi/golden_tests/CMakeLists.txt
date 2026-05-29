project(chaotic-openapi-golden-tests)

include(GoogleTest)
include(ChaoticGen)

file(GLOB_RECURSE SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.yaml")

userver_target_generate_openapi_client(
    ${PROJECT_NAME}-chgen-client
    NAME test
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/client"
    SCHEMAS ${SCHEMAS}
)

userver_target_generate_openapi_handlers(
    ${PROJECT_NAME}-chgen-handlers
    NAME test
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/handlers"
    SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/handlers"
    SCHEMAS ${SCHEMAS}
)

add_test(
    NAME chaotic-openapi-golden
    # Diff returns 0 if files are the same, 1 if they differ
    COMMAND diff -uNrpB "${CMAKE_CURRENT_SOURCE_DIR}/output" "${CMAKE_CURRENT_BINARY_DIR}/src"
)

add_custom_target(
    update-openapi-golden-tests
    DEPENDS ${PROJECT_NAME}-chgen-client ${PROJECT_NAME}-chgen-handlers
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/output"
)
