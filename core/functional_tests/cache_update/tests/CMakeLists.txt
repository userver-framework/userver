userver_testsuite_add(
    SERVICE_TARGET userver-core-tests-cache-update
    PYTEST_ARGS
    --service-config=${CMAKE_CURRENT_SOURCE_DIR}/../static_config.yaml
    --service-binary=${CMAKE_CURRENT_BINARY_DIR}/../userver-core-tests-cache-update
    --service-config-vars=${CMAKE_CURRENT_SOURCE_DIR}/../config_vars.yaml
)
