include_guard(GLOBAL)

get_property(_userver_testsuite_dir GLOBAL PROPERTY userver_testsuite_dir)
userver_testsuite_register_database(
    NAMES ydb
    REQUIREMENTS_FILE "${_userver_testsuite_dir}/requirements-ydb.txt"
    FEATURE_VAR USERVER_FEATURE_YDB
    TARGET userver::ydb
)
unset(_userver_testsuite_dir)
