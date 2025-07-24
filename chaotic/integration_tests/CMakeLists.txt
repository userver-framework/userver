project(chaotic-tests)

include(GoogleTest)
include(ChaoticGen)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*pp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} userver-chaotic userver-universal-utest)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_google_tests(${PROJECT_NAME})

file(GLOB_RECURSE SCHEMAS ${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.yaml)
userver_target_generate_chaotic(
    ${PROJECT_NAME}-chgen
    PARSE_EXTRA_FORMATS
    GENERATE_SERIALIZERS
    INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    LAYOUT
    "/definitions/([^/]*)/=ns::{0}"
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/src
    SCHEMAS ${SCHEMAS}
    RELATIVE_TO ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-chgen)
