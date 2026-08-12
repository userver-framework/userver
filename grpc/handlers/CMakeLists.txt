project(userver-grpc-handlers CXX)

userver_module(
    grpc-handlers
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    INSTALL_COMPONENT grpc
    LINK_LIBRARIES userver-grpc-internal
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
)

userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS health/v1/health.proto)
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}-proto)

if(USERVER_BUILD_TESTS)
    add_subdirectory(functional_tests)
endif()

_userver_install_targets(COMPONENT grpc TARGETS ${PROJECT_NAME}-proto)
