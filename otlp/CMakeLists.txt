project(userver-otlp CXX)

include(GrpcTargets)
include(SetupOpentelemetryProto)

find_package(Boost REQUIRED regex)

file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

file(GLOB_RECURSE BENCH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp
)
list (REMOVE_ITEM SOURCES ${BENCH_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories (${PROJECT_NAME} PRIVATE
    $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
    userver-otlp-proto
)

_userver_directory_install(COMPONENT otlp
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT otlp TARGETS ${PROJECT_NAME} userver-otlp-proto)

_userver_directory_install(COMPONENT otlp FILES
  "${USERVER_ROOT_DIR}/cmake/install/userver-otlp-config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}-unittest ${UNIT_TEST_SOURCES})
  target_include_directories(${PROJECT_NAME}-unittest PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  target_link_libraries(${PROJECT_NAME}-unittest
      userver-utest
      userver-grpc-utest
      ${PROJECT_NAME}
  )
  add_google_tests(${PROJECT_NAME}-unittest)
endif()
