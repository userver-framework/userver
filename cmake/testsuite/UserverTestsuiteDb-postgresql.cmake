include_guard(GLOBAL)

get_property(_userver_testsuite_dir GLOBAL PROPERTY userver_testsuite_dir)
userver_testsuite_register_database(
    NAMES postgresql postgres
    PIP_MODULE postgresql
    PIP_MODULE_DARWIN postgresql-binary
    REQUIREMENTS_FILE "${_userver_testsuite_dir}/requirements-postgres.txt"
    FEATURE_VAR USERVER_FEATURE_POSTGRESQL
    TARGET userver::postgresql
)
unset(_userver_testsuite_dir)
