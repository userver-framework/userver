project(userver-grpc-handlers CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*pp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*pp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC userver-core)
target_link_libraries(${PROJECT_NAME} PUBLIC userver-grpc-internal)

userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS healthchecking/healthchecking.proto)
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}-proto)

if (USERVER_IS_THE_ROOT_PROJECT)
    add_executable(${PROJECT_NAME}-unittest ${UNIT_TEST_SOURCES})
    target_include_directories(${PROJECT_NAME}-unittest PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(${PROJECT_NAME}-unittest
      PUBLIC
        ${PROJECT_NAME}
        userver-utest
    )
    add_google_tests(${PROJECT_NAME}-unittest)

    add_subdirectory(functional_tests)
endif()
