include_guard(GLOBAL)

get_property(_userver_testsuite_dir GLOBAL PROPERTY userver_testsuite_dir)
userver_testsuite_register_database(
    NAMES mongo
    PIP_MODULE mongodb
    REQUIREMENTS_FILE "${_userver_testsuite_dir}/requirements-mongo.txt"
    FEATURE_VAR USERVER_FEATURE_MONGODB
    TARGET userver::mongo
)
unset(_userver_testsuite_dir)
