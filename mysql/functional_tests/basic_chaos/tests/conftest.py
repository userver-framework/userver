import json
import typing

import pytest

from pytest_userver import chaos

from testsuite.databases.mysql import discover

pytest_plugins = ['pytest_userver.plugins.mysql']


@pytest.fixture(scope='session')
def gate_settings() -> typing.Tuple[str, int]:
    return ('localhost', 19355)


@pytest.fixture(scope='session')
def service_env(gate_settings) -> dict:
    SECDIST_CONFIG = {
        'mysql_settings': {
            'key-value-db': {
                'hosts': [gate_settings[0]],
                'port': gate_settings[1],
                'password': '',
                'user': 'root',
                'database': 'key-value-database',
            },
        },
    }

    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}


@pytest.fixture(scope='session')
async def _gate(loop, gate_settings, mysql_conninfo):
    gate_config = chaos.GateRoute(
        name='mysql proxy',
        host_for_client=gate_settings[0],
        port_for_client=gate_settings[1],
        host_to_server=mysql_conninfo.hostname,
        port_to_server=mysql_conninfo.port,
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
def mysql_local(service_source_dir):
    return discover.find_schemas(
        schema_dirs=[service_source_dir.joinpath('schemas', 'mysql')],
        dbprefix='',
    )
