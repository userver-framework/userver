project(userver-sqs CXX)

if(USERVER_CONAN)
    find_package(AWSSDK REQUIRED CONFIG COMPONENTS core sqs)
else()
    include(SetupAwsSdkCpp)
endif()

userver_module(
    sqs
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES AWS::aws-sdk-cpp-core AWS::aws-sdk-cpp-sqs
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/sqs/*test.cpp"
    COMPONENT_DEPENDS core
)

_userver_directory_install(
    COMPONENT sqs
    FILES "${USERVER_ROOT_DIR}/cmake/SetupAwsSdkCpp.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)
