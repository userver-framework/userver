find_package_required_version(libbson-static-1.0
    "libyandex-taxi-mongo-c-driver-dev" 1.15.0)

if (BSON_STATIC_LIBRARIES AND NOT TARGET libbson-static-1.0)
  add_library(libbson-static-1.0 INTERFACE)
  target_link_libraries(libbson-static-1.0 INTERFACE ${BSON_STATIC_LIBRARIES})
  target_include_directories(libbson-static-1.0 INTERFACE ${BSON_STATIC_INCLUDE_DIRS})
endif()
