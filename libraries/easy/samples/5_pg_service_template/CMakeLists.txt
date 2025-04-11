project(userver-easy-samples-pg-service-template CXX)

find_package(userver COMPONENTS easy REQUIRED)

add_executable(${PROJECT_NAME} "src/main.cpp")
target_link_libraries(${PROJECT_NAME} userver::easy)

userver_testsuite_add_simple()
