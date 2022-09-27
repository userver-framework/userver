import pytest

from pytest_userver import chaos

from testsuite.databases.pgsql import discover


pytest_plugins = [
    'pytest_userver.plugins',
    'pytest_userver.plugins.samples',
    # Database related plugins
    'testsuite.databases.pgsql.pytest_plugin',
]


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'pg', [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture(scope='session')
async def _gate_started(loop):
    gate_config = chaos.GateRoute(
        name='postgres proxy',
        host_left='localhost',
        port_left=11433,
        host_right='localhost',
        port_right=15433,
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture
def client_deps(_gate_started, pgsql):
    pass


@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate_started):
    _gate_started.to_right_pass()
    _gate_started.to_left_pass()
    _gate_started.start_accepting()

    await _gate_started.wait_for_connectons()
    yield _gate_started
