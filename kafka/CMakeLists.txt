project(userver-kafka CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*pp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*pp
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

include(SetupRdKafka)

target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-ignored-qualifiers")
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
  PRIVATE
    rdkafka
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (KAFKA_CPM)
  message(STATUS "Linking wrapper ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/include")
  target_include_directories(
    ${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/include
  )
endif()

_userver_directory_install(COMPONENT kafka
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT kafka TARGETS ${PROJECT_NAME})

_userver_directory_install(COMPONENT kafka FILES
    "${USERVER_ROOT_DIR}/cmake/install/userver-kafka-config.cmake"
    "${CMAKE_BINARY_DIR}/cmake_generated/FindRdKafka.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_subdirectory(functional_tests)
endif()
