import os

import pytest

from . import client
from . import discover
from . import service


@pytest.fixture
def ydb(_ydb_client, _ydb_init):
    return _ydb_client


@pytest.fixture
def _ydb_client(_ydb_service, _ydb_service_settings):
    endpoint = '{}:{}'.format(
        _ydb_service_settings.host, _ydb_service_settings.grpc_port,
    )
    return client.YdbClient(endpoint, _ydb_service_settings.database)


@pytest.fixture
def _ydb_service(pytestconfig, ensure_service_started, _ydb_service_settings):
    if os.environ.get('YDB_ENDPOINT') or pytestconfig.option.ydb_host:
        return
    ensure_service_started('ydb', settings=_ydb_service_settings)


@pytest.fixture(scope='session')
def ydb_service_settings(_ydb_service_settings):
    return _ydb_service_settings


@pytest.fixture(scope='session')
def _ydb_service_settings(pytestconfig):
    endpoint_from_env = os.environ.get('YDB_ENDPOINT')
    database = os.environ.get('YDB_DATABASE', 'local')

    if endpoint_from_env:
        host, grpc_port = endpoint_from_env.split(':', 1)
        return service.ServiceSettings(
            host=host,
            grpc_port=grpc_port,
            mon_port=None,
            ic_port=None,
            database=database,
        )

    if pytestconfig.option.ydb_host:
        return service.ServiceSettings(
            host=pytestconfig.option.ydb_host,
            grpc_port=pytestconfig.option.ydb_grpc_port,
            mon_port=pytestconfig.option.ydb_mon_port,
            ic_port=pytestconfig.option.ydb_ic_port,
            database=database,
        )
    return service.get_service_settings()


@pytest.fixture(scope='session')
def _ydb_service_schemas(service_source_dir):
    service_schemas_ydb = service_source_dir / 'ydb' / 'schemas'
    return discover.find_schemas([service_schemas_ydb])


@pytest.fixture(scope='session')
def ydb_settings_substitute(_ydb_service_settings):
    def secdist_settings(*args, **kwargs):
        return {
            'endpoint': '{}:{}'.format(
                _ydb_service_settings.host, _ydb_service_settings.grpc_port,
            ),
            'database': '/{}'.format(_ydb_service_settings.database),
            'token': '',
        }

    return {'ydb_settings': secdist_settings}


@pytest.fixture(scope='session')
def _ydb_state():
    class State:
        def __init__(self):
            self.init = False
            self.tables = []

    return State()


@pytest.fixture
def _ydb_init(
        request,
        _ydb_service_schemas,
        _ydb_client,
        _ydb_state,
        load,
        load_yaml,
):
    def ydb_mark_queries(files=(), queries=()):
        result_queries = []
        for path in files:
            result_queries.append(load(path))
        result_queries.extend(queries)
        return result_queries

    if not _ydb_state.init:
        _ydb_state.init = True
        for schema_path in _ydb_service_schemas:
            tables_schemas = load_yaml(schema_path)
            for table_schema in tables_schemas:
                client.drop_table(_ydb_client, table_schema['path'])
                client.create_table(_ydb_client, table_schema)
                _ydb_state.tables.append(table_schema['path'])

    for table in _ydb_state.tables:
        _ydb_client.execute('DELETE FROM `{}`'.format(table))

    for mark in request.node.iter_markers('ydb'):
        queries = ydb_mark_queries(**mark.kwargs)
        for query in queries:
            _ydb_client.execute(query)
