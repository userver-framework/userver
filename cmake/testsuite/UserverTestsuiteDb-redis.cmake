include_guard(GLOBAL)

get_property(_userver_testsuite_dir GLOBAL PROPERTY userver_testsuite_dir)
userver_testsuite_register_database(
    NAMES redis redis-cluster
    PIP_MODULE redis
    REQUIREMENTS_FILE "${_userver_testsuite_dir}/requirements-redis.txt"
    FEATURE_VAR USERVER_FEATURE_REDIS
    TARGET userver::redis
)
unset(_userver_testsuite_dir)
