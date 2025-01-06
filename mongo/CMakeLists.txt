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
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES bson
    LINK_LIBRARIES_PRIVATE mongoc

    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    DBTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_mongotest.cpp"
    DBTEST_DATABASES mongo
    # Tests that use IsCollectionWriteConcernTimeout require multi-instance mongo
    DBTEST_ENV TESTSUITE_MONGO_RS_INSTANCE_COUNT=5
    UBENCH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_benchmark.cpp"
)

_userver_directory_install(COMPONENT mongo FILES
    "${USERVER_ROOT_DIR}/cmake/modules/Findmongoc.cmake"
    "${USERVER_ROOT_DIR}/cmake/modules/Findbson.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

if (USERVER_IS_THE_ROOT_PROJECT)
  target_include_directories(${PROJECT_NAME}-unittest PRIVATE
      # TODO: hack, expose formats/common/merge_test.hpp as public API
      $<TARGET_PROPERTY:userver-universal,INCLUDE_DIRECTORIES>
  )
  target_compile_definitions(${PROJECT_NAME}-unittest PRIVATE
      -DUSERVER_FEATURE_NO_PATCHED_BSON=1
  )

  add_subdirectory(functional_tests)
endif()
