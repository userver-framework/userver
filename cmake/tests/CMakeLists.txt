add_test(
    NAME userver-cmake-install-component-config
    COMMAND
        ${CMAKE_COMMAND} "-DUSERVER_SOURCE_DIR=${USERVER_ROOT_DIR}" -S
        "${CMAKE_CURRENT_SOURCE_DIR}/install-component-config" -B
        "${CMAKE_CURRENT_BINARY_DIR}/install-component-config"
)
