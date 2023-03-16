project(userver-samples-etcd_service CXX)

add_executable(${PROJECT_NAME} "etcd_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-etcd)

userver_sample_testsuite_add()
