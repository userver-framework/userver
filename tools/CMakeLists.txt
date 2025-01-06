project(userver-tools CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(congestion-control-emulator)
add_dependencies(${PROJECT_NAME} userver-tool-congestion-control-emulator)

add_subdirectory(dns-resolver)
add_dependencies(${PROJECT_NAME} userver-tool-dns-resolver)

add_subdirectory(engine-perf)
add_dependencies(${PROJECT_NAME} userver-tool-engine-perf)

add_subdirectory(http-client-perf)
add_dependencies(${PROJECT_NAME} userver-tool-http-client-perf)

add_subdirectory(netcat)
add_dependencies(${PROJECT_NAME} userver-tool-netcat)
