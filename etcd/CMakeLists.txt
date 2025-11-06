project(userver-etcd CXX)

file(GLOB_RECURSE SCHEMAS ${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.yaml)
userver_target_generate_chaotic(${PROJECT_NAME}-chgen
    GENERATE_SERIALIZERS
    LAYOUT
        "/components/schemas/([^/]*)/=etcd_schemas::{0}"
    OUTPUT_DIR
        ${CMAKE_CURRENT_BINARY_DIR}/src
    SCHEMAS
        ${SCHEMAS}
    RELATIVE_TO
        ${CMAKE_CURRENT_SOURCE_DIR}
)

userver_module(etcd
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    LINK_LIBRARIES ${PROJECT_NAME}-chgen
)

if (USERVER_BUILD_TESTS)
  add_subdirectory(functional_tests)
endif()
