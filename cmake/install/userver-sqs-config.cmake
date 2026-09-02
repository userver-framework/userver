if(USERVER_SQS_CONFIG_PHASE STREQUAL "deps")
    if(_USERVER_SQS_DEPS_LOADED)
        return()
    endif()

    find_dependency(AWSSDK REQUIRED CONFIG COMPONENTS core sqs)

    set(_USERVER_SQS_DEPS_LOADED TRUE)
    return()
endif()

if(userver_sqs_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

if(USERVER_CONAN)
    find_package(AWSSDK REQUIRED CONFIG COMPONENTS core sqs)
else()
    include("${USERVER_CMAKE_DIR}/SetupAwsSdkCpp.cmake")
endif()

set(userver_sqs_FOUND TRUE)
