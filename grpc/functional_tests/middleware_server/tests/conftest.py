import pytest
import samples.greeter_pb2_grpc as greeter_services  # noqa: E402, E501

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_services.GreeterServiceStub(grpc_channel)
