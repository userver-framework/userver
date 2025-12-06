project(userver-impl-libpq-version-generator)

add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

find_package(PostgreSQL REQUIRED)
find_package(GssApi REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE PostgreSQL::PostgreSQL GssApi)
