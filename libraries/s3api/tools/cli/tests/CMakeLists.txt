add_executable(userver-s3-cli-unittest cli_test.cpp ${S3_CLI_SOURCES})
set_target_properties(userver-s3-cli-unittest PROPERTIES EXCLUDE_FROM_ALL FALSE)
target_link_libraries(userver-s3-cli-unittest PRIVATE userver-s3api userver-utest Boost::program_options)
