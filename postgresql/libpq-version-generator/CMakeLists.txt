project(userver-impl-libpq-version-generator)

add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

if(NOT USERVER_CONAN)
    find_library(PostgreSQL_LIBRARY NAMES libpq.so libpq.dylib REQUIRED)
endif()
find_package(PostgreSQL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE PostgreSQL::PostgreSQL userver-internal-compile-options)
