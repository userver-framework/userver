project(chaotic-golden-tests)

include(GoogleTest)
include(ChaoticGen)

file(GLOB_RECURSE SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.yaml")
userver_target_generate_chaotic(
    ${PROJECT_NAME}-chgen
    PARSE_EXTRA_FORMATS
    GENERATE_SERIALIZERS
    INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    LAYOUT
    "/components/schemas/([^/]*)/=ns::{0}"
    FORMAT ON
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src"
    SCHEMAS ${SCHEMAS}
    RELATIVE_TO "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME chaotic-golden COMMAND # Diff returns 0 if files are the same, 1 if they differ
                                     diff -uNrpB "${CMAKE_CURRENT_SOURCE_DIR}/output" "${CMAKE_CURRENT_BINARY_DIR}/src"
)
add_custom_target(
    update-golden-tests
    DEPENDS ${PROJECT_NAME}-chgen
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/output"
)
