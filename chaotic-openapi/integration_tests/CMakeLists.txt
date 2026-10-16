project(userver-chaotic-openapi-integration-tests CXX)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*pp)
add_executable(${PROJECT_NAME} ${SOURCES})
add_google_tests(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} userver-chaotic-openapi userver-universal-utest userver-utest)

set(TEST_TYPES_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(
    GLOB CLIENTS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/clients/
    ${CMAKE_CURRENT_SOURCE_DIR}/clients/*
)
foreach(CLIENT ${CLIENTS})
    string(REPLACE "-" "_" CLIENT_NAME ${CLIENT})
    file(GLOB SCHEMAS ${CMAKE_CURRENT_SOURCE_DIR}/clients/${CLIENT}/*.yaml)
    userver_target_generate_openapi_client(
        ${PROJECT_NAME}-client-${CLIENT}
        NAME ${CLIENT_NAME}
        SCHEMAS ${SCHEMAS}
        ARGS -I ${TEST_TYPES_INCLUDE_DIR}
    )
    target_include_directories(${PROJECT_NAME}-client-${CLIENT} PUBLIC ${TEST_TYPES_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-client-${CLIENT})
endforeach()

file(
    GLOB HANDLERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/handlers/
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/*
)
foreach(HANDLER ${HANDLERS})
    file(GLOB SCHEMAS ${CMAKE_CURRENT_SOURCE_DIR}/handlers/${HANDLER}/*.yaml)
    userver_target_generate_openapi_handlers(
        ${PROJECT_NAME}-handler-${HANDLER}
        NAME ${HANDLER}
        SCHEMAS ${SCHEMAS}
        SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
        ARGS -I ${TEST_TYPES_INCLUDE_DIR}
    )
    target_include_directories(${PROJECT_NAME}-handler-${HANDLER} PUBLIC ${TEST_TYPES_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-handler-${HANDLER})
endforeach()
