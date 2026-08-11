project(chaotic-golden-tests)

include(GoogleTest)
include(ChaoticGen)

file(GLOB_RECURSE SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.yaml")
userver_target_generate_chaotic(
    ${PROJECT_NAME}-chgen
    PARSE_EXTRA_FORMATS GENERATE_SERIALIZERS
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/../include
    LAYOUT "/components/schemas/([^/]*)/=ns::{0}"
    FORMAT OFF
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src"
    SCHEMAS ${SCHEMAS}
    RELATIVE_TO "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(
    NAME chaotic-golden
    COMMAND
        "${USERVER_CHAOTIC_PYTEST_PYTHON_BINARY}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/diff-clang-format.py"
        --golden-dir "${CMAKE_CURRENT_SOURCE_DIR}/output/schemas" --generated-dir
        "${CMAKE_CURRENT_BINARY_DIR}/src/schemas"
)

userver_target_generate_chaotic_dynamic_configs(${PROJECT_NAME}-dynamic-configs dynamic_configs/*.yaml)

add_test(
    NAME chaotic-golden-dynconfigs
    COMMAND
        "${USERVER_CHAOTIC_PYTEST_PYTHON_BINARY}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/diff-clang-format.py"
        --golden-dir "${CMAKE_CURRENT_SOURCE_DIR}/output/dynamic_configs" --generated-dir
        "${CMAKE_CURRENT_BINARY_DIR}/dynamic_configs"
)

add_custom_target(
    update-golden-tests
    DEPENDS ${PROJECT_NAME}-chgen ${PROJECT_NAME}-dynamic-configs
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/output"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/dynamic_configs"
            "${CMAKE_CURRENT_SOURCE_DIR}/output/dynamic_configs"
)
