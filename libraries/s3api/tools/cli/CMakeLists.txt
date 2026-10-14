file(GLOB S3_CLI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
list(REMOVE_ITEM S3_CLI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_executable(userver-s3-cli main.cpp ${S3_CLI_SOURCES})
target_link_libraries(userver-s3-cli PRIVATE userver-s3api Boost::program_options)

if(USERVER_BUILD_TESTS)
    add_subdirectory(tests)
endif()
