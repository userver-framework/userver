import pytest

import samples.greeter_pb2_grpc as greeter_pb2_grpc

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_pb2_grpc.GreeterServiceStub(grpc_channel)
