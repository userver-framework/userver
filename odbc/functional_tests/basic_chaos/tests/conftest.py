import json

import pytest
from pytest_userver import chaos

from testsuite.databases.pgsql import discover

pytest_plugins = ['pytest_userver.plugins.postgresql']


@pytest.fixture(name='pgsql_local', scope='session')
def _pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'pg',
        [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture(scope='session')
async def _gate_started(pgsql_local):
    gate_config = chaos.GateRoute(
        name='odbc postgres proxy',
        host_to_server=pgsql_local['key_value'].host,
        port_to_server=pgsql_local['key_value'].port,
    )
    async with chaos.TcpGate(gate_config) as proxy:
        yield proxy


@pytest.fixture
def extra_client_deps(_gate_started):
    pass


@pytest.fixture(scope='session')
def service_env(pgsql_local, _gate_started):
    host, port = _gate_started.get_sockname_for_clients()
    db_info = pgsql_local['key_value']

    dsn = (
        f'Driver={{PostgreSQL Unicode}};'
        f'Server={host};'
        f'Port={port};'
        f'Database={db_info.dbname};'
        f'Uid={db_info.user or "testsuite"};'
        f'Pwd={db_info.password or ""};'
    )

    secdist_config = {
        'odbc_settings': {
            'databases': {
                'key-value-db': {
                    'dsn': dsn,
                },
            },
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}


@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate_started):
    await _gate_started.to_server_pass()
    await _gate_started.to_client_pass()
    _gate_started.start_accepting()

    await service_client.delete('/chaos?key=_warmup')
    await _gate_started.wait_for_connections(timeout=10)
    return _gate_started
