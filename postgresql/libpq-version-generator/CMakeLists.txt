project(userver-impl-libpq-version-generator)

add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

include(SetupPostgresqlDeps)
target_link_libraries(${PROJECT_NAME} PRIVATE PostgreSQLInternal userver-internal-compile-options)
