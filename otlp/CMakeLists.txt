project(userver-otlp CXX)

include(GrpcTargets)

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

userver_add_grpc_library(${PROJECT_NAME}-proto
  PROTOS
    opentelemetry/proto/collector/trace/v1/trace_service.proto
    opentelemetry/proto/collector/logs/v1/logs_service.proto
    opentelemetry/proto/common/v1/common.proto
    opentelemetry/proto/logs/v1/logs.proto
    opentelemetry/proto/resource/v1/resource.proto
    opentelemetry/proto/trace/v1/trace.proto
)

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
    ${PROJECT_NAME}-proto
)

_userver_directory_install(COMPONENT otlp
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT otlp TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-proto)

_userver_directory_install(COMPONENT otlp FILES
  "${USERVER_ROOT_DIR}/cmake/install/userver-otlp-config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)
