import pytest

from pytest_userver import chaos

from testsuite.databases.pgsql import connection
from testsuite.databases.pgsql import discover


pytest_plugins = ['pytest_userver.plugins.postgresql']


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
def extra_client_deps(_gate_started):
    pass


@pytest.fixture(scope='session')
def userver_pg_config(pgsql_local, _gate_started):
    def _hook_db_config(config_yaml, config_vars):
        host, port = _gate_started.get_sockname_for_clients()

        db_info = pgsql_local['key_value']
        db_chaos_gate = connection.PgConnectionInfo(
            host=host,
            port=port,
            user=db_info.user,
            password=db_info.password,
            options=db_info.options,
            sslmode=db_info.sslmode,
            dbname=db_info.dbname,
        )

        components = config_yaml['components_manager']['components']
        db = components['key-value-database']
        db['dbconnection'] = db_chaos_gate.get_uri()

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


@pytest.fixture(
    autouse=True,
    params=[False, True],
    ids=['pipeline_disabled', 'pipeline_enabled'],
)
async def pipeline_mode(request, service_client, dynamic_config):
    dynamic_config.set_values(
        {'POSTGRES_CONNECTION_PIPELINE_ENABLED': request.param},
    )
    await service_client.update_server_state()
