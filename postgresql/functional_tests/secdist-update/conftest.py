import pytest

from testsuite.databases.pgsql import discover

pytest_plugins = ['pytest_userver.plugins.postgresql']


@pytest.fixture(name='pgsql_local', scope='session')
def _pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'pg',
        [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture(name='userver_config_testsuite', scope='session')
def _userver_config_testsuite(userver_config_testsuite):
    def patch_config(config_yaml, config_vars):
        userver_config_testsuite(config_yaml, config_vars)

        components: dict = config_yaml['components_manager']['components']
        testsuite_support = components['testsuite-support']
        testsuite_support['testsuite-pg-execute-timeout'] = '0ms'
        testsuite_support['testsuite-pg-statement-timeout'] = '0ms'

    return patch_config
