project(userver-ydb CXX)

include(SetupYdbCppSDK)

set(YDB_LIBRARIES
    YDB-CPP-SDK::Iam
    YDB-CPP-SDK::Coordination
    YDB-CPP-SDK::Driver
    YDB-CPP-SDK::Operation
    YDB-CPP-SDK::Result
    YDB-CPP-SDK::Scheme
    YDB-CPP-SDK::SolomonStats
    YDB-CPP-SDK::Table
    YDB-CPP-SDK::Topic
    YDB-CPP-SDK::Types
)

userver_module(ydb
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES ${YDB_LIBRARIES}
    INCLUDE_DIRS ${ydb-cpp-sdk_INCLUDE_DIRS}
    UTEST_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/tests"
)

# Enforce that userver-ydb requires C++20
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
if (CMAKE_CXX_STANDARD LESS 20)
  message(FATAL_ERROR "YDB driver requires CMAKE_CXX_STANDARD >= 20 (got: ${CMAKE_CXX_STANDARD})")
endif()

_userver_directory_install(COMPONENT ydb
    FILES "${USERVER_ROOT_DIR}/cmake/SetupYdbCPPSDK.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_subdirectory(functional_tests)
endif()
