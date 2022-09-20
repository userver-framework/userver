import sys

import grpc
import pytest

pytest_plugins = [
    'pytest_userver.plugins',
    'pytest_userver.plugins.samples',
    'pytest_userver.plugins.grpc',
]


@pytest.fixture(scope='session')
def greeter_protos():
    return grpc.protos('greeter.proto')


@pytest.fixture(scope='session')
def greeter_services():
    return grpc.services('greeter.proto')


@pytest.fixture
def grpc_service(grpc_channel, greeter_services, service_client):
    return greeter_services.GreeterServiceStub(grpc_channel)


def pytest_configure(config):
    sys.path.append(str(config.rootpath / 'grpc_service/proto/samples'))
