project(userver-mongo-tests-metrics CXX)

add_executable(${PROJECT_NAME} "mongo_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::mongo)

set(mongo_pool_implementations legacy)
if(USERVER_MONGO_HAS_EXPERIMENTAL_DRIVER_API)
    list(APPEND mongo_pool_implementations experimental)
endif()

foreach(pool_implementation IN LISTS mongo_pool_implementations)
    if(pool_implementation STREQUAL "experimental")
        set(driver_impl mongo-c-driver-experimental)
        set(thread_backend true)
    else()
        set(driver_impl mongo-c-driver)
        set(thread_backend false)
    endif()

    set(config_vars_path "${CMAKE_CURRENT_BINARY_DIR}/config_vars-${pool_implementation}.yaml")
    file(WRITE "${config_vars_path}" "mongo-driver: ${driver_impl}\nmongo-thread-backend: ${thread_backend}\n")

    foreach(tests_directory IN ITEMS tests tests-values)
        userver_testsuite_add_simple(
            WORKING_DIRECTORY "${tests_directory}"
            TEST_SUFFIX "${tests_directory}-${pool_implementation}"
            PYTHON_BINARY "${TESTSUITE_PYTHON_BINARY}"
            CONFIG_VARS_PATH "${config_vars_path}"
        )
    endforeach()
endforeach()
