import json
import os

import pytest

from pytest_userver import chaos

from testsuite.databases.pgsql import discover


pytest_plugins = [
    'pytest_userver.plugins',
    'pytest_userver.plugins.samples',
    # Database related plugins
    'testsuite.databases.pgsql.pytest_plugin',
]


# /// [gate start]
@pytest.fixture(name='for_clinet_gate_port', scope='session')
def _for_clinet_gate_port():
    return 11433


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'pg', [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture(scope='session')
async def _gate_started(loop, for_clinet_gate_port, pgsql_local):
    gate_config = chaos.GateRoute(
        name='postgres proxy',
        host_for_client='localhost',
        port_for_client=for_clinet_gate_port,
        host_to_server='localhost',
        port_to_server=pgsql_local['key_value'].port,
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture
def client_deps(_gate_started, pgsql):
    pass
    # /// [gate start]


# /// [gate fixture]
@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate_started):
    _gate_started.to_server_pass()
    _gate_started.to_client_pass()
    _gate_started.start_accepting()

    await _gate_started.wait_for_connectons()
    yield _gate_started
    # /// [gate fixture]


@pytest.fixture(scope='session')
def config_service_defaults():
    with open(
            os.path.join(
                os.path.dirname(os.path.dirname(__file__)),
                'dynamic_config_fallback.json',
            ),
    ) as fp:
        return json.load(fp)
