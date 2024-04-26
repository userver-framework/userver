# pylint: disable=redefined-outer-name
import concurrent.futures
import contextlib
import os
import pathlib
import subprocess
from typing import List
from typing import Optional

import pytest
import yaml

from testsuite.environment import shell

from pytest_userver import sql
from . import client
from . import discover
from . import service

if hasattr(yaml, 'CLoader'):
    _YamlLoader = yaml.CLoader  # type: ignore
else:
    _YamlLoader = yaml.Loader  # type: ignore

USERVER_CONFIG_HOOKS = ['userver_config_ydb']


@pytest.fixture
def ydb(_ydb_client, _ydb_init):
    return _ydb_client


@pytest.fixture(scope='session')
def _ydb_client(_ydb_client_pool):
    with _ydb_client_pool() as ydb_client:
        yield ydb_client


@pytest.fixture(scope='session')
def _ydb_client_pool(_ydb_service, ydb_service_settings):
    endpoint = '{}:{}'.format(
        ydb_service_settings.host, ydb_service_settings.grpc_port,
    )
    pool = []

    @contextlib.contextmanager
    def get_client():
        try:
            ydb_client = pool.pop()
        except IndexError:
            ydb_client = client.YdbClient(
                endpoint, ydb_service_settings.database,
            )
        try:
            yield ydb_client
        finally:
            pool.append(ydb_client)

    return get_client


def pytest_service_register(register_service):
    register_service('ydb', service.create_ydb_service)


@pytest.fixture(scope='session')
def _ydb_service(pytestconfig, ensure_service_started, ydb_service_settings):
    if os.environ.get('YDB_ENDPOINT') or pytestconfig.option.ydb_host:
        return
    ensure_service_started('ydb', settings=ydb_service_settings)


@pytest.fixture(scope='session')
def ydb_service_settings(pytestconfig) -> service.ServiceSettings:
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
def ydb_settings_substitute(ydb_service_settings):
    def secdist_settings(*args, **kwargs):
        return {
            'endpoint': '{}:{}'.format(
                ydb_service_settings.host, ydb_service_settings.grpc_port,
            ),
            'database': '/{}'.format(ydb_service_settings.database),
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


@pytest.fixture(scope='session')
def ydb_migrate_dir(service_source_dir) -> pathlib.Path:
    return service_source_dir / 'ydb' / 'migrations'


def _ydb_migrate(ydb_service_settings, ydb_migrate_dir):
    if not ydb_migrate_dir.exists():
        return
    if not list(ydb_migrate_dir.iterdir()):
        return

    if not _get_goose():
        return

    host = ydb_service_settings.host
    port = ydb_service_settings.grpc_port

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


def _ydb_fetch_table_names(ydb_service_settings) -> List[str]:
    try:
        import yatest

        host = ydb_service_settings.host
        port = ydb_service_settings.grpc_port
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


@pytest.fixture(scope='session')
def _ydb_prepare(
        _ydb_client,
        _ydb_service_schemas,
        ydb_service_settings,
        _ydb_state,
        ydb_migrate_dir,
):
    if _ydb_service_schemas and ydb_migrate_dir.exists():
        raise Exception(
            'Both ydb/schema and ydb/migrations exist, '
            'which are mutually exclusive',
        )

    # testsuite legacy
    for schema_path in _ydb_service_schemas:
        with open(schema_path) as fp:
            tables_schemas = yaml.load(fp.read(), Loader=_YamlLoader)
        for table_schema in tables_schemas:
            client.drop_table(_ydb_client, table_schema['path'])
            client.create_table(_ydb_client, table_schema)
            _ydb_state.tables.append(table_schema['path'])

    # goose
    _ydb_migrate(ydb_service_settings, ydb_migrate_dir)

    _ydb_state.init = True


@pytest.fixture(scope='session')
def _ydb_tables(_ydb_state, _ydb_prepare, ydb_service_settings):
    tables = {
        *_ydb_state.tables,
        *_ydb_fetch_table_names(ydb_service_settings),
    }
    return tuple(sorted(tables))


@pytest.fixture
def _ydb_init(
        request,
        _ydb_client,
        _ydb_state,
        ydb_service_settings,
        _ydb_prepare,
        _ydb_tables,
        _ydb_client_pool,
        load,
):
    def ydb_mark_queries(files=(), queries=()):
        result_queries = []
        for path in files:
            result_queries.append(load(path))
        result_queries.extend(queries)
        return result_queries

    def drop_table(table):
        with _ydb_client_pool() as ydb_client:
            ydb_client.execute('DELETE FROM `{}`'.format(table))

    if _ydb_tables:
        with concurrent.futures.ThreadPoolExecutor(
                max_workers=len(_ydb_tables),
        ) as executer:
            executer.map(drop_table, _ydb_tables)

    for mark in request.node.iter_markers('ydb'):
        queries = ydb_mark_queries(**mark.kwargs)
        for query in queries:
            _ydb_client.execute(query)


@pytest.fixture
def userver_ydb_trx(testpoint) -> sql.RegisteredTrx:
    """
    The fixture maintains transaction fault injection state using
    RegisteredTrx class.

    @see pytest_userver.sql.RegisteredTrx

    @snippet integration_tests/tests/test_trx_failure.py  fault injection

    @ingroup userver_testsuite_fixtures
    """

    registered = sql.RegisteredTrx()

    @testpoint('ydb_trx_commit')
    def _pg_trx_tp(data):
        should_fail = registered.is_failure_enabled(data['trx_name'])
        return {'trx_should_fail': should_fail}

    return registered


@pytest.fixture(scope='session')
def userver_config_ydb(ydb_service_settings):
    """
    Returns a function that adjusts the static configuration file for testsuite.

    For all `ydb.databases`, sets `endpoint` and `database` to the local test
    YDB instance.

    @ingroup userver_testsuite_fixtures
    """

    endpoint = f'{ydb_service_settings.host}:{ydb_service_settings.grpc_port}'
    database = (
        '' if ydb_service_settings.database.startswith('/') else '/'
    ) + ydb_service_settings.database

    def patch_config(config, config_vars):
        ydb_component = config['components_manager']['components']['ydb']
        if isinstance(ydb_component, str):
            ydb_component = config_vars[ydb_component[1:]]
        databases = ydb_component['databases']
        for dbname, dbconfig in databases.items():
            dbconfig['endpoint'] = endpoint
            dbconfig['database'] = database

    return patch_config
