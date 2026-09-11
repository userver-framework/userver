include_guard(GLOBAL)

userver_testsuite_register_database(
    NAMES kafka
    PIP_MODULE kafka
    FEATURE_VAR USERVER_FEATURE_KAFKA
    TARGET userver::kafka
)
