project(userver-ydb CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

file(GLOB_RECURSE FUNCTIONAL_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/functional_tests/*
)
list(REMOVE_ITEM SOURCES ${FUNCTIONAL_TEST_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_library(userver::ydb ALIAS ${PROJECT_NAME})

# Enforce that userver-ydb requires C++20
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
if (CMAKE_CXX_STANDARD LESS 20)
  message(FATAL_ERROR "YDB driver requires CMAKE_CXX_STANDARD >= 20 (got: ${CMAKE_CXX_STANDARD})")
endif()

include(SetupYdbCppSDK)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
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
    userver-core
)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    ${ydb-cpp-sdk_INCLUDE_DIRS}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

_userver_directory_install(COMPONENT ydb
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT ydb TARGETS ${PROJECT_NAME})

_userver_directory_install(COMPONENT ydb FILES
    "${USERVER_ROOT_DIR}/cmake/install/userver-ydb-config.cmake"
    "${USERVER_ROOT_DIR}/cmake/SetupYdbCPPSDK.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_subdirectory(tests)
  add_subdirectory(functional_tests)
endif()
