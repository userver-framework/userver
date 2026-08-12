cmake_minimum_required(VERSION 3.14)
project(userver-samples-periodic-task CXX)

# /// [find_userver]
find_package(
    userver
    COMPONENTS core
    REQUIRED
)
# Note: If userver was added via add_subdirectory(path/to/userver), then the userver_setup_environment() should be
# called here. /// [find_userver]

# /// [executable]
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} userver::core)
# /// [executable]

# /// [testsuite] Makes a virtualenv suitable for running pytests from `test` directory and integrates with `ctest`.
userver_testsuite_add_simple()
# /// [testsuite]
