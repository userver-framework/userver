import pytest

from pytest_userver import chaos

from testsuite.databases.mongo import connection


pytest_plugins = ['pytest_userver.plugins.mongo']

MONGO_COLLECTIONS = {
    'translations': {
        'settings': {
            'collection': 'translations',
            'connection': 'admin',
            'database': 'admin',
        },
        'indexes': [],
    },
}


@pytest.fixture(scope='session')
def mongodb_settings():
    return MONGO_COLLECTIONS


@pytest.fixture(scope='session')
async def _gate_started(
        loop, mongo_connection_info: connection.ConnectionInfo,
):
    gate_config = chaos.GateRoute(
        name='mongo proxy',
        host_to_server=mongo_connection_info.host,
        port_to_server=mongo_connection_info.port,
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture
def extra_client_deps(_gate_started):
    pass


@pytest.fixture(scope='session')
def userver_mongo_config(_gate_started):
    def _hook_db_config(config_yaml, config_vars):
        host, port = _gate_started.get_sockname_for_clients()

        components = config_yaml['components_manager']['components']
        db = components['key-value-database']
        db['dbconnection'] = f'mongodb://{host}:{port}/admin'

    return _hook_db_config


@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate_started):
    _gate_started.to_server_pass()
    _gate_started.to_client_pass()
    _gate_started.start_accepting()

    await _gate_started.wait_for_connections()
    yield _gate_started
