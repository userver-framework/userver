project(userver-kafka CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE KAFKA_LIB_UTEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/utest/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/utest/*.hpp)
list(REMOVE_ITEM SOURCES ${KAFKA_LIB_UTEST_SOURCES})

file(GLOB_RECURSE KAFKA_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*test.cpp)
list(REMOVE_ITEM SOURCES ${KAFKA_TEST_SOURCES})

file(GLOB_RECURSE KAFKA_FUNCTIONAL_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/functional_tests/*)
list(REMOVE_ITEM SOURCES ${KAFKA_FUNCTIONAL_TEST_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})

if(USERVER_CONAN)
  find_package(RdKafka REQUIRED)
  set_target_properties(RdKafka::rdkafka++ PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(rdkafka ALIAS RdKafka::rdkafka++)
else()
  include(SetupRdKafka)
endif()

target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-ignored-qualifiers")
target_link_libraries(${PROJECT_NAME}
  PUBLIC userver::core
)
target_include_directories(${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(${PROJECT_NAME}
  PRIVATE "${RdKafka_LIBRARIES}" sasl2 z CURL::libcurl OpenSSL::SSL OpenSSL::Crypto
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "${RdKafka_INCLUDE_DIRS}"
)

if(KAFKA_CPM)
  message(STATUS "Including from wrapper ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/include")
  target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/include
  )
endif()

_userver_directory_install(COMPONENT kafka
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/.."
)
_userver_install_targets(COMPONENT kafka TARGETS ${PROJECT_NAME})

_userver_directory_install(COMPONENT kafka
  FILES
  "${USERVER_ROOT_DIR}/cmake/install/userver-kafka-config.cmake"
  "${USERVER_ROOT_DIR}/cmake/modules/FindRdKafka.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

if(USERVER_FEATURE_UTEST)
  add_subdirectory(utest)
endif()

if(USERVER_IS_THE_ROOT_PROJECT)
  add_subdirectory(functional_tests)
  add_subdirectory(tests)
endif()
