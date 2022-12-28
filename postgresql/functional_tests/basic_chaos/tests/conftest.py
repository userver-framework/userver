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
@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'pg', [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture(scope='session')
async def _gate_started(loop, pgsql_local):
    gate_config = chaos.GateRoute(
        name='postgres proxy',
        host_to_server=pgsql_local['key_value'].host,
        port_to_server=pgsql_local['key_value'].port,
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture
def client_deps(_gate_started, pgsql):
    pass


USERVER_CONFIG_HOOKS = ['hook_db_config']


@pytest.fixture(scope='session')
def hook_db_config(pgsql_local, _gate_started):
    def _hook_db_config(config_yaml, config_vars):
        _DB_NAME = 'pg_key_value'
        host, port = _gate_started.get_sockname_for_clients()

        components = config_yaml['components_manager']['components']
        db = components['key-value-database']
        db['dbconnection'] = f'postgresql://testsuite@{host}:{port}/{_DB_NAME}'

    return _hook_db_config
    # /// [gate start]


# /// [gate fixture]
@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate_started):
    _gate_started.to_server_pass()
    _gate_started.to_client_pass()
    _gate_started.start_accepting()

    await _gate_started.wait_for_connections()
    yield _gate_started
    # /// [gate fixture]
