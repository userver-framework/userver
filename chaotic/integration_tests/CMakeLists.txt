project(chaotic-tests)

include(GoogleTest)
include(ChaoticGen)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*pp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    userver-chaotic
    userver-universal-utest
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE SCHEMAS ${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.yaml)
userver_target_generate_chaotic(${PROJECT_NAME}-chgen
    ARGS
        -n "/definitions/([^/]*)/=ns::{0}"
        -f "(.*)={0}"
        -I ${CMAKE_CURRENT_SOURCE_DIR}/include
        -I ${CMAKE_CURRENT_SOURCE_DIR}/../include
        --clang-format=
        --parse-extra-formats
        --generate-serializers
    OUTPUT_DIR
        ${CMAKE_CURRENT_BINARY_DIR}/src
    SCHEMAS
        ${SCHEMAS}
    RELATIVE_TO
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-chgen)

add_google_tests(${PROJECT_NAME})
