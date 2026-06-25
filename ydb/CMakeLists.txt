project(userver-ydb CXX)

include(SetupYdbCppSDK)

set(YDB_LIBRARIES
    YDB-CPP-SDK::Iam
    YDB-CPP-SDK::Coordination
    YDB-CPP-SDK::Driver
    YDB-CPP-SDK::Operation
    YDB-CPP-SDK::Query
    YDB-CPP-SDK::Result
    YDB-CPP-SDK::Scheme
    YDB-CPP-SDK::SolomonStats
    YDB-CPP-SDK::Table
    YDB-CPP-SDK::Topic
    YDB-CPP-SDK::FederatedTopic
    YDB-CPP-SDK::Types
)

userver_module(
    ydb
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES ${YDB_LIBRARIES}
    INCLUDE_DIRS ${ydb-cpp-sdk_INCLUDE_DIRS}
    UTEST_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    UTEST_LINK_LIBRARIES userver-ydb-utest
    GENERATE_DYNAMIC_CONFIGS
    COMPONENT_DEPENDS core
    EMBED_FILES src/ydb/component.yaml src/ydb/dist_lock/component_base.yaml src/ydb/topic_writer_component.yaml
)

_userver_directory_install(
    COMPONENT ydb
    FILES "${USERVER_ROOT_DIR}/cmake/SetupYdbCPPSDK.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)

add_subdirectory(utest)

if(USERVER_BUILD_TESTS)
    add_subdirectory(functional_tests)
endif()
