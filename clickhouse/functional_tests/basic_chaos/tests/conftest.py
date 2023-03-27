import json
import typing

import pytest

from pytest_userver import chaos

from testsuite.databases.clickhouse import discover

pytest_plugins = ['pytest_userver.plugins.clickhouse']


@pytest.fixture(scope='session')
def gate_settings() -> typing.Tuple[str, int]:
    return ('localhost', 19354)


@pytest.fixture(scope='session')
def service_env(gate_settings) -> dict:
    SECDIST_CONFIG = {
        'clickhouse_settings': {
            'clickhouse-database-alias': {
                'hosts': [gate_settings[0]],
                'port': gate_settings[1],
                'password': '',
                'user': 'default',
                'dbname': 'key-value-database',
            },
        },
    }

    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}


@pytest.fixture(scope='session')
async def _gate(loop, gate_settings, clickhouse_conn_info):
    gate_config = chaos.GateRoute(
        name='clickhouse proxy',
        host_for_client=gate_settings[0],
        port_for_client=gate_settings[1],
        host_to_server=clickhouse_conn_info.host,
        port_to_server=clickhouse_conn_info.tcp_port,
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate):
    _gate.to_client_pass()
    _gate.to_server_pass()
    _gate.start_accepting()

    await _gate.wait_for_connections(timeout=5.0)
    yield _gate


@pytest.fixture(scope='session')
def clickhouse_local(service_source_dir):
    return discover.find_schemas(
        schema_dirs=[service_source_dir.joinpath('schemas', 'clickhouse')],
        dbprefix='',
    )
