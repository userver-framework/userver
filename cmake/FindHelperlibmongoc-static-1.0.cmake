find_package_required_version(libmongoc-static-1.0
    "libyandex-taxi-mongo-c-driver-dev" 1.14.0)

if (MONGOC_STATIC_LIBRARIES AND NOT TARGET libmongoc-static-1.0)
  add_library(libmongoc-static-1.0 INTERFACE)
  target_link_libraries(libmongoc-static-1.0 INTERFACE ${MONGOC_STATIC_LIBRARIES})
  target_include_directories(libmongoc-static-1.0 INTERFACE ${MONGOC_STATIC_INCLUDE_DIRS})
endif()
