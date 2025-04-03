# /// [Prepare modules]
import pytest

import samples.greeter_pb2_grpc as greeter_services

pytest_plugins = ['pytest_userver.plugins.grpc']
# /// [Prepare modules]


# /// [grpc client]
@pytest.fixture
def grpc_client(grpc_channel, service_client):
    return greeter_services.GreeterServiceStub(grpc_channel)
    # /// [grpc client]
