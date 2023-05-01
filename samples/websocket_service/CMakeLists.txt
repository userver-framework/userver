project(userver-samples-websocket_service CXX)

add_executable(${PROJECT_NAME} "websocket_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/server.crt
    ${PROJECT_BINARY_DIR}/server.crt)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/server.key
    ${PROJECT_BINARY_DIR}/server.key)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/config_dev.yaml
    ${PROJECT_BINARY_DIR}/config_dev.yaml)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/dynamic_config_fallback.json
    ${PROJECT_BINARY_DIR}/dynamic_config_fallback.json)
