find_package_required_version(PostgreSQL
    "libpq-dev postgresql-server-dev-10" 10)

if (PostgreSQL_LIBRARIES AND NOT TARGET PostgreSQL)
  add_library(PostgreSQL INTERFACE)
  target_link_libraries(PostgreSQL INTERFACE ${PostgreSQL_LIBRARIES})
  target_include_directories(PostgreSQL INTERFACE ${PostgreSQL_INCLUDE_DIRS})
endif()
