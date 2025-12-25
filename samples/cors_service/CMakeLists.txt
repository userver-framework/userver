cmake_minimum_required(VERSION 3.14)
project(userver-samples-cors_service CXX)

# /// [find_userver]
find_package(
    userver
    COMPONENTS core
    REQUIRED
)
# /// [find_userver]

# /// [executable]
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} userver::core)
# /// [executable]

# /// [testsuite] Makes a virtualenv suitable for running pytests from `test` directory and integrates with `ctest`.
userver_testsuite_add_simple()
# /// [testsuite]
