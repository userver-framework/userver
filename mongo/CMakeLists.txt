project(userver-mongo CXX)

if(USERVER_CONAN)
    find_package(mongoc-1.0 REQUIRED)
else()
    include(SetupMongoDeps)
endif()

userver_module(
    mongo
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES mongo::bson_static
    LINK_LIBRARIES_PRIVATE mongo::mongoc_static
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    DBTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_mongotest.cpp"
    DBTEST_DATABASES mongo
    # Tests that use IsCollectionWriteConcernTimeout require multi-instance mongo
    DBTEST_ENV TESTSUITE_MONGO_RS_INSTANCE_COUNT=5
    UBENCH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_benchmark.cpp"
    GENERATE_DYNAMIC_CONFIGS
)

_userver_directory_install(
    COMPONENT mongo
    FILES "${USERVER_ROOT_DIR}/cmake/SetupMongoDeps.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

_userver_directory_install(
    COMPONENT mongo
    FILES "${USERVER_ROOT_DIR}/cmake/modules/Findmongoc.cmake" "${USERVER_ROOT_DIR}/cmake/modules/Findbson.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules
)

if(USERVER_BUILD_TESTS)
    target_include_directories(
        ${PROJECT_NAME}-unittest PRIVATE # TODO: hack, expose formats/common/merge_test.hpp as public API
                                         $<TARGET_PROPERTY:userver-universal,INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(${PROJECT_NAME}-unittest PRIVATE -DUSERVER_FEATURE_NO_PATCHED_BSON=1)

    add_subdirectory(functional_tests)
endif()
