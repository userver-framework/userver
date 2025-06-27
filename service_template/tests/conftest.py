import pathlib
import sys  # grpc template current

import handlers.hello_pb2_grpc as hello_services  # grpc template current
import pytest

from testsuite.databases.pgsql import discover  # postgresql template current

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.postgresql',  # postgresql template current
    'pytest_userver.plugins.mongo',  # mongo template current
    'pytest_userver.plugins.grpc',  # grpc template current
]


# mongo template on
MONGO_COLLECTIONS = {
    'hello_users': {
        'settings': {
            'collection': 'hello_users',
            'connection': 'admin',
            'database': 'admin',
        },
        'indexes': [],
    },
}


@pytest.fixture(scope='session')
def mongodb_settings():
    return MONGO_COLLECTIONS


# mongo template off


# grpc template on
@pytest.fixture
def grpc_service(grpc_channel, service_client):
    return hello_services.HelloServiceStub(grpc_channel)


# grpc template off


# postgresql template on
@pytest.fixture(scope='session')
def initial_data_path(service_source_dir):
    """Path for find files with data"""
    return [
        service_source_dir / 'postgresql/data',
    ]


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    """Create schemas databases for tests"""
    databases = discover.find_schemas(
        'service_template',
        [service_source_dir.joinpath('postgresql/schemas')],
    )
    return pgsql_local_create(list(databases.values()))


# postgresql template off
