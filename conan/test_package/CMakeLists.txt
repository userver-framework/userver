cmake_minimum_required(VERSION 3.1)
project(PackageTest CXX)

find_package(userver REQUIRED)

add_executable(hello_service hello_service.cpp)
set_property(TARGET hello_service PROPERTY CXX_STANDARD 17)

target_link_libraries(hello_service PRIVATE userver::userver)
