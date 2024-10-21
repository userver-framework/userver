project(userver-mongo CXX)

if (USERVER_CONAN)
  find_package(mongoc-1.0 REQUIRED)
  set_target_properties(mongo::bson_static PROPERTIES IMPORTED_GLOBAL TRUE)
  set_target_properties(mongo::mongoc_static PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(bson ALIAS mongo::bson_static)
  add_library(mongoc ALIAS mongo::mongoc_static)
  find_package(cyrus-sasl REQUIRED)
else()
  find_package(bson REQUIRED)
  find_package(mongoc REQUIRED)
endif()

userver_module(mongo
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBRARIES
    userver-core
    bson
  LINK_LIBRARIES_PRIVATE
    mongoc

  DBTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*_mongotest.cpp
  DBTEST_DATABASES
    mongo
)

_userver_directory_install(COMPONENT mongo FILES
    "${USERVER_ROOT_DIR}/cmake/modules/Findmongoc.cmake"
    "${USERVER_ROOT_DIR}/cmake/modules/Findbson.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

if (USERVER_IS_THE_ROOT_PROJECT)
  target_compile_definitions(${PROJECT_NAME}-unittest
      PUBLIC
        -DUSERVER_FEATURE_NO_PATCHED_BSON=1
  )
  # Tests that use IsCollectionWriteConcernTimeout require multi-instance mongo
  set_property(
    TEST
      ${PROJECT_NAME}-dbtest
    PROPERTY ENVIRONMENT "TESTSUITE_MONGO_RS_INSTANCE_COUNT=5"
  )

  add_subdirectory(functional_tests)
endif()
