include_guard(GLOBAL)

userver_testsuite_register_database(
    NAMES clickhouse
    PIP_MODULE clickhouse
    FEATURE_VAR USERVER_FEATURE_CLICKHOUSE
    TARGET userver::clickhouse
)
