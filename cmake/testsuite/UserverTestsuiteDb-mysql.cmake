include_guard(GLOBAL)

userver_testsuite_register_database(
    NAMES mysql
    PIP_MODULE mysql
    FEATURE_VAR USERVER_FEATURE_MYSQL
    TARGET userver::mysql
)
