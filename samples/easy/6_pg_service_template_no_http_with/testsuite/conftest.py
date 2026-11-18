import pytest
import pytest_userver.config

from testsuite.databases.pgsql import discover

pytest_plugins = ['pytest_userver.plugins.postgresql']


@pytest.fixture(scope='session')
def initial_data_path(service_source_dir):
    """Path for find files with data"""
    return [service_source_dir / 'postgresql/data']


@pytest.fixture(scope='session')
@pytest_userver.config.patch
def userver_actions_service(mockserver_info):
    def do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['action-client']['service-url'] = mockserver_info.url(
            '/v1/action',
        )

    return do_patch


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    """Create schemas databases for tests"""
    databases = discover.find_schemas(
        'pg_service_template',  # service name that goes to the DB connection
        [service_source_dir.joinpath('postgresql/schemas')],
    )
    return pgsql_local_create(list(databases.values()))
