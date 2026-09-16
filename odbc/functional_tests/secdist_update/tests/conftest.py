import json

import pytest

from testsuite.databases.pgsql import discover

pytest_plugins = ['pytest_userver.plugins.postgresql']
USERVER_CONFIG_HOOKS = ['userver_config_secdist']


@pytest.fixture(name='pgsql_local', scope='session')
def _pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'pg',
        [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture(scope='session')
def secdist_path(service_tmpdir):
    path = service_tmpdir / 'secdist.json'
    path.write_text(
        json.dumps({
            'odbc_settings': {
                'databases': {
                    'odbc-test': {
                        'dsn': (
                            'Driver={PostgreSQL Unicode};Server=localhost;Port=1;Database=postgres;Uid=testsuite;Pwd=;'
                        ),
                    },
                },
            },
        }),
    )
    return path


@pytest.fixture(scope='session')
def userver_config_secdist(secdist_path):
    def _hook(config_yaml, _config_vars):
        components = config_yaml['components_manager']['components']
        components['default-secdist-provider']['config'] = str(secdist_path)

    return _hook
