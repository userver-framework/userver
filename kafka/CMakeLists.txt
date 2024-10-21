project(userver-kafka CXX)

if(USERVER_CONAN)
  find_package(OpenSSL COMPONENTS SSL Crypto REQUIRED)
  find_package(CURL REQUIRED)
  find_package(libz REQUIRED)
  find_package(RdKafka REQUIRED)
else()
  include(SetupRdKafka)
endif()

userver_module(kafka
  SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}

  LINK_LIBRARIES_PRIVATE
    "${RdKafka_LIBRARIES}"
    sasl2
    z
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
  INCLUDE_DIRS_PRIVATE
    "${RdKafka_INCLUDE_DIRS}"

  UTEST_LINK_LIBRARIES
    userver-kafka-utest
)

file(GLOB_RECURSE KAFKA_LIB_UTEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/utest/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/utest/*.hpp)

file(GLOB_RECURSE KAFKA_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*test.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-ignored-qualifiers")

if(KAFKA_CPM)
  message(STATUS "Including from wrapper ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/include")
  target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/include
  )
endif()

_userver_directory_install(COMPONENT kafka
  FILES
    "${USERVER_ROOT_DIR}/cmake/modules/FindRdKafka.cmake"
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

if(USERVER_FEATURE_UTEST)
  add_subdirectory(utest)
endif()

if(USERVER_IS_THE_ROOT_PROJECT)
  add_subdirectory(functional_tests)
  add_subdirectory(tests)
endif()
