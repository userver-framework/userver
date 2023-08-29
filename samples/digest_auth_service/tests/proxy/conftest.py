import pathlib

import pytest

from testsuite.databases.pgsql import discover


pytest_plugins = ['pytest_userver.plugins.postgresql']

USERVER_CONFIG_HOOKS = ['prepare_service_config']
 
 
@pytest.fixture(scope='session')
def prepare_service_config():
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        components['auth-digest-checker-settings']['is-proxy'] = 'true'
 
    return patch_config


@pytest.fixture(scope='session')
def service_source_dir():
    """Path to root directory service."""
    return pathlib.Path(__file__).parent.parent.parent


@pytest.fixture(scope='session')
def initial_data_path(service_source_dir):
    """Path for find files with data"""
    return [
        service_source_dir / 'postgresql/data',
    ]


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    """Create schemas databases for tests"""
    databases = discover.find_schemas(
        'auth',
        [service_source_dir.joinpath('postgresql/schemas')],
    )
    return pgsql_local_create(list(databases.values()))
