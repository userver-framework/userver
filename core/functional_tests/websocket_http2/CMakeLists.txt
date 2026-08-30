project(userver-core-tests-websocket-http2 CXX)

# The very same handlers as the HTTP/1.1 websocket test: a websocket handler is not
# supposed to notice which transport bootstrapped it.
add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/../websocket/service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_chaos_testsuite_add()
