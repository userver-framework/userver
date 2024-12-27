project(userver-kafka CXX)

if(USERVER_CONAN)
  find_package(OpenSSL COMPONENTS SSL Crypto REQUIRED CONFIG)
  find_package(CURL REQUIRED CONFIG)
  find_package(ZLIB REQUIRED CONFIG)

  find_package(zstd REQUIRED CONFIG)
  # According to https://conan.io/center/recipes/zstd should be
  # zstd::libzstd_static, but it does not work that way
  if (TARGET zstd::libzstd_static)
    add_library(libzstd ALIAS zstd::libzstd_static)
  else()
    add_library(libzstd ALIAS zstd::libzstd_shared)
  endif()
  
  find_package(RdKafka REQUIRED CONFIG)
  set_target_properties(RdKafka::rdkafka PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(RdKafka ALIAS RdKafka::rdkafka)

  find_package(cyrus-sasl REQUIRED CONFIG)
  add_library(sasl2 ALIAS cyrus-sasl::cyrus-sasl)
else()
  include(SetupRdKafka)
endif()

userver_module(kafka
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  LINK_LIBRARIES_PRIVATE RdKafka sasl2 ZLIB::ZLIB libzstd lz4::lz4 CURL::libcurl OpenSSL::SSL OpenSSL::Crypto
  DBTEST_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  DBTEST_LINK_LIBRARIES userver::kafka-utest
  DBTEST_DATABASES kafka
  DBTEST_ENV
  "TESTSUITE_KAFKA_SERVER_START_TIMEOUT=120.0"
  "TESTSUITE_KAFKA_SERVER_HOST=[::1]"
  "TESTSUITE_KAFKA_SERVER_PORT=8099"
  "TESTSUITE_KAFKA_CONTROLLER_PORT=8100"
  "TESTSUITE_KAFKA_CUSTOM_TOPICS=bt:4,lt-1:4,lt-2:4,tt-1:1,tt-2:1,tt-3:1,tt-4:1,tt-5:1,tt-6:1,tt-7:1,tt-8:1"
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
    "${USERVER_ROOT_DIR}/cmake/modules/Findlz4.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules"
)

if(USERVER_FEATURE_UTEST)
  add_subdirectory(utest)
endif()

if(USERVER_BUILD_TESTS)
  add_subdirectory(functional_tests)
endif()
