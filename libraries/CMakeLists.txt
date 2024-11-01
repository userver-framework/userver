option(USERVER_FEATURE_S3API "Build S3 api client library" "${USERVER_LIB_ENABLED_DEFAULT}")

if (USERVER_FEATURE_S3API)
  add_subdirectory(s3api)
endif()

