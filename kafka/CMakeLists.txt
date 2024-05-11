project(userver-kafka CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

file(GLOB_RECURSE KAFKA_FUNCTIONAL_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/functional_tests/*
)
list(REMOVE_ITEM SOURCES ${KAFKA_FUNCTIONAL_TEST_SOURCES})

message(STATUS "Statically linking ${PROJECT_NAME} with sources: ${SOURCES}")
add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_library(userver::kafka ALIAS ${PROJECT_NAME})

message(STATUS "Dirs:")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "CMAKE_INSTALL_INCLUDEDIR: ${CMAKE_INSTALL_INCLUDEDIR}")

include(SetupKafkaDeps)

message(STATUS "Linking ${PROJECT_NAME} to userver-core and rdkafka")
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
    rdkafka
)

message(STATUS "Build interface: ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "Library Sources: ${CMAKE_CURRENT_SOURCE_DIR}/src")
message(STATUS "Kafka Sources: ${rdkafka_SOURCE_DIR}/src")

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

message(STATUS "CMAKE_INSTALL_LIBDIR: ${CMAKE_INSTALL_LIBDIR}")

_userver_directory_install(COMPONENT kafka
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT kafka TARGETS ${PROJECT_NAME})

_userver_directory_install(COMPONENT kafka FILES
    "${USERVER_ROOT_DIR}/cmake/install/userver-kafka-config.cmake"
    "${USERVER_ROOT_DIR}/cmake/SetupKafkaDeps.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_subdirectory(functional_tests)
endif()
