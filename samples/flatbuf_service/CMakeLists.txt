project(userver-samples-flatbuf_service CXX)

add_executable(${PROJECT_NAME} "flatbuf_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)
