project(userver-core-tests CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(basic_chaos)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-basic-chaos)

add_subdirectory(dump_coroutines)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-dump-coroutines)

add_subdirectory(early_monitor_port_open)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-early-monitor-port-open)

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

add_subdirectory(sharded_http_handler_metrics)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-sharded-http-handler-metrics)

add_subdirectory(static_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-static-service)

add_subdirectory(slow_start)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-slow-start)

add_subdirectory(signal_during_boot)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-signal-during-boot)

add_subdirectory(tracing)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-tracing)

add_subdirectory(trx_tracker)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-trx-tracker)

add_subdirectory(uctl)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-uctl)

add_subdirectory(cache_update)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-cache-update)

add_subdirectory(websocket)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-websocket)

# WebSocket client requires curl >= 7.86 with WebSocket support
if(CURL_VERSION_STRING VERSION_GREATER_EQUAL "7.86")
    add_subdirectory(websocket_client)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-websocket_client)
endif()
