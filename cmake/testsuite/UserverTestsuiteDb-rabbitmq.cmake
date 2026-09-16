include_guard(GLOBAL)

userver_testsuite_register_database(
    NAMES rabbitmq
    PIP_MODULE rabbitmq
    FEATURE_VAR USERVER_FEATURE_RABBITMQ
    TARGET userver::rabbitmq
)
