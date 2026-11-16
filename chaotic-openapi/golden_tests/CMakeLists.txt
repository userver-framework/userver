project(chaotic-openapi-golden-tests)

include(GoogleTest)
include(ChaoticGen)

file(GLOB_RECURSE SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.yaml")

userver_target_generate_openapi_client(
    ${PROJECT_NAME}-chgen-client
    NAME test
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/client"
    FORMAT OFF
    SCHEMAS ${SCHEMAS}
)

userver_target_generate_openapi_handlers(
    ${PROJECT_NAME}-chgen-handlers
    NAME test
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/handlers"
    SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/handlers"
    FORMAT OFF
    SCHEMAS ${SCHEMAS}
)

add_test(NAME chaotic-openapi-golden # Diff returns 0 if files are the same, 1 if they differ
         COMMAND diff -uNrpB "${CMAKE_CURRENT_SOURCE_DIR}/output" "${CMAKE_CURRENT_BINARY_DIR}/src"
)

# Documents with x-usrv-cpp-namespace: the schemas, a client and handlers share one output root
set(TAGGED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tagged")
set(TAGGED_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/tagged")

userver_target_generate_chaotic(
    ${PROJECT_NAME}-chgen-tagged-schemas
    OUTPUT_DIR "${TAGGED_OUTPUT_DIR}"
    FORMAT OFF
    SCHEMAS "${TAGGED_DIR}/common/errors.yaml" "${TAGGED_DIR}/common/responses.yaml" "${TAGGED_DIR}/schemas/items.yaml"
)

userver_target_generate_openapi_client(
    ${PROJECT_NAME}-chgen-tagged-client
    NAME shop
    OUTPUT_DIR "${TAGGED_OUTPUT_DIR}"
    FORMAT OFF
    SCHEMAS "${TAGGED_DIR}/api/shop.yaml"
)
target_link_libraries(${PROJECT_NAME}-chgen-tagged-client ${PROJECT_NAME}-chgen-tagged-schemas)

userver_target_generate_openapi_handlers(
    ${PROJECT_NAME}-chgen-tagged-handlers
    NAME shop
    OUTPUT_DIR "${TAGGED_OUTPUT_DIR}"
    SRC_DIR "${TAGGED_OUTPUT_DIR}/views"
    FORMAT OFF
    SCHEMAS "${TAGGED_DIR}/api/shop.yaml"
)
target_link_libraries(${PROJECT_NAME}-chgen-tagged-handlers ${PROJECT_NAME}-chgen-tagged-schemas)

add_test(NAME chaotic-openapi-golden-tagged COMMAND diff -uNrpB "${TAGGED_DIR}/output" "${TAGGED_OUTPUT_DIR}")

add_custom_target(
    update-openapi-golden-tests
    DEPENDS ${PROJECT_NAME}-chgen-client ${PROJECT_NAME}-chgen-handlers ${PROJECT_NAME}-chgen-tagged-schemas
            ${PROJECT_NAME}-chgen-tagged-client ${PROJECT_NAME}-chgen-tagged-handlers
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/output"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${TAGGED_OUTPUT_DIR}" "${TAGGED_DIR}/output"
)
