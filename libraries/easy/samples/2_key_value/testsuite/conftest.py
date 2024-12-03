import os
import subprocess

import pytest

from testsuite.databases.pgsql import discover

pytest_plugins = ['pytest_userver.plugins.postgresql']


@pytest.fixture(scope='session')
def schema_path(service_binary, service_tmpdir):
    path = service_tmpdir.joinpath('schemas')
    os.mkdir(path)
    subprocess.run([service_binary, '--dump-db-schema', path / '0_pg.sql'])
    return path


@pytest.fixture(scope='session')
def pgsql_local(schema_path, pgsql_local_create):
    databases = discover.find_schemas('admin', [schema_path])
    return pgsql_local_create(list(databases.values()))
