project(userver-grpc-handlers CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*pp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*pp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

userver_add_library(${PROJECT_NAME} SOURCES ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC userver-core)
target_link_libraries(${PROJECT_NAME} PUBLIC userver-grpc-internal)

add_grpc_library(${PROJECT_NAME}_proto PROTOS healthchecking/healthchecking.proto)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<INSTALL_INTERFACE:include/userver/grpc/handlers/proto>
  )

set_target_properties(${PROJECT_NAME}_proto PROPERTIES LINKER_LANGUAGE CXX)

userver_export(TARGETS ${PROJECT_NAME}_proto)

target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}_proto)

if (USERVER_IS_THE_ROOT_PROJECT)
    add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
    target_include_directories(${PROJECT_NAME}_unittest PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(${PROJECT_NAME}_unittest
      PUBLIC
        ${PROJECT_NAME}
        userver-utest
    )
    add_google_tests(${PROJECT_NAME}_unittest)

    target_compile_definitions(${PROJECT_NAME}_unittest PRIVATE
      DEFAULT_DYNAMIC_CONFIG_FILENAME="${CMAKE_SOURCE_DIR}/grpc/tests/dynamic_config_fallback.json"
    )

    add_subdirectory(functional_tests)
endif()

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/proto"
  DESTINATION "include/userver/grpc/handlers/"
  FILES_MATCHING
  PATTERN "*.pb.hpp"
  PATTERN "*.pb.h"
  )

userver_export(TARGETS ${PROJECT_NAME})
