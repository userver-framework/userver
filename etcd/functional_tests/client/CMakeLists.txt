project(userver-etcd-tests-client CXX)

add_executable(${PROJECT_NAME} "etcd_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-etcd)

userver_chaos_testsuite_add()
