project(chaotic-openapi-golden-tests)

include(GoogleTest)
include(ChaoticGen)

file(GLOB_RECURSE SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.yaml")
userver_target_generate_openapi_client(
    ${PROJECT_NAME}-chgen
    NAME test
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src"
    SCHEMAS ${SCHEMAS}
)

add_test(NAME chaotic-openapi-golden
         COMMAND # Diff returns 0 if files are the same, 1 if they differ
                 diff -uNrpB "${CMAKE_CURRENT_SOURCE_DIR}/output" "${CMAKE_CURRENT_BINARY_DIR}/src"
)
add_custom_target(
    update-openapi-golden-tests
    DEPENDS ${PROJECT_NAME}-chgen
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/output"
)
