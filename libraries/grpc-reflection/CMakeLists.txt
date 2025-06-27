project(userver-grpc-reflection CXX)

userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS reflection.proto)

_userver_install_targets(COMPONENT grpc-reflection TARGETS ${PROJECT_NAME}-proto)

userver_module(
    grpc-reflection
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES ${PROJECT_NAME}-proto userver::grpc
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
)

if(USERVER_BUILD_TESTS)
    add_subdirectory(functional_tests)
endif()
