project(userver-core-tests-graceful-shutdown CXX)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*pp")
add_executable(${PROJECT_NAME} ${SOURCES} "main.cpp")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_chaos_testsuite_add()
