import os
import pathlib
import subprocess
from typing import List
from typing import Optional

import pytest

from testsuite.environment import shell

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


def pytest_service_register(register_service):
    register_service('ydb', service.create_ydb_service)


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


@pytest.fixture(name='ydb_migrate_dir')
def _ydb_migrate_dir(service_source_dir) -> pathlib.Path:
    return service_source_dir / 'ydb' / 'migrations'


def _ydb_migrate(_ydb_service_settings, ydb_migrate_dir):
    if not ydb_migrate_dir.exists():
        return
    if not list(ydb_migrate_dir.iterdir()):
        return

    if not _get_goose():
        return

    host = _ydb_service_settings.host
    port = _ydb_service_settings.grpc_port

    command = [
        str(_get_goose()),
        '-dir',
        str(ydb_migrate_dir),
        'ydb',
        (
            f'grpc://{host}:{port}/local?go_query_mode=scripting&'
            'go_fake_tx=scripting&go_query_bind=declare,numeric'
        ),
        'up',
    ]
    try:
        shell.execute(command, verbose=True, command_alias='ydb/migrations')
    except shell.SubprocessFailed as exc:
        raise Exception(f'YDB run migration failed:\n\n{exc}')


def _get_goose() -> Optional[pathlib.Path]:
    try:
        import yatest

        return yatest.common.runtime.binary_path(
            'contrib/go/patched/goose/cmd/goose/goose',
        )
    except ImportError:
        return None


def _ydb_fetch_table_names(_ydb_service_settings) -> List[str]:
    try:
        import yatest

        host = _ydb_service_settings.host
        port = _ydb_service_settings.grpc_port
        output = subprocess.check_output(
            [
                yatest.common.runtime.binary_path('contrib/ydb/apps/ydb/ydb'),
                '-e',
                f'grpc://{host}:{port}',
                '-d',
                '/local',
                'scheme',
                'ls',
                '-lR',
            ],
            encoding='utf-8',
        )
        tables = []

        for line in output.split('\n'):
            if ' table ' not in line:
                continue
            if '.sys' in line:
                continue
            path = line.split('│')[6].strip()
            tables.append(path)
        return tables
    except ImportError:
        return []


@pytest.fixture
def _ydb_init(
        request,
        _ydb_service_schemas,
        _ydb_client,
        _ydb_state,
        _ydb_service_settings,
        ydb_migrate_dir,
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
        if _ydb_service_schemas and ydb_migrate_dir.exists():
            raise Exception(
                'Both ydb/schema and ydb/migrations exist, '
                'which are mutually exclusive',
            )

        # testsuite legacy
        for schema_path in _ydb_service_schemas:
            tables_schemas = load_yaml(schema_path)
            for table_schema in tables_schemas:
                client.drop_table(_ydb_client, table_schema['path'])
                client.create_table(_ydb_client, table_schema)
                _ydb_state.tables.append(table_schema['path'])

        # goose
        _ydb_migrate(_ydb_service_settings, ydb_migrate_dir)

        _ydb_state.init = True

    # testsuite legacy
    for table in _ydb_state.tables:
        _ydb_client.execute('DELETE FROM `{}`'.format(table))

    # goose
    for table in _ydb_fetch_table_names(_ydb_service_settings):
        _ydb_client.execute('DELETE FROM `{}`'.format(table))

    for mark in request.node.iter_markers('ydb'):
        queries = ydb_mark_queries(**mark.kwargs)
        for query in queries:
            _ydb_client.execute(query)
