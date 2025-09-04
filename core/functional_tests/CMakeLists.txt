project(userver-core-tests CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(basic_chaos)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-basic-chaos)

add_subdirectory(dynamic_configs)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-dynamic-configs)

add_subdirectory(graceful_shutdown)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-graceful-shutdown)

add_subdirectory(https)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-https)

add_subdirectory(http2server)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-http2server)

add_subdirectory(https_no_passphrase)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-https-no-passphrase)

add_subdirectory(metrics)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-metrics)

add_subdirectory(static_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-static-service)

add_subdirectory(tracing)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-tracing)

add_subdirectory(uctl)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-uctl)

add_subdirectory(cache_update)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-cache-update)

add_subdirectory(websocket)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-websocket)
