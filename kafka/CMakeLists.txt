project(userver-kafka CXX)

if(USERVER_CONAN)
  find_package(OpenSSL COMPONENTS SSL Crypto REQUIRED)
  find_package(CURL REQUIRED)
  find_package(libz REQUIRED)
  find_package(RdKafka REQUIRED)
  set_target_properties(RdKafka::rdkafka PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(RdKafka ALIAS RdKafka::rdkafka)
else()
  include(SetupRdKafka)
endif()

userver_module(kafka
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  LINK_LIBRARIES_PRIVATE RdKafka sasl2 z libzstd LZ4 CURL::libcurl OpenSSL::SSL OpenSSL::Crypto
  DBTEST_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  DBTEST_LINK_LIBRARIES userver::kafka-utest
  DBTEST_DATABASES kafka
  DBTEST_ENV
  "TESTSUITE_KAFKA_SERVER_START_TIMEOUT=120.0"
  "TESTSUITE_KAFKA_SERVER_HOST=[::1]"
  "TESTSUITE_KAFKA_SERVER_PORT=8099"
  "TESTSUITE_KAFKA_CONTROLLER_PORT=8100"
  "TESTSUITE_KAFKA_CUSTOM_TOPICS=lt-1:4,lt-2:4,tt-1:1,tt-2:1,tt-3:1,tt-4:1,tt-5:1,tt-6:1,tt-7:1,tt-8:1"
)

target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-ignored-qualifiers")

if(KAFKA_CPM)
  message(STATUS "Including from wrapper ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/include")
  target_include_directories(${PROJECT_NAME}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/include"
  )
endif()

_userver_directory_install(COMPONENT kafka
  FILES
  "${USERVER_ROOT_DIR}/cmake/modules/FindRdKafka.cmake"
  "${USERVER_ROOT_DIR}/cmake/modules/FindLZ4.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)

if(USERVER_FEATURE_UTEST)
  add_subdirectory(utest)
endif()

if(USERVER_IS_THE_ROOT_PROJECT)
  add_subdirectory(functional_tests)
endif()
