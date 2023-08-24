project(userver-mongo-tests-basic-chaos CXX)

add_executable(${PROJECT_NAME} "mongo_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-mongo)

userver_chaos_testsuite_add()
