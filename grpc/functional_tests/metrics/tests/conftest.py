# /// [Prepare modules]
import pytest

import samples.greeter_pb2_grpc as greeter_services

pytest_plugins = ['pytest_userver.plugins.grpc']
# /// [Prepare modules]


# /// [Prepare server mock]
@pytest.fixture
def greeter_mock(grpc_mockserver):
    return grpc_mockserver.mock_factory(greeter_services.GreeterServiceServicer)
    # /// [Prepare server mock]


@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_services.GreeterServiceStub(grpc_channel)
